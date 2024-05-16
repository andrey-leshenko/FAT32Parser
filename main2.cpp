#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <string>

using std::string;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define BLOCK_SIZE 512

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

struct fat32_dir_entry {
    char name[11];
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

#define CLUSTER_NUM(x) (x & ((1 << 28) - 1))

#define RECORD_UNUSED   '\xe5'

#define LAST_CLUSTER    0xffffffff

// VFAT long filename support
// https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#VFAT

#define LFN_ENTRY_LAST      0x40
#define LFN_ENTRY_SEQ_MASK  0x1f
#define LFN_ENTRY_CHARS     26

void fatal(const char *msg)
{
    printf("error: %s\n", msg);
    exit(1);
}

u32 cluster_lba(u32 cluster_begin_lba, u32 sectors_per_cluster, u32 cluster_number)
{
    return cluster_begin_lba + (cluster_number - 2) * sectors_per_cluster;
}

// string read_fat32_cluster_chain(FILE *dev, int first_cluster, )

void short2long_fname(const char filename[11], char *result)
{
    // TODO
}

struct fat32_consts {

};

string read_fat32_file(FILE *dev, int start_lba, const string &path)
{
    int res;
    fat32_volume_id vid;

    char long_name[256 * 2] = {0};
    bool in_long_name = false;

    fseek(dev, start_lba * BLOCK_SIZE, SEEK_SET);

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

    assert(vid.bytes_per_sec == 512);
    assert(vid.num_fats == 2);

    u32 fat_begin_lba = start_lba + vid.rsvd_sec_cnt;
    u32 cluster_begin_lba = start_lba + vid.rsvd_sec_cnt + (vid.num_fats * vid.sc_per_fat);
    u32 sectors_per_cluster = vid.sc_per_clus;
    u32 root_dir_first_cluster = vid.root_clus;

    printf("Custer start: %#x\n", cluster_begin_lba);

    fseek(dev, cluster_begin_lba * BLOCK_SIZE, SEEK_SET);

    while (true) {
        fat32_dir_entry ent;
        res = fread(&ent, sizeof(ent), 1, dev);
        if (res != 1)
            fatal("fread");

        if (ent.name[0] == 0)
            break;
        if (ent.name[0] == RECORD_UNUSED)
            continue;

        if (ent.attr == ATTRS_LFN) {
            char name_part[LFN_ENTRY_CHARS];
            char *ent_bytes = (char*)&ent;

            memcpy(name_part +  0, ent_bytes + 0x01, 10);
            memcpy(name_part + 10, ent_bytes + 0x0e, 12);
            memcpy(name_part + 22, ent_bytes + 0x1c, 4);

            int seq = ent.name[0] & LFN_ENTRY_SEQ_MASK;
            memcpy(long_name + (seq - 1) * LFN_ENTRY_CHARS, name_part, LFN_ENTRY_CHARS);

            in_long_name = true;
        }

        if ((ent.attr & ATTR_VOLUME_ID) == 0) {
            if (in_long_name) {
                // TODO: Handle non-ASCII
                char ascii_name[14];
                int i;
                for (i = 0; long_name[i * 2]; i++)
                    ascii_name[i] = long_name[i * 2];
                ascii_name[i] = 0;
                printf("long name: %s\n", ascii_name);

                in_long_name = false;
                memset(long_name, 0, sizeof(long_name));
            }

            string name(ent.name, 11);
            printf("short name: %s\n\n", name.c_str());
        }
    }

    return "";
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

    read_fat32_file(dev, 0x800, "/");

    return 0;
}