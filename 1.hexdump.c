#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
 
#define U8		unsigned char
#define S8		char
#define U16		unsigned short
#define U32		unsigned int
#define U64		unsigned __int64
 
U32  HDD_read (U8 drv, U32 SecAddr, U32 blocks, U8* buf);
U32  HDD_write(U8 drv, U32 SecAddr, U32 blocks, U8* buf);
void HexDump  (U8 *addr, U32 len);
 
int main(void) {
	U8 dumpData[512];
 
	HDD_read(0, 0, 1, dumpData);
	HexDump( dumpData, 512);
	return 0;
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
 		if(ret)	ret = 1;	
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
 	g_hDevice=CreateFile(cur_drv, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
 	
 	if(g_hDevice==INVALID_HANDLE_VALUE)	return 0; 
 
 	ldistanceLow  = SecAddr << 9;
 	ldistanceHigh = SecAddr >> (32 - 9);
 	dwpointer = SetFilePointer(g_hDevice, ldistanceLow, (long *)&ldistanceHigh, FILE_BEGIN);
 	
 	if(dwpointer != 0xFFFFFFFF)	{
 		bytestoread = blocks * 512;
 		ret = ReadFile(g_hDevice, buf, bytestoread, (unsigned long*)&numread, NULL);
 		if(ret)	ret = 1;		
 		else		ret = 0;		
 	}
 	
 	CloseHandle(g_hDevice);
 	return ret;
 }
 
 void HexDump  (U8 *addr, U32 len){
 	U8		*s=addr, *endPtr=(U8*)((U32)addr+len);
 	U32		i, remainder=len%16;
 		
 	printf("\n Offset        Hex Value         Ascii value\n");
 	
 	// print out 16 byte blocks.
 	while (s+16<=endPtr){
 		
 		// offset 출력.
 		printf("0x%08lx  ", (long)(s-addr));
 		
 		for (i=0; i<16; i++){
 			printf("%02x ", s[i]);
 		}
 		printf(" ");
 		
 		for (i=0; i<16; i++){
 			if (s[i]>=32 && s[i]<=125)printf("%c", s[i]);
 			else printf(".");
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
 			if (s[i]>=32 && s[i]<=125) printf("%c", s[i]);
 			else	printf(".");
 		}
 		for (i=0; i<(16-remainder); i++){
 			printf(" ");
 		}
 		printf("\n");
 	}
 	return;
 }	// HexDump.