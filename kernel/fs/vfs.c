#include "fs/vfs.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "validate.h"
#include "refcount.h"
#include "process.h"
#include "tmpfs.h"
#include "ext2.h"
#include "epoll.h"

#define VFS_MAX_FD 256
#define VFS_MAX_BACKENDS 8

typedef enum {
    VFS_BACKEND_NONE = 0,
    VFS_BACKEND_TMPFS,
    VFS_BACKEND_EXT2,
} vfs_backend_type_t;

/*
 * A single open file. Shared (refcounted) between descriptors created by dup()
 * and between parent/child after fork(), so a seek in one is seen in the other
 * exactly like Linux's struct file.
 */
typedef struct file_obj {
    vfs_file_t* file;
    vfs_ops_t* ops;
    vfs_backend_type_t backend;
    refcount_t refcount;
} file_obj_t;

/* Per-process descriptor table (hangs off process_t.files). */
typedef struct files_struct {
    file_obj_t* fd[VFS_MAX_FD];
    u8 cloexec[VFS_MAX_FD];
} files_struct_t;

/* Fallback table for kernel threads / early boot (no owning process yet). */
static files_struct_t kernel_files;
static vfs_fs_ops_t* mounted_fs[VFS_MAX_BACKENDS];
static u32 mount_count = 0;
static bool ext2_mounted = false;

static files_struct_t* current_files(void) {
    process_t* p = process_current();
    if (p && p->files) {
        return (files_struct_t*)p->files;
    }
    return &kernel_files;
}

static file_obj_t* fobj_new(vfs_file_t* f, vfs_ops_t* ops, vfs_backend_type_t be) {
    file_obj_t* o = (file_obj_t*)kzalloc(sizeof(file_obj_t));
    if (!o) {
        return NULL;
    }
    o->file = f;
    o->ops = ops;
    o->backend = be;
    o->refcount.count = 1;
    spinlock_init(&o->refcount.lock);
    return o;
}

/* Drop one reference; close + free the underlying file at the last one. */
static void fobj_put(file_obj_t* o) {
    if (!o) {
        return;
    }
    if (refcount_put(&o->refcount) == 0) {
        if (o->ops && o->ops->close && o->file) {
            o->ops->close(o->file);
        }
        kfree(o->file);
        kfree(o);
    }
}

void vfs_init(void) {
    memset(&kernel_files, 0, sizeof(kernel_files));
    mount_count = 0;
    ext2_mounted = false;
    printk("VFS: Initialized (tmpfs + ext2)\n");
}

void* files_create(void) {
    return kzalloc(sizeof(files_struct_t));
}

void* files_clone(void* src) {
    files_struct_t* s = src ? (files_struct_t*)src : &kernel_files;
    files_struct_t* d = (files_struct_t*)kzalloc(sizeof(files_struct_t));
    if (!d) {
        return NULL;
    }
    for (int fd = 0; fd < VFS_MAX_FD; fd++) {
        if (s->fd[fd]) {
            refcount_get(&s->fd[fd]->refcount);
            d->fd[fd] = s->fd[fd];
            d->cloexec[fd] = s->cloexec[fd];
        }
    }
    return d;
}

void files_on_exec(void* files) {
    files_struct_t* f = files ? (files_struct_t*)files : &kernel_files;
    for (int fd = 0; fd < VFS_MAX_FD; fd++) {
        if (f->fd[fd] && f->cloexec[fd]) {
            fobj_put(f->fd[fd]);
            f->fd[fd] = NULL;
            f->cloexec[fd] = 0;
        }
    }
}

void files_destroy(void* files) {
    if (!files || files == &kernel_files) {
        return;
    }
    files_struct_t* f = (files_struct_t*)files;
    for (int fd = 0; fd < VFS_MAX_FD; fd++) {
        if (f->fd[fd]) {
            fobj_put(f->fd[fd]);
            f->fd[fd] = NULL;
        }
    }
    kfree(f);
}

int vfs_dup2_fd(int old_fd, int new_fd) {
    if (old_fd < 0 || old_fd >= VFS_MAX_FD || new_fd < 0 || new_fd >= VFS_MAX_FD) {
        return -1;
    }
    files_struct_t* fs = current_files();
    if (!fs->fd[old_fd]) {
        return -1;
    }
    if (old_fd == new_fd) {
        return new_fd;
    }
    if (fs->fd[new_fd]) {
        fobj_put(fs->fd[new_fd]);
    }
    refcount_get(&fs->fd[old_fd]->refcount);
    fs->fd[new_fd] = fs->fd[old_fd];
    fs->cloexec[new_fd] = 0;   /* dup2 clears close-on-exec on the copy */
    return new_fd;
}

int vfs_set_cloexec(int fd, int on) {
    if (fd < 0 || fd >= VFS_MAX_FD) {
        return -1;
    }
    files_struct_t* fs = current_files();
    if (!fs->fd[fd]) {
        return -1;
    }
    fs->cloexec[fd] = on ? 1 : 0;
    return 0;
}

static vfs_backend_type_t vfs_path_backend(const char* path) {
    if (!path) {
        return VFS_BACKEND_NONE;
    }
    if (strncmp(path, "/tmp/", 5) == 0 || strcmp(path, "/tmp") == 0) {
        return VFS_BACKEND_TMPFS;
    }
    if (strncmp(path, "/etc/", 5) == 0) {
        return VFS_BACKEND_TMPFS;
    }
    if (strncmp(path, "/boot/", 6) == 0 || strncmp(path, "/", 1) == 0) {
        if (ext2_mounted) {
            return VFS_BACKEND_EXT2;
        }
    }
    return VFS_BACKEND_TMPFS;
}

int vfs_mount(const char* source, const char* target, vfs_fs_ops_t* fs_ops) {
    VALIDATE_STRING(source, 4096);
    VALIDATE_STRING(target, 4096);
    VALIDATE_PTR(fs_ops);

    if (mount_count >= VFS_MAX_BACKENDS) {
        return -1;
    }
    mounted_fs[mount_count++] = fs_ops;
    if (fs_ops->mount) {
        int r = fs_ops->mount(source, target);
        if (strcmp(source, "hda") == 0 || strstr(source, "ext2")) {
            ext2_mounted = (r == 0);
        }
        return r;
    }
    return 0;
}

int vfs_unmount(const char* target) {
    VALIDATE_STRING(target, 4096);
    (void)target;
    return 0;
}

static int vfs_alloc_fd(vfs_file_t* file, vfs_ops_t* ops, vfs_backend_type_t backend) {
    files_struct_t* fs = current_files();
    for (int fd = 3; fd < VFS_MAX_FD; fd++) {   /* 0/1/2 reserved for stdio */
        if (!fs->fd[fd]) {
            file_obj_t* o = fobj_new(file, ops, backend);
            if (!o) {
                return -1;
            }
            fs->fd[fd] = o;
            fs->cloexec[fd] = 0;
            return fd;
        }
    }
    return -1;
}

vfs_file_t* vfs_open(const char* path, u64 flags) {
    VALIDATE_STRING_NULL(path, 4096);
    VALIDATE_FLAGS_NULL(flags, 0xFFFFFFFF);

    vfs_backend_type_t be = vfs_path_backend(path);
    vfs_ops_t* ops = NULL;

    if (be == VFS_BACKEND_TMPFS) {
        ops = tmpfs_get_file_ops();
    } else if (be == VFS_BACKEND_EXT2) {
        ops = ext2_get_file_ops();
    }

    if (!ops || !ops->open) {
        return NULL;
    }

    vfs_file_t* file = (vfs_file_t*)kmalloc(sizeof(vfs_file_t));
    if (!file) {
        return NULL;
    }
    memset(file, 0, sizeof(*file));
    file->flags = flags;
    file->type = VFS_FILE;

    if (ops->open(file, path) < 0) {
        kfree(file);
        return NULL;
    }
    /* Record the backend separately: private_data belongs to the FS open(). */
    file->backend = (int)be;
    return file;
}

int vfs_open_fd(const char* path, u64 flags) {
    vfs_file_t* f = vfs_open(path, flags);
    if (!f) {
        return -1;
    }
    vfs_backend_type_t be = (vfs_backend_type_t)f->backend;
    vfs_ops_t* ops = (be == VFS_BACKEND_EXT2) ? ext2_get_file_ops() : tmpfs_get_file_ops();
    return vfs_alloc_fd(f, ops, be);
}

int vfs_close_fd(int fd) {
    if (fd < 0 || fd >= (int)VFS_MAX_FD) {
        return -1;
    }
    files_struct_t* fs = current_files();
    file_obj_t* o = fs->fd[fd];
    if (!o) {
        return -1;
    }
    fs->fd[fd] = NULL;
    fs->cloexec[fd] = 0;
    fobj_put(o);            /* closes the file only at the last reference */
    return 0;
}

ssize_t vfs_read_fd(int fd, void* buf, size_t count) {
    if (fd < 0 || fd >= (int)VFS_MAX_FD) {
        return -1;
    }
    file_obj_t* o = current_files()->fd[fd];
    if (!o || !o->ops || !o->ops->read || !o->file) {
        return -1;
    }
    return o->ops->read(o->file, buf, count);
}

ssize_t vfs_write_fd(int fd, const void* buf, size_t count) {
    if (fd < 0 || fd >= (int)VFS_MAX_FD) {
        return -1;
    }
    file_obj_t* o = current_files()->fd[fd];
    if (!o || !o->ops || !o->ops->write || !o->file) {
        return -1;
    }
    return o->ops->write(o->file, buf, count);
}

int vfs_close(vfs_file_t* file) {
    VALIDATE_PTR(file);
    kfree(file);
    return 0;
}

ssize_t vfs_read(vfs_file_t* file, void* buf, size_t count) {
    VALIDATE_PTR_RET(file, -1);
    VALIDATE_PTR_RET(buf, -1);
    vfs_backend_type_t be = (vfs_backend_type_t)file->backend;
    vfs_ops_t* ops = (be == VFS_BACKEND_EXT2) ? ext2_get_file_ops() : tmpfs_get_file_ops();
    if (ops && ops->read) {
        return ops->read(file, buf, count);
    }
    return -1;
}

ssize_t vfs_write(vfs_file_t* file, const void* buf, size_t count) {
    VALIDATE_PTR_RET(file, -1);
    VALIDATE_PTR_RET(buf, -1);
    vfs_backend_type_t be = (vfs_backend_type_t)file->backend;
    vfs_ops_t* ops = (be == VFS_BACKEND_EXT2) ? ext2_get_file_ops() : tmpfs_get_file_ops();
    if (ops && ops->write) {
        return ops->write(file, buf, count);
    }
    return -1;
}

ssize_t vfs_read_path(const char* path, void* buf, size_t count) {
    int fd = vfs_open_fd(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    ssize_t n = vfs_read_fd(fd, buf, count);
    vfs_close_fd(fd);
    return n;
}

ssize_t vfs_write_path(const char* path, const void* buf, size_t count) {
    int fd = vfs_open_fd(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        return -1;
    }
    ssize_t n = vfs_write_fd(fd, buf, count);
    vfs_close_fd(fd);
    return n;
}

void vfs_register_filesystem(const char* name, vfs_fs_ops_t* fs_ops) {
    VALIDATE_STRING_VOID(name, 256);
    VALIDATE_PTR_VOID(fs_ops);
    vfs_mount(name, "/", fs_ops);
}

int vfs_fd_poll_events(int fd) {
    if (fd < 0 || fd >= (int)VFS_MAX_FD) {
        return 0;
    }
    file_obj_t* o = current_files()->fd[fd];
    if (!o) {
        return 0;
    }
    vfs_file_t* f = o->file;
    if (!f) {
        return 0;
    }
    if (f->offset < f->size || f->size > 0) {
        return EPOLLIN;
    }
    return 0;
}
