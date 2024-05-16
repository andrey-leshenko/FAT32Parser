#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <string>

using std::string;
using std::min;
using std::max;

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

#define CLUSTER_NUM(x)  (x & ((1 << 28) - 1))
#define ENT_CLUSTER(ent)(ent.cluster_low + (ent.cluster_high << 16))

#define RECORD_UNUSED   '\xe5'

#define LAST_CLUSTER    0xffffffff
#define IS_EOC(cluster) (cluster >= 0x0FFFFFF8)

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

// string read_fat32_cluster_chain(FILE *dev, int first_cluster, )

void short2long_fname(const char filename[11], char *result)
{
    // TODO
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

    c.fat_begin_lba = start_lba + vid.rsvd_sec_cnt;
    c.cluster_begin_lba = start_lba + vid.rsvd_sec_cnt + vid.num_fats * vid.sc_per_fat;
    c.sectors_per_cluster = vid.sc_per_clus;
    c.root_dir_first_cluster = vid.root_clus;

    return c;
}

u32 cluster2addr(const fat32_consts &c, u32 cluster)
{
    return (c.cluster_begin_lba + (cluster - 2) * c.sectors_per_cluster) * BLOCK_SIZE;
}

u32 addr2cluster(const fat32_consts &c, u32 addr)
{
    return (addr / BLOCK_SIZE - c.cluster_begin_lba) / c.sectors_per_cluster + 2;
}

u32 cluster_read_next(FILE *dev, const fat32_consts &c, u32 cluster)
{
    u32 next;
    int res;

    fseek(dev, c.fat_begin_lba * BLOCK_SIZE + cluster * sizeof(next), SEEK_SET);
    res = fread(&next, sizeof(next), 1, dev);
    if (res != 1)
        fatal("fread");
    return CLUSTER_NUM(next);
}

// Read from data cluster, and switch to next cluster if we finish reading this one

bool fat32_read_aligned(FILE *dev, const fat32_consts &c, void *buffer, u32 size)
{
    long addr = ftell(dev);
    u32 cluster_size = c.sectors_per_cluster * BLOCK_SIZE;
    u32 cluster = addr2cluster(c, addr);
    u32 cluster_offset = addr - cluster2addr(c, cluster);
    int res;

    if (cluster_offset + size > cluster_size)
        fatal("read crossing sector boundary");

    res = fread(buffer, size, 1, dev);
    if (res != 1)
        fatal("fread");

    if (cluster_offset + size == cluster_size) {
        printf("Going to next cluster from %d", cluster);
        cluster = cluster_read_next(dev, c, cluster);
        printf("to %d\n", cluster);
        if (IS_EOC(cluster))
            return false;
        fseek(dev, cluster2addr(c, cluster), SEEK_SET);
    }
    return true;
}

string read_fat32_file(FILE *dev, const struct fat32_consts &c, u32 cluster, u32 size)
{
    static char block[BLOCK_SIZE];
    string file;

    file.reserve(size);
    fseek(dev, cluster2addr(c, cluster), SEEK_SET);

    while (size > 0) {
        u32 read_size = min(size, (u32)BLOCK_SIZE);
        fat32_read_aligned(dev, c, block, read_size);
        file.append(block, read_size);
        size -= read_size;
    }

    return file;
}

bool read_fat32_dir_ent(FILE *dev, const struct fat32_consts &c, fat32_dir_entry *ent, char *long_name)
{
    bool in_long_name = false;
    bool res;

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

            in_long_name = true;
            continue;
        }

        return true;
    }
}

void print_fat32_root_dir(FILE *dev, const struct fat32_consts &c)
{
    char long_name[256 * 2];
    fat32_dir_entry ent;

    fseek(dev, c.cluster_begin_lba * BLOCK_SIZE, SEEK_SET);

    while (read_fat32_dir_ent(dev, c, &ent, long_name)) {
        if (long_name[0]) {
            // TODO: Handle non-ASCII
            char ascii_name[256];
            int i;
            for (i = 0; long_name[i * 2]; i++)
                ascii_name[i] = long_name[i * 2];
            ascii_name[i] = 0;
            printf("long name: %s\n", ascii_name);
        }

        string name(ent.name, 11);
        printf("short name: %s\n", name.c_str());
        printf("cluster: %d\n", ent.cluster_low + (ent.cluster_high << 16));
        printf("size: %d\n\n", ent.file_size);
    }
}

bool dirent_name_matches(const fat32_dir_entry *ent, const char *long_name, const char *target_name)
{
    // TODO: Look at short name too
    // TODO: Handle non-ASCII
    char ascii_name[256];
    int i;
    for (i = 0; long_name[i * 2]; i++)
        ascii_name[i] = long_name[i * 2];
    ascii_name[i] = 0;
    return strcmp(target_name, ascii_name) == 0;
}

string read_fat32_file(FILE *dev, const struct fat32_consts &c, const char *_path)
{
    char path_copy[256];
    char *path;
    char *sep = NULL;

    char long_name[256 * 2];
    fat32_dir_entry ent;

    assert(strlen(_path) + 1 <= sizeof(path_copy));
    strcpy(path_copy, _path);
    path = path_copy;

    // Seek to root dir
    fseek(dev, c.cluster_begin_lba * BLOCK_SIZE, SEEK_SET);

    while (true) {
        sep = strcasestr(path, "/");
        if (sep)
            *sep = 0;

        printf("searching for %s\n", path);

        u32 next_cluster = 0;

        while (read_fat32_dir_ent(dev, c, &ent, long_name)) {
            if (dirent_name_matches(&ent, long_name, path)) {
                next_cluster = ENT_CLUSTER(ent);
                break;
            }
        }

        if (!next_cluster) {
            printf("Could not find directory named %s\n", path);
            fatal("Traversal error");
        }
        if (!!sep != !!(ent.attr & ATTR_DIRECTORY))
            fatal("wrong file type");

        fseek(dev, cluster2addr(c, next_cluster), SEEK_SET);

        if (!sep)
            break;
        path = sep + 1;
    }

    return read_fat32_file(dev, c, ENT_CLUSTER(ent), ent.file_size);
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

    fat32_consts c = read_fat32_volume_id(dev, 0x800);
    print_fat32_root_dir(dev, c);
    string f = read_fat32_file(dev, c, "dir1/dir2/file");
    // string f = read_fat32_file_2(dev, c, 54, 508894);
    printf("file: %s\n", f.c_str());

    return 0;
}
