#define U8		unsigned char
#define S8		char
#define U16		unsigned short
#define	U32		unsigned int
#define	S32		int
#define	U64		unsigned __int64

typedef struct _VOL_struct{
	U32		Drive;
	U64		VolBeginSec;
	U64		MFTStartSec;
	U64		MFTMirrStartSec;
	U32		ClusSize;
	U32		SecPerClus;
	U64		TotalSec;
	U32		SizeOfMFTEntry;
	U32		SizeOfIndexRecord;
}VolStruct;

#pragma pack(1)
typedef struct _NTFS_BPB_struct{
	U8		JmpBoot[3];
	U8		OEMName[8];
	U16		BytsPerSec;
	U8		SecPerClus;
	U16		RsvdSecCnt;
	U8		Unused1[5];	
	U8		Media;
	U8		Unused2[18];
	U64		TotalSector;	
	U64		StartOfMFT;
	U64		StartOfMFTMirr;
	S8		SizeOfMFTEntry;
	U8		Unused3[3];
	S8		SizeOfIndexRecord;
	U8		Unused4[3];	
	U64		SerialNumber;
	U32		Unused;

	U8		BootCodeArea[426];
	
	U16		Signature;
}NTFS_BPB;
#pragma pack()