 #include <stdio.h>
 #include <windows.h>
 #include <stdlib.h>
 
 #pragma pack(1)
 typedef struct _DIR_struct{
 	U8		Name[11];
 	U8		Attr;
 	U8		NTRes;
 	U8		CrtTimeTenth;
 	U16		CrtTime;
 	U16		CrtDate;
 	U16		LstAccDate;
 	U16		FstClusHi;
 	U16		WriteTime;
 	U16		WriteDate;
 	U16		FstClusLow;
 	U32		FileSize;
 }DirEntry;
 #pragma pack()
 
 #pragma pack(1)
 typedef struct _LONG_DIR_struct{
 	U8		Order;
 	U8		Name1[10];
 	U8		Attr;
 	U8		Type;
 	U8		chksum;
 	U8		Name2[12];
 	U16		FstClusLo;
 	U8		Name3[4];	
 }LongDirEntry;
 #pragma pack()
 
 void print_longName(LongDirEntry* pLongDir, U32 EntryNum);
 U32  show_dir (DirEntry* pDir);
 U32  get_BPB_info(FAT16_BPB* BPB, VolStruct* pVol);
 U32  HDD_read (U8 drv, U32 SecAddr, U32 blocks, U8* buf);
 
 VolStruct gVol;
 
 int main(void) {
 	U8		    buf[512];
 	U8*		    pRootBuf;
 
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
 		
 	pRootBuf = (U8*)malloc( gVol.RootDirSecCnt * 512);
 	
 	if( HDD_read(gVol.Drive, gVol.RootDirSec, gVol.RootDirSecCnt, pRootBuf) == 0 ){
 		printf("Root Dir Read Failed \n");
 		return 1;
 	}
 
 	show_dir((DirEntry*)pRootBuf);	
 
 	return 0;
 }
 
 U32 show_dir(DirEntry* pDir) {	
 	U32 i, y, LongEntryEn=0;		
 	
 	for(i=0; i<=gVol.RootEntCnt; i++)
 	{	
 		switch( (U8) pDir[i].Name[0]){
 			case 0x00:	return 1;	// End of Entry
 			case 0xE5:	continue;
 		}
 		
 		if(pDir[i].Attr == 0x0F){
 			LongEntryEn = 1;	      //Long File Name Entry
 			continue;
 		}
 		
 		printf("---------Entry Number %d ------------ \n", i);
  
  		if(pDir[i].Attr == 0x10) printf("Directory Name: ");
  		else	                      printf("File Name: ");
 
 		if(LongEntryEn == 1){   // print long file name
 			print_longName((LongDirEntry*)pDir, i-1);
 			LongEntryEn = 0;
 		}else{	                    //printf short file name
 			for(y=0; y < 11; y++) 
 				printf("%c",pDir[i].Name[y]);
 		}
 		printf("\n");
 		printf("File Size     : %d \n",pDir[i].FileSize);
 		printf("Start Cluster : %d \n",(pDir[i].FstClusLow | pDir[i].FstClusHi << 16) );
 	}		
 	return 1;
 }
 
 void print_longName(LongDirEntry* pLongDir, U32 EntryNum){
 	wchar_t 	filename[512];
 	char		final[512];
 	U32		nameOffset = 0;
 	
 	do{
 		memcpy(&filename[nameOffset],pLongDir[EntryNum].Name1, 10); nameOffset += 5;
 		memcpy(&filename[nameOffset],pLongDir[EntryNum].Name2, 12); nameOffset += 6;
 		memcpy(&filename[nameOffset],pLongDir[EntryNum].Name3, 4 ); nameOffset += 2;
 	}while( (pLongDir[EntryNum--].Order & 0x40) == 0 );
 	
 	filename[nameOffset] = 0x0000;
 
 	wcstombs( final, filename, 512 );     
     printf( "%s", final );	
 }
 	do{
 		memcpy(&filename[nameOffset],pLongDir[EntryNum].Name1, 10); nameOffset += 5;
 		memcpy(&filename[nameOffset],pLongDir[EntryNum].Name2, 12); nameOffset += 6;
 		memcpy(&filename[nameOffset],pLongDir[EntryNum].Name3, 4 ); nameOffset += 2;
 	}while( (pLongDir[EntryNum--].Order & 0x40) == 0 );
 	
 	filename[nameOffset] = 0x0000;
 
 	wcstombs( final, filename, 512 );     
         printf( "%s", final );	
 }
