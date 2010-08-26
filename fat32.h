#ifndef _FAT32_H
#define _FAT32_H

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

struct dir_entry{
  char DIR_Name[11];
  char DIR_Attr;
  char DIR_NTRes;
  char DIR_CrtTimeTenth;
  short DIR_CrtTime;
  short DIR_CrtDate;
  short DIR_LstAccDate;
  short DIR_FstClusHI;
  short DIR_WrtTime;
  short DIR_WrtDate;
  short DIR_FstClusLO;
  unsigned int DIR_FileSize;
} __attribute__((packed));


#endif
