#include "ext2.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "drivers/ata.h"
#include "string.h"

#define EXT2_MAGIC 0xEF53
#define EXT2_ROOT_INO 2

static ext2_superblock_t* superblock = NULL;
static ext2_bg_desc_t* bg_descriptors = NULL;
static u32 block_size = EXT2_BLOCK_SIZE;
static void* block_buffer = NULL;

/* Read block from device */
static int ext2_read_block(u32 block_num, void* buffer) {
    u64 lba = (u64)block_num * (block_size / 512);
    return ata_read_sectors(lba, block_size / 512, buffer);
}

/* Write block to device */
static int ext2_write_block(u32 block_num, const void* buffer) {
    u64 lba = (u64)block_num * (block_size / 512);
    return ata_write_sectors(lba, block_size / 512, buffer);
}

/* Read inode */
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

/* VFS operations */
static int ext2_open(vfs_file_t* file, const char* path) {
    (void)file;
    (void)path;
    /* Simplified - would lookup inode here */
    return 0;
}

static int ext2_close(vfs_file_t* file) {
    (void)file;
    return 0;
}

static ssize_t ext2_read(vfs_file_t* file, void* buf, size_t count) {
    (void)file;
    (void)buf;
    (void)count;
    /* Simplified - would read from inode blocks */
    return 0;
}

static ssize_t ext2_write(vfs_file_t* file, const void* buf, size_t count) {
    (void)file;
    (void)buf;
    (void)count;
    /* Simplified - would write to inode blocks */
    return 0;
}

static int ext2_seek(vfs_file_t* file, ssize_t offset, int whence) {
    (void)file;
    (void)offset;
    (void)whence;
    return 0;
}

static int ext2_readdir(vfs_file_t* file, vfs_dirent_t* dirent) {
    (void)file;
    (void)dirent;
    return 0;
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
    
    /* Allocate buffers */
    superblock = (ext2_superblock_t*)kmalloc(sizeof(ext2_superblock_t));
    bg_descriptors = (ext2_bg_desc_t*)kmalloc(32 * sizeof(ext2_bg_desc_t));
    block_buffer = kmalloc(block_size);
    
    if (!superblock || !bg_descriptors || !block_buffer) {
        DEBUG_ERROR("Failed to allocate EXT2 buffers");
        return -1;
    }
    
    /* Read superblock (block 1, offset 1024) */
    if (ext2_read_block(1, block_buffer) < 0) {
        DEBUG_ERROR("Failed to read superblock");
        return -1;
    }
    
    memcpy(superblock, (u8*)block_buffer + 1024, sizeof(ext2_superblock_t));
    
    if (superblock->magic != EXT2_MAGIC) {
        DEBUG_ERROR("Invalid EXT2 magic: 0x%x", superblock->magic);
        return -1;
    }
    
    block_size = 1024 << superblock->log_block_size;
    
    /* Read block group descriptors */
    u32 bgd_block = 2;
    if (ext2_read_block(bgd_block, block_buffer) < 0) {
        DEBUG_ERROR("Failed to read block group descriptors");
        return -1;
    }
    
    memcpy(bg_descriptors, block_buffer, 32 * sizeof(ext2_bg_desc_t));
    
    DEBUG_INFO("EXT2 filesystem mounted: %u blocks, %u inodes",
               superblock->blocks_count, superblock->inodes_count);
    
    return 0;
}

static int ext2_unmount_fs(const char* target) {
    (void)target;
    
    if (superblock) kfree(superblock);
    if (bg_descriptors) kfree(bg_descriptors);
    if (block_buffer) kfree(block_buffer);
    
    superblock = NULL;
    bg_descriptors = NULL;
    block_buffer = NULL;
    
    return 0;
}

static vfs_fs_ops_t ext2_fs_ops = {
    .mount = ext2_mount_fs,
    .unmount = ext2_unmount_fs,
    .get_ops = NULL
};

int ext2_init(void) {
    DEBUG_INFO("EXT2 filesystem driver initialized");
    return 0;
}

int ext2_mount(const char* device, const char* mountpoint) {
    return vfs_mount(device, mountpoint, &ext2_fs_ops);
}

vfs_fs_ops_t* ext2_get_fs_ops(void) {
    return &ext2_fs_ops;
}
