#pragma once

#include "SimpleMapTable.h"
#include "../Framework/Command.h"

class NandErrorModel
{
public:
	NandErrorModel(const Configurer& config)
	{
		topErrBit = 0;
		errConf.ECC_BIT_MAX = config.GetHwConfig().ECC_BIT_MAX;
		errConf.DBG_ERASE_COUNT_SCALE = config.GetSimConfig().DBG_ERASE_COUNT_SCALE;
		
		BlockErrorConfig blkErrConfig;
		blkErrConfig.A_MIN = config.GetHwConfig().A_MIN;
		blkErrConfig.A_MAX = config.GetHwConfig().A_MAX;
		blkErrConfig.B_MIN = config.GetHwConfig().B_MIN;
		blkErrConfig.B_MAX = config.GetHwConfig().B_MAX;

		blkErrConfig.A_ERROR_MIN = config.GetHwConfig().A_ERROR_MIN;
		blkErrConfig.A_ERROR_MAX = config.GetHwConfig().A_ERROR_MAX;
		blkErrConfig.B_ERROR_MIN = config.GetHwConfig().B_ERROR_MIN;
		blkErrConfig.B_ERROR_MAX = config.GetHwConfig().B_ERROR_MAX;

		blkErrConfig.A_RANGE = (blkErrConfig.A_MAX - blkErrConfig.A_MIN);
		blkErrConfig.B_RANGE = (blkErrConfig.B_MAX - blkErrConfig.B_MIN);
		blkErrConfig.A_ERR_RANGE = (blkErrConfig.A_ERROR_MAX - blkErrConfig.A_ERROR_MIN);
		blkErrConfig.B_ERR_RANGE = (blkErrConfig.B_ERROR_MAX - blkErrConfig.B_ERROR_MIN);

		Int32 BLOCK_PER_PLANE = config.GetHwConfig().BLOCK_PER_PLANE + Int32(config.GetHwConfig().BLOCK_PER_PLANE * double(config.GetHwConfig().OVER_PROVISION_BLOCK_RATIO / 100.0));

		rberTab.resize(config.GetHwConfig().WAY_PER_CHANNEL);
		for (Int32 i = 0; i < config.GetHwConfig().WAY_PER_CHANNEL; i++)
		{
			rberTab[i].resize(config.GetHwConfig().PLANE_PER_WAY);
			for (Int32 j = 0; j < config.GetHwConfig().PLANE_PER_WAY; j++)
			{

				rberTab[i][j].resize(BLOCK_PER_PLANE);
				for (Int32 k = 0; k < BLOCK_PER_PLANE; k++)
				{
					normal_distribution<double> aStdRand(((blkErrConfig.A_MIN + blkErrConfig.A_MAX) / 2 * 1E-4), (blkErrConfig.A_RANGE / 4 * 1E-4));
					double aMean = aStdRand(randGenerator);
					aMean = (aMean > 0) ? aMean : 0;

					double aError = (RANDOM(blkErrConfig.A_ERROR_MIN, blkErrConfig.A_ERROR_MAX)) * 1E-5;

					double bMean = (RANDOM(blkErrConfig.B_MIN, blkErrConfig.B_MAX)) * 1E-8;
					double bError = (RANDOM(blkErrConfig.B_ERROR_MIN, blkErrConfig.B_ERROR_MAX)) * 1E-10;


					rberTab[i][j][k].aValueDist = new normal_distribution<double>(aMean, aError);
					rberTab[i][j][k].bValueDist = new normal_distribution<double>(bMean, bError);

					rberTab[i][j][k].eraseCnt = 0;
					rberTab[i][j][k].lastRber = 0;
				}
			}
		}

	}

	void EraseErrBitUpdate(Int32 wayNo, Int32 planeNo, Int32 blkNo)
	{
		rberTab[wayNo][planeNo][blkNo].eraseCnt += errConf.DBG_ERASE_COUNT_SCALE;

		auto& aValueDist = *rberTab[wayNo][planeNo][blkNo].aValueDist;
		auto& bValueDist = *rberTab[wayNo][planeNo][blkNo].bValueDist;

		Int32 errBitCnt = Int32(aValueDist(randGenerator) * exp(bValueDist(randGenerator) * rberTab[wayNo][planeNo][blkNo].eraseCnt));
		topErrBit = (errBitCnt > topErrBit) ? errBitCnt : topErrBit;

		if (errBitCnt > 100)
			printf("WPB (%d,%d,%d) ec %d / rber hi %d cur %d \n", wayNo, planeNo, blkNo, rberTab[wayNo][planeNo][blkNo].eraseCnt, rberTab[wayNo][planeNo][blkNo].lastRber, errBitCnt);


		rberTab[wayNo][planeNo][blkNo].lastRber
			= (errBitCnt > rberTab[wayNo][planeNo][blkNo].lastRber) ? errBitCnt : rberTab[wayNo][planeNo][blkNo].lastRber;

	}

	Int32 GetEraseCnt(Int32 wayNo, Int32 planeNo, Int32 blkNo)
	{
		return rberTab[wayNo][planeNo][blkNo].eraseCnt;
	}
	Int32 GetErrBitCnt(Int32 wayNo, Int32 planeNo, Int32 blkNo)
	{
		return rberTab[wayNo][planeNo][blkNo].lastRber;
	}

	const Int32 GetTopErrBitCnt() { return topErrBit; }
	Int32 topErrBit;


	struct BlockErrorConfig
	{
		int A_MAX;
		int A_MIN;
		int B_MAX;
		int B_MIN;

		int A_ERROR_MIN;
		int A_ERROR_MAX;
		int B_ERROR_MIN;
		int B_ERROR_MAX;

		int A_RANGE;
		int B_RANGE;
		int A_ERR_RANGE;
		int B_ERR_RANGE;
	};

	struct ErrorConfig
	{
		Int32 ECC_BIT_MAX;
		Int32 DBG_ERASE_COUNT_SCALE;
	}errConf;

	struct BlkErrorEntry
	{
		normal_distribution<double>* aValueDist;
		normal_distribution<double>* bValueDist;

		Int32 eraseCnt;
		Int32 lastRber;
	};

	vector<vector<vector<BlkErrorEntry>>> rberTab;
	default_random_engine randGenerator;


protected:
	
};

struct ChunkContent
{
	ChunkContent() : wp(0), wli(0), ec(0) {}

	Int32 wp; //-1: bad, 0: free, > 1 && < LBA_PER_BLOCK: open, == LBA_PER_BLOCK: closed
	Int32 wli;
	Int32 ec;
};

class OcChunkMngInterface
{
public:
	virtual void TransPhyLbaAddr(Int32 pu, Int32 chunk, Int32 lba, PhyAddr& pAddr) = 0;
	virtual void TransPhyChunkAddr(Int32 pu, Int32 chunk, vector<PhyAddr>& pAddr) = 0;

	virtual void WriteUpdate(Int32 pu, Int32 chunk, Int32 lba) = 0;
	virtual bool EraseUpdate(Int32 pu, Int32 chunk) = 0;

	virtual void RberUpdate(Int32 pu, Int32 chunk, Int32 offset, Int32 rber) = 0;

	virtual Int32 GetEraseCnt(Int32 pu, Int32 chunk) = 0;
	virtual Int32 GetWritePtr(Int32 pu, Int32 chunk) = 0;
	const ChunkContent& GetChunkStat(Int32 pu, Int32 chunk) { return chunkTab[pu][chunk]; }

protected:

	vector<vector<ChunkContent>> chunkTab; //chunkTab[pu][chunk]
	vector<Int32> badCntTab;

};

class MultiPlaneOcStaticChunkModel : public OcChunkMngInterface
{
public:
	MultiPlaneOcStaticChunkModel(const class Configurer& config)
		: ocConf(config.GetOcConfig())
	{
		WAY_CNT = config.GetHwConfig().WAY_PER_CHANNEL;
		PLANE_PER_WAY = config.GetHwConfig().PLANE_PER_WAY;
		BLOCK_PER_PLANE = config.GetHwConfig().BLOCK_PER_PLANE + Int32(config.GetHwConfig().BLOCK_PER_PLANE * double(config.GetHwConfig().OVER_PROVISION_BLOCK_RATIO / 100.0));
		PAGE_PER_BLOCK = config.GetHwConfig().PAGE_PER_BLOCK;
		
		chunkTab.resize(ocConf.PU_PER_GROUP);
		badCntTab.resize(ocConf.PU_PER_GROUP);
		for (Int32 i = 0; i < ocConf.PU_PER_GROUP; i++)
		{
			chunkTab[i].resize(ocConf.CHUNK_PER_PU);
			badCntTab[i] = 0;
			for (Int32 j = 0; j < ocConf.CHUNK_PER_PU; j++)
			{
				chunkTab[i][j].ec = 0;
				chunkTab[i][j].wli = 0;
				chunkTab[i][j].wp = 0;
			}
		}

		ECC_BIT_MAX = config.GetHwConfig().ECC_BIT_MAX;
		if (config.GetSwConfig().LIMIT_BAD_CHUNK_RATIO == -1)
			LIMIT_BAD_CNT = INT_MAX;
		else
			LIMIT_BAD_CNT = Int32(config.GetHwConfig().BLOCK_PER_PLANE * (config.GetSwConfig().LIMIT_BAD_CHUNK_RATIO/100.0));
	}

	void TransPhyLbaAddr(Int32 pu, Int32 chunk, Int32 lba, PhyAddr& pAddr) override
	{
		pAddr.planeNo = GetPhyPlaneNo(lba);
		pAddr.wayNo = GetPhyWayNo(pu, lba);
		pAddr.blkNo = GetPhyBlkNo(pu, chunk, lba);
		pAddr.pageNo = GetPhyPageNo(lba);
	}

	void TransPhyChunkAddr(Int32 pu, Int32 chunk, vector<PhyAddr>& pAddr) override
	{
		pAddr.resize(ocConf.PLANE_PER_CHUNK);
		for (Int32 i = 0; i < ocConf.PLANE_PER_CHUNK; i++)
		{
			pAddr[i].planeNo = GetPhyPlaneNo(i);
			pAddr[i].wayNo = GetPhyWayNo(pu, i);
			pAddr[i].blkNo = GetPhyBlkNo(pu, chunk, i);
			pAddr[i].pageNo = INV_VALUE;
		}
	}

	void WriteUpdate(Int32 pu, Int32 chunk, Int32 lba) override
	{
		if (chunkTab[pu][chunk].wp != lba)
			if ((chunkTab[pu][chunk].wp == INV_VALUE) && (lba == 0))
			{
				chunkTab[pu][chunk].wp = 0;
				printf("bad chunk open pu %d, chunk %d, lba %d \n", pu, chunk, lba);
			}
			else
				ErrMsg("lba와 WP가 다름 PU %d, chunk %d, lba %d, wp %d\n", pu, chunk, lba, chunkTab[pu][chunk].wp);
		else if (lba > ocConf.LBA_PER_CHUNK)
			ErrMsg("lba의 최대를 초과함\n");

		chunkTab[pu][chunk].wp++;
	}

	bool EraseUpdate(Int32 pu, Int32 chunk) override
	{
		chunkTab[pu][chunk].ec++;
		chunkTab[pu][chunk].wp = 0;
		if (chunkTab[pu][chunk].wli > ECC_BIT_MAX)
		{
			printf("Device bad update pu %d chunk %d, wli %d \n", pu, chunk, chunkTab[pu][chunk].wli);
			chunkTab[pu][chunk].wp = INV_VALUE;
			badCntTab[pu]++;
			if (badCntTab[pu] > LIMIT_BAD_CNT)
				return false;
		}

		return true;
	}

	void RberUpdate(Int32 pu, Int32 chunk, Int32 offset, Int32 rber) override
	{		
		chunkTab[pu][chunk].wli = (rber > chunkTab[pu][chunk].wli) ? rber : chunkTab[pu][chunk].wli;
		if (chunkTab[pu][chunk].wli > ECC_BIT_MAX)
			printf("UPDATE pu %d, chunk %d, wli %d\n", pu, chunk, chunkTab[pu][chunk].wli);
	}

	Int32 GetEraseCnt(Int32 pu, Int32 chunk) override
	{
		return chunkTab[pu][chunk].ec;
	}

	Int32 GetWritePtr(Int32 pu, Int32 chunk) override
	{
		return chunkTab[pu][chunk].wp;
	}
protected:
	inline Int32 GetPhyWayNo(Int32 pu, Int32 lba) { return (pu * ocConf.WAY_PER_PU) + ((lba / PLANE_PER_WAY) % ocConf.WAY_PER_PU); }
	inline Int32 GetPhyPlaneNo(Int32 lba) { return (lba % PLANE_PER_WAY); }
	virtual inline Int32 GetPhyBlkNo(Int32 pu, Int32 chunk, Int32 lba) { return chunk; }
	inline Int32 GetPhyPageNo(Int32 lba) { return (lba /*/LBA_PER_PAGE*/ / ocConf.PLANE_PER_CHUNK % PAGE_PER_BLOCK); }

	Int32 WAY_CNT;
	Int32 PLANE_PER_WAY;
	Int32 BLOCK_PER_PLANE;
	Int32 PAGE_PER_BLOCK;

	Int32 ECC_BIT_MAX;
	Int32 LIMIT_BAD_CNT;

	const OcConfig ocConf;
};

class MultiPlaneOcStaticReplaceChunkModel : public MultiPlaneOcStaticChunkModel
{
public:
	MultiPlaneOcStaticReplaceChunkModel(const class Configurer& config)
		: MultiPlaneOcStaticChunkModel(config)
	{
		blkMap.resize(WAY_CNT);
		freeBlockList.resize(WAY_CNT);
		badBlockTab.resize(WAY_CNT);
		for (Int32 i = 0; i < WAY_CNT; i++)
		{
			blkMap[i].resize(PLANE_PER_WAY);
			freeBlockList[i].resize(PLANE_PER_WAY);
			badBlockTab[i].resize(PLANE_PER_WAY);
			for (Int32 j = 0; j < PLANE_PER_WAY; j++)
			{
				blkMap[i][j].resize(BLOCK_PER_PLANE);
				for (Int32 k = 0; k < BLOCK_PER_PLANE; k++)
				{
					struct ListedIndex temp(k, 0);
					freeBlockList[i][j].push_back(temp);
				}
			}
		}

		for (Int32 i = 0; i < ocConf.PU_PER_GROUP; i++)
		{
			for (Int32 j = 0; j < ocConf.CHUNK_PER_PU; j++)
			{
				for (Int32 k = 0; k < ocConf.PLANE_PER_CHUNK; k++)
				{
					Int32 wayNo = (i * ocConf.WAY_PER_PU) + ((k / PLANE_PER_WAY) % ocConf.WAY_PER_PU);
					Int32 planeNo = k % PLANE_PER_WAY;
			
					blkMap[wayNo][planeNo][j] = freeBlockList[wayNo][planeNo].front();
					freeBlockList[wayNo][planeNo].pop_front();
				}
			}
		}

		LIMIT_BAD_BLOCK_PER_PLANE = BLOCK_PER_PLANE - config.GetHwConfig().BLOCK_PER_PLANE; // OP BLOCK		
	}

	bool EraseUpdate(Int32 pu, Int32 chunk) override
	{
		chunkTab[pu][chunk].ec++;
		chunkTab[pu][chunk].wp = 0;
		
		for (Int32 i = 0; i < ocConf.PLANE_PER_CHUNK; i++)
		{
			Int32 wayNo = GetPhyWayNo(pu, i);
			Int32 planeNo = GetPhyPlaneNo(i);

			if (blkMap[wayNo][planeNo][chunk].value > ECC_BIT_MAX)
			{
				badBlockTab[wayNo][planeNo].push_back(blkMap[wayNo][planeNo][chunk].index);
				if (badBlockTab[wayNo][planeNo].size() >= LIMIT_BAD_BLOCK_PER_PLANE)
				{
					chunkTab[pu][chunk].wp = INV_VALUE;
					badCntTab[pu]++;
					if(badCntTab[pu] > LIMIT_BAD_CNT)
						return false;
				}
				else
				{
					blkMap[wayNo][planeNo][chunk] = freeBlockList[wayNo][planeNo].front();
					freeBlockList[wayNo][planeNo].pop_front();
				}
			}
		}
	
		return true;
	}

	void RberUpdate(Int32 pu, Int32 chunk, Int32 offset, Int32 rber) override
	{
		Int32 wayNo = GetPhyWayNo(pu, offset);
		Int32 planeNo = GetPhyPlaneNo(offset);

		blkMap[wayNo][planeNo][chunk].value	
			= (rber > blkMap[wayNo][planeNo][chunk].value) 
			? rber : blkMap[wayNo][planeNo][chunk].value;


		chunkTab[pu][chunk].wli
			= (blkMap[wayNo][planeNo][chunk].value > chunkTab[pu][chunk].wli)
			? blkMap[wayNo][planeNo][chunk].value : chunkTab[pu][chunk].wli;
		
	/*	if (chunkTab[pu][chunk].wli > ECC_BIT_MAX)
		{
			printf("UPDATE pu %d, chunk %d, wli %d\n", pu, chunk, chunkTab[pu][chunk].wli);
			for (Int32 i = 0; i < ocConf.PLANE_PER_CHUNK; i++)
			{
				Int32 wayNo = GetPhyWayNo(pu, i);
				Int32 planeNo = GetPhyPlaneNo(i);
				
				printf("maped off %d wayNo %d, plNo %d, blk %d, wli %d\n", i, wayNo, planeNo, blkMap[wayNo][planeNo][chunk].index, blkMap[wayNo][planeNo][chunk].value);
			}

			for (Int32 i = 0; i < ocConf.PLANE_PER_CHUNK; i++)
			{
				Int32 wayNo = GetPhyWayNo(pu, i);
				Int32 planeNo = GetPhyPlaneNo(i);
				printf("way %d pl %d free blk list size %zd\n", wayNo, planeNo, freeBlockList[wayNo][planeNo].size());
			}
		}*/

	}

protected:
	inline Int32 GetPhyBlkNo(Int32 pu, Int32 chunk, Int32 lba)	override { return blkMap[GetPhyWayNo(pu, lba)][GetPhyPlaneNo(lba)][chunk].index;	}

	vector<vector<vector<ListedIndex>>> blkMap; //blkMap[way][pln][blk]
	vector<vector<list<ListedIndex>>> freeBlockList; //freeBlockList[way][pln]
	vector<vector<vector<Int32>>> badBlockTab;

	Int32 LIMIT_BAD_BLOCK_PER_PLANE;
};

class MultiPlaneOcAutoChunkModel : public MultiPlaneOcStaticReplaceChunkModel
{
public:
	MultiPlaneOcAutoChunkModel(const class Configurer& config)
		: MultiPlaneOcStaticReplaceChunkModel(config)
	{
		blkMap.clear();
		freeBlockList.clear();


		blkMap.resize(WAY_CNT);
		freeBlockList.resize(WAY_CNT);
		for (Int32 i = 0; i < WAY_CNT; i++)
		{
			blkMap[i].resize(PLANE_PER_WAY);
			freeBlockList[i].resize(PLANE_PER_WAY);
			for (Int32 j = 0; j < PLANE_PER_WAY; j++)
			{
				blkMap[i][j].resize(BLOCK_PER_PLANE);
				for (Int32 k = 0; k < BLOCK_PER_PLANE; k++)
				{
					struct ListedIndex temp(k, 0);
					freeBlockList[i][j].push_back(temp);
				}
			}
		}

	}

	bool EraseUpdate(Int32 pu, Int32 chunk) override
	{
		chunkTab[pu][chunk].ec++;
		chunkTab[pu][chunk].wp = 0;

		for (Int32 i = 0; i < ocConf.PLANE_PER_CHUNK; i++)
		{
			Int32 wayNo = GetPhyWayNo(pu, i);
			Int32 planeNo = GetPhyPlaneNo(i);

			if (blkMap[wayNo][planeNo][chunk].value > ECC_BIT_MAX)
			{
				printf("Device bad update pu %d chunk %d, ch wli %d, blk wli %d\n", pu, chunk, chunkTab[pu][chunk].wli, blkMap[wayNo][planeNo][chunk].value);
				
				badBlockTab[wayNo][planeNo].push_back(blkMap[wayNo][planeNo][chunk].index);
				if (badBlockTab[wayNo][planeNo].size() >= LIMIT_BAD_BLOCK_PER_PLANE)
				{
					chunkTab[pu][chunk].wp = INV_VALUE;
					badCntTab[pu]++;
					if (badCntTab[pu] > LIMIT_BAD_CNT)
						return false;
				}
			}
			else
			{
				freeBlockList[wayNo][planeNo].push_back(blkMap[wayNo][planeNo][chunk]);
				freeBlockList[wayNo][planeNo].sort(CmpValue);
			}
		}

		return true;
	}

	void WriteUpdate(Int32 pu, Int32 chunk, Int32 lba) override
	{
		MultiPlaneOcStaticChunkModel::WriteUpdate(pu, chunk, lba);

		if (chunkTab[pu][chunk].wp == 1) // Is first write operation
		{
			for (Int32 i = 0; i < ocConf.PLANE_PER_CHUNK; i++)
			{
				Int32 wayNo = GetPhyWayNo(pu, i);
				Int32 planeNo = GetPhyPlaneNo(i);

				blkMap[wayNo][planeNo][chunk] = freeBlockList[wayNo][planeNo].front();
				freeBlockList[wayNo][planeNo].pop_front();
			}
		}
	}

protected:

};

class MultiPlaneOcDynamicChunkModel : public MultiPlaneOcAutoChunkModel
{
public:
	MultiPlaneOcDynamicChunkModel(const class Configurer& config)
		: MultiPlaneOcAutoChunkModel(config)
	{
		freeChunkList.resize(ocConf.PU_PER_GROUP);
		for (Int32 i = 0; i < ocConf.PU_PER_GROUP; i++)
		{
			for (Int32 j = 0; j < ocConf.CHUNK_PER_PU; j++)
			{
				struct ListedIndex temp(j, 0);
				freeChunkList[i].push_back(temp);
			}
		}
	}
	bool EraseUpdate(Int32 pu, Int32 chunk) override
	{
		if (MultiPlaneOcAutoChunkModel::EraseUpdate(pu, chunk) == false)
			return false;

		freeChunkList[pu].push_back(ListedIndex(chunk, chunkTab[pu][chunk].wli));
		freeChunkList[pu].sort(CmpValue);

		return true;
	}
	void WriteUpdate(Int32 pu, Int32 chunk, Int32 lba) override
	{
		MultiPlaneOcStaticChunkModel::WriteUpdate(pu, chunk, lba);

		if (chunkTab[pu][chunk].wp == 1)
		{
			Int32 chunkWliOffset = 0;
			for (auto iter = freeChunkList[pu].begin(); iter != freeChunkList[pu].end(); iter++)
			{
				if (iter->index == chunk)
				{
					freeChunkList[pu].erase(iter);
					break;
				}
				else
					chunkWliOffset++;
			}

			for (Int32 i = 0; i < ocConf.PLANE_PER_CHUNK; i++)
			{
				Int32 wayNo = GetPhyWayNo(pu, i);
				Int32 planeNo = GetPhyPlaneNo(i);

				Int32 blockWliOffset = 0;
				for (auto iter = freeBlockList[wayNo][planeNo].begin(); iter != freeBlockList[wayNo][planeNo].end(); iter++)
				{
					if (blockWliOffset == chunkWliOffset)
					{

						//printf("Way %d, pln %d, block no %d WLI %d, [Worst %d]\n", wayNo, planeNo, iter->index, iter->value, freeBlockList[wayNo][planeNo].back().value);
						blkMap[wayNo][planeNo][chunk] = *iter;
						freeBlockList[wayNo][planeNo].erase(iter);
						break;
					}
					else
						blockWliOffset++;
				}
			}
		}
	}

protected:

	vector<list<ListedIndex>> freeChunkList; //freeChunkList[pu]
};
