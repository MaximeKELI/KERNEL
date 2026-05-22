#include "tmpfs.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "fs/vfs.h"
#include "string.h"

#define TMPFS_MAX_FILES 32
#define TMPFS_FILE_SIZE (64 * 1024)

typedef struct tmpfs_file_entry {
    char path[128];
    u8* data;
    size_t size;
    size_t capacity;
    bool used;
} tmpfs_file_entry_t;

static void* tmpfs_root = NULL;
static size_t tmpfs_size = 0;
static tmpfs_file_entry_t tmpfs_files[TMPFS_MAX_FILES];

static tmpfs_file_entry_t* tmpfs_find(const char* path) {
    for (u32 i = 0; i < TMPFS_MAX_FILES; i++) {
        if (tmpfs_files[i].used && strcmp(tmpfs_files[i].path, path) == 0) {
            return &tmpfs_files[i];
        }
    }
    return NULL;
}

static tmpfs_file_entry_t* tmpfs_create(const char* path) {
    tmpfs_file_entry_t* e = tmpfs_find(path);
    if (e) {
        return e;
    }
    for (u32 i = 0; i < TMPFS_MAX_FILES; i++) {
        if (!tmpfs_files[i].used) {
            e = &tmpfs_files[i];
            strncpy(e->path, path, sizeof(e->path) - 1);
            e->capacity = TMPFS_FILE_SIZE;
            e->data = (u8*)kmalloc(e->capacity);
            if (!e->data) {
                return NULL;
            }
            e->size = 0;
            e->used = true;
            return e;
        }
    }
    return NULL;
}

static int tmpfs_open(vfs_file_t* file, const char* path) {
    tmpfs_file_entry_t* e = tmpfs_find(path);
    if (!e) {
        e = tmpfs_create(path);
    }
    if (!e) {
        return -1;
    }
    file->private_data = e;
    file->size = e->size;
    file->offset = 0;
    return 0;
}

static int tmpfs_close(vfs_file_t* file) {
    (void)file;
    return 0;
}

static ssize_t tmpfs_read(vfs_file_t* file, void* buf, size_t count) {
    tmpfs_file_entry_t* e = (tmpfs_file_entry_t*)file->private_data;
    if (!e || !buf) {
        return -1;
    }
    if (file->offset >= e->size) {
        return 0;
    }
    size_t avail = e->size - file->offset;
    if (count > avail) {
        count = avail;
    }
    memcpy(buf, e->data + file->offset, count);
    file->offset += count;
    return (ssize_t)count;
}

static ssize_t tmpfs_write(vfs_file_t* file, const void* buf, size_t count) {
    tmpfs_file_entry_t* e = (tmpfs_file_entry_t*)file->private_data;
    if (!e || !buf) {
        return -1;
    }
    if (file->offset + count > e->capacity) {
        count = e->capacity - file->offset;
    }
    memcpy(e->data + file->offset, buf, count);
    file->offset += count;
    if (file->offset > e->size) {
        e->size = file->offset;
        file->size = e->size;
    }
    return (ssize_t)count;
}

static int tmpfs_seek(vfs_file_t* file, ssize_t offset, int whence) {
    tmpfs_file_entry_t* e = (tmpfs_file_entry_t*)file->private_data;
    if (!e) {
        return -1;
    }
    if (whence == 0) {
        file->offset = (u64)offset;
    } else if (whence == 1) {
        file->offset += (u64)offset;
    } else if (whence == 2) {
        file->offset = e->size + (u64)offset;
    }
    if (file->offset > e->size) {
        file->offset = e->size;
    }
    return 0;
}

static int tmpfs_readdir(vfs_file_t* file, vfs_dirent_t* dirent) {
    (void)file;
    (void)dirent;
    return -1;
}

static vfs_ops_t tmpfs_file_ops = {
    .open = tmpfs_open,
    .close = tmpfs_close,
    .read = tmpfs_read,
    .write = tmpfs_write,
    .seek = tmpfs_seek,
    .readdir = tmpfs_readdir
};

static int tmpfs_mount_fs(const char* source, const char* target) {
    (void)source;
    (void)target;
    tmpfs_size = 10 * 1024 * 1024;
    tmpfs_root = vmm_alloc_pages((tmpfs_size + PAGE_SIZE - 1) / PAGE_SIZE);
    memset(tmpfs_files, 0, sizeof(tmpfs_files));
    if (!tmpfs_root) {
        return -1;
    }
    DEBUG_INFO("Tmpfs mounted: %u KB", (u32)(tmpfs_size / 1024));
    return 0;
}

static int tmpfs_unmount_fs(const char* target) {
    (void)target;
    for (u32 i = 0; i < TMPFS_MAX_FILES; i++) {
        if (tmpfs_files[i].used && tmpfs_files[i].data) {
            kfree(tmpfs_files[i].data);
        }
    }
    if (tmpfs_root) {
        vmm_free_pages(tmpfs_root, (tmpfs_size + PAGE_SIZE - 1) / PAGE_SIZE);
        tmpfs_root = NULL;
    }
    return 0;
}

static vfs_fs_ops_t tmpfs_fs_ops = {
    .mount = tmpfs_mount_fs,
    .unmount = tmpfs_unmount_fs,
    .get_ops = NULL
};

int tmpfs_init(void) {
    DEBUG_INFO("Tmpfs initialized");
    return 0;
}

int tmpfs_mount(const char* mountpoint, size_t size) {
    tmpfs_size = size;
    return vfs_mount("tmpfs", mountpoint, &tmpfs_fs_ops);
}

vfs_fs_ops_t* tmpfs_get_fs_ops(void) {
    return &tmpfs_fs_ops;
}

vfs_ops_t* tmpfs_get_file_ops(void) {
    return &tmpfs_file_ops;
}
