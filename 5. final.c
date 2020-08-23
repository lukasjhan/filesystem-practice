#include <time.h>

 BOOL DeleteDirEntry(SBINFO *pSbi, int idir, U32 inum) {
	INODE inode;
	DIRENTRY *pPrevDir = NULL, *pCurDir = NULL;
	char *pBuf;
	int i = 0;

	pBuf = (char*)malloc(pSbi->blocksize);
	InodeIO(pSbi, idir, &inode, FALSE);
	BlockIO(pSbi, inode.i_block[0], pBuf, FALSE);
	pPrevDir = pCurDir = (DIRENTRY *)pBuf;
	 	while( i < pSbi->blocksize ) {
		if(pCurDir->inode == inum) {
			 			pPrevDir->rec_len += pCurDir->rec_len;
			BlockIO(pSbi, inode.i_block[0], pBuf, TRUE);
			free(pBuf);
			return TRUE;
		}
		i += pCurDir->rec_len;
		pPrevDir = pCurDir;
		pCurDir = (DIRENTRY *)((char*)pCurDir + pCurDir->rec_len);
	}
	free(pBuf);
	return FALSE;
}

 void DeleteInodeNBlocks(SBINFO *pSbi, int inum) {
	SUPERBLOCK *sb = pSbi->sb;
	GROUP_DESC * gdp;
	INODE inode;
	char *pBitmap;	 	U32 block_group;
	int ipos, bpos;
	int bytes, bits, i;
	 	InodeIO(pSbi, inum, &inode, FALSE);
	inode.i_dtime = time(NULL);
	InodeIO(pSbi, inum, &inode, TRUE);
	
	block_group = (inum - 1) / sb->inodes_per_group;
	gdp = pSbi->group_desc + block_group;
	pBitmap = (char*)malloc(pSbi->blocksize);
	
	 	BlockIO(pSbi, gdp->inode_bitmap, pBitmap, FALSE);
	ipos = inum - (block_group * sb->inodes_per_group);
	bytes = ipos / 8;
	bits = ipos % 8;
	pBitmap[bytes] &= ~(1 << bits);
	 	BlockIO(pSbi, gdp->inode_bitmap, pBitmap, TRUE);
	gdp->free_inodes_count++;
	sb->free_inodes_count++;

	 	 	BlockIO(pSbi, gdp->block_bitmap, pBitmap, FALSE);
	for(i=0; i<EXT2_NDIR_BLOCKS && inode.i_block[i]; i++) {
		bpos = inode.i_block[i] - (block_group * sb->blocks_per_group);
		bytes = bpos / 8;
		bits = bpos % 8;
		pBitmap[bytes] &= ~(1 << bits);
		gdp->free_blocks_count++;
		sb->free_blocks_count++;
	}
	BlockIO(pSbi, gdp->block_bitmap, pBitmap, TRUE);
	free(pBitmap);
	 	SyncGroupDesc(pSbi);
	SyncSuperBlock(pSbi);
}

 static int FindGroupForFile(SBINFO *pSbi, int parent_inum) {
	SUPERBLOCK *sb = pSbi->sb;
	GROUP_DESC *desc;
	 	int parent_group = (parent_inum - 1) / sb->inodes_per_group;
	int ngroups = pSbi->groups_count;
	int group, i;
	 	group = parent_group;
	desc = pSbi->group_desc + group;
	if (desc && desc->free_inodes_count && desc->free_blocks_count)
		goto found;
	 	group = (group + parent_inum) % ngroups;
	for (i = 1; i < ngroups; i <<= 1) {
		group += i;
		if (group >= ngroups)
			group -= ngroups;
		desc = pSbi->group_desc + group;
		if (desc && desc->free_inodes_count && desc->free_blocks_count)
			goto found;
	}
	 	group = parent_group;
	for (i = 0; i < ngroups; i++) {
		if (++group >= ngroups)
			group = 0;
		desc = pSbi->group_desc + group;
		if (desc && desc->free_inodes_count)
			goto found;
	}
	return -1;
found:
	return group;
}

 int AllocInode(SBINFO *pSbi, int group) {
	int *pBitmap;	 	int i,j;
	int int_per_block = pSbi->blocksize/4;
	pBitmap = (int*)malloc(pSbi->blocksize);
	BlockIO(pSbi, pSbi->group_desc[group].inode_bitmap, (char*)pBitmap, FALSE);
	if(group == 0)
		j = EXT2_GOOD_OLD_FIRST_INO + 1;
	else
		j = 0;
	for(i=0; i<int_per_block; i++) {
		if(pBitmap[i] != 0xFFFFFFFF) {
			int value = pBitmap[i];
			for(; j<32; j++) {
				if( ((value >> j) & 0x01) == 0 ) {
					value |= (1 << j);
					pBitmap[i] = value;
					
					BlockIO(pSbi, pSbi->group_desc[group].inode_bitmap, (char*)pBitmap, TRUE);
					free(pBitmap);
					 					return (pSbi->inodes_per_group * group) + (i * 32 + j);
				}  			}  			j = 0;
		}  	}  	free(pBitmap);
	return 0;
}

 int AllocBlock(SBINFO *pSbi, int group, int alloc_cnt, int *pBlock) {
	int *pBBitmap;	 	int i,j;
	int int_per_block = pSbi->blocksize/4;
	int cnt = 0;
	int offset = pSbi->blocks_per_group * group;

	pBBitmap = (int*)malloc(pSbi->blocksize);
	BlockIO(pSbi, pSbi->group_desc[group].block_bitmap, (char*)pBBitmap, FALSE);
	for(i=0; i<int_per_block; i++) {
block_alloc_start:
		if(pBBitmap[i] != 0xFFFFFFFF) {
			int value = pBBitmap[i];
			for(j=0; j<32; j++) {
				if( ((value >> j) & 0x01) == 0 ) {
					pBlock[cnt++] = offset + (i * 32 + j);
					value |= (1 << j);
					pBBitmap[i] = value;
					 					if(cnt == alloc_cnt)
						goto block_alloc_end;
					else
						goto block_alloc_start;
				}  			}  		}  	}  	return 0;

block_alloc_end:
	BlockIO(pSbi, pSbi->group_desc[group].block_bitmap, (char*)pBBitmap, TRUE);
	free(pBBitmap);

	pSbi->sb->free_blocks_count -= cnt;
	pSbi->group_desc[group].free_blocks_count -= cnt;
	return cnt;
}

 int NewInode(SBINFO *pSbi, int dir_inum, int *pgroup, INODE *inode) {
	int group = FindGroupForFile(pSbi, dir_inum);
	int ino = AllocInode(pSbi, group);
	*pgroup = group;
	memset(inode, 0, sizeof(INODE));
	inode->i_atime = inode->i_ctime = inode->i_mtime = time(NULL);
	inode->i_mode = 0x81a4;	 	inode->i_links_count = 1;
	pSbi->sb->free_inodes_count--;
	pSbi->group_desc[group].free_inodes_count--;
	return ino;
}

  void AllocDirentry(SBINFO *pSbi, char *pszDest, int dir_inum, int inum, INODE *pInode) {
	DIRENTRY direntry;
	DIRENTRY *pCurDir, *pPrevDir;
	char *pBuf;
	int slashmark = 0, new_rec_len, i;
	int pathlen = strlen(pszDest);
	 	for(i=0; i<pathlen; i++) {
		if(pszDest[i] == '/')
			slashmark = i+1;
	}
	 	memset(&direntry, 0, sizeof(DIRENTRY));
	direntry.inode = inum;
	direntry.file_type = 1;
	direntry.name_len = pathlen - slashmark;
	direntry.rec_len = 8 + ((direntry.name_len / 4) + 1) * 4;
	strcpy(direntry.name, pszDest + slashmark);

	 	InodeIO(pSbi, dir_inum, pInode, FALSE);
	pBuf = (char *)malloc(pSbi->blocksize);
	BlockIO(pSbi, pInode->i_block[0], pBuf, FALSE);
	pCurDir = (DIRENTRY *)pBuf;
	
	i = 0;
	while( i < pSbi->blocksize ) {
		i += pCurDir->rec_len;
		pPrevDir = pCurDir;
		pCurDir = (DIRENTRY *)((char*)pCurDir + pCurDir->rec_len);
	}
	new_rec_len = 8 + ((pPrevDir->name_len / 4) + 1) * 4;
	 	direntry.rec_len = pPrevDir->rec_len - new_rec_len;
	pPrevDir->rec_len = new_rec_len;
	
	memcpy(((char*)pPrevDir + pPrevDir->rec_len), &direntry, direntry.rec_len);
	BlockIO(pSbi, pInode->i_block[0], pBuf, TRUE);
	free(pBuf);
}

 void CopyFileLocalToExt(SBINFO *pSbi, char *pszSrcPath, char *pszDest)
{
	INODE inode;
	int block[EXT2_NDIR_BLOCKS] = {0,};
	int dir_inum = 0, inum = 0, group, file_size, blks;
	int write_size = 0, towrite, i;
	char *pFileData;

	 	FILE *pFile = fopen(pszSrcPath, "rb+");
	file_size = filelength(fileno(pFile));
	blks = (file_size / pSbi->blocksize) + ((file_size % pSbi->blocksize) > 0 ? 1 : 0);
	pFileData = (char *)malloc(blks * pSbi->blocksize);
	fread(pFileData, 1, file_size, pFile);
	fclose(pFile);	

	 	if(file_size > pSbi->blocksize * EXT2_NDIR_BLOCKS)
		return ;

	 	SearchFile(pSbi, pszDest, &dir_inum);
	inum = NewInode(pSbi, dir_inum, &group, &inode);
	inode.i_size = file_size;
	AllocBlock(pSbi, group, blks, block);
	memcpy(inode.i_block, block, EXT2_NDIR_BLOCKS);
	InodeIO(pSbi, inum, &inode, TRUE);

	SyncSuperBlock(pSbi);
	SyncGroupDesc(pSbi);

	 	for(i=0; write_size < file_size; i++) {
		towrite = min(file_size - write_size, pSbi->blocksize);
		BlockIO(pSbi, block[i], pFileData+write_size, TRUE);
		write_size += towrite;
	}
	AllocDirentry(pSbi, pszDest, dir_inum, inum, &inode);
}

U32 main2(int argc, char* argv[]) {
	PARTITION PartitionArr[50];
	U32 nPartitionCnt = 0;
	SUPERBLOCK *sb;
	SBINFO sbi;
	int inum = 0, idir;
	char *pBuf = NULL;
	if(argc == 1)
		return 0;
	
	 	memset(&PartitionArr, 0, sizeof(PARTITION) * 50);
	nPartitionCnt = GetLocalPartition(1, PartitionArr);
	nPStart = PartitionArr[0].start;
	 	sb = ReadSuperBlock();
	InitSbInfo(sb, &sbi);
	
	if( strcmp(argv[1], "c") == 0 ) {
		CopyFileLocalToExt(&sbi, argv[2], argv[3]);
	}
	else if( strcmp(argv[1], "d") == 0 ) {
		inum = SearchFile(&sbi, argv[2], &idir);
		if(!inum)
			return 0;
		DeleteDirEntry(&sbi, idir, inum);
		DeleteInodeNBlocks(&sbi, inum);
	}
	
	if(pBuf)
		free(pBuf);
	free(sbi.group_desc);
	free(sb);
	return 0;
}
