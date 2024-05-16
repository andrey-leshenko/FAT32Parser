#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define BLOCK_SIZE 512

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
0x1B8	4	Optional "Unique Disk ID / Signature"2
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

#define PART_EXTENDED_PART 0x05
#define PART_FAT32_CHS 0x0b
#define PART_FAT32_LBA 0x0c
#define PART_LINUX_NATIVE 0x83

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

void print_hex_str(u8 *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
        printf("%02x\n", data[i]);
}

void print_partition(partition *p)
{
    printf("    boot_indicator: %02x\n", p->part_boot_indicator);
    printf("    sysid: %02x\n", p->part_sysid);
    printf("    rel_sector: %08x\n", p->part_lba_start);
    printf("    sectors: %08x\n", p->part_sectors);
}

void print_mbr(mbr *m)
{
    printf("bootstrap_code: ...\n");
    printf("uid: %08x\n", m->mbr_uid);
    printf("reserved: %04x\n", m->mbr_reserved_1bc);
    for (int i = 0; i < 4; i++) {
        printf("sector %d:\n", i);
        print_partition(m->mbr_partitions + i);
    }
    printf("signature: %04x\n", m->mbr_signature);
}

void fatal(const char *msg)
{
    printf("error: %s\n", msg);
    exit(1);
}

void read_fat_volume_id(FILE *dev)
{
    int res;
    fat32_volume_id vid;

    res = fread(&vid, sizeof(vid), 1, dev);
    if (res != 1)
        fatal("fread");
    
    if (vid.signature != 0xaa55)
        fatal("invalid MBR signature");
    
    printf("---------- FAT ----------\n");
    
    printf("bytes_per_sec: %#x\n", vid.bytes_per_sec);
    printf("sc_per_clus: %#x\n", vid.sc_per_clus);
    printf("rsvd_sec_cnt: %#x\n", vid.rsvd_sec_cnt);
    printf("num_fats: %#x\n", vid.num_fats);
    printf("sc_per_fat: %#x\n", vid.sc_per_fat);
    printf("root_clus: %#x\n", vid.root_clus);
    printf("signature: %#x\n", vid.signature);
}

// https://en.wikipedia.org/wiki/Extended_boot_record

void read_ebr(FILE *dev)
{
    int res;
    mbr m;

    long start_offset = ftell(dev);

    printf("---------- EBR ----------\n");

    while (1) {
        res = fread(&m, sizeof(m), 1, dev);
        if (res != 1)
            fatal("fread");
        
        if (m.mbr_signature != 0xaa55)
            fatal("invalid MBR signature");
        
        print_partition(m.mbr_partitions + 0);

        if (m.mbr_partitions[1].part_lba_start == 0)
            break;
        
        fseek(dev, start_offset + m.mbr_partitions[1].part_lba_start, SEEK_SET);
    }
}

void read_mbr(FILE *dev)
{
    int res;
    mbr m;

    res = fread(&m, sizeof(m), 1, dev);
    if (res != 1)
        fatal("fread");

    if (m.mbr_signature != 0xaa55)
        fatal("invalid MBR signature");

    printf("---------- MBR ----------\n");
    print_mbr(&m);

    for (int i = 0; i < 4; i++) {
        if (m.mbr_partitions[i].part_sysid == PART_FAT32_CHS) {
            fseek(dev, m.mbr_partitions[i].part_lba_start * BLOCK_SIZE, SEEK_SET);
            read_fat_volume_id(dev);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("usage: %s MBR_DEV\n", argv[0]);
        return 1;
    }

    FILE *dev = fopen(argv[1], "r");
    if (!dev)
        fatal("fopen");

    read_mbr(dev);

    // // rel_sector of the main partition
    // fseek(dev, 0x00100ffe * BLOCK_SIZE, SEEK_SET);
    // read_ebr(dev);

    return 0;
}