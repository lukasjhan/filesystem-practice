 #include <stdio.h>
 #include <windows.h>
 #include <stdlib.h>
 
 typedef struct _File_struct{	
 	U32		curCluster;
 	U32		FileSize;
 	U32		FileRestByte;
 	U32		ClusRestByte;
 	U8*		ClusBuf;
 }FileStruct;
 
 U32  HDD_read (U8 drv, U32 SecAddr, U32 blocks, U8* buf);
 U32  read_file(FileStruct* pFile, U32 read_size, U8* buf);
 U32 open_file(char* FileName, char* FileExtender,DirEntry* pDir, FileStruct* pFile);
 U32  get_BPB_info(FAT16_BPB* BPB, VolStruct* pVol);
 U32  clus_convTo_sec(U32 clusterNum);
 U32  get_next_cluster(U32 curCluster);
 
 VolStruct gVol;
 
 int main(void) {
 	U8		buf[512];
 	U8*		pRootBuf;	
 	U32		readBytes;
 	FileStruct	readFile;
 	FILE*		outFile;
 
 	gVol.Drive	= 0x2;
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
 	
 	if( HDD_read(gVol.Drive, gVol.RootDirSec,gVol.RootDirSecCnt, pRootBuf) == 0 ){
 		printf("Root Dir Read Failed \n");
 		return 1;
 	}
 
 	if( open_file("TEST", "TXT",(DirEntry*)pRootBuf, &readFile) != 0){
 		printf("File Find Error\n");
 		return 1;
 	}
 
 	outFile = fopen("c:\\test.txt","wb");
 
 	if(outFile == NULL){
 		printf("Can't Create File\n");
 		return 1;
 	}
 	
 	do{
 		readBytes = read_file(&readFile, 512, buf);
 		fwrite(buf,readBytes,1,outFile);
 	}while( readBytes != 0 );
 
 	fclose(outFile);
 	
 	return 0;
 }
 
 U32 read_file(FileStruct* pFile, U32 read_size, U8* buf) {
   U32	bkFileRestByte = 0;
   U32      curClusStartSec = 0;
   U32	ThisClusOffset = 0;	
   U32	ThisReadBytes = 0;
 
   bkFileRestByte = pFile->FileRestByte;
 	
   while(1){
     if( pFile->ClusRestByte == 0 ){	
 			
        curClusStartSec = clus_convTo_sec(pFile->curCluster);
 
 	    if( HDD_read(gVol.Drive, curClusStartSec, gVol.SecPerClus, pFile->ClusBuf) == 0 )
 				return 0;
 
 	   pFile->curCluster   = get_next_cluster(pFile->curCluster);
 	   pFile->ClusRestByte = gVol.ClusterSize;
     }
 
     ThisClusOffset = gVol.ClusterSize - pFile->ClusRestByte;
 		
     read_size =(read_size >= pFile->FileRestByte)?pFile->FileRestByte:read_size;		
     ThisReadBytes = (read_size >= pFile->ClusRestByte)?pFile->ClusRestByte:read_size;
 		
     if(ThisReadBytes != 0){			
       memcpy(buf, &pFile->ClusBuf[ThisClusOffset], ThisReadBytes);
 	   pFile->ClusRestByte -= ThisReadBytes;
 	   pFile->FileRestByte -= ThisReadBytes;
 	   read_size             -= ThisReadBytes;			
 	 }else	
 	   return bkFileRestByte - pFile->FileRestByte;	 	 

    }//while()
 }
 
 U32 open_file(char* FileName, char* FileExtender, DirEntry* pDir, FileStruct* pFile) {	
 	U32		i;
 	char		compFile[11];
 	
 	//File Name Copy
 	memset(compFile,      0x20,           sizeof(compFile));
 	memcpy(&compFile[0], FileName,      strlen(FileName));
 	memcpy(&compFile[8], FileExtender, strlen(FileExtender));
 	
 	for(i=0; i<=gVol.RootEntCnt; i++){
      switch( (U8) pDir[i].Name[0]){
 	    case 0x00:	break; // End of Entry
 	    case 0xE5:	continue;  //Deleted Entry
 	  }
 
 	  if(pDir[i].Attr != 0x0F){ 
 	     if( (memcmp(pDir[i].Name, compFile,sizeof(compFile))) == 0){
 	   	  pFile->curCluster   = pDir[i].FstClusLow | pDir[i].FstClusHi << 16;
 		  pFile->FileSize     = pDir[i].FileSize;
 		  pFile->FileRestByte = pFile->FileSize;	
 		  pFile->ClusRestByte = 0;
 		  pFile->ClusBuf      = (U8*)malloc(gVol.ClusterSize);
 				
 		  return 0; // File Found
 	     }
 	  }		
 	}//for
 
 	return 1; // File Not Found
 }
 
 U32 get_next_cluster(U32 curCluster){
 	U16 FATtable[256];
 	U32 ThisFATSecNum, ThisFATEntOffset, tmpFATNum;
 		
 	tmpFATNum = curCluster / 256;
 	ThisFATSecNum = gVol.FATStartSec + tmpFATNum;
 	ThisFATEntOffset = curCluster - ( tmpFATNum * 256 );
 
 	HDD_read(gVol.Drive, ThisFATSecNum, 1, (U8*)FATtable);
 
     return FATtable[ThisFATEntOffset];
 }
 
 U32 clus_convTo_sec(U32 clusterNum){	
 	return ((clusterNum - 2) * gVol.SecPerClus) + gVol.FirstDataSec;
 }
