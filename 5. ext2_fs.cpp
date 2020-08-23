#include "ext2_fs.h"
#include <io.h> 

int nPStart = 0; 
SUPERBLOCK* ReadSuperBlock() {
	int offset = 2;
	SUPERBLOCK *sb = (SUPERBLOCK *)malloc(sizeof(*sb));
	// sector * 2 = 1024(ext default block size)
	if( HDD_read(1, nPStart + offset, 2, (char *)sb) == 0 ) {
		printf("Boot Sector Read Failed \n");
		free(sb);
		return NULL;
	}
	return sb;
}

GROUP_DESC* ReadGroupDesc(SBINFO* pSbi) {
	GROUP_DESC *pBlkGrps;
	int nBlkGrpNum = pSbi->groups_count;
	int nBlks = 0;				
	while(1) {
		nBlks++;
		nBlkGrpNum -= pSbi->desc_per_block_bits;
		if(nBlkGrpNum <= 0)
			break;
	}
	pBlkGrps = (GROUP_DESC *)malloc(pSbi->blocksize * nBlks);
	if( HDD_read(1, nPStart + pSbi->lba_per_block, nBlks * pSbi->lba_per_block, (char *)pBlkGrps) == 0 ) {
		return NULL;
	}
	pSbi->gdb_count = nBlks;
	return pBlkGrps;
}

BOOL SyncSuperBlock(SBINFO *pSbi) {
	int offset = 2;
	if( HddWrite(1, nPStart + offset, 2, (char *)pSbi->sb) == 0 ) {
		return FALSE;
	}
	return TRUE;
}

BOOL SyncGroupDesc(SBINFO *pSbi) {
	if( HDD_read(1, nPStart + pSbi->lba_per_block, pSbi->gdb_count * pSbi->lba_per_block, (char *)pSbi->group_desc) == 0 ) {
		return FALSE;
	}
	return TRUE;
}

// Block Input/Output
BOOL BlockIO(SBINFO * pSbi, U32 block, char *pBlkBuf, BOOL bWrite) {
	if(bWrite) {
		if( HddWrite(1, nPStart + pSbi->lba_per_block * block, pSbi->lba_per_block, pBlkBuf) == 0 ) {
			printf("Block Write Failed \n");
			return FALSE;
		}
	}
	else {
		if( HDD_read(1, nPStart + pSbi->lba_per_block * block, pSbi->lba_per_block, pBlkBuf) == 0 ) {
			printf("Block Read Failed \n");
			return FALSE;
		}
	}
	return TRUE;
}

// Inode Input/Output (Read or Write)
void InodeIO(SBINFO * pSbi, int nIno, INODE *pInode, BOOL bWrite) {
	SUPERBLOCK *sb = pSbi->sb;
	GROUP_DESC * gdp;
	U32 nBlockGroup, nBlock, nOffset;
	char *pBlkBuf;
	
	nBlockGroup = (nIno - 1) / sb->inodes_per_group;
	gdp = pSbi->group_desc + nBlockGroup;
	nOffset = ((nIno - 1) % sb->inodes_per_group) * pSbi->inode_size;
	nBlock = gdp->inode_table + (nOffset >> pSbi->ext2_block_size_bits);
	nOffset &= (pSbi->blocksize - 1);
	
	pBlkBuf = (char*)malloc(pSbi->blocksize);
	BlockIO(pSbi, nBlock, pBlkBuf, FALSE);
	if(bWrite) {	
		memcpy(pBlkBuf + nOffset, pInode, sizeof(INODE));
		BlockIO(pSbi, nBlock, pBlkBuf, TRUE);
	}
	else {
		memcpy(pInode, pBlkBuf + nOffset, sizeof(INODE));
	}
	free(pBlkBuf);
}

void InitSbInfo(SUPERBLOCK* sb, SBINFO * pSbi) {
	memset(pSbi, 0, sizeof(SBINFO));
	pSbi->sb = sb;
	pSbi->inode_size = sb->rev_level == EXT2_GOOD_OLD_REV ? 
									EXT2_GOOD_OLD_INODE_SIZE : sb->inode_size;
	pSbi->first_ino = EXT2_GOOD_OLD_FIRST_INO;
	pSbi->blocksize = 1024 << sb->log_block_size;
	pSbi->blocks_per_group = sb->blocks_per_group;
	pSbi->inodes_per_group = sb->inodes_per_group;
	pSbi->inodes_per_block = pSbi->blocksize / pSbi->inode_size;
	pSbi->itb_per_group = pSbi->inodes_per_group / pSbi->inodes_per_block;
	pSbi->desc_per_block = pSbi->blocksize / sizeof (GROUP_DESC);
	pSbi->addr_per_block_bits = pSbi->blocksize / sizeof (U32);
	pSbi->desc_per_block_bits = pSbi->blocksize / sizeof (GROUP_DESC);
	pSbi->groups_count = (sb->blocks_count -	sb->first_data_block + 
				sb->blocks_per_group - 1) / sb->blocks_per_group;
	pSbi->lba_per_block = pSbi->blocksize / 512;
	pSbi->ext2_block_size_bits = sb->log_block_size + 10;
	pSbi->group_desc = ReadGroupDesc(pSbi);
}

int SearchFile(SBINFO * pSbi, char *pszPath, int *pidir) {
	char aszTmp[256];
	char *pszStr = NULL;
	char *pBuf = NULL;
	INODE TmpInode;
	DIRENTRY *pTmpDir = NULL;
	int i, inum;
	
	strcpy(aszTmp, pszPath);
	pszStr = strtok(aszTmp, "/");
	// ROOT DIR
	if(pszStr == NULL)
		return EXT2_ROOT_INO;
	inum = EXT2_ROOT_INO;
	pBuf = (char*)malloc(pSbi->blocksize);
	while(pszStr)
	{
		InodeIO(pSbi, inum, &TmpInode,FALSE);
		if(TmpInode.i_mode & 0x4000 == 0) {
			free(pBuf);
			return 0;
		}
		
		*pidir = inum;
		BlockIO(pSbi, TmpInode.i_block[0], pBuf, FALSE);
		pTmpDir = (DIRENTRY *)pBuf;
		i = inum = 0;
		
		while( i < pSbi->blocksize )	{
			
			if( strncmp(pszStr, pTmpDir->name, pTmpDir->name_len) == 0 ) {
				inum = pTmpDir->inode;
				break;
			}
			i += pTmpDir->rec_len;
			pTmpDir = (DIRENTRY *)((char*)pTmpDir + pTmpDir->rec_len);
		}
		
		if(!inum) {
			free(pBuf);
			return 0;
		}
		pszStr = strtok(NULL, "/");
	}
	free(pBuf);
	return inum;
}

int ReadBlocks(SBINFO *pSbi, char *pData, int *pnPtr, int nLimitBlock, int nLimitSize) {
	int nReadSize = 0, nToRead, i;
	char *pBuf = (char*)malloc(pSbi->blocksize);
	for(i=0; i<nLimitBlock && nReadSize < nLimitSize; i++)
	{
		BlockIO(pSbi, pnPtr[i], pBuf, FALSE);
		nToRead = min(nLimitSize - nReadSize, pSbi->blocksize);
		memcpy(pData+nReadSize, pBuf, nToRead);
		nReadSize += nToRead;
	}
	free(pBuf);
	return nReadSize;
}

char * ReadExtFile(SBINFO *pSbi, INODE *pInode) {
	char *pData = (char *)malloc(pInode->i_size);
	char *pBuf, *pBuf2;
	U32 nReadSize = 0, int_per_block, i;
	
	int_per_block = pSbi->blocksize/4;
	nReadSize = ReadBlocks(pSbi, pData, (int*)(pInode->i_block), EXT2_NDIR_BLOCKS, pInode->i_size);
	if(nReadSize == pInode->i_size)
		return pData;
	
	pBuf = (char*)malloc(pSbi->blocksize);
	BlockIO(pSbi, pInode->i_block[EXT2_IND_BLOCK], pBuf, FALSE);
	nReadSize += ReadBlocks(pSbi, pData+nReadSize, (int*)pBuf, int_per_block, pInode->i_size-nReadSize);
	if(nReadSize == pInode->i_size) {
		free(pBuf);
		return pData;
	}
	
	BlockIO(pSbi, pInode->i_block[EXT2_DIND_BLOCK], pBuf, FALSE);
	pBuf2 = (char*)malloc(pSbi->blocksize);
	for(i=0; i<int_per_block; i++) {
		BlockIO(pSbi, pBuf[i], pBuf2, FALSE);
		nReadSize += ReadBlocks(pSbi, pData+nReadSize, (int*)pBuf, int_per_block, pInode->i_size-nReadSize);
		if(nReadSize == pInode->i_size) {
			free(pBuf);
			free(pBuf2);
			return pData;
		}
	}
	free(pBuf);
	free(pBuf2);
	return NULL;
}

void WriteLocalFile(char *pszFile, char *pBuf, U32 nSize) {
	FILE *pFile = fopen(pszFile, "wb+");
	fwrite(pBuf, 1, nSize, pFile);
	fclose(pFile);
}

U32 main(int argc, char* argv[]) {
	PARTITION PartitionArr[50];
	U32 nPartitionCnt = 0;
	SUPERBLOCK *sb;
	SBINFO sbi;
	INODE Inode;
	int inum = 0, idir;
	char *pBuf = NULL;
	if(argc == 1)
		return 0;

	memset(&PartitionArr, 0, sizeof(PARTITION) * 50);
	nPartitionCnt = GetLocalPartition(1, PartitionArr);
	nPStart = PartitionArr[0].start;
	
	sb = ReadSuperBlock();
	InitSbInfo(sb, &sbi);

	if( strcmp(argv[1], "r") == 0 )	{
		inum = SearchFile(&sbi, argv[2], &idir);
		if(!inum)
			return 0;
		InodeIO(&sbi, inum, &Inode, FALSE);
		pBuf = ReadExtFile(&sbi, &Inode);
		if(pBuf)
			WriteLocalFile(argv[3], pBuf, Inode.i_size);
	}
	if(pBuf)
		free(pBuf);
	free(sbi.group_desc);
	free(sb);
	return 0;
}
