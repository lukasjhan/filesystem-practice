#ifndef _LINUX_EXT2_FS_H
#define _LINUX_EXT2_FS_H

#include "Partition.h"	

#define min(a,b)            (((a) < (b)) ? (a) : (b))
#define EXT2_BAD_INO             1      /* Bad blocks inode */
#define EXT2_ROOT_INO            2      /* Root inode */
#define EXT2_GOOD_OLD_FIRST_INO 11

typedef struct ext2_group_desc {
    U32  block_bitmap;         /* Blocks bitmap block */
    U32  inode_bitmap;         /* Inodes bitmap block */
    U32  inode_table;         /* Inodes table block */
    U16  free_blocks_count;   /* Free blocks count */
    U16  free_inodes_count;   /* Free inodes count */
    U16  used_dirs_count;     /* Directories count */
    U16  pad;
    U32  reserved[3];
} GROUP_DESC;

#define EXT2_NDIR_BLOCKS                12
#define EXT2_IND_BLOCK                  EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK                 (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK                 (EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS                   (EXT2_TIND_BLOCK + 1)

// Revision levels
#define EXT2_GOOD_OLD_REV       0       /* The good old (original) format */
#define EXT2_GOOD_OLD_INODE_SIZE 128

typedef struct ext2_inode {
    U16  i_mode;         /* File mode */
    U16  i_uid;          /* Low 16 bits of Owner Uid */
    U32  i_size;         /* Size in bytes */
    U32  i_atime;        /* Access time */
    U32  i_ctime;        /* Creation time */
    U32  i_mtime;        /* Modification time */
    U32  i_dtime;        /* Deletion Time */
    U16  i_gid;          /* Low 16 bits of Group Id */
    U16  i_links_count;  /* Links count */
    U32  i_blocks;       /* Blocks count */
    U32  i_flags;        /* File flags */
    U32  l_i_reserved1;	// OS dependent 1
    U32  i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
    U32  i_generation;   /* File version (for NFS) */
    U32  i_file_acl;     /* File ACL */
    U32  i_dir_acl;      /* Directory ACL */
    U32  i_faddr;        /* Fragment address */
    U32  l_i_reserved2[3];	// OS dependent 2
} INODE;

typedef struct ext2_super_block {
    U32  inodes_count;         /* Inodes count */
    U32  blocks_count;         /* Blocks count */
    U32  r_blocks_count;       /* Reserved blocks count */
    U32  free_blocks_count;    /* Free blocks count */
    U32  free_inodes_count;    /* Free inodes count */
    U32  first_data_block;     /* First Data Block */
    U32  log_block_size;       /* Block size */
    U32  log_frag_size;        /* Fragment size */
    U32  blocks_per_group;     /* # Blocks per group */
    U32  frags_per_group;      /* # Fragments per group */
    U32  inodes_per_group;     /* # Inodes per group */
    U32  mtime;                /* Mount time */
    U32  wtime;                /* Write time */
    U16  mnt_count;            /* Mount count */
    U16  max_mnt_count;        /* Maximal mount count */
    U16  magic;                /* Magic signature */
    U16  state;                /* File system state */
    U16  errors;               /* Behaviour when detecting errors */
    U16  minor_rev_level;      /* minor revision level */
    U32  lastcheck;            /* time of last check */
    U32  checkinterval;        /* max. time between checks */
    U32  creator_os;           /* OS */
    U32  rev_level;            /* Revision level */
    U16  def_resuid;           /* Default uid for reserved blocks */
    U16  def_resgid;           /* Default gid for reserved blocks */

    U32  first_ino;            /* First non-reserved inode */
    U16  inode_size;          /* size of inode structure */
    U16  block_group_nr;       /* block group # of this superblock */
    U32  feature_compat;       /* compatible feature set */
    U32  feature_incompat;     /* incompatible feature set */
    U32  feature_ro_compat;    /* readonly-compatible feature set */
    U8   uuid[16];             /* 128-bit uuid for volume */
    char volume_name[16];      /* volume name */
    char last_mounted[64];     /* directory where last mounted */
    U32  algorithm_usage_bitmap; /* For compression */

    U8   prealloc_blocks;      /* Nr of blocks to try to preallocate*/
    U8   prealloc_dir_blocks;  /* Nr to preallocate for dirs */
    U16  padding1;
	
    U8    journal_uuid[16];     /* uuid of journal superblock */
    U32   journal_inum;         /* inode number of journal file */
    U32   journal_dev;          /* device number of journal file */
    U32   last_orphan;          /* start of list of inodes to delete */
    U32   hash_seed[4];         /* HTREE hash seed */
    U8    def_hash_version;     /* Default hash version to use */
    U8    reserved_char_pad;
    U16   reserved_word_pad;
    U32   default_mount_opts;
    U32   first_meta_bg;        /* First metablock block group */
    U32   reserved[190];        /* Padding to the end of the block */
} SUPERBLOCK;

#define EXT2_NAME_LEN 255

typedef struct ext2_dir_entry {
    U32  inode;                  /* Inode number */
    U16  rec_len;                /* Directory entry length */
    U16  name_len;               /* Name length */
    char name[EXT2_NAME_LEN];    /* File name */
} DIRENTRY_1;


typedef struct ext2_dir_entry_2 {
    U32  inode;                  /* Inode number */
    U16  rec_len;                /* Directory entry length */
    U8   name_len;               /* Name length */
    U8   file_type;
    char name[EXT2_NAME_LEN];    /* File name */
} DIRENTRY;

typedef struct ext2_sb_info {
	U32 inodes_per_block;   
	U32 blocks_per_group;     
	U32 inodes_per_group;     
	U32 itb_per_group;  
	U32 gdb_count;      
	U32 desc_per_block; 
	U32 groups_count;     
	SUPERBLOCK * sb; 
	GROUP_DESC * group_desc;  
	int addr_per_block_bits;
	int desc_per_block_bits;
	int inode_size;	 
	int first_ino;    
	int blocksize;     
	int lba_per_block;	
	int ext2_block_size_bits;
} SBINFO;

#endif  /* _LINUX_EXT2_FS_H */
