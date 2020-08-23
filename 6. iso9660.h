#ifndef __ISO9660_H__
#define __ISO9660_H__

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

#pragma pack(1)	
typedef struct { 
	U32 fwd;
	U32 rev;
} U32_Both_Endian;	

typedef struct { 
	U16 fwd;
	U16 rev;
} U16_Both_Endian;	

typedef struct { 
	char Year[4];	// 0001..9999
	char month[2];	// 01..12
	char Day[2];	// 01..31
	char Hour[2];	// 00..23
	char Minute[2];	// 00..59
	char Second[2];	// 00..59
	char Millis[2];	// 00..99
	U8 GmtOffset;	
} Datetime17;	// Volume Descriptor

typedef struct { 
	U8 Years;		// Since 1900
	U8 Month;
	U8 Day;
	U8 Hour;
	U8 Minute;
	U8 Second;
	signed char GmtOffset;
} Datetime7;	// Directory Record

typedef struct {
	U8 DirectoryRecordLength;
	U8 ExtendedAttributeRecordLength;
	U32_Both_Endian LocationOfExtent;
	U32_Both_Endian DataLength;
	Datetime7 RecordingDatetime;
	U8 FileFlags;
	U8 FileunitSize;
	U8 InterleaveGapSize;
	U16_Both_Endian VolumeSequenceNumber;
	U8 FileIdentifierLength;
	char FileIdentifier[1];
} DirectoryRecord;	// Directory Record 

typedef struct {
	U8 Type;
	char CD001[5];
	U8 Version;
	union {

    struct {	// Boot Volume Descriptor
      char BootSystemIdentifier[32];
      char BootIdentifier[32];
      U8 BootSystemUse[1977];
    } Boot;

    struct {	// Supplementary Volume Descriptor & Primary Volume Descriptor
		U8 VolumeFlags;				// Primary Volume Descriptor : char Unused1[1];
		char SystemIdentifier[32];
		char VolumeIdentifier[32];
		char Unused2[8];
		U32_Both_Endian VolumeSpaceSize;
		char EscapeSequences[32];	// Primary Volume Descriptor : char Unused3[32];
		U16_Both_Endian VolumeSetsize;
		U16_Both_Endian VolumeSequenceNumber;
		U16_Both_Endian LogicalBlockSize;
		U32_Both_Endian PathTableSize;
		U32 LocTypeLPathTable;
		U32 LocOptTypeLPathTable;
		U32 LocTypeMPathTable;
		U32 LocOptTypeMPathTable;
		DirectoryRecord RootDirectoryRecord;
		char VolumeSetIdentifier[128];
		char PublisherIdentifier[128];
		char DataPreparerIdentifier[128];
		char ApplicationIdentifier[128];
		char CopyrightFileIdentifier[37];
		char AbstractFileIdentifier[37];
		char BibliographicFileIdentifier[37];
		Datetime17 VolumeCreationDatetime;
		Datetime17 VolumeModificationDatetime;
		Datetime17 VolumeExpirationDatetime;
		Datetime17 VolumeEffectiveDatetime;
		char FileStructureVersion[1];
		char Unused4[1];
		char ApplicationUse[512];
		char Unused5[653];
    } Main;

    struct {	// Volume Partition Descriptor
      char Unused1[1];
      char SystemIdentifier[32];
      char VolumePartitionIdentifier[32];
      U32_Both_Endian VolumePartitionLocation;
      U32_Both_Endian VolumePartitionSize;
      char NotSpecified[1960];
    } Partition;

    struct {	// Terminator Volume Descriptor
      char Zeroes[2041];
    } Terminator;
  } vd;
} VolumeDescriptor;

typedef struct {
	U8 LenDirectoryIdentifier;
	U8 ExtendedAttributeRecordLength;
	U32 Location;
	U16 ParentDirNum;
	char PathName[1];
} PathTableEntry;	
#pragma pack()
#endif
