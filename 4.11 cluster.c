#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <ctype.h>
#include "ntfs.h"

U32  HDD_read (U8 drv, U32 SecAddr, U32 blocks, U8* buf);
S32  get_MFTEntry(int MFTNum, MFTEntry* curMFTEntry);
void ChangeFixupData(U16* data, U32 ArrayOffset, U32 AttayCnt,U32 Size);
void read_file(MFTEntry mft, U32 read_size, char* read_buf);
S32  find_file(U32 MFTNum, char* filename, MFTEntry* findMft);
U64  fileFile_inNode(U8* p, U64* child_vcn, char* filename);
void get_RunList(U8* data, RunData* runData);
S32  readCluster(U64 vcn, U32 ClusCnt, RunData* runData, U8* buf);
void* getAttr(U32 AttrNum, MFTEntry* mft);
void ChangetoUpper(char* name);

VolStruct gVol;

int main(void) {
	NTFS_BPB	boot;
	MFTEntry	mft;
	char		filename[128];
	char		buf[4096] = {0};

	gVol.Drive			= 0x1;
	gVol.VolBeginSec	= 63;
	
	strcpy(filename, "test.txt");
		
	if( HDD_read(gVol.Drive, gVol.VolBeginSec, 1, (U8*)&boot) == 0 ){
		printf("Boot Sector Read Failed \n");
		return 1;
	}

	get_BPB_info( &boot, &gVol);	
	find_file(135, filename, &mft);		
	read_file(mft, 4096, buf);	
	printf("%s\n", buf);
	
	return 0;
}

void read_file(MFTEntry mft, U32 read_size, char* read_buf){
	RunData		runData[MAX_CLUS_RUN] = {0};
	U32			ClusCnt;

	ClusCnt = ( read_size + gVol.ClusSize - 1) / gVol.ClusSize;	

	get_RunList( (U8*)getAttr(128, &mft), runData);		
	readCluster(0, ClusCnt, runData, read_buf);
	
	return;
}

void* getAttr(U32 AttrNum, MFTEntry* mft){
	AttrHeader	*header;
	U32			offset;	
	void*		pAttr;
	
	offset = mft->Header.AddrFirstAttr;
	header = (AttrHeader*)&mft->Data[offset];
		
	do{			
		if( header->non_resident_flag == 0)
			pAttr = &mft->Data[offset+header->Res.AttrOffset];
		else
			pAttr = &mft->Data[offset+header->NonRes.RunListOffset];

		if(header->AttrTypeID == AttrNum)	return pAttr;		
		
		offset = offset + header->Length;
		header = (AttrHeader*)&mft->Data[offset];	
	}while(header->AttrTypeID != -1);

	return 0;
}


S32 find_file(U32 MFTNum, char* filename, MFTEntry* findMft){
	MFTEntry	mft;
	RunData		runData[MAX_CLUS_RUN] = {0};
	U8			IndexRecord[4096];
	U64			child_vcn;
	U32			IndexRecordClusSize;
	U64			ret;	
	
	AttrINDEX_ROOT*			pRoot;
	INDEX_RECORD_HEADER*	pIndexRecHeader;

	ChangetoUpper(filename);
	
	get_MFTEntry(MFTNum, &mft);
	
	ChangeFixupData((U16*)&mft,
					mft.Header.AddrFixUpArray,
					mft.Header.CountFixUpArray,
					gVol.SizeOfMFTEntry);
			
	pRoot = (AttrINDEX_ROOT*)getAttr(144, &mft);	
	IndexRecordClusSize = pRoot->IndexRecordClusSize;		
	
	ret = fileFile_inNode( (U8*) pRoot + sizeof(AttrINDEX_ROOT), &child_vcn, filename );		

	if(ret == 0) return 0;	
	else if(ret > 1){
		get_MFTEntry(ret, findMft);
	
		ChangeFixupData((U16*)findMft,
			findMft->Header.AddrFixUpArray,
			findMft->Header.CountFixUpArray,
			gVol.SizeOfMFTEntry);

		return 1;
	}	
	
	get_RunList( (U8*)getAttr(160, &mft), runData);			
	
	do{
		readCluster(child_vcn, IndexRecordClusSize, runData, IndexRecord);		

		pIndexRecHeader = (INDEX_RECORD_HEADER*) IndexRecord;

		ChangeFixupData((U16*)&IndexRecord,
						pIndexRecHeader->AddrFixUpArray,
						pIndexRecHeader->CountFixUpArray,
						gVol.SizeOfIndexRecord);

		ret = fileFile_inNode( (U8*) IndexRecord + sizeof(INDEX_RECORD_HEADER), &child_vcn , filename);	
		
		switch(ret){
		case 0: 
			 return 0;
		case 1: 
			 break;
		default: 
			get_MFTEntry(ret, findMft);
	
			ChangeFixupData((U16*)findMft,
							findMft->Header.AddrFixUpArray,
							findMft->Header.CountFixUpArray,
							gVol.SizeOfMFTEntry);
			return 1;
		}
		
	}while(1);	

	return 0;
}

U64 fileFile_inNode(U8* p, U64* child_vcn, char* filename){
	NodeHeader*		pNode;
	IndexEntry*		pEntry;
	AttrFILENAME*	pAttrName;	
	char			final[512]={0,};	
	
	pNode =  (NodeHeader*) p ;
	pEntry =  (IndexEntry*)((char*) pNode + pNode->AddrFirstIndex);
	
	while(1){
		if(pEntry->Header.Flags & 0x02){
			if(pEntry->Header.Flags & 0x01){
				*child_vcn = *((U64*)&pEntry->Data[pEntry->Header.LenOfEntry-8]);									
				return 1;
			}else				
				return 0;			
		}
		pAttrName = (AttrFILENAME*)&pEntry->Data[16];
		wcstombs( final, &pAttrName->FileName, pAttrName->LenOfName );
		final[pAttrName->LenOfName] = 0;

		ChangetoUpper(final);		
		
		if( strcmp(final,filename) > 0 ){
			if(pEntry->Header.Flags & 0x01){
				*child_vcn = *((U64*)&pEntry->Data[pEntry->Header.LenOfEntry-8]);				
				return 1;
			}else
				return 0;			
		}

		if( ! strcmp(filename, final) )			
			return (pEntry->Header.FileReferrence & 0x0000FFFFFFFFFFFF);
		
		pEntry  = (IndexEntry*)((char*)pEntry + pEntry->Header.LenOfEntry);
	}	
}

int readCluster(U64 vcn, U32 ClusCnt, RunData* runData, U8* buf){
	U32 i;
	U32	curVCN = 0;
	U32	len = 0;
	U32	bufOffset = 0;
	U32 readLen = 0, readLCN = 0;
	U32 readSec = 0, readSecCnt = 0;

	for(i = 0; i < MAX_CLUS_RUN; i++){
		
		//read over
		if(runData[i].Len == 0)		return 1; 
		
		if(curVCN == vcn){		
			readLen = runData[i].Len;
			readLCN = runData[i].Offset;
		}
		//�ش� VCN�� ��� �ִ� ���
		else if(vcn < runData[i].Len + curVCN){
			int temp;						
			temp = vcn - curVCN;
			
			readLen = runData[i].Len - temp;
			readLCN = runData[i].Offset + temp;			
		}else{			
			curVCN	= runData[i].Len + curVCN;
			continue;
		}

		curVCN	= runData[i].Len + curVCN;		
		readSec = (readLCN * gVol.SecPerClus) + gVol.VolBeginSec;

		if(ClusCnt > readLen){
			readSecCnt = readLen * gVol.SecPerClus;	
			
			HDD_read(gVol.Drive, readSec, readSecCnt, (char*)&buf[bufOffset]);			
			
			bufOffset = bufOffset + (readLen * gVol.ClusSize);
			vcn		= vcn + readLen;
			ClusCnt = ClusCnt - readLen;
		}else if(ClusCnt <= readLen){			
			readSecCnt = ClusCnt * gVol.SecPerClus;				
			HDD_read(gVol.Drive, readSec, readSecCnt, (char*)&buf[bufOffset]);			
			bufOffset = bufOffset + (ClusCnt * gVol.ClusSize);
			return 0;
		}		
	}
	return 0;
}

void ChangetoUpper(char* name){
	int i = 0;
	while( name[i] != 0 ) name[i++] = toupper(name[i]);
}

void get_RunList(U8* data, RunData* runData){
	int offset = 0;
	int LenSize;
	int OffsetSize;
	int i = 0,runCnt = 0;
	int prevRunOffset = 0;
	
	while(data[offset] != 0){
	
		OffsetSize	= data[offset] >> 4;
		LenSize		= data[offset++] & 0x0F;

		for(i = 0; LenSize != 0; LenSize--, i += 8)
			runData[runCnt].Len |= data[offset++] << i;	
		
		if(OffsetSize == 0){ //sparse			
			runData[runCnt].Offset = 0;
			continue;
		}

		for(i = 4; i > 0; i--){			
			runData[runCnt].Offset = runData[runCnt].Offset >> 8;			
			
			if(OffsetSize != 0){				
				runData[runCnt].Offset = runData[runCnt].Offset & 0x00FFFFFF;
				runData[runCnt].Offset |= data[offset++] << 24;
				OffsetSize--;
			}
		}
		
		runData[runCnt].Offset = prevRunOffset + runData[runCnt].Offset;
		prevRunOffset = runData[runCnt].Offset;		
		runCnt++;
	}
}

//----------------------------

S32  get_MFTEntry(S32 MFTNum, MFTEntry* curMFTEntry){
	S32 readSec = (S32)gVol.MFTStartSec + ( MFTNum * 2 );

	if( HDD_read(gVol.Drive, readSec, 2, (U8*)curMFTEntry) == 0 ){
		printf("MFT Read Failed \n");
		return 1;
	}
	return 0;

}

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