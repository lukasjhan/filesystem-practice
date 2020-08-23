#include <windows.h>
#include <stdio.h>

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

#define PARTITION_TBL_POS	0x1BE
#define PARTITION_TYPE_EXT	0x0F

typedef struct _PARTITION
{
	U8 active;
	U8 begin[3];
	U8 type;
	U8 end[3];
	U32 start;
	U32 length;
} PARTITION, *PPARTITION;

U32	ExtPartionBase;

intHDD_read(char drv, U32 LBA, U32 blocks, char *buf);


U32		GetLocalPartition(U8 Drv, PPARTITION pPartition);

void	SearchExtPartition(U8 drv, PPARTITION pPartition, U32 *pnPartitionCnt, int BaseAddr);

void	GetPartitionTbl(U8 Drv, U32 nSecPos, PPARTITION pPartition, int nReadCnt);

void	PrintPartitionInfo(PPARTITION pPartition, U32 nIdx);

U32 main(void)
{
	PARTITION PartitionArr[50];
	U32 nPartitionCnt = 0;
	U32 i;

	memset(&PartitionArr, 0, sizeof(PARTITION) * 50);
	nPartitionCnt = GetLocalPartition(0, PartitionArr);

	for(i=0; i<nPartitionCnt; i++)
		PrintPartitionInfo(&PartitionArr[i], i);

	return 0;
}

U32 GetLocalPartition(U8 Drv, PPARTITION pPartition)
{
	U32 i;
	U32 nPartitionCnt = 0;
	PPARTITION pPriExtPartition = NULL;


	GetPartitionTbl(Drv, 0, pPartition, 4);
	for(i=0; i<4 && pPartition->length != 0; i++) {
		if(pPartition->type == PARTITION_TYPE_EXT) {
pPriExtPartition = pPartition;
		}
		pPartition++;
		nPartitionCnt++;
	}

	if(!pPriExtPartition)
		return nPartitionCnt;


	ExtPartionBase = pPriExtPartition->start;

	SearchExtPartition(Drv, pPriExtPartition, &nPartitionCnt, 0);
	return nPartitionCnt;
}

void GetPartitionTbl(U8 Drv, U32 nSecPos, PPARTITION pPartition, int nReadCnt)
{
	char pSecBuf[512];


	if( HDD_read(Drv, nSecPos, 1, pSecBuf) == 0 ) {
		printf("Boot Sector Read Failed \n");
		return;
	}

	memcpy(pPartition, (pSecBuf + PARTITION_TBL_POS), sizeof(PARTITION)*nReadCnt);
}

void SearchExtPartition(U8 drv, PPARTITION pPartition, U32 *pnPartitionCnt, int BaseAddr)
{
	int nExtStart= pPartition->start + BaseAddr;
	static int nCnt = 0;


	pPartition++;

	GetPartitionTbl(drv, nExtStart, pPartition, 2);
	while(pPartition->length != 0 && nCnt == 0)
	{
		(*pnPartitionCnt)++;
		if(pPartition->type == PARTITION_TYPE_EXT)
		{
SearchExtPartition(drv, pPartition, pnPartitionCnt, ExtPartionBase);
		} else {
pPartition++;
		}

		if( pPartition->length == 0 )
nCnt = 1;
	}
}

void PrintPartitionInfo(PPARTITION pPartition, U32 nIdx)
{
	fprintf(stdout, "[PARTITION #%d]\n", nIdx);
	fprintf(stdout, "Bootable : 0x%X\n", pPartition->active);
	fprintf(stdout, "Type : 0x%X\n", pPartition->type);
	fprintf(stdout, "Start : %d\n", pPartition->start);
	fprintf(stdout, "  Length : %d\n", pPartition->length);
	fprintf(stdout, "Partition Size : %d MB\n", pPartition->length / 1024 * 512 / 1024);
	fprintf(stdout, "------------------------\n\n");
}