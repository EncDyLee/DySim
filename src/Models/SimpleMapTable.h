#pragma once

#include "../Framework/Configurer.h"

class FullMapTable
{
public:
	FullMapTable(Int64 TOTAL_SECTOR_CNT);

	Int64 GetVirtualSectorAddr(Int64 logicalAddr) { return fullMapLtoV[logicalAddr]; }
	Int64 GetLogicalSectorAddr(Int64 virtualAddr) { return fullMapVtoL[virtualAddr]; }

	void SetLtovMap(Int64 logicalAddr, Int64 virtualAddr) { fullMapLtoV[logicalAddr] = virtualAddr; }
	void SetVtolMap(Int64 virtualAddr, Int64 logicalAddr) { fullMapVtoL[virtualAddr] = logicalAddr; }

private:
	vector<Int64> fullMapLtoV;
	vector<Int64> fullMapVtoL;
};

class ChunkMapTable
{
public:
	ChunkMapTable(Int32 TOTAL_CHUNK_CNT);

	list<ListedIndex> freeList;
	list<ListedIndex> dataList;
	list<ListedIndex> badList;
	list<ListedIndex> invList;

	ListedIndex hostOpenChk;
	ListedIndex gcOpenChk;
	ListedIndex wlOpenChk;

	const Int32 GetEc(Int32 chunkNo) { return ecTab[chunkNo]; }
	const Int32 GetWp(Int32 chunkNo) { return wpTab[chunkNo]; }
	const Int32 GetInv(Int32 chunkNo) { return invTab[chunkNo]; }
	const Int32 GetWli(Int32 chunkNo) { return wliTab[chunkNo]; }

	void SetEc(Int32 chunkNo) { ecTab[chunkNo]++; }
	void SetEc(Int32 chunkNo, Int32 value) { ecTab[chunkNo] = value; }

	void SetWp(Int32 chunkNo) { wpTab[chunkNo]++; }
	void SetWp(Int32 chunkNo, Int32 value) { wpTab[chunkNo] = value; }

	void SetInv(Int32 chunkNo) { invTab[chunkNo]++; }
	void SetInv(Int32 chunkNo, Int32 value) { invTab[chunkNo] = value; }

	void SetWli(Int32 chunkNo, Int32 value) { wliTab[chunkNo] = value; }
	
private:

	vector<Int32> invTab;
	vector<Int32> ecTab;
	vector<Int32> wpTab;
	vector<Int32> wliTab;
};
