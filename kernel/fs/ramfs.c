#include "fs/ramfs.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"

static ramfs_node_t* root = NULL;
static u64 next_ino = 1;

static ramfs_node_t* node_new(const char* name, u32 mode, ramfs_node_t* parent) {
    ramfs_node_t* n = (ramfs_node_t*)kzalloc(sizeof(ramfs_node_t));
    if (!n) {
        return NULL;
    }
    strncpy(n->name, name, RAMFS_NAME_MAX);
    n->name[RAMFS_NAME_MAX] = '\0';
    n->mode = mode;
    n->ino = next_ino++;
    n->parent = parent;
    n->nlink = 1;
    return n;
}

void ramfs_init(void) {
    if (root) {
        return;
    }
    root = node_new("/", RAMFS_IFDIR, NULL);
    root->parent = root;   /* ".." of root is root */
    root->nlink = 2;
    next_ino = 2;
}

/* Lazily bring up the root so the filesystem works even if vfs_init() (which
 * normally calls ramfs_init) has not run yet, e.g. in the minimal test boot. */
static void ramfs_ensure(void) {
    if (!root) {
        ramfs_init();
    }
}

ramfs_node_t* ramfs_root(void) {
    ramfs_ensure();
    return root;
}

static ramfs_node_t* dir_lookup(ramfs_node_t* dir, const char* name, size_t len) {
    if (!dir || (dir->mode & RAMFS_IFDIR) == 0) {
        return NULL;
    }
    if (len == 1 && name[0] == '.') {
        return dir;
    }
    if (len == 2 && name[0] == '.' && name[1] == '.') {
        return dir->parent;
    }
    for (ramfs_node_t* c = dir->children; c; c = c->sibling) {
        if (strlen(c->name) == len && strncmp(c->name, name, len) == 0) {
            return c;
        }
    }
    return NULL;
}

/* Resolve path to its node; if want_parent, return the parent dir and copy the
 * final component into last_out (len-limited). */
static ramfs_node_t* resolve_internal(const char* path, bool want_parent,
                                      char* last_out) {
    ramfs_ensure();
    if (!root || !path || path[0] != '/') {
        return NULL;
    }
    ramfs_node_t* cur = root;
    const char* p = path + 1;

    while (*p) {
        const char* start = p;
        while (*p && *p != '/') {
            p++;
        }
        size_t len = (size_t)(p - start);
        bool last = (*p == '\0') || (*p == '/' && *(p + 1) == '\0');

        if (len == 0) {          /* trailing or double slash */
            if (*p == '/') p++;
            continue;
        }
        if (last && want_parent) {
            if (last_out) {
                size_t cl = len > RAMFS_NAME_MAX ? RAMFS_NAME_MAX : len;
                memcpy(last_out, start, cl);
                last_out[cl] = '\0';
            }
            return cur;          /* the parent directory */
        }

        ramfs_node_t* next = dir_lookup(cur, start, len);
        if (!next) {
            return NULL;
        }
        cur = next;
        if (*p == '/') {
            p++;
        }
    }
    /* Path was "/" */
    return want_parent ? NULL : cur;
}

ramfs_node_t* ramfs_resolve(const char* path) {
    ramfs_ensure();
    if (path && strcmp(path, "/") == 0) {
        return root;
    }
    return resolve_internal(path, false, NULL);
}

static void dir_add(ramfs_node_t* dir, ramfs_node_t* child) {
    child->parent = dir;
    child->sibling = dir->children;
    dir->children = child;
    if (child->mode & RAMFS_IFDIR) {
        dir->nlink++;          /* the child's ".." references us */
    }
}

static ramfs_node_t* create_node(const char* path, u32 mode) {
    char name[RAMFS_NAME_MAX + 1];
    ramfs_node_t* parent = resolve_internal(path, true, name);
    if (!parent || (parent->mode & RAMFS_IFDIR) == 0) {
        return NULL;
    }
    if (dir_lookup(parent, name, strlen(name))) {
        return NULL;          /* already exists */
    }
    ramfs_node_t* n = node_new(name, mode, parent);
    if (!n) {
        return NULL;
    }
    if (mode & RAMFS_IFDIR) {
        n->nlink = 2;
    }
    dir_add(parent, n);
    return n;
}

ramfs_node_t* ramfs_create(const char* path, u32 mode) {
    ramfs_node_t* existing = ramfs_resolve(path);
    if (existing) {
        return (existing->mode & RAMFS_IFREG) ? existing : NULL;
    }
    return create_node(path, RAMFS_IFREG | (mode & 0x0FFF));
}

ramfs_node_t* ramfs_mkdir(const char* path) {
    return create_node(path, RAMFS_IFDIR | 0755);
}

static int dir_remove(ramfs_node_t* dir, ramfs_node_t* child) {
    ramfs_node_t** link = &dir->children;
    while (*link) {
        if (*link == child) {
            *link = child->sibling;
            if (child->mode & RAMFS_IFDIR) {
                dir->nlink--;
            }
            return 0;
        }
        link = &(*link)->sibling;
    }
    return -1;
}

int ramfs_unlink(const char* path) {
    ramfs_node_t* n = ramfs_resolve(path);
    if (!n || (n->mode & RAMFS_IFREG) == 0 || n == root) {
        return -1;
    }
    dir_remove(n->parent, n);
    if (n->data) {
        kfree(n->data);
    }
    kfree(n);
    return 0;
}

int ramfs_rmdir(const char* path) {
    ramfs_node_t* n = ramfs_resolve(path);
    if (!n || (n->mode & RAMFS_IFDIR) == 0 || n == root) {
        return -1;
    }
    if (n->children) {
        return -1;          /* not empty */
    }
    dir_remove(n->parent, n);
    kfree(n);
    return 0;
}

static int ensure_capacity(ramfs_node_t* n, u64 need) {
    if (need <= n->capacity) {
        return 0;
    }
    u64 cap = n->capacity ? n->capacity : 256;
    while (cap < need) {
        cap *= 2;
    }
    u8* data = (u8*)kmalloc(cap);
    if (!data) {
        return -1;
    }
    if (n->data) {
        memcpy(data, n->data, n->size);
        kfree(n->data);
    }
    memset(data + n->size, 0, cap - n->size);
    n->data = data;
    n->capacity = cap;
    return 0;
}

ssize_t ramfs_read(ramfs_node_t* n, u64 offset, void* buf, size_t count) {
    if (!n || (n->mode & RAMFS_IFREG) == 0 || !buf) {
        return -1;
    }
    if (offset >= n->size) {
        return 0;
    }
    u64 avail = n->size - offset;
    if (count > avail) {
        count = (size_t)avail;
    }
    memcpy(buf, n->data + offset, count);
    return (ssize_t)count;
}

ssize_t ramfs_write(ramfs_node_t* n, u64 offset, const void* buf, size_t count) {
    if (!n || (n->mode & RAMFS_IFREG) == 0 || !buf) {
        return -1;
    }
    if (ensure_capacity(n, offset + count) < 0) {
        return -1;
    }
    if (offset > n->size) {
        memset(n->data + n->size, 0, offset - n->size);   /* sparse hole */
    }
    memcpy(n->data + offset, buf, count);
    if (offset + count > n->size) {
        n->size = offset + count;
    }
    return (ssize_t)count;
}

int ramfs_truncate(ramfs_node_t* n, u64 size) {
    if (!n || (n->mode & RAMFS_IFREG) == 0) {
        return -1;
    }
    if (size > n->capacity && ensure_capacity(n, size) < 0) {
        return -1;
    }
    if (size > n->size) {
        memset(n->data + n->size, 0, size - n->size);
    }
    n->size = size;
    return 0;
}

int ramfs_readdir_index(ramfs_node_t* dir, u32 index,
                        char* name_out, u64* ino_out, u32* mode_out) {
    if (!dir || (dir->mode & RAMFS_IFDIR) == 0) {
        return 0;
    }
    if (index == 0) {
        if (name_out) strcpy(name_out, ".");
        if (ino_out) *ino_out = dir->ino;
        if (mode_out) *mode_out = RAMFS_IFDIR;
        return 1;
    }
    if (index == 1) {
        if (name_out) strcpy(name_out, "..");
        if (ino_out) *ino_out = dir->parent->ino;
        if (mode_out) *mode_out = RAMFS_IFDIR;
        return 1;
    }
    u32 i = 2;
    for (ramfs_node_t* c = dir->children; c; c = c->sibling, i++) {
        if (i == index) {
            if (name_out) { strncpy(name_out, c->name, RAMFS_NAME_MAX); name_out[RAMFS_NAME_MAX] = '\0'; }
            if (ino_out) *ino_out = c->ino;
            if (mode_out) *mode_out = c->mode;
            return 1;
        }
    }
    return 0;
}
