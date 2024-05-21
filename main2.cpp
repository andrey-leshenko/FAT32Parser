#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define BLOCK_SIZE 512

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void fatal(const char *msg)
{
    fprintf(stderr, "error: %s\n", msg);
    exit(1);
}

// ====================
// MBR Partition Parsing
// ====================

/*
Offset	Size (bytes)	Description
0x00	1	Drive attributes (bit 7 set = active or bootable)
0x01	3	CHS Address of partition start
0x04	1	Partition type
0x05	3	CHS address of last partition sector
0x08	4	LBA of partition start
0x0C	4	Number of sectors in partition
*/

struct partition {
    u8 part_boot_indicator;
    u8 part_chs_start[3];
    u8 part_sysid;
    u8 part_chs_end[3];
    u32 part_lba_start;
    u32 part_sectors;
};

/*
Offset	Size (bytes)	Description
0x000	4401	MBR Bootstrap (flat binary executable code)
0x1B8	4	Optional "Unique Disk ID / Signature"
0x1BC	2	Optional, reserved 0x00003
0x1BE	16	First partition table entry
0x1CE	16	Second partition table entry
0x1DE	16	Third partition table entry
0x1EE	16	Fourth partition table entry
0x1FE	2	(0x55, 0xAA) "Valid bootsector" signature bytes
*/

struct mbr {
    u8 mbr_bootstrap[0x1b8];
    u32 mbr_uid;
    u16 mbr_reserved_1bc;
    partition mbr_partitions[4];
    u16 mbr_signature;
} __attribute__((packed));


// https://en.wikipedia.org/wiki/Partition_type

#define PART_FAT32_CHS 0x0b
#define PART_FAT32_LBA 0x0c

bool is_fat32_sysid(int sysid)
{
    return sysid == PART_FAT32_CHS || sysid == PART_FAT32_LBA;
}

void read_mbr(FILE *dev, mbr *m)
{
    int res;

    res = fread(m, sizeof(*m), 1, dev);
    if (res != 1)
        fatal("fread");

    if (m->mbr_signature != 0xaa55)
        fatal("invalid MBR signature");
}

// ====================
// FAT32 Parsing
// ====================

// https://www.pjrc.com/tech/8051/ide/fat32.html

struct fat32_volume_id {
    u8 _padding1[0xb];
    u16 bytes_per_sec;
    u8 sc_per_clus;
    u16 rsvd_sec_cnt;
    u8 num_fats;
    u8 _padding2[0x13];
    u32 sc_per_fat;
    u8 _padding3[0x4];
    u32 root_clus;
    u8 _padding4[0x1ce];
    u16 signature;
} __attribute__((packed));

#define SHORTNAME_LEN 11

struct fat32_dir_entry {
    char name[SHORTNAME_LEN];
    u8 attr;
    u8 _padding1[8];
    u16 cluster_high;
    u8 _padding2[4];
    u16 cluster_low;
    u32 file_size;
};

#define ATTR_READ_ONLY  (1 << 0)
#define ATTR_HIDDEN     (1 << 1)
#define ATTR_SYSTEM     (1 << 2)
#define ATTR_VOLUME_ID  (1 << 3)
#define ATTR_DIRECTORY  (1 << 4)
#define ATTR_ARCHIVE    (1 << 5)

#define ATTRS_LFN       0x0f

#define CLUSTER_NUM(x)  (x & ((1 << 28) - 1))

#define RECORD_UNUSED   '\xe5'

#define LAST_CLUSTER    0xffffffff
#define IS_EOC(cluster) (cluster >= 0x0FFFFFF8)

// VFAT long filename support
// https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#VFAT

#define LFN_ENTRY_LAST      0x40
#define LFN_ENTRY_SEQ_MASK  0x1f
#define LFN_ENTRY_CHARS     26

void shortname2longname(const char filename[SHORTNAME_LEN], char *result)
{
    for (int i = 0; i < SHORTNAME_LEN; i++) {
        if (filename[i] == ' ')
            continue;
        
        if (i == 8)
            *(result++) = '.';

        *(result++) = filename[i];
    }
    *(result++) = 0;
}

u32 fat32_cluster(struct fat32_dir_entry *ent)
{
    return ent->cluster_low + (ent->cluster_high << 16);
}

struct fat32_consts {
    u32 fat_begin_lba;
    u32 cluster_begin_lba;
    u32 sectors_per_cluster;
    u32 root_dir_first_cluster;
};

struct fat32_consts read_fat32_volume_id(FILE *dev, int start_lba)
{
    struct fat32_consts c;
    fat32_volume_id vid;
    int res;

    fseek(dev, start_lba * BLOCK_SIZE, SEEK_SET);

    res = fread(&vid, sizeof(vid), 1, dev);
    if (res != 1)
        fatal("fread");
    if (vid.signature != 0xaa55)
        fatal("invalid MBR signature");

    assert(vid.bytes_per_sec == 512);
    assert(vid.num_fats == 2);

    c.fat_begin_lba = start_lba + vid.rsvd_sec_cnt;
    c.cluster_begin_lba = start_lba + vid.rsvd_sec_cnt + vid.num_fats * vid.sc_per_fat;
    c.sectors_per_cluster = vid.sc_per_clus;
    c.root_dir_first_cluster = vid.root_clus;

    return c;
}

u32 cluster2addr(fat32_consts *c, u32 cluster)
{
    return (c->cluster_begin_lba + (cluster - 2) * c->sectors_per_cluster) * BLOCK_SIZE;
}

u32 addr2cluster(fat32_consts *c, u32 addr)
{
    return (addr / BLOCK_SIZE - c->cluster_begin_lba) / c->sectors_per_cluster + 2;
}

u32 cluster_read_next(FILE *dev, fat32_consts *c, u32 cluster)
{
    u32 next;
    int res;

    fseek(dev, c->fat_begin_lba * BLOCK_SIZE + cluster * sizeof(next), SEEK_SET);
    res = fread(&next, sizeof(next), 1, dev);
    if (res != 1)
        fatal("fread");
    return CLUSTER_NUM(next);
}

// Read from data cluster, and switch to next cluster if we finish reading this one

bool fat32_read_aligned(FILE *dev, fat32_consts *c, void *buffer, u32 size)
{
    long addr = ftell(dev);
    u32 cluster_size = c->sectors_per_cluster * BLOCK_SIZE;
    u32 cluster = addr2cluster(c, addr);
    u32 cluster_offset = addr - cluster2addr(c, cluster);
    int res;

    if (cluster_offset + size > cluster_size)
        fatal("read crossing sector boundary");

    res = fread(buffer, size, 1, dev);
    if (res != 1)
        fatal("fread");

    if (cluster_offset + size == cluster_size) {
        cluster = cluster_read_next(dev, c, cluster);
        if (IS_EOC(cluster))
            return false;
        fseek(dev, cluster2addr(c, cluster), SEEK_SET);
    }
    return true;
}

void fat32_print_file(FILE *dev, struct fat32_consts *c, u32 cluster, u32 size)
{
    static char block[BLOCK_SIZE];

    fseek(dev, cluster2addr(c, cluster), SEEK_SET);

    while (size > 0) {
        u32 read_size = MIN(size, BLOCK_SIZE);
        fat32_read_aligned(dev, c, block, read_size);
        fwrite(block, read_size, 1, stdout);
        size -= read_size;
    }
}

void fat32_print_file(FILE *dev, struct fat32_consts *c, struct fat32_dir_entry *ent)
{
    fat32_print_file(dev, c, fat32_cluster(ent), ent->file_size);
}

bool fat32_read_dir_ent(FILE *dev, struct fat32_consts *c, fat32_dir_entry *ent, char *long_name)
{
    long_name[0] = 0;

    while (true) {
        // TODO: Verify we don't read if no more data is left
        fat32_read_aligned(dev, c, ent, sizeof(*ent));

        if (ent->name[0] == 0)
            return false;
        if (ent->name[0] == RECORD_UNUSED)
            continue;
        if (ent->attr == ATTRS_LFN) {
            char name_part[LFN_ENTRY_CHARS];
            char *ent_bytes = (char*)ent;

            memcpy(name_part +  0, ent_bytes + 0x01, 10);
            memcpy(name_part + 10, ent_bytes + 0x0e, 12);
            memcpy(name_part + 22, ent_bytes + 0x1c, 4);

            int seq = ent->name[0] & LFN_ENTRY_SEQ_MASK;
            memcpy(long_name + (seq - 1) * LFN_ENTRY_CHARS, name_part, LFN_ENTRY_CHARS);
            // Last entry might be unterminated
            if (ent->name[0] & LFN_ENTRY_LAST)
                long_name[seq * LFN_ENTRY_CHARS] = 0;

            continue;
        }

        return true;
    }
}

void fat32_list_dir(FILE *dev, struct fat32_consts *c, struct fat32_dir_entry *dir_ent)
{
    struct fat32_dir_entry ent;
    char long_name[256 * 2];

    fseek(dev, cluster2addr(c, fat32_cluster(dir_ent)), SEEK_SET);

    while (fat32_read_dir_ent(dev, c, &ent, long_name)) {
        char shortname[SHORTNAME_LEN + 2];

        shortname2longname(ent.name, shortname);

        printf("%c%c%c%c%c %10d %10d %-12s",
            ent.attr & ATTR_HIDDEN     ? 'H' : '-',
            ent.attr & ATTR_SYSTEM     ? 'S' : '-',
            ent.attr & ATTR_VOLUME_ID  ? 'V' : '-',
            ent.attr & ATTR_DIRECTORY  ? 'D' : '-',
            ent.attr & ATTR_ARCHIVE    ? 'A' : '-',
            fat32_cluster(&ent), ent.file_size, shortname);

        if (long_name[0]) {
            // TODO: Handle non-ASCII
            char ascii_name[256];
            int i;
            for (i = 0; long_name[i * 2]; i++) {
                ascii_name[i] = long_name[i * 2];
            }
            ascii_name[i] = 0;
            printf(" %s", ascii_name);
        }

        printf("\n");
    }
}

bool fat32_dirent_name_matches(const fat32_dir_entry *ent, const char *long_name, const char *target_name)
{
    // TODO: Handle non-ASCII
    char ascii_name[256];
    char shortname[SHORTNAME_LEN + 2];
    int i;

    shortname2longname(ent->name, shortname);
    if (!strcasecmp(shortname, target_name))
        return true;

    for (i = 0; long_name[i * 2]; i++)
        ascii_name[i] = long_name[i * 2];
    ascii_name[i] = 0;
    return strcasecmp(target_name, ascii_name) == 0;
}

void fat32_follow_path(FILE *dev, struct fat32_consts *c, const char *_path, fat32_dir_entry *ent)
{
    char path_copy[256];
    char *path;
    char *sep = NULL;

    char long_name[256 * 2];

    assert(strlen(_path) + 1 <= sizeof(path_copy));
    strcpy(path_copy, _path);
    path = path_copy;

    // Seek to root dir
    fseek(dev, c->cluster_begin_lba * BLOCK_SIZE, SEEK_SET);

    // Fill out a fake dir entry for the root directory
    memset(ent, 0, sizeof(*ent));
    ent->attr = ATTR_DIRECTORY;
    ent->cluster_low = c->root_dir_first_cluster & 0xffff;
    ent->cluster_high = c->root_dir_first_cluster >> 16;

    while (true) {
        while (*path && *path == '/')
            path++;
        
        if (!*path)
            break;

        sep = strstr(path, "/");
        if (sep)
            *sep = 0;

        u32 next_cluster = 0;

        while (fat32_read_dir_ent(dev, c, ent, long_name)) {
            if (fat32_dirent_name_matches(ent, long_name, path)) {
                next_cluster = fat32_cluster(ent);
                break;
            }
        }

        if (!next_cluster) {
            fprintf(stderr, "could not find file/directory named '%s'\n", path);
            fatal("path not found: no such file or directory");
        }
        if (sep && !(ent->attr & ATTR_DIRECTORY)) {
            fprintf(stderr, "cannot access '%s'\n", path);
            fatal("path not found: not a directory");
        }

        fseek(dev, cluster2addr(c, next_cluster), SEEK_SET);

        if (!sep)
            break;
        path = sep + 1;
    }
}

int main(int argc, char *argv[])
{
    FILE *dev;
    mbr m;
    int part_id;
    fat32_consts c;
    const char *path;
    fat32_dir_entry dir_entry;

    // Open device

    if (argc < 2) {
        printf("usage: %s DEV [PART [PATH]]\n", argv[0]);
        return 1;
    }

    dev = fopen(argv[1], "r");
    if (!dev)
        fatal("fopen");
    
    // Read MBR
    
    read_mbr(dev, &m);

    if (argc < 3) {
        printf("---------- MBR ----------\n");
        printf("bootstrap_code: ...\n");
        printf("uid: %08x\n", m.mbr_uid);
        printf("reserved: %04x\n", m.mbr_reserved_1bc);
        for (int i = 0; i < 4; i++) {
            partition *p = m.mbr_partitions + i;
            printf("partition %d:\n", i);
            if (p->part_sectors == 0) {
                printf("    (empty)\n");
            }
            else {
                printf("    boot_indicator: %02x\n", p->part_boot_indicator);
                printf("    sysid: %02x%s\n", p->part_sysid, is_fat32_sysid(p->part_sysid) ? " (FAT32)" : "");
                printf("    rel_sector: %08x\n", p->part_lba_start);
                printf("    sectors: %08x\n", p->part_sectors);
            }
        }
        printf("signature: %04x\n", m.mbr_signature);
        return 0;
    }

    // Find partition

    part_id = atoi(argv[2]);

    if (part_id < 0 || part_id >= 4 || (part_id == 0 && strcmp(argv[2], "0"))) {
        fatal("invalid partition number");
    }
    if (!is_fat32_sysid(m.mbr_partitions[part_id].part_sysid)) {
        fatal("target partition not fat32");
    }

    // Read FAT32

    c = read_fat32_volume_id(dev, m.mbr_partitions[part_id].part_lba_start);

    if (argc < 4) {
        path = "/";
    }
    else {
        path = argv[3];
    }

    fat32_follow_path(dev, &c, path, &dir_entry);

    if (dir_entry.attr & ATTR_DIRECTORY) {
        fat32_list_dir(dev, &c, &dir_entry);
    }
    else {
        fat32_print_file(dev, &c, &dir_entry);
    }

    return 0;
}
