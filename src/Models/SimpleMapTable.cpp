#include "SimpleMapTable.h"

FullMapTable::FullMapTable(Int64 TOTAL_SECTOR_CNT)
	: fullMapLtoV(TOTAL_SECTOR_CNT, INV_VALUE)
	, fullMapVtoL(TOTAL_SECTOR_CNT, INV_VALUE)
{

}

ChunkMapTable::ChunkMapTable(Int32 TOTAL_CHUNK_CNT)
{
	invTab.resize(TOTAL_CHUNK_CNT);
	ecTab.resize(TOTAL_CHUNK_CNT);
	wpTab.resize(TOTAL_CHUNK_CNT);
	wliTab.resize(TOTAL_CHUNK_CNT);

	for (Int32 i = 0; i < TOTAL_CHUNK_CNT; i++)
	{
		invTab[i] = 0;
		ecTab[i] = 0;
		wpTab[i] = 0;

		ListedIndex temp(i, 0);
		freeList.push_back(temp);
	}

}