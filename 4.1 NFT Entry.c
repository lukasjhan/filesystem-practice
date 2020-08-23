#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include "ntfs.h"

U32  HDD_read (U8 drv, U32 SecAddr, U32 blocks, U8* buf);
int  get_MFTEntry(int MFTNum, MFTEntry* curMFTEntry);
void show_mft(MFTEntry mft);
void show_attr(MFTEntry mft);
void attr_FILENAME(AttrFILENAME* filename, char* final);
void attr_STD_INFO(AttrSTDINFO* stdInfo);
void attr_ATTR_LIST(ListEntry* pEntry, U32 AttrLen);
void ChangeFixupData(U16* data, U32 ArrayOffset, U32 AttayCnt,U32 Size);

VolStruct gVol;

int main(void) {
	NTFS_BPB	boot;
	MFTEntry	mft;

	gVol.Drive			= 0x1;
	gVol.VolBeginSec	= 63;	
		
	if( HDD_read(gVol.Drive, gVol.VolBeginSec, 1, (U8*)&boot) == 0 ){
		printf("Boot Sector Read Failed \n");
		return 1;
	}

	get_BPB_info( &boot, &gVol);

	get_MFTEntry(1, &mft);
	
	show_mft(mft);
	ChangeFixupData((U16*)&mft, 
					mft.Header.AddrFixUpArray,
					mft.Header.CountFixUpArray,
					gVol.SizeOfMFTEntry);
	show_attr(mft);
	
	return 0;
}

int  get_MFTEntry(int MFTNum, MFTEntry* curMFTEntry){
	int readSec = gVol.MFTStartSec + ( MFTNum * 2 );

	if( HDD_read(gVol.Drive, readSec, 2, curMFTEntry) == 0 ){
		printf("MFT Read Failed \n");
		return 1;
	}
	return 0;
}

void show_mft(MFTEntry mft){
	printf("<<<<<<<<<<<<< MFT Entry Info >>>>>>>>>>>>>\n");
	printf("LSN = %d \n",mft.Header.LSN);
	printf("Sequence Value = %d \n",mft.Header.SeqeunceValue);
	printf("Link Count = %d \n",mft.Header.LinkCnt);
	printf("First Attr Offset = %d \n",mft.Header.AddrFirstAttr);
	printf("Flags %x \n", mft.Header.Flags);
	printf("Used Size of MFT = %d \n",mft.Header.UsedSizeOfEntry);
	printf("Alloc size of MFT = %d \n",mft.Header.AllocSizeOfEntry);
	printf("File Referrence to base record = %d \n",mft.Header.FileRefer);
	printf("Next Attr ID = %d \n",mft.Header.NextAttrID);		
}

void ChangeFixupData(U16* data, U32 ArrayOffset, U32 AttayCnt, U32 Size){
	U16* FixUpVal = data+(ArrayOffset/2);
	U32	 ChangeCnt = Size/512;
	U32	 i, Offset = 255;
	
	for(i = 0; i < ChangeCnt; i++){
		if( data[Offset] != FixUpVal[0] ){
			printf("Fixup Val is invalid\n");
			return;
		}		
		data[Offset] = FixUpVal[i+1];
		Offset = Offset + 256;		
	}
}

void show_attr(MFTEntry mft){
	int			offset = mft.Header.AddrFirstAttr;
	void		*pAttr;
	AttrHeader	*header = (AttrHeader*)&mft.Data[offset];	
	
	do{		
		
		if( header->non_resident_flag == 0)	{			
			pAttr = &mft.Data[offset+header->Res.AttrOffset];
		}else{		
			pAttr = &mft.Data[offset+header->NonRes.RunListOffset];
		}

		if(header->AttrTypeID == 48){
			char		final[512]={0,};
			attr_FILENAME( (AttrFILENAME*)		pAttr, final);
		}else if(header->AttrTypeID == 32){		
			attr_ATTR_LIST( (ListEntry*) pAttr, header->Res.AttrSize);
		}else if(header->AttrTypeID == 16){
			attr_STD_INFO( (AttrSTDINFO*)		pAttr);
		}
		
		offset = offset + header->Length;
		header = (AttrHeader*)&mft.Data[offset];
	}while(header->AttrTypeID != -1);	
}


void attr_ATTR_LIST(ListEntry* pEntry, U32 AttrLen){
	U32 offset = 0;
	printf("---- ATTR LIST ATTR ---- \n");

	do{		
		printf("<<<<< Type = %d >>>>> \n", pEntry->Type);
		printf("Entry Len = %d \n", pEntry->EntryLen);
		printf("Name Len = %d \n", pEntry->NameLen);
		printf("Name Offset = %d \n", pEntry->NameOffset);
		printf("Start VCN = %I64d \n", pEntry->StartVCN);
		printf("MFT Attr = %I64d \n", pEntry->BaseMFTAddr & 0x0000FFFFFFFFFFFF);
		printf("Attr ID = %d \n", pEntry->AttrID);	

		offset = offset + pEntry->EntryLen;
		pEntry = (U8*) pEntry + pEntry->EntryLen;		
	}while(offset < AttrLen );
}

void attr_STD_INFO(AttrSTDINFO* stdInfo){
	printf("---- STD INFO ATTR ---- \n");
	printf("Created time = %I64d \n", stdInfo->CreateTime);
	printf("Modified time = %I64d \n", stdInfo->ModifiedTime);
	printf("MFT Modified time = %I64d \n", stdInfo->MFTmodifiedTime);
	printf("Accessed time = %I64d \n", stdInfo->AccessTime);
	printf("Flag = %x \n", stdInfo->Flags);
}

void attr_FILENAME(AttrFILENAME* filename, char* final){
	wcstombs( final, &filename->FileName, filename->LenOfName );
	final[filename->LenOfName] = 0;	
	
	printf("---- FILE NAME ATTR ---- \n");
	printf("Flags = %x \n", filename->Flag);
	printf("Name Length = %d \n", filename->LenOfName);
	printf("Name Space = %d \n", filename->NameSpace);	
	
	printf("Name = %s \n",final);	
}

//------------------------------------------------------------------------------------------

U32 get_BPB_info(NTFS_BPB* pBPB, VolStruct* pVol) {	

	pVol->ClusSize = pBPB->SecPerClus * pBPB->BytsPerSec;

	pVol->SecPerClus = pBPB->SecPerClus;

	pVol->MFTStartSec = pBPB->StartOfMFT * pBPB->SecPerClus + pVol->VolBeginSec;

	pVol->MFTMirrStartSec = pBPB->StartOfMFTMirr * pBPB->SecPerClus + pVol->VolBeginSec;

	pVol->TotalSec = pBPB->TotalSector;

	if( pBPB->SizeOfMFTEntry < 0 ){
		pVol->SizeOfMFTEntry = 1 << (pBPB->SizeOfMFTEntry * -1);
	}else{
		pVol->SizeOfMFTEntry = pBPB->SizeOfMFTEntry * pVol->ClusSize;
	}

	pVol->SizeOfIndexRecord = pBPB->SizeOfIndexRecord * pVol->ClusSize;

	return 1;
}


U32  HDD_write(U8 drv, U32 SecAddr, U32 blocks, U8* buf) {
	U32 ret = 0;
	U32 ldistanceLow, ldistanceHigh, dwpointer, bytestoread, numread;

	char cur_drv[100];
	HANDLE g_hDevice;

	sprintf(cur_drv,"\\\\.\\PhysicalDrive%d",(U32)drv);
	g_hDevice=CreateFile(cur_drv, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	
	if(g_hDevice==INVALID_HANDLE_VALUE)	return 0; 

	ldistanceLow  = SecAddr << 9;
	ldistanceHigh = SecAddr >> (32 - 9);
	dwpointer = SetFilePointer(g_hDevice, ldistanceLow, (long *)&ldistanceHigh, FILE_BEGIN);
	
	if(dwpointer != 0xFFFFFFFF)	{
		bytestoread = blocks * 512;
		ret = WriteFile(g_hDevice, buf, bytestoread, (unsigned long*)&numread, NULL);
		if(ret)		ret = 1;	
		else    	ret = 0;	
	}
	
	CloseHandle(g_hDevice);
	return ret;
}

U32  HDD_read (U8 drv, U32 SecAddr, U32 blocks, U8* buf){
	U32 ret;
	U32 ldistanceLow, ldistanceHigh, dwpointer, bytestoread, numread;

	char cur_drv[100];
	HANDLE g_hDevice;

	sprintf(cur_drv,"\\\\.\\PhysicalDrive%d",(U32)drv);
	g_hDevice=CreateFile(cur_drv, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	
	if(g_hDevice==INVALID_HANDLE_VALUE)	return 0; 

	ldistanceLow  = SecAddr << 9;
	ldistanceHigh = SecAddr >> (32 - 9);
	dwpointer = SetFilePointer(g_hDevice, ldistanceLow, (long *)&ldistanceHigh, FILE_BEGIN);
	
	if(dwpointer != 0xFFFFFFFF)	{
		bytestoread = blocks * 512;
		ret = ReadFile(g_hDevice, buf, bytestoread, (unsigned long*)&numread, NULL);
		if(ret)		ret = 1;		
		else		ret = 0;		
	}
	
	CloseHandle(g_hDevice);
	return ret;
}

void HexDump  (U8 *addr, U32 len){
	U8		*s=addr, *endPtr=(U8*)((U32)addr+len);
	U32		i, remainder=len%16;
	
	printf("\n");
	printf("Offset      Hex Value                                        Ascii value\n");
	
	// print out 16 byte blocks.
	while (s+16<=endPtr){
		
		printf("0x%08lx  ", (long)(s-addr));
		
		for (i=0; i<16; i++){
			printf("%02x ", s[i]);
		}
		printf(" ");
		
		for (i=0; i<16; i++){
			if		(s[i]>=32 && s[i]<=125)	printf("%c", s[i]);
			else							printf(".");
		}
		s += 16;
		printf("\n");
	}
	
	// Print out remainder.
	if (remainder){
		
		printf("0x%08lx  ", (long)(s-addr));
		
		for (i=0; i<remainder; i++){
			printf("%02x ", s[i]);
		}
		for (i=0; i<(16-remainder); i++){
			printf("   ");
		}

		printf(" ");
		for (i=0; i<remainder; i++){
			if		(s[i]>=32 && s[i]<=125)	printf("%c", s[i]);
			else							printf(".");
		}
		for (i=0; i<(16-remainder); i++){
			printf(" ");
		}
		printf("\n");
	}
	return;
}	// HexDump.