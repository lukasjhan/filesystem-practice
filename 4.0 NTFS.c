#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include "ntfs.h"

void HexDump  (U8 *addr, U32 len);
U32  HDD_read (U8 drv, U32 SecAddr, U32 blocks, U8* buf);
U32  get_BPB_info(NTFS_BPB* pBPB, VolStruct* pVol);

VolStruct gVol;

int main(void) {
	NTFS_BPB	boot;

	gVol.Drive			= 0x1;
	gVol.VolBeginSec	= 63;	
		
	if( HDD_read(gVol.Drive, gVol.VolBeginSec, 1, (U8*)&boot) == 0 ){
		printf("Boot Sector Read Failed \n");
		return 1;
	}

	get_BPB_info( &boot, &gVol);

	printf("[[[[[[ Volume Information ]]]]]]\n");		
	printf("Total Sector \t = %I64d\n",		gVol.TotalSec);
	printf("Cluster Size \t = %d\n",		gVol.ClusSize);
	printf("MFT Start Sec \t = %d \n",		gVol.MFTStartSec);	
	printf("MFTMirr Start Sec \t = %d \n",	gVol.MFTMirrStartSec);
	printf("Size of MFT Entry \t = %d\n",	gVol.SizeOfMFTEntry);
	printf("Size of Index Record \t = %d \n",gVol.SizeOfIndexRecord);
	
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