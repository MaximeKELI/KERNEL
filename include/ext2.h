#ifndef EXT2_H
#define EXT2_H

#include "types.h"
#include "fs/vfs.h"

/* EXT2 filesystem implementation */

/* Block size */
#define EXT2_BLOCK_SIZE 1024

/* Superblock */
typedef struct __packed {
    u32 inodes_count;
    u32 blocks_count;
    u32 reserved_blocks;
    u32 free_blocks;
    u32 free_inodes;
    u32 first_data_block;
    u32 log_block_size;
    u32 log_frag_size;
    u32 blocks_per_group;
    u32 frags_per_group;
    u32 inodes_per_group;
    u32 mtime;
    u32 wtime;
    u16 mnt_count;
    u16 max_mnt_count;
    u16 magic;
    u16 state;
    u16 errors;
    u16 minor_rev_level;
    u32 lastcheck;
    u32 checkinterval;
    u32 creator_os;
    u32 rev_level;
    u16 def_resuid;
    u16 def_resgid;
    u32 first_ino;
    u16 inode_size;
    u16 block_group_nr;
    u32 feature_compat;
    u32 feature_incompat;
    u32 feature_ro_compat;
    u8 uuid[16];
    char volume_name[16];
    char last_mounted[64];
    u32 algorithm_usage_bitmap;
    u8 prealloc_blocks;
    u8 prealloc_dir_blocks;
    u16 reserved_gdt_blocks;
    u8 journal_uuid[16];
    u32 journal_inum;
    u32 journal_dev;
    u32 last_orphan;
    u32 hash_seed[4];
    u8 def_hash_version;
    u8 reserved_char_pad;
    u16 reserved_word_pad;
    u32 default_mount_opts;
    u32 first_meta_bg;
    u32 unused[190];
} ext2_superblock_t;

/* Block group descriptor */
typedef struct __packed {
    u32 block_bitmap;
    u32 inode_bitmap;
    u32 inode_table;
    u16 free_blocks_count;
    u16 free_inodes_count;
    u16 used_dirs_count;
    u16 pad;
    u8 reserved[12];
} ext2_bg_desc_t;

/* Inode structure */
typedef struct __packed {
    u16 mode;
    u16 uid;
    u32 size;
    u32 atime;
    u32 ctime;
    u32 mtime;
    u32 dtime;
    u16 gid;
    u16 links_count;
    u32 blocks;
    u32 flags;
    u32 osd1;
    u32 block[15];
    u32 generation;
    u32 file_acl;
    u32 dir_acl;
    u32 faddr;
    u8 osd2[12];
} ext2_inode_t;

/* Directory entry */
typedef struct __packed {
    u32 inode;
    u16 rec_len;
    u8 name_len;
    u8 file_type;
    char name[];
} ext2_dirent_t;

/* Initialize EXT2 */
int ext2_init(void);

/* Mount EXT2 filesystem */
int ext2_mount(const char* device, const char* mountpoint);

/* Get filesystem operations */
vfs_fs_ops_t* ext2_get_fs_ops(void);
vfs_ops_t* ext2_get_file_ops(void);

#endif /* EXT2_H */
