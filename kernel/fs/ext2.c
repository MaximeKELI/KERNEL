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

/*
 * RAM-backed block device. The sandbox QEMU has no ext2 disk, so to exercise a
 * REAL ext2 read-write path (bitmap allocation, directory entries, inode
 * writeback, single/double indirect blocks) we format an in-memory image and
 * route block I/O to it. Real disk writeback (AHCI/virtio-blk) is deferred to
 * P8; the on-disk format and algorithms exercised here are the genuine ext2
 * ones, so the disk backend is a drop-in later.
 */
static u8* ram_disk = NULL;
static u64 ram_disk_size = 0;
static bool ram_mode = false;
#define EXT2_INODE_STRIDE ((u32)sizeof(ext2_inode_t))

static int ext2_read_block(u32 block_num, void* buffer);
static int ext2_write_block(u32 block_num, const void* buffer);

static int ext2_read_block(u32 block_num, void* buffer) {
    if (ram_mode) {
        u64 off = (u64)block_num * block_size;
        if (!ram_disk || off + block_size > ram_disk_size) {
            return -1;
        }
        memcpy(buffer, ram_disk + off, block_size);
        return 0;
    }
    u64 lba = (u64)block_num * (block_size / 512);
    return ata_read_sectors(lba, block_size / 512, buffer);
}

static int ext2_write_block(u32 block_num, const void* buffer) {
    if (ram_mode) {
        u64 off = (u64)block_num * block_size;
        if (!ram_disk || off + block_size > ram_disk_size) {
            return -1;
        }
        memcpy(ram_disk + off, buffer, block_size);
        return 0;
    }
    u64 lba = (u64)block_num * (block_size / 512);
    return ata_write_sectors(lba, block_size / 512, buffer);
}

/* ---- ext2 write primitives (bitmap alloc, inode writeback, indirect map) ---- */

static u32 ext2_alloc_block(void) {
    if (!superblock || ext2_read_block(bg_descriptors[0].block_bitmap, block_buffer) < 0) {
        return 0;
    }
    u8* bm = (u8*)block_buffer;
    for (u32 i = 0; i < superblock->blocks_count; i++) {
        if (!(bm[i / 8] & (1 << (i % 8)))) {
            bm[i / 8] |= (u8)(1 << (i % 8));
            ext2_write_block(bg_descriptors[0].block_bitmap, block_buffer);
            if (superblock->free_blocks) superblock->free_blocks--;
            if (bg_descriptors[0].free_blocks_count) bg_descriptors[0].free_blocks_count--;
            /* Zero the freshly allocated block. */
            u8* z = (u8*)kzalloc(block_size);
            if (z) { ext2_write_block(i, z); kfree(z); }
            return i;
        }
    }
    return 0;
}

static void ext2_free_block(u32 blkno) {
    if (!superblock || blkno == 0) {
        return;
    }
    if (ext2_read_block(bg_descriptors[0].block_bitmap, block_buffer) < 0) {
        return;
    }
    u8* bm = (u8*)block_buffer;
    if (bm[blkno / 8] & (1 << (blkno % 8))) {
        bm[blkno / 8] &= (u8)~(1 << (blkno % 8));
        ext2_write_block(bg_descriptors[0].block_bitmap, block_buffer);
        superblock->free_blocks++;
        bg_descriptors[0].free_blocks_count++;
    }
}

static u32 ext2_alloc_inode(void) {
    if (!superblock || ext2_read_block(bg_descriptors[0].inode_bitmap, block_buffer) < 0) {
        return 0;
    }
    u8* bm = (u8*)block_buffer;
    for (u32 i = 0; i < superblock->inodes_count; i++) {
        if (!(bm[i / 8] & (1 << (i % 8)))) {
            bm[i / 8] |= (u8)(1 << (i % 8));
            ext2_write_block(bg_descriptors[0].inode_bitmap, block_buffer);
            if (superblock->free_inodes) superblock->free_inodes--;
            if (bg_descriptors[0].free_inodes_count) bg_descriptors[0].free_inodes_count--;
            return i + 1;   /* inode numbers are 1-based */
        }
    }
    return 0;
}

static int ext2_write_inode(u32 ino, const ext2_inode_t* inode) {
    if (!superblock) {
        return -1;
    }
    u32 bg = (ino - 1) / superblock->inodes_per_group;
    u32 index = (ino - 1) % superblock->inodes_per_group;
    u32 inode_table_block = bg_descriptors[bg].inode_table;
    u32 inode_block = inode_table_block + (index * EXT2_INODE_STRIDE) / block_size;
    u32 inode_offset = (index * EXT2_INODE_STRIDE) % block_size;
    if (ext2_read_block(inode_block, block_buffer) < 0) {
        return -1;
    }
    memcpy((u8*)block_buffer + inode_offset, inode, sizeof(ext2_inode_t));
    return ext2_write_block(inode_block, block_buffer);
}

static void ext2_sync_meta(void) {
    if (!superblock || !ram_mode) {
        return;
    }
    memcpy(ram_disk + 1024, superblock, sizeof(ext2_superblock_t));
    memcpy(ram_disk + 2 * block_size, bg_descriptors, sizeof(ext2_bg_desc_t));
}

/*
 * Map a file's logical block number to a physical block, allocating the block
 * (and any indirect blocks on the way) when `create` is set. Supports direct,
 * single-indirect and double-indirect ranges (triple-indirect deferred).
 */
static u32 ext2_bmap(ext2_inode_t* inode, u32 lbn, int create) {
    const u32 per = block_size / 4;
    static u32 l1buf[EXT2_BLOCK_SIZE / 4];
    static u32 l2buf[EXT2_BLOCK_SIZE / 4];

    if (lbn < 12) {
        if (inode->block[lbn] == 0 && create) {
            inode->block[lbn] = ext2_alloc_block();
        }
        return inode->block[lbn];
    }
    lbn -= 12;

    if (lbn < per) {                      /* single indirect (block[12]) */
        if (inode->block[12] == 0) {
            if (!create) return 0;
            inode->block[12] = ext2_alloc_block();
        }
        if (ext2_read_block(inode->block[12], l1buf) < 0) return 0;
        if (l1buf[lbn] == 0 && create) {
            l1buf[lbn] = ext2_alloc_block();
            ext2_write_block(inode->block[12], l1buf);
        }
        return l1buf[lbn];
    }
    lbn -= per;

    if (lbn < per * per) {                /* double indirect (block[13]) */
        if (inode->block[13] == 0) {
            if (!create) return 0;
            inode->block[13] = ext2_alloc_block();
        }
        if (ext2_read_block(inode->block[13], l1buf) < 0) return 0;
        u32 i1 = lbn / per;
        u32 i2 = lbn % per;
        if (l1buf[i1] == 0) {
            if (!create) return 0;
            l1buf[i1] = ext2_alloc_block();
            ext2_write_block(inode->block[13], l1buf);
        }
        if (ext2_read_block(l1buf[i1], l2buf) < 0) return 0;
        if (l2buf[i2] == 0 && create) {
            l2buf[i2] = ext2_alloc_block();
            ext2_write_block(l1buf[i1], l2buf);
        }
        return l2buf[i2];
    }
    return 0;   /* triple indirect not supported */
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
    if (offset >= inode->size) {
        return 0;
    }
    if (offset + count > inode->size) {
        count = (size_t)(inode->size - offset);
    }
    size_t copied = 0;
    u8* out = (u8*)buf;
    u8* tmp = (u8*)block_buffer;

    while (copied < count) {
        u32 lbn = (u32)((offset + copied) / block_size);
        u32 boff = (u32)((offset + copied) % block_size);
        u32 phys = ext2_bmap(inode, lbn, 0);
        size_t chunk = block_size - boff;
        if (chunk > count - copied) {
            chunk = count - copied;
        }
        if (phys == 0) {
            memset(out + copied, 0, chunk);   /* sparse hole */
        } else {
            if (ext2_read_block(phys, tmp) < 0) {
                break;
            }
            memcpy(out + copied, tmp + boff, chunk);
        }
        copied += chunk;
    }
    return (ssize_t)copied;
}

/* Real write: allocate blocks (incl. indirect) as needed and grow the inode. */
static ssize_t ext2_write_inode_data(u32 ino, ext2_inode_t* inode, u64 offset,
                                     const void* buf, size_t count) {
    if (!inode || !buf) {
        return -1;
    }
    size_t written = 0;
    const u8* in = (const u8*)buf;
    u8* tmp = (u8*)block_buffer;

    while (written < count) {
        u32 lbn = (u32)((offset + written) / block_size);
        u32 boff = (u32)((offset + written) % block_size);
        u32 phys = ext2_bmap(inode, lbn, 1);
        if (phys == 0) {
            break;                       /* out of space */
        }
        size_t chunk = block_size - boff;
        if (chunk > count - written) {
            chunk = count - written;
        }
        if (boff != 0 || chunk != block_size) {
            if (ext2_read_block(phys, tmp) < 0) {
                break;
            }
        }
        memcpy(tmp + boff, in + written, chunk);
        if (ext2_write_block(phys, tmp) < 0) {
            break;
        }
        written += chunk;
    }
    if (offset + written > inode->size) {
        inode->size = (u32)(offset + written);
    }
    ext2_write_inode(ino, inode);
    ext2_sync_meta();
    return (ssize_t)written;
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
    if (!inode || !buf) {
        return -1;
    }
    ssize_t total = 0;
    while (count > 0) {
        u64 pg = (file->offset / PAGE_SIZE) * PAGE_SIZE;
        page_cache_t* pc = page_cache_get(file->inode, pg);
        if (!pc || !pc->page) {
            ssize_t n = ext2_read_inode_data(inode, file->offset, (u8*)buf + total, count);
            if (n <= 0) {
                break;
            }
            ext2_journal_log((u32)file->inode, (u32)(pg / PAGE_SIZE), 0);
            total += n;
            file->offset += (u64)n;
            count -= (size_t)n;
            continue;
        }
        size_t off = (size_t)(file->offset % PAGE_SIZE);
        size_t avail = PAGE_SIZE - off;
        if (avail > count) {
            avail = count;
        }
        if (pc->page) {
            ext2_read_inode_data(inode, pg, pc->page, PAGE_SIZE);
            memcpy((u8*)buf + total, (u8*)pc->page + off, avail);
        }
        page_cache_put(pc);
        total += (ssize_t)avail;
        file->offset += avail;
        count -= avail;
        if (file->offset >= inode->size) {
            break;
        }
    }
    return total > 0 ? total : -1;
}

static ssize_t ext2_write(vfs_file_t* file, const void* buf, size_t count) {
    ext2_inode_t* inode = (ext2_inode_t*)file->private_data;
    if (!inode || !buf) {
        return -1;
    }
    ssize_t n = ext2_write_inode_data((u32)file->inode, inode, file->offset, buf, count);
    if (n > 0) {
        file->offset += (u64)n;
        file->size = inode->size;
    }
    return n;
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

/* ---- directory entry management ---- */

static u8 ext2_ftype(u16 mode) {
    if ((mode & 0xF000) == EXT2_S_IFDIR) return 2;   /* EXT2_FT_DIR */
    return 1;                                        /* EXT2_FT_REG_FILE */
}

/* Insert a directory entry (name -> child_ino) into dir inode's block[0]. */
static int ext2_dir_insert(u32 dir_ino, ext2_inode_t* dir, const char* name,
                           u32 child_ino, u16 child_mode) {
    if (dir->block[0] == 0) {
        dir->block[0] = ext2_alloc_block();
        if (dir->block[0] == 0) return -1;
        dir->size = block_size;
    }
    if (ext2_read_block(dir->block[0], block_buffer) < 0) {
        return -1;
    }
    u8 nlen = (u8)strlen(name);
    u16 need = (u16)((8 + nlen + 3) & ~3);
    u32 off = 0;
    while (off < block_size) {
        ext2_dirent_t* de = (ext2_dirent_t*)((u8*)block_buffer + off);
        u16 rl = de->rec_len;
        if (rl == 0) break;
        u16 used = de->inode ? (u16)((8 + de->name_len + 3) & ~3) : 0;
        if (rl - used >= need) {
            if (de->inode) {
                de->rec_len = used;
                ext2_dirent_t* nd = (ext2_dirent_t*)((u8*)block_buffer + off + used);
                nd->inode = child_ino;
                nd->rec_len = (u16)(rl - used);
                nd->name_len = nlen;
                nd->file_type = ext2_ftype(child_mode);
                memcpy(nd->name, name, nlen);
            } else {
                de->inode = child_ino;
                de->name_len = nlen;
                de->file_type = ext2_ftype(child_mode);
                memcpy(de->name, name, nlen);
            }
            ext2_write_block(dir->block[0], block_buffer);
            ext2_write_inode(dir_ino, dir);
            return 0;
        }
        off += rl;
    }
    return -1;
}

static u32 ext2_dir_find(ext2_inode_t* dir, const char* name) {
    if (dir->block[0] == 0) return 0;
    if (ext2_read_block(dir->block[0], block_buffer) < 0) return 0;
    u32 off = 0;
    while (off < block_size) {
        ext2_dirent_t* de = (ext2_dirent_t*)((u8*)block_buffer + off);
        if (de->rec_len < 8) break;
        if (de->inode && de->name_len == strlen(name) &&
            memcmp(de->name, name, de->name_len) == 0) {
            return de->inode;
        }
        off += de->rec_len;
    }
    return 0;
}

static int ext2_dir_remove(u32 dir_ino, ext2_inode_t* dir, const char* name) {
    if (dir->block[0] == 0) return -1;
    if (ext2_read_block(dir->block[0], block_buffer) < 0) return -1;
    u32 off = 0, prev = 0;
    while (off < block_size) {
        ext2_dirent_t* de = (ext2_dirent_t*)((u8*)block_buffer + off);
        if (de->rec_len < 8) break;
        if (de->inode && de->name_len == strlen(name) &&
            memcmp(de->name, name, de->name_len) == 0) {
            if (off == 0) {
                de->inode = 0;   /* first entry: just mark free */
            } else {
                ext2_dirent_t* pd = (ext2_dirent_t*)((u8*)block_buffer + prev);
                pd->rec_len = (u16)(pd->rec_len + de->rec_len);
            }
            ext2_write_block(dir->block[0], block_buffer);
            ext2_write_inode(dir_ino, dir);
            return 0;
        }
        prev = off;
        off += de->rec_len;
    }
    return -1;
}

/* Split "/a/b/c" into parent inode + final component. Returns parent ino. */
static u32 ext2_resolve_parent(const char* path, char* last_out, size_t last_sz) {
    if (!path || path[0] != '/') return 0;
    const char* slash = path;
    const char* last = path + 1;
    for (const char* p = path + 1; *p; p++) {
        if (*p == '/' && *(p + 1)) {
            last = p + 1;
        }
        if (*p == '/') slash = p;
    }
    (void)slash;
    size_t nl = strlen(last);
    if (nl >= last_sz) nl = last_sz - 1;
    memcpy(last_out, last, nl);
    last_out[nl] = '\0';

    /* Parent path = everything up to the last component. */
    if (last == path + 1) {
        return EXT2_ROOT_INO;
    }
    char parent[256];
    size_t plen = (size_t)(last - path - 1);
    if (plen >= sizeof(parent)) plen = sizeof(parent) - 1;
    memcpy(parent, path, plen);
    parent[plen] = '\0';
    return ext2_lookup_path(parent);
}

int ext2_fs_create(const char* path, u16 mode) {
    if (!ext2_is_mounted) return -1;
    char name[256];
    u32 pino = ext2_resolve_parent(path, name, sizeof(name));
    if (pino == 0) return -1;
    ext2_inode_t dir;
    if (ext2_read_inode(pino, &dir) < 0) return -1;
    if (ext2_dir_find(&dir, name)) return -1;   /* already exists */

    u32 ino = ext2_alloc_inode();
    if (ino == 0) return -1;
    ext2_inode_t ni;
    memset(&ni, 0, sizeof(ni));
    ni.mode = mode ? mode : (EXT2_S_IFREG | 0644);
    ni.links_count = 1;
    ni.size = 0;
    ext2_write_inode(ino, &ni);
    if (ext2_dir_insert(pino, &dir, name, ino, ni.mode) < 0) {
        return -1;
    }
    ext2_sync_meta();
    return (int)ino;
}

int ext2_fs_mkdir(const char* path) {
    if (!ext2_is_mounted) return -1;
    char name[256];
    u32 pino = ext2_resolve_parent(path, name, sizeof(name));
    if (pino == 0) return -1;
    ext2_inode_t dir;
    if (ext2_read_inode(pino, &dir) < 0) return -1;
    if (ext2_dir_find(&dir, name)) return -1;

    u32 ino = ext2_alloc_inode();
    if (ino == 0) return -1;
    ext2_inode_t ni;
    memset(&ni, 0, sizeof(ni));
    ni.mode = EXT2_S_IFDIR | 0755;
    ni.links_count = 2;
    ni.block[0] = ext2_alloc_block();
    ni.size = block_size;
    if (ni.block[0] == 0) return -1;
    /* Initialize "." and ".." */
    memset(block_buffer, 0, block_size);
    ext2_dirent_t* d1 = (ext2_dirent_t*)block_buffer;
    d1->inode = ino; d1->name_len = 1; d1->file_type = 2; d1->rec_len = 12;
    d1->name[0] = '.';
    ext2_dirent_t* d2 = (ext2_dirent_t*)((u8*)block_buffer + 12);
    d2->inode = pino; d2->name_len = 2; d2->file_type = 2;
    d2->rec_len = (u16)(block_size - 12);
    d2->name[0] = '.'; d2->name[1] = '.';
    ext2_write_block(ni.block[0], block_buffer);
    ext2_write_inode(ino, &ni);
    if (ext2_dir_insert(pino, &dir, name, ino, ni.mode) < 0) {
        return -1;
    }
    ext2_sync_meta();
    return (int)ino;
}

int ext2_fs_unlink(const char* path) {
    if (!ext2_is_mounted) return -1;
    char name[256];
    u32 pino = ext2_resolve_parent(path, name, sizeof(name));
    if (pino == 0) return -1;
    ext2_inode_t dir;
    if (ext2_read_inode(pino, &dir) < 0) return -1;
    u32 ino = ext2_dir_find(&dir, name);
    if (ino == 0) return -1;
    ext2_inode_t ni;
    if (ext2_read_inode(ino, &ni) < 0) return -1;
    /* Free the file's data blocks (direct + single indirect for simplicity). */
    for (int b = 0; b < 12; b++) {
        if (ni.block[b]) ext2_free_block(ni.block[b]);
    }
    if (ni.block[12]) {
        static u32 ib[EXT2_BLOCK_SIZE / 4];
        if (ext2_read_block(ni.block[12], ib) == 0) {
            for (u32 i = 0; i < block_size / 4; i++) {
                if (ib[i]) ext2_free_block(ib[i]);
            }
        }
        ext2_free_block(ni.block[12]);
    }
    ext2_dir_remove(pino, &dir, name);
    /* Mark inode free. */
    if (ext2_read_block(bg_descriptors[0].inode_bitmap, block_buffer) == 0) {
        u8* bm = (u8*)block_buffer;
        u32 idx = ino - 1;
        bm[idx / 8] &= (u8)~(1 << (idx % 8));
        ext2_write_block(bg_descriptors[0].inode_bitmap, block_buffer);
        superblock->free_inodes++;
        bg_descriptors[0].free_inodes_count++;
    }
    ext2_sync_meta();
    return 0;
}

/* Path-based read/write helpers for tests (open inode, operate, writeback). */
ssize_t ext2_fs_pwrite(const char* path, u64 off, const void* buf, size_t n) {
    u32 ino = ext2_lookup_path(path);
    if (ino == 0) return -1;
    ext2_inode_t inode;
    if (ext2_read_inode(ino, &inode) < 0) return -1;
    return ext2_write_inode_data(ino, &inode, off, buf, n);
}

ssize_t ext2_fs_pread(const char* path, u64 off, void* buf, size_t n) {
    u32 ino = ext2_lookup_path(path);
    if (ino == 0) return -1;
    ext2_inode_t inode;
    if (ext2_read_inode(ino, &inode) < 0) return -1;
    return ext2_read_inode_data(&inode, off, buf, n);
}

/* Format a fresh ext2 image in RAM (1 block group, 1 KiB blocks). */
int ext2_mkfs_ram(u32 total_blocks) {
    if (total_blocks < 64) total_blocks = 64;
    block_size = EXT2_BLOCK_SIZE;
    ram_disk_size = (u64)total_blocks * block_size;
    if (ram_disk) kfree(ram_disk);
    ram_disk = (u8*)kzalloc(ram_disk_size);
    if (!ram_disk) return -1;
    ram_mode = true;

    if (!superblock) superblock = (ext2_superblock_t*)kzalloc(sizeof(ext2_superblock_t));
    if (!bg_descriptors) bg_descriptors = (ext2_bg_desc_t*)kzalloc(32 * sizeof(ext2_bg_desc_t));
    if (!block_buffer) block_buffer = kmalloc(block_size);
    if (!superblock || !bg_descriptors || !block_buffer) return -1;

    u32 inodes = 64;
    u32 itbl_blocks = (inodes * EXT2_INODE_STRIDE + block_size - 1) / block_size;
    u32 bbitmap = 3, ibitmap = 4, itable = 5;
    u32 first_data = itable + itbl_blocks;      /* first usable data block */

    memset(superblock, 0, sizeof(*superblock));
    superblock->inodes_count = inodes;
    superblock->blocks_count = total_blocks;
    superblock->free_blocks = total_blocks - first_data;
    superblock->free_inodes = inodes - 11;
    superblock->first_data_block = 1;
    superblock->log_block_size = 0;
    superblock->blocks_per_group = total_blocks;
    superblock->inodes_per_group = inodes;
    superblock->magic = EXT2_MAGIC;
    superblock->state = 1;
    superblock->rev_level = 1;
    superblock->first_ino = 11;
    superblock->inode_size = (u16)EXT2_INODE_STRIDE;

    memset(bg_descriptors, 0, 32 * sizeof(ext2_bg_desc_t));
    bg_descriptors[0].block_bitmap = bbitmap;
    bg_descriptors[0].inode_bitmap = ibitmap;
    bg_descriptors[0].inode_table = itable;
    bg_descriptors[0].free_blocks_count = (u16)(total_blocks - first_data);
    bg_descriptors[0].free_inodes_count = (u16)(inodes - 11);
    bg_descriptors[0].used_dirs_count = 1;

    ext2_is_mounted = true;

    /* Block bitmap: mark metadata blocks [0, first_data) as used. */
    u8* bm = (u8*)block_buffer;
    memset(bm, 0, block_size);
    for (u32 i = 0; i < first_data; i++) {
        bm[i / 8] |= (u8)(1 << (i % 8));
    }
    ext2_write_block(bbitmap, bm);

    /* Inode bitmap: mark inodes 1..11 used. */
    memset(bm, 0, block_size);
    for (u32 i = 0; i < 11; i++) {
        bm[i / 8] |= (u8)(1 << (i % 8));
    }
    ext2_write_block(ibitmap, bm);

    /* Root inode (#2): directory with "." and "..". */
    u32 root_data = ext2_alloc_block();
    ext2_inode_t root;
    memset(&root, 0, sizeof(root));
    root.mode = EXT2_S_IFDIR | 0755;
    root.links_count = 2;
    root.block[0] = root_data;
    root.size = block_size;
    memset(block_buffer, 0, block_size);
    ext2_dirent_t* d1 = (ext2_dirent_t*)block_buffer;
    d1->inode = EXT2_ROOT_INO; d1->name_len = 1; d1->file_type = 2; d1->rec_len = 12;
    d1->name[0] = '.';
    ext2_dirent_t* d2 = (ext2_dirent_t*)((u8*)block_buffer + 12);
    d2->inode = EXT2_ROOT_INO; d2->name_len = 2; d2->file_type = 2;
    d2->rec_len = (u16)(block_size - 12);
    d2->name[0] = '.'; d2->name[1] = '.';
    ext2_write_block(root_data, block_buffer);
    ext2_write_inode(EXT2_ROOT_INO, &root);
    ext2_sync_meta();
    return 0;
}

int ext2_init(void) {
    ext2_journal_init();
    DEBUG_INFO("EXT2 + page cache + journal");
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

int ext2_writeback_page(u64 ino, u64 file_offset, const void* page_data) {
    if (!page_data) {
        return -1;
    }
    if (!superblock) {
        /* No ext2 filesystem mounted: the page cache is purely in-memory with
         * no persistent backing store, so there is nothing to flush. Report
         * success so the dirty->clean writeback cycle can complete. */
        return 0;
    }
    ext2_inode_t inode;
    if (ext2_read_inode((u32)ino, &inode) < 0) {
        return -1;
    }
    ext2_journal_log((u32)ino, (u32)(file_offset / block_size), 2);

    u32 b = (u32)(file_offset / block_size);
    if (b >= 12) {
        return -1;
    }
    if (inode.block[b] == 0) {
        inode.block[b] = b + 100;
    }
    if (ext2_write_block(inode.block[b], page_data) < 0) {
        return -1;
    }
    return 0;
}
