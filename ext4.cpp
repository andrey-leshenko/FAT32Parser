#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// https://blogs.oracle.com/linux/post/understanding-ext4-disk-layout-part-1

struct ext4_super_block {
/*00*/  u32 s_inodes_count;         /* Inodes count */
        u32 s_blocks_count_lo;      /* Blocks count */
        u32 s_r_blocks_count_lo;    /* Reserved blocks count */
        u32 s_free_blocks_count_lo; /* Free blocks count */
/*10*/  u32 s_free_inodes_count;    /* Free inodes count */
        u32 s_first_data_block;     /* First Data Block */
        u32 s_log_block_size;       /* Block size */
        u32 s_log_cluster_size;     /* Allocation cluster size */
/*20*/  u32 s_blocks_per_group;     /* # Blocks per group */
        u32 s_clusters_per_group;   /* # Clusters per group */
        u32 s_inodes_per_group;     /* # Inodes per group */
        u32 s_mtime;                /* Mount time */
/*30*/  u32 s_wtime;                /* Write time */
        u16 s_mnt_count;            /* Mount count */
        u16 s_max_mnt_count;        /* Maximal mount count */
        u16 s_magic;                /* Magic signature */
        u16 s_state;                /* File system state */
        u16 s_errors;               /* Behaviour when detecting errors */
        u16 s_minor_rev_level;      /* minor revision level */
/*40*/  u32 s_lastcheck;            /* time of last check */
        u32 s_checkinterval;        /* max. time between checks */
        u32 s_creator_os;           /* OS */
        u32 s_rev_level;            /* Revision level */
/*50*/  u16 s_def_resuid;           /* Default uid for reserved blocks */
        u16 s_def_resgid;           /* Default gid for reserved blocks */
        /*
         * These fields are for EXT4_DYNAMIC_REV Superblocks only.
         *
         * Note: the difference between the compatible feature set and
         * the incompatible feature set is that if there is a bit set
         * in the incompatible feature set that the kernel doesn't
         * know about, it should refuse to mount the filesystem.
         *
         * e2fsck's requirements are more strict; if it doesn't know
         * about a feature in either the compatible or incompatible
         * feature set, it must abort and not try to meddle with
         * things it doesn't understand...
         */
        u32 s_first_ino;            /* First non-reserved inode */
        u16 s_inode_size;           /* size of inode structure */
        u16 s_block_group_nr;       /* block group # of this Superblock */
        u32 s_feature_compat;       /* compatible feature set */
/*60*/  u32 s_feature_incompat;     /* incompatible feature set */
        u32 s_feature_ro_compat;    /* readonly-compatible feature set */
/*68*/  u8  s_uuid[16];             /* 128-bit uuid for volume */
/*78*/  char    s_volume_name[16];      /* volume name */
/*88*/  char    s_last_mounted[64]; /* directory where last mounted */
/*C8*/  u32 s_algorithm_usage_bitmap; /* For compression */
        /*
         * Performance hints.  Directory preallocation should only
         * happen if the EXT4_FEATURE_COMPAT_DIR_PREALLOC flag is on.
         */
        u8  s_prealloc_blocks;      /* Nr of blocks to try to preallocate*/
        u8  s_prealloc_dir_blocks;  /* Nr to preallocate for dirs */
        u16 s_reserved_gdt_blocks;  /* Per group desc for online growth */
        /*
         * Journaling support valid if EXT4_FEATURE_COMPAT_HAS_JOURNAL set.
         */
/*D0*/  u8  s_journal_uuid[16];     /* uuid of journal Superblock */
/*E0*/  u32 s_journal_inum;         /* inode number of journal file */
        u32 s_journal_dev;          /* device number of journal file */
        u32 s_last_orphan;          /* start of list of inodes to delete */
        u32 s_hash_seed[4];         /* HTREE hash seed */
        u8  s_def_hash_version;     /* Default hash version to use */
        u8  s_jnl_backup_type;
        u16 s_desc_size;            /* size of group descriptor */
/*100*/ u32 s_default_mount_opts;
        u32 s_first_meta_bg;        /* First metablock block group */
        u32 s_mkfs_time;            /* When the filesystem was created */
        u32 s_jnl_blocks[17];       /* Backup of the journal inode */
        /* 64bit support valid if EXT4_FEATURE_COMPAT_64BIT */
/*150*/ u32 s_blocks_count_hi;      /* Blocks count */
        u32 s_r_blocks_count_hi;    /* Reserved blocks count */
        u32 s_free_blocks_count_hi; /* Free blocks count */
        u16 s_min_extra_isize;      /* All inodes have at least # bytes */
        u16 s_want_extra_isize;     /* New inodes should reserve # bytes */
        u32 s_flags;                /* Miscellaneous flags */
        u16 s_raid_stride;          /* RAID stride */
        u16 s_mmp_update_interval;  /* # seconds to wait in MMP checking */
        u64 s_mmp_block;            /* Block for multi-mount protection */
        u32 s_raid_stripe_width;    /* blocks on all data disks (N*stride)*/
        u8  s_log_groups_per_flex;  /* FLEX_BG group size */
        u8  s_checksum_type;        /* metadata checksum algorithm used */
        u8  s_encryption_level;     /* versioning level for encryption */
        u8  s_reserved_pad;         /* Padding to next 32bits */
        u64 s_kbytes_written;       /* nr of lifetime kilobytes written */
        u32 s_snapshot_inum;        /* Inode number of active snapshot */
        u32 s_snapshot_id;          /* sequential ID of active snapshot */
        u64 s_snapshot_r_blocks_count; /* reserved blocks for active
                                              snapshot's future use */
        u32 s_snapshot_list;        /* inode number of the head of the
                                           on-disk snapshot list */
#define EXT4_S_ERR_START offsetof(struct ext4_super_block, s_error_count)
        u32 s_error_count;          /* number of fs errors */
        u32 s_first_error_time;     /* first time an error happened */
        u32 s_first_error_ino;      /* inode involved in first error */
        u64 s_first_error_block;    /* block involved of first error */
        u8  s_first_error_func[32];     /* function where the error happened */
        u32 s_first_error_line;     /* line number where error happened */
        u32 s_last_error_time;      /* most recent time of an error */
        u32 s_last_error_ino;       /* inode involved in last error */
        u32 s_last_error_line;      /* line number where error happened */
        u64 s_last_error_block;     /* block involved of last error */
        u8  s_last_error_func[32];      /* function where the error happened */
#define EXT4_S_ERR_END offsetof(struct ext4_super_block, s_mount_opts)
        u8  s_mount_opts[64];
        u32 s_usr_quota_inum;       /* inode for tracking user quota */
        u32 s_grp_quota_inum;       /* inode for tracking group quota */
        u32 s_overhead_clusters;    /* overhead blocks/clusters in fs */
        u32 s_backup_bgs[2];        /* groups with sparse_super2 SBs */
        u8  s_encrypt_algos[4];     /* Encryption algorithms in use  */
        u8  s_encrypt_pw_salt[16];  /* Salt used for string2key algorithm */
        u32 s_lpf_ino;              /* Location of the lost+found inode */
        u32 s_prj_quota_inum;       /* inode for tracking project quota */
        u32 s_checksum_seed;        /* crc32c(uuid) if csum_seed set */
        u8  s_wtime_hi;
        u8  s_mtime_hi;
        u8  s_mkfs_time_hi;
        u8  s_lastcheck_hi;
        u8  s_first_error_time_hi;
        u8  s_last_error_time_hi;
        u8  s_pad[2];
        u16 s_encoding;             /* Filename charset encoding */
        u16 s_encoding_flags;       /* Filename charset encoding flags */
        u32 s_reserved[95];         /* Padding to the end of the block */
        u32 s_checksum;             /* crc32c(Superblock) */
};

struct ext4_group_desc
{
        u32  bg_block_bitmap_lo;     /* Blocks bitmap block */
        u32  bg_inode_bitmap_lo;     /* Inodes bitmap block */
        u32  bg_inode_table_lo;      /* Inodes table block */
        u16  bg_free_blocks_count_lo;/* Free blocks count */
        u16  bg_free_inodes_count_lo;/* Free inodes count */
        u16  bg_used_dirs_count_lo;  /* Directories count */
        u16  bg_flags;               /* EXT4_BG_flags (INODE_UNINIT, etc) */
        u32  bg_exclude_bitmap_lo;   /* Exclude bitmap for snapshots */
        u16  bg_block_bitmap_csum_lo;/* crc32c(s_uuid+grp_num+bbitmap) LE */
        u16  bg_inode_bitmap_csum_lo;/* crc32c(s_uuid+grp_num+ibitmap) LE */
        u16  bg_itable_unused_lo;    /* Unused inodes count */
        u16  bg_checksum;            /* crc16(sb_uuid+group+desc) */
        u32  bg_block_bitmap_hi;     /* Blocks bitmap block MSB */
        u32  bg_inode_bitmap_hi;     /* Inodes bitmap block MSB */
        u32  bg_inode_table_hi;      /* Inodes table block MSB */
        u16  bg_free_blocks_count_hi;/* Free blocks count MSB */
        u16  bg_free_inodes_count_hi;/* Free inodes count MSB */
        u16  bg_used_dirs_count_hi;  /* Directories count MSB */
        u16  bg_itable_unused_hi;    /* Unused inodes count MSB */
        u32  bg_exclude_bitmap_hi;   /* Exclude bitmap block MSB */
        u16  bg_block_bitmap_csum_hi;/* crc32c(s_uuid+grp_num+bbitmap) BE */
        u16  bg_inode_bitmap_csum_hi;/* crc32c(s_uuid+grp_num+ibitmap) BE */
        u32  bg_reserved;
};


void print_ext4_super_block(ext4_super_block *s)
{
    printf("ext4_super_block:\n");
    printf("    s_inodes_count: %08x\n", s->s_inodes_count);
    printf("    s_blocks_count_lo: %08x\n", s->s_blocks_count_lo);
    printf("    s_r_blocks_count_lo: %08x\n", s->s_r_blocks_count_lo);
    printf("    s_free_blocks_count_lo: %08x\n", s->s_free_blocks_count_lo);
    printf("    s_free_inodes_count: %08x\n", s->s_free_inodes_count);
    printf("    s_first_data_block: %08x\n", s->s_first_data_block);
    printf("    s_log_block_size: %08x\n", s->s_log_block_size);
    printf("    s_log_cluster_size: %08x\n", s->s_log_cluster_size);
    printf("    s_blocks_per_group: %08x\n", s->s_blocks_per_group);
    printf("    s_clusters_per_group: %08x\n", s->s_clusters_per_group);
    printf("    s_inodes_per_group: %08x\n", s->s_inodes_per_group);
    printf("    s_first_ino: %08x\n", s->s_first_ino);
    printf("    s_inode_size: %04x\n", s->s_inode_size);
    printf("    s_block_group_nr: %04x\n", s->s_block_group_nr);
}

void print_ext4_group_desc(ext4_group_desc *g)
{
    printf("ext4_group_desc:\n");
    printf("    bg_block_bitmap_lo: %08x\n", g->bg_block_bitmap_lo);
    printf("    bg_inode_bitmap_lo: %08x\n", g->bg_inode_bitmap_lo);
    printf("    bg_inode_table_lo: %08x\n", g->bg_inode_table_lo);
    printf("    bg_free_blocks_count_lo: %04x\n", g->bg_free_blocks_count_lo);
    printf("    bg_free_inodes_count_lo: %04x\n", g->bg_free_inodes_count_lo);
    printf("    bg_used_dirs_count_lo: %04x\n", g->bg_used_dirs_count_lo);
    printf("    bg_flags: %04x\n", g->bg_flags);
    printf("    bg_exclude_bitmap_lo: %08x\n", g->bg_exclude_bitmap_lo);
    printf("    bg_block_bitmap_csum_lo: %04x\n", g->bg_block_bitmap_csum_lo);
    printf("    bg_inode_bitmap_csum_lo: %04x\n", g->bg_inode_bitmap_csum_lo);
    printf("    bg_itable_unused_lo: %04x\n", g->bg_itable_unused_lo);
    printf("    bg_checksum: %04x\n", g->bg_checksum);
    printf("    bg_block_bitmap_hi: %08x\n", g->bg_block_bitmap_hi);
    printf("    bg_inode_bitmap_hi: %08x\n", g->bg_inode_bitmap_hi);
    printf("    bg_inode_table_hi: %08x\n", g->bg_inode_table_hi);
    printf("    bg_free_blocks_count_hi: %04x\n", g->bg_free_blocks_count_hi);
    printf("    bg_free_inodes_count_hi: %04x\n", g->bg_free_inodes_count_hi);
    printf("    bg_used_dirs_count_hi: %04x\n", g->bg_used_dirs_count_hi);
    printf("    bg_itable_unused_hi: %04x\n", g->bg_itable_unused_hi);
    printf("    bg_exclude_bitmap_hi: %08x\n", g->bg_exclude_bitmap_hi);
    printf("    bg_block_bitmap_csum_hi: %04x\n", g->bg_block_bitmap_csum_hi);
    printf("    bg_inode_bitmap_csum_hi: %04x\n", g->bg_inode_bitmap_csum_hi);
    printf("    bg_reserved: %08x\n", g->bg_reserved);
}

void read_ext4(FILE *dev)
{
    ext4_super_block s;
    ext4_group_desc g;
    int res;

    long start_offset = ftell(dev);

    // Skip over free space
    res = fseek(dev, start_offset + 0x400, SEEK_SET);
    if (res)
        fatal("fseek");

    res = fread(&s, sizeof(s), 1, dev);
    if (res != 1)
        fatal("fread");
    
    printf("---------- ext4 ----------\n");
    print_ext4_super_block(&s);

    // Skip over free space
    res = fseek(dev, start_offset + 0x1000, SEEK_SET);
    if (res)
        fatal("fseek");
    
    res = fread(&g, sizeof(g), 1, dev);
    if (res != 1)
        fatal("fread");
    
    // printf("first_data_block: %08x\n", s.s_first_data_block);
    print_ext4_group_desc(&g);
}