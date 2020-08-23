#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ISO9660.h"

// Volume Descriptor
U32 ReadVolumeDescriptor(FILE *pFile, VolumeDescriptor *pVD, int Type) {
	int i;
	for(i=0; i<5; i++
	{
		fseek(pFile, 0x8000 + i*0x800, SEEK_SET);		// 0x8000 = 2048 * 16
		fread(pVD, 1, sizeof(*pVD), pFile);
		if(pVD->Type == Type)		
			return 1;
		else if(pVD->Type == 0xFF)	// Terminator Volume Descriptor
			return 0;
	}
	return 0;
}

void ReadPathTable(FILE *pFile, VolumeDescriptor *pVD, PathTableEntry *pPathTable) {
	int size = pVD->vd.Main.PathTableSize.fwd;
	int block_size = pVD->vd.Main.LogicalBlockSize.fwd;
	fseek(pFile, block_size * pVD->vd.Main.LocTypeLPathTable, SEEK_SET);
	fread(pPathTable, size, 1, pFile);
}

void ReadDirectoryRecord(FILE *pFile, VolumeDescriptor *pVD, int DirLoc, DirectoryRecord *pDR) {
	int size = pVD->vd.Main.PathTableSize.fwd;
	int block_size = pVD->vd.Main.LogicalBlockSize.fwd;
	fseek(pFile, block_size * DirLoc, SEEK_SET);
	fread(pDR, block_size, 1, pFile);
}

int ToAnsiEng(char *pszStr, int len) {
	int i, j;
	char *pszDest = pszStr;
	for(i=0, j=0; i<len; i++) {
		if(pszStr[i] != 0)
			pszDest[j++] = pszStr[i];
	}
	pszDest[j] = '\0';
	return j;
}

int FindPathEntry(PathTableEntry * pPathTable, int size, int nParentNum, char * pszPath, int *pDirLoc) {
	int i, PathNum;
	int nThisSize;
	char aszPathName[32];
	
	for(i=0, PathNum=1; i<size; PathNum++) {
		nThisSize = (sizeof(PathTableEntry)-1);
		nThisSize += pPathTable->LenDirectoryIdentifier + pPathTable->ExtendedAttributeRecordLength;
		if(nThisSize % 2 == 1)
			nThisSize++;
		
		if(pPathTable->ParentDirNum == nParentNum) {
			memcpy(aszPathName, pPathTable->PathName, pPathTable->LenDirectoryIdentifier);
			aszPathName[pPathTable->LenDirectoryIdentifier] = '\0';
			ToAnsiEng(aszPathName, pPathTable->LenDirectoryIdentifier);
			if(strcmp(aszPathName, pszPath) == 0) {
				*pDirLoc = pPathTable->Location;
				return PathNum;
			}
		}
		pPathTable = (PathTableEntry *)(((char*)pPathTable) + nThisSize);
		i += nThisSize;
	}
	return 0;
}

int SearchDirectory(PathTableEntry * pPathTable, int size, char *pszPath) {
	char aszTmp[256];
	char *pszStr = NULL;
	int nParentNum = 1;		
	int DirLoc = 0;			// Directory Location
	
	strcpy(aszTmp, pszPath);
	pszStr = strtok(aszTmp, "/");
	while(pszStr) {
		int PathNum = FindPathEntry(pPathTable, size, nParentNum, pszStr, &DirLoc);
		if(PathNum == 0 && DirLoc != 0)
			return DirLoc;

		nParentNum = PathNum;
		pszStr = strtok(NULL, "/");
	}
	return 0;
}

DirectoryRecord * SearchFile(FILE *pFile, VolumeDescriptor *pVD, int DirLoc, char *pszPath) {
	char buf[2048];		
	char aszTmp[256];
	char aszTmp2[256];
	DirectoryRecord *pDR;
	char *pszStr = NULL;
	char *pszFile = NULL;
	int nDirSize, i;	
	
	strcpy(aszTmp, pszPath);
	pszStr = strtok(aszTmp, "/");
	while(1) {
		if(pszStr)
			pszFile = pszStr;
		else
			break;
		pszStr = strtok(NULL, "/");
	}
	
	pDR = (DirectoryRecord *)buf;
	ReadDirectoryRecord(pFile, pVD, DirLoc, pDR);
	
	nDirSize = pDR->DataLength.fwd;
	for(i=0; i<nDirSize; ) {
		if(pDR->DirectoryRecordLength == 0) {
			pDR = (DirectoryRecord *)buf;
			ReadDirectoryRecord(pFile, pVD, ++DirLoc, pDR);
			int block_size = pVD->vd.Main.LogicalBlockSize.fwd;
			i += (block_size - (i % block_size));
			continue;
		}
		
		memcpy(aszTmp2, pDR->FileIdentifier, pDR->FileIdentifierLength);
		aszTmp2[pDR->FileIdentifierLength] = '\0';
		ToAnsiEng(aszTmp2, pDR->FileIdentifierLength);
		if(strstr(aszTmp2, pszFile) != NULL) {
			
			DirectoryRecord *pDestDR = (DirectoryRecord *)malloc(pDR->DirectoryRecordLength);
			memcpy(pDestDR, pDR, pDR->DirectoryRecordLength);
			return pDestDR;
		}
		
		i += pDR->DirectoryRecordLength;
		pDR = (DirectoryRecord *)((char*)pDR + pDR->DirectoryRecordLength);
	}
	return NULL;
}

void CopyFile(FILE *pFile, VolumeDescriptor *pVD, DirectoryRecord *pDR, char *pszPath) {
	int i = 0;
	int size = pVD->vd.Main.PathTableSize.fwd;
	int block_size = pVD->vd.Main.LogicalBlockSize.fwd;
	int file_size = pDR->DataLength.fwd;
	void *pBuf = malloc(block_size);
	FILE *pOutFile = fopen(pszPath, "wb+");
	int WriteSize = 0;

	fseek(pFile, block_size * pDR->LocationOfExtent.fwd, SEEK_SET);
	while(file_size > 0) {
		if(file_size > block_size)
			WriteSize = block_size;
		else
			WriteSize = file_size;
		fread(pBuf, WriteSize, 1, pFile);		
		fwrite(pBuf, WriteSize, 1, pOutFile);	
		file_size -= block_size;
	}
	fclose(pOutFile);
	free(pBuf);
}

int main(int argc, char* argv[]) {
	if(argc != 4) {
		printf("USAGE : ReadCDFS [src-file] [src-path] [dest-file]\n");
		return 0;
	}

	char *pszSrcFile = argv[1];
	char *pszSrcPath = argv[2];
	char *pszDestFile = argv[3];

	FILE *pFile = fopen(pszSrcFile, "rb");
	VolumeDescriptor vd;
	
	int nRet = ReadVolumeDescriptor(pFile, &vd, 0x02);
	if(!nRet)
		ReadVolumeDescriptor(pFile, &vd, 0x01);

	int nPathSize = vd.vd.Main.PathTableSize.fwd;
	PathTableEntry *pPathTable = (PathTableEntry *)malloc(nPathSize);
	ReadPathTable(pFile, &vd, pPathTable);	

	int DirLoc = SearchDirectory(pPathTable, nPathSize, pszSrcPath);
	if(DirLoc == 0) 
		DirLoc = vd.vd.Main.RootDirectoryRecord.LocationOfExtent.fwd;
	DirectoryRecord *pDestDR = SearchFile(pFile, &vd, DirLoc, pszSrcPath);
	if(pDestDR) {
		CopyFile(pFile, &vd, pDestDR, pszDestFile);
		free(pDestDR);
	}

	fclose(pFile);
	if(pPathTable)
		free(pPathTable);
	return 0;
}
