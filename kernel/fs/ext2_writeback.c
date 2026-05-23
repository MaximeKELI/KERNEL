#include "ext2.h"
#include "cache.h"
#include "drivers/ata.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"

extern ext2_inode_t* ext2_inode_from_vfs(void* private_data);
extern u32 ext2_block_size_val(void);
extern int ext2_write_inode_blocks(ext2_inode_t* inode, u64 file_offset, const void* buf, size_t len);

void ext2_journal_log(u32 ino, u32 block, u32 op);

int ext2_writeback_page(u64 ino, u64 file_offset, const void* page_data) {
    if (!page_data) {
        return -1;
    }
    ext2_journal_log((u32)ino, (u32)(file_offset / PAGE_SIZE), 2);

    ext2_inode_t inode;
    memset(&inode, 0, sizeof(inode));
    inode.size = (u32)(file_offset + PAGE_SIZE);
    inode.block[0] = (u32)(file_offset / PAGE_SIZE) + 1;

    return ext2_write_inode_blocks(&inode, file_offset, page_data, PAGE_SIZE);
}

int ext2_write_inode_blocks(ext2_inode_t* inode, u64 file_offset, const void* buf, size_t len) {
    if (!inode || !buf || len == 0) {
        return -1;
    }
    u32 bsize = 1024;
    u32 block_idx = (u32)(file_offset / bsize);
    if (block_idx >= 12) {
        return -1;
    }
    u32 lba = (u64)inode->block[block_idx] * (bsize / 512);
    if (inode->block[block_idx] == 0) {
        inode->block[block_idx] = block_idx + 100;
        lba = (u64)inode->block[block_idx] * (bsize / 512);
    }
    return ata_write_sectors(lba, (bsize / 512), buf);
}
