#include "ext2.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "drivers/ata.h"
#include "string.h"
#include "cache.h"

extern void ext2_journal_init(void);
extern void ext2_journal_log(u32 ino, u32 block, u32 op);

#define EXT2_MAGIC 0xEF53
#define EXT2_ROOT_INO 2
#define EXT2_S_IFREG 0x8000
#define EXT2_S_IFDIR 0x4000

static ext2_superblock_t* superblock = NULL;
static ext2_bg_desc_t* bg_descriptors = NULL;
static u32 block_size = EXT2_BLOCK_SIZE;
static void* block_buffer = NULL;
static bool ext2_is_mounted = false;

static int ext2_read_block(u32 block_num, void* buffer) {
    u64 lba = (u64)block_num * (block_size / 512);
    return ata_read_sectors(lba, block_size / 512, buffer);
}

static int ext2_read_inode(u32 ino, ext2_inode_t* inode) {
    u32 bg = (ino - 1) / superblock->inodes_per_group;
    u32 index = (ino - 1) % superblock->inodes_per_group;
    u32 inode_table_block = bg_descriptors[bg].inode_table;
    u32 inode_block = inode_table_block + (index * sizeof(ext2_inode_t)) / block_size;
    u32 inode_offset = (index * sizeof(ext2_inode_t)) % block_size;

    if (ext2_read_block(inode_block, block_buffer) < 0) {
        return -1;
    }
    memcpy(inode, (u8*)block_buffer + inode_offset, sizeof(ext2_inode_t));
    return 0;
}

static u32 ext2_lookup_path(const char* path) {
    u32 ino = EXT2_ROOT_INO;
    if (!path || path[0] != '/') {
        return 0;
    }
    if (strcmp(path, "/") == 0) {
        return ino;
    }

    char copy[256];
    strncpy(copy, path, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    char* p = copy + 1;
    while (*p) {
        char* slash = strchr(p, '/');
        if (slash) {
            *slash = '\0';
        }

        ext2_inode_t dir_inode;
        if (ext2_read_inode(ino, &dir_inode) < 0) {
            return 0;
        }
        if ((dir_inode.mode & 0xF000) != EXT2_S_IFDIR) {
            return 0;
        }

        u32 found = 0;
        for (int b = 0; b < 12 && !found; b++) {
            if (dir_inode.block[b] == 0) {
                break;
            }
            if (ext2_read_block(dir_inode.block[b], block_buffer) < 0) {
                break;
            }
            u32 off = 0;
            while (off < block_size) {
                ext2_dirent_t* de = (ext2_dirent_t*)((u8*)block_buffer + off);
                if (de->rec_len < 12) {
                    break;
                }
                if (de->inode != 0 && de->name_len > 0) {
                    char name[256];
                    memcpy(name, de->name, de->name_len);
                    name[de->name_len] = '\0';
                    if (strcmp(name, p) == 0) {
                        ino = de->inode;
                        found = 1;
                        break;
                    }
                }
                off += de->rec_len;
            }
        }
        if (!found) {
            return 0;
        }
        if (!slash) {
            break;
        }
        *slash = '/';
        p = slash + 1;
    }
    return ino;
}

static ssize_t ext2_read_inode_data(ext2_inode_t* inode, u64 offset, void* buf, size_t count) {
    if (!inode || !buf) {
        return -1;
    }
    size_t copied = 0;
    u8* out = (u8*)buf;

    for (int b = 0; b < 12 && copied < count; b++) {
        if (inode->block[b] == 0) {
            break;
        }
        if (ext2_read_block(inode->block[b], block_buffer) < 0) {
            return -1;
        }
        u64 block_off = (u64)b * block_size;
        if (offset >= block_off + block_size) {
            continue;
        }
        u64 start = (offset > block_off) ? (offset - block_off) : 0;
        size_t avail = block_size - (size_t)start;
        size_t need = count - copied;
        if (need > avail) {
            need = avail;
        }
        memcpy(out + copied, (u8*)block_buffer + start, need);
        copied += need;
        offset += need;
    }
    return (ssize_t)copied;
}

static int ext2_open(vfs_file_t* file, const char* path) {
    u32 ino = ext2_lookup_path(path);
    if (ino == 0) {
        return -1;
    }
    ext2_inode_t* inode = (ext2_inode_t*)kmalloc(sizeof(ext2_inode_t));
    if (!inode || ext2_read_inode(ino, inode) < 0) {
        kfree(inode);
        return -1;
    }
    file->inode = ino;
    file->size = inode->size;
    file->offset = 0;
    file->private_data = inode;
    return 0;
}

static int ext2_close(vfs_file_t* file) {
    if (file && file->private_data) {
        kfree(file->private_data);
        file->private_data = NULL;
    }
    return 0;
}

static ssize_t ext2_read(vfs_file_t* file, void* buf, size_t count) {
    ext2_inode_t* inode = (ext2_inode_t*)file->private_data;
    if (!inode) {
        return -1;
    }
    ssize_t n = ext2_read_inode_data(inode, file->offset, buf, count);
    if (n > 0) {
        file->offset += (u64)n;
    }
    return n;
}

static ssize_t ext2_write(vfs_file_t* file, const void* buf, size_t count) {
    (void)file;
    (void)buf;
    (void)count;
    return -1;
}

static int ext2_seek(vfs_file_t* file, ssize_t offset, int whence) {
    ext2_inode_t* inode = (ext2_inode_t*)file->private_data;
    if (!inode) {
        return -1;
    }
    if (whence == 0) {
        file->offset = (u64)offset;
    } else if (whence == 2) {
        file->offset = inode->size + (u64)offset;
    }
    return 0;
}

static int ext2_readdir(vfs_file_t* file, vfs_dirent_t* dirent) {
    (void)file;
    (void)dirent;
    return -1;
}

static vfs_ops_t ext2_file_ops = {
    .open = ext2_open,
    .close = ext2_close,
    .read = ext2_read,
    .write = ext2_write,
    .seek = ext2_seek,
    .readdir = ext2_readdir
};

static int ext2_mount_fs(const char* source, const char* target) {
    (void)source;
    (void)target;

    superblock = (ext2_superblock_t*)kmalloc(sizeof(ext2_superblock_t));
    bg_descriptors = (ext2_bg_desc_t*)kmalloc(32 * sizeof(ext2_bg_desc_t));
    block_buffer = kmalloc(block_size);
    if (!superblock || !bg_descriptors || !block_buffer) {
        return -1;
    }

    if (ext2_read_block(1, block_buffer) < 0) {
        return -1;
    }
    memcpy(superblock, (u8*)block_buffer + 1024, sizeof(ext2_superblock_t));
    if (superblock->magic != EXT2_MAGIC) {
        DEBUG_ERROR("Invalid EXT2 magic: 0x%x", superblock->magic);
        return -1;
    }

    block_size = 1024 << superblock->log_block_size;
    if (ext2_read_block(2, block_buffer) < 0) {
        return -1;
    }
    memcpy(bg_descriptors, block_buffer, 32 * sizeof(ext2_bg_desc_t));
    ext2_is_mounted = true;
    DEBUG_INFO("EXT2 mounted: %u blocks", superblock->blocks_count);
    return 0;
}

static int ext2_unmount_fs(const char* target) {
    (void)target;
    if (superblock) {
        kfree(superblock);
    }
    if (bg_descriptors) {
        kfree(bg_descriptors);
    }
    if (block_buffer) {
        kfree(block_buffer);
    }
    superblock = NULL;
    bg_descriptors = NULL;
    block_buffer = NULL;
    ext2_is_mounted = false;
    return 0;
}

static vfs_fs_ops_t ext2_fs_ops = {
    .mount = ext2_mount_fs,
    .unmount = ext2_unmount_fs,
    .get_ops = NULL
};

int ext2_init(void) {
    DEBUG_INFO("EXT2 driver initialized");
    return 0;
}

int ext2_mount(const char* device, const char* mountpoint) {
    return vfs_mount(device, mountpoint, &ext2_fs_ops);
}

vfs_fs_ops_t* ext2_get_fs_ops(void) {
    return &ext2_fs_ops;
}

vfs_ops_t* ext2_get_file_ops(void) {
    return &ext2_file_ops;
}
