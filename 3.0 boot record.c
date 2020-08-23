#include <stdio.h>
#include <windows.h>
#include <stdlib.h>

#pragma pack(1)
typedef struct _FAT16_BPB_struct{
	U8		JmpBoot[3];
	U8		OEMName[8];
	U16		BytsPerSec;
	U8		SecPerClus;
	U16		RsvdSecCnt;
	U8		NumFATs;
	U16		RootEntCnt;
	U16		TotSec16;
	U8		Media;
	U16		FATs16;
	U16		SecPerTrk;
	U16		NumHeads;
	U32		HiddSec;
	U32		TotSec32;
	
	U8		DriveNumber;
	U8		Reserved1;
	U8		BootSignal;
	U32		VolumeID;
	U8		VolumeLabel[11];
	U8		FilSysType[8];
	

	U8		BootCodeArea[448];
	
	U16		Signature;
}FAT16_BPB;
#pragma pack()

typedef struct _VOL_struct{
	U32		Drive;
	U32		VolBeginSec;
	U32		FirstDataSec;
	U32		RootDirSec;
	U32		RootEntCnt;	
	U32		RootDirSecCnt;
	U32		FATSize;
	U32		FATStartSec;
	U32		TotalClusCnt;
	U32		TotalSec;
	U32		DataSecSize;
	U32		ClusterSize;
	U32		SecPerClus;
}VolStruct;

U32  HDD_read (U8 drv, U32 SecAddr, U32 blocks, U8* buf);
U32  get_BPB_info(FAT16_BPB* BPB, VolStruct* pVol);

VolStruct gVol;

int main(void) {
	U8		    buf[512];
	
	gVol.Drive		= 0x2;
	gVol.VolBeginSec	= 0x0;	

	if( HDD_read(gVol.Drive, gVol.VolBeginSec, 1, buf) == 0 ){
		printf("Boot Sector Read Failed \n");
		return 1;
	}

	if (get_BPB_info((FAT16_BPB*)buf, &gVol) == 0){
		printf("It is not FAT16 File System \n"); 
		return 1;
	}
	
	printf("[[[[[[[ Volume Information ]]]]]]] \n");

	printf("Total Sector      = %d sectors \n", gVol.TotalSec);
	printf("FAT size           = %d sectors \n", gVol.FATSize);
	printf("Root Dir Sector   = %d          \n", gVol.RootDirSec);
	printf("Root Dir Sector Count = %d   \n", gVol.RootDirSecCnt);
	printf("First Data Sector    = %d      \n", gVol.FirstDataSec);
	printf("Data Sector Count = %d Sectors\n", gVol.DataSecSize);
	printf("Total Cluster      = %d         \n", gVol.TotalClusCnt);
	printf("Size of Cluster    = %d         \n", gVol.ClusterSize);
	return 0;
}

U32 get_BPB_info(FAT16_BPB* BPB, VolStruct* pVol) {	
	if(BPB->RootEntCnt == 0 || BPB->Signature != 0xAA55) 
		return 0;
	
	// Get Total Sector	
	if( BPB->TotSec16 != 0 ) pVol->TotalSec = BPB->TotSec16;	
	else			    pVol->TotalSec = BPB->TotSec32;	

	// Get FAT Size	
	pVol->FATSize = BPB->FATs16;

	// Get FAT Start Sector
	pVol->FATStartSec = pVol->VolBeginSec + BPB->RsvdSecCnt;
	
	// Get Root Dir Entry Count
	pVol->RootEntCnt = BPB->RootEntCnt;
	
	// Get Root Dir Sector
	pVol->RootDirSec = pVol->VolBeginSec + BPB->RsvdSecCnt + (BPB->NumFATs * BPB->FATs16);
	
	// Get Root Dir Sector Count;
	pVol->RootDirSecCnt = (( BPB->RootEntCnt * 32) + (BPB->BytsPerSec - 1)) / BPB->BytsPerSec;

	// GET FAT Start Sector
	pVol->FirstDataSec = pVol->VolBeginSec + BPB->RsvdSecCnt + (BPB->NumFATs * pVol->FATSize) + pVol->RootDirSecCnt;

	// Get Size of Data Area	
	pVol->DataSecSize = pVol->TotalSec - (BPB->RsvdSecCnt + (BPB->NumFATs * pVol->FATSize) + pVol->RootDirSecCnt);

	// Get Total Cluster Count;	
	pVol->TotalClusCnt = pVol->DataSecSize / BPB->SecPerClus;

	// Get Size of Cluster
	pVol->ClusterSize = BPB->SecPerClus * BPB->BytsPerSec;

	// Get Sector Per Cluster
	pVol->SecPerClus = BPB->SecPerClus;

	return 1;
}
