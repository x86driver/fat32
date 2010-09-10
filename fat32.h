#ifndef _FAT32_H
#define _FAT32_H

#define SECTOR_SIZE 512

struct FAT32 {
  char BS_jmpBoot[3];
  char BS_OEMName[8];
  unsigned short BPB_BytsPerSec;
  unsigned char BPB_SecPerClus;
  unsigned short BPB_ResvdSecCnt;
  unsigned char BPB_NumFATs;
  unsigned short BPB_RootEntCnt;
  unsigned short BPB_TotSec16;
  char BPB_Media;
  unsigned short BPB_FATSz16;
  unsigned short BPB_SecPerTrk;
  unsigned short NumHeads;
  unsigned int BPB_HiddSec;
  unsigned int BPB_TotSec32;
  unsigned int BPB_FATSz32;
  unsigned short BPB_flags;
  unsigned short BPB_FSVer;
  unsigned int BPB_RootClus;
  unsigned short BPB_FSInfo;
  unsigned short BPB_BkBootSec;
  char BPB_Reserved[12];
  unsigned char BS_DrvNum;
  unsigned char BS_Reserved1;
  char BS_BootSig;
  unsigned int BS_VolID;
  char BS_VolLAB[11];
  char BS_FilSysType[8];
  char code[420];
  char bootsig[2];
} __attribute__((packed));

#define MSDOS_NAME      11

struct dir_entry {
        unsigned char   name[MSDOS_NAME];/* name and extension */
        unsigned char   attr;           /* attribute bits */
        unsigned char   lcase;          /* Case for base and extension */
        unsigned char   ctime_cs;       /* Creation time, centiseconds (0-199) */
        unsigned short  ctime;          /* Creation time */
        unsigned short  cdate;          /* Creation date */
        unsigned short  adate;          /* Last access date */
        unsigned short  starthi;        /* High 16 bits of cluster in FAT32 */
        unsigned short  time,date,start;/* time, date and first cluster */
        unsigned int    size;           /* file size (in bytes) */
} __attribute__((packed));

struct dir_long_entry {
        unsigned char    id;             /* sequence number for slot */
        unsigned char    name0_4[10];    /* first 5 characters in name */
        unsigned char    attr;           /* attribute byte */
        unsigned char    reserved;       /* always 0 */
        unsigned char    alias_checksum; /* checksum for 8.3 alias */
        unsigned char    name5_10[12];   /* 6 more characters in name */
        unsigned short   start;         /* starting cluster number, 0 in long slots */
        unsigned char    name11_12[4];   /* last 2 characters in name */
};

struct inode {
	unsigned int cluster;
	unsigned int size;
	unsigned int pos;
};

struct Partition {
        unsigned char status;
        unsigned char head;
        unsigned char sector;
        unsigned char cylinder;
        unsigned char type;
        unsigned char endhead;
        unsigned char endsector;
        unsigned char endcylinder;
        unsigned int startlba;
        unsigned int totalsec;
};

struct msdos_sb {
	unsigned int sec_per_clus;
	unsigned int root_sec;
	unsigned int first_fat_sec;
	unsigned int first_data_sec;
	unsigned int cur_dir_clus;
};

extern struct FAT32 fat;
extern unsigned int fat_table;
extern struct msdos_sb dosb;

static inline unsigned int fat_get_sec(unsigned int cluster)
{
        return ((cluster - 2) * dosb.sec_per_clus) + dosb.first_data_sec;
}

unsigned int fat_next_cluster(unsigned int currentry);

static inline void namecpy(char *dst, const unsigned char *src, int len)
{
	while (len--) {
		*dst++ = *src++;
		++src;
	}
}

static inline unsigned char fat_checksum(const unsigned char *name)
{
        unsigned char s = name[0];
        s = (s<<7) + (s>>1) + name[1];  s = (s<<7) + (s>>1) + name[2];
        s = (s<<7) + (s>>1) + name[3];  s = (s<<7) + (s>>1) + name[4];
        s = (s<<7) + (s>>1) + name[5];  s = (s<<7) + (s>>1) + name[6];
        s = (s<<7) + (s>>1) + name[7];  s = (s<<7) + (s>>1) + name[8];
        s = (s<<7) + (s>>1) + name[9];  s = (s<<7) + (s>>1) + name[10];
        return s;
}

#endif
