#pragma once

#include "Common.h"
#include "Command.h"
#include "Configurer.h"

#include "StatArchiver.h"
#include "../Models/SimpleMapTable.h"
#include "../Models/SimpleFtlModel.h"

class WorkloadInterface
{
public:
	WorkloadInterface(const class Configurer& config, class StatArchiver& stat)
		: stop(false), lastTime(0), WORKLOAD_CQ_CAPACITY(config.GetWkdConfig().WORKLOAD_CQ_CAPACITY), stat(stat) {}

	bool IsEnd() { return stop; }

	void EnqueTrace()
	{
		EnqueHostCmd();

		while (IsFetchableWorkload())
		{
			wkdCq.push(hostCq.front());
			hostCq.pop();
		}
	}

	virtual void UpdateStat() {};
	queue<HostCmd> wkdCq;
protected:
	virtual bool IsFetchableWorkload() { return ((wkdCq.size() > WORKLOAD_CQ_CAPACITY) || (hostCq.empty()) )? false : true; };
	virtual void EnqueHostCmd() = 0;

	Int64 lastTime;
	bool stop;
	Int32 WORKLOAD_CQ_CAPACITY;

	queue<HostCmd> hostCq;
	class StatArchiver& stat;
};
//
//class SeqEraseSynTrace : public WorkloadInterface
//{
//public:
//	SeqEraseSynTrace(const class Configurer& config)
//		: WorkloadInterface(config)
//		, ocConf(config.GetOcConfig())
//		, synConf(config.GetWkdConfig().synConfig)
//	{
//		openChunk.resize(config.GetOcConfig().PU_PER_GROUP);
//		lastPu = 0;
//
//		for(Int32 i = 0; i < ocConf.PU_PER_GROUP; i++)
//		{
//			for (Int32 j = 0; j < ocConf.CHUNK_PER_PU; j++)
//			{
//				OcAddr addr(i,j,0);
//				hostCq.push(HostCmd(lastTime += synConf.TIME_INTERVAL, Opcode::WRITE, addr, 1));
//			}
//		}
//	}
//
//	void EnqueHostCmd() override
//	{
//		OcAddr addr;
//		addr.pu = lastPu;
//		addr.chunk = openChunk[addr.pu].chunk;
//		hostCq.push(HostCmd(lastTime += synConf.TIME_INTERVAL, Opcode::ERASE, addr, INV_VALUE));
//
//		addr.lb = 0;
//		hostCq.push(HostCmd(lastTime += synConf.TIME_INTERVAL, Opcode::WRITE, addr, 1));
//
//		openChunk[addr.pu].chunk++;
//		if (openChunk[addr.pu].chunk == ocConf.CHUNK_PER_PU)
//		{
//			lastPu++;
//			lastPu %= ocConf.PU_PER_GROUP;
//			openChunk[addr.pu].chunk = 0;
//		}
//	}
//
//	struct OpenChunk
//	{
//		OpenChunk() : chunk(0), wp(0) {}
//
//		Int32 chunk;
//		Int32 wp;
//	};
//
//	vector<OpenChunk> openChunk;
//
//protected:
//	const OcConfig ocConf;
//	const WkdConfig::SyntheticWorkloadConfig synConf;
//
//	Int32 lastPu;
//};
//
//class GausEraseSynTrace : public WorkloadInterface
//{
//public:
//	GausEraseSynTrace(const class Configurer& config)
//		: WorkloadInterface(config)
//		, ocConf(config.GetOcConfig())
//		, synConf(config.GetWkdConfig().synConfig)
//		, distribution(0.5, config.GetWkdConfig().synConfig.GAUSSIAN_STD_DEV / 1000.0)
//	{
//		USER_CHUNK_RANGE = Int32((config.GetWkdConfig().USER_ADDR_RATE / 100.0) * ocConf.CHUNK_PER_PU);
//		FREE_CHUNK_CNT = ocConf.CHUNK_PER_PU - USER_CHUNK_RANGE;
//		MIN_CHUNK_NO = FREE_CHUNK_CNT / 2;
//		MAX_CHUNK_NO = MIN_CHUNK_NO + USER_CHUNK_RANGE;
//
//		for (Int32 i = 0; i < ocConf.PU_PER_GROUP; i++)
//		{
//			for (Int32 j = MIN_CHUNK_NO; j < MAX_CHUNK_NO; j++)
//			{
//				OcAddr addr(i, j, 0);
//				hostCq.push(HostCmd(lastTime += synConf.TIME_INTERVAL, Opcode::WRITE, addr, 1));
//			}
//		}
//
//		generator.seed(config.GetSimConfig().RAND_SEED);
//		
//	}
//
//	
//	void EnqueHostCmd() override
//	{
//		OcAddr addr;
//		addr.pu = (rand() % ocConf.PU_PER_GROUP);
//
//		while (true)
//		{
//			double gauAddr = distribution(generator);
//			if ((gauAddr >= 0.0) && (gauAddr < 1.0))
//			{
//				addr.chunk = Int32(MIN_CHUNK_NO + (gauAddr * USER_CHUNK_RANGE));
//				break;
//			}
//		}
//
//		hostCq.push(HostCmd(lastTime += synConf.TIME_INTERVAL, Opcode::ERASE, addr, INV_VALUE));
//
//		addr.lb = 0;
//		hostCq.push(HostCmd(lastTime += synConf.TIME_INTERVAL, Opcode::WRITE, addr, 1));
//	}
//
//protected:
//	Int32 USER_CHUNK_RANGE;
//	Int32 FREE_CHUNK_CNT;
//	Int32 MIN_CHUNK_NO;
//	Int32 MAX_CHUNK_NO;
//
//	std::normal_distribution<double> distribution;
//	std::default_random_engine generator;
//};


class WearLevelerModule
{
public:
	WearLevelerModule() {}
	virtual ~WearLevelerModule() {}

	virtual bool CheckTriggerCondition(Int32 puNo, Int32 freeChunk, Int32 dataChunk) { return false; }
	virtual void SetCloseChunk(Int32 pu, Int32 chunk) {}
	virtual void SetEraseChunk(Int32 pu, Int32 chunk) {}
};

class BciWl : public WearLevelerModule
{
public:
	BciWl(const class Configurer& config)	
	{
		thBci = config.GetSwConfig().WL_THRESHOLD;

		bciTab.resize(config.GetOcConfig().PU_PER_GROUP);
		for (Int32 i = 0; i < config.GetOcConfig().PU_PER_GROUP; i++)
		{
			bciTab[i].resize(config.GetOcConfig().CHUNK_PER_PU,INV_VALUE);
		}
		curBci.resize(config.GetOcConfig().PU_PER_GROUP);
		dataChkCnt.resize(config.GetOcConfig().PU_PER_GROUP);
	}

	bool CheckTriggerCondition(Int32 puNo, Int32 freeChunk, Int32 dataChunk)
	{
		if ((curBci[puNo] - bciTab[puNo][dataChunk]) > (thBci * dataChkCnt[puNo]))
			return true;
		else
			return false;
	}
	
	void SetCloseChunk(Int32 pu, Int32 chunk)
	{
		bciTab[pu][chunk] = curBci[pu]++;
		dataChkCnt[pu]++;
	}

	void SetEraseChunk(Int32 pu, Int32 chunk)
	{
		bciTab[pu][chunk] = INV_VALUE;
		dataChkCnt[pu]--;
	}

protected:
	struct BciStat
	{
		BciStat(Int32 chunkNo, Int32 closedIndex)
			:chunkNo(chunkNo), closedIndex(closedIndex) {}
		Int32 chunkNo;
		Int32 closedIndex;
	};
	vector<vector<Int32>> bciTab;
	vector<Int32> curBci;
	vector<Int32> dataChkCnt;

	Int32 thBci;
};

class HostFtlModule
{
public:
	HostFtlModule(const class Configurer& config, class OcChunkMngInterface& ocIf)
		: ocIf(ocIf), wlIf(nullptr)
		, mapTab(config.GetOcConfig().PU_PER_GROUP, FullMapTable(Int64(config.GetOcConfig().LBA_PER_CHUNK) * config.GetOcConfig().CHUNK_PER_PU))
		, chkTab(config.GetOcConfig().PU_PER_GROUP, ChunkMapTable(config.GetOcConfig().CHUNK_PER_PU))
	{
		GC_TRIGGER_FREE_CHUNK_LIMIT_CNT = Int32(Int64(config.GetOcConfig().CHUNK_PER_PU) * (config.GetSwConfig().GC_TRIGGER_FREE_CHUNK_RATIO / 100.0));
		if (GC_TRIGGER_FREE_CHUNK_LIMIT_CNT == 0)
			ErrMsg("호스트 GC 최소 청크 개수 설정이 너무 작음\n");

		MAX_SINGLE_REQ_SIZE = config.GetHwConfig().MAX_SINGLE_REQ_SIZE;

		PU_PER_GROUP = config.GetOcConfig().PU_PER_GROUP;;

		LBA_PER_CHUNK = config.GetOcConfig().LBA_PER_CHUNK;
		LBA_PER_PU = LBA_PER_CHUNK * config.GetOcConfig().CHUNK_PER_PU;
		LBA_PER_GROUP = Int64(LBA_PER_PU) * config.GetOcConfig().PU_PER_GROUP;

		USER_LBA_PER_PU = Int64(config.GetOcConfig().LBA_PER_CHUNK) * Int64(config.GetOcConfig().CHUNK_PER_PU * (double(config.GetWkdConfig().USER_ADDR_RATE) / 100.0));
		USER_LBA_PER_GROUP = USER_LBA_PER_PU * config.GetOcConfig().PU_PER_GROUP;

		if (config.GetSwConfig().BLOCK_MANAGEMENT == 0)
			wlIf = new WearLevelerModule;
		else if (config.GetSwConfig().BLOCK_MANAGEMENT == 1)
			wlIf = new BciWl(config);

		WL_THRESHOLD = config.GetSwConfig().WL_THRESHOLD;
		ECC_BIT_MAX = config.GetHwConfig().ECC_BIT_MAX;

		totalWriteVolume = 0;

	}
	Int32 GeFreeVbCnt() { 
		Int32 totalChk = 0;
		for (Int32 i = 0; i < PU_PER_GROUP; i++)
		{
			totalChk += Int32(chkTab[i].freeList.size());
		}
		return totalChk;
	}
	Int32 GeDataVbCnt() {
		Int32 totalChk = 0;
		for (Int32 i = 0; i < PU_PER_GROUP; i++)
		{
			totalChk += Int32(chkTab[i].dataList.size());
		}
		return totalChk;
	}
	Int64 totalWriteVolume;

protected:

	virtual void PushHostCmd(const Int64& time, const Opcode& cmd, const OcAddr& addr, const Int32& size) = 0;


	void ReadOperation(InitCmd& initCmd);
	void WriteOperation(InitCmd& initCmd);
	void ChunkMngOperation(InitCmd& initCmd);

	Int32 GetGcVictimChunk(Int32 puNo);

	void GcEraseOperation(Int64 time, Int32 puNo);
	void GcCopyOperation(Int64 time, Int32 puNo);

	void WlOperation(Int64 time, Int32 puNo)
	{
		Int32 victimChk = chkTab[puNo].dataList.front().index;
		Int32 validCnt = LBA_PER_CHUNK - chkTab[puNo].GetInv(victimChk);
		//printf("WL victim CHUNK %d, valid %d Wli free %d, data %d , datachk total cnt %d \n", victimChk, validCnt, chkTab[puNo].freeList.back().value, chkTab[puNo].dataList.front().value, chkTab[puNo].dataList.size());

		vector<Int64> victimLba;
		Int64 vba = Int64(victimChk) * LBA_PER_CHUNK;
		while (validCnt > 0)
		{
			Int64 lba = mapTab[puNo].GetLogicalSectorAddr(vba++);
			if (lba != INV_VALUE)
			{
				victimLba.push_back(lba);
				validCnt--;
			}
		}

		OcAddr lastAddr;
		for (Int32 i = 0; i < victimLba.size(); i++)
		{
			OverWriteUpdate(puNo, victimLba[i]);

			OcAddr nexAddr(puNo, GetWlOpenChunk(puNo).index, chkTab[puNo].GetWp(GetWlOpenChunk(puNo).index));
			NewWriteUpdate(puNo, victimLba[i], GetWlOpenChunk(puNo));

			if (lastAddr.vectLbaMap.empty())
				lastAddr = nexAddr;
			else if (lastAddr.chunk != nexAddr.chunk)
			{
				PushHostCmd(time, Opcode::COPY, lastAddr, lastAddr.vectLbaMap.size());
				lastAddr.vectLbaMap.clear();

				lastAddr = nexAddr;
			}

			lastAddr.vectLbaMap.push_back(victimLba[i]);
		}

		PushHostCmd(time, Opcode::COPY, lastAddr, lastAddr.vectLbaMap.size());
	}


	ListedIndex& GetHostOpenChunk(Int32 pu);

	ListedIndex& GetGcOpenChunk(Int32 pu);
	ListedIndex& GetWlOpenChunk(Int32 pu);

	void OverWriteUpdate(Int32 pu, Int64 lba);
	void NewWriteUpdate(Int32 pu, Int64 lba, ListedIndex& openChk);

	Int32 PU_PER_GROUP;

	Int64 USER_LBA_PER_GROUP;
	Int64 USER_LBA_PER_PU;

	Int64 LBA_PER_GROUP;
	Int32 LBA_PER_PU;
	Int32 LBA_PER_CHUNK;

	Int32 GC_TRIGGER_FREE_CHUNK_LIMIT_CNT;
	Int32 MAX_SINGLE_REQ_SIZE;

	Int32 WL_THRESHOLD;
	Int32 ECC_BIT_MAX;

	vector<FullMapTable> mapTab;
	vector<ChunkMapTable> chkTab;

	class OcChunkMngInterface& ocIf;
	class WearLevelerModule* wlIf;
};

class TraceWorkload : public WorkloadInterface, HostFtlModule
{
public:
	TraceWorkload(const class Configurer& config, class OcChunkMngInterface& ocIf, class StatArchiver& stat)
		: WorkloadInterface(config, stat), HostFtlModule(config, ocIf)
		, INFINITE_PLAY(config.GetWkdConfig().realConfig.INFINITE_PLAY)
		, IGNORE_CMD(config.GetWkdConfig().realConfig.IGNORE_CMD)
		, baseTimestamp(0), totalPlayTimeStamp(0), playCycle(0)
	{
		string filePath = (config.GetWkdConfig().realConfig.TRACE_FILE_PATH + config.GetConfigToken().trace + ".csv");
		importFile = fopen(filePath.c_str(), "rb");

		ChkOpenedFile(importFile, filePath.c_str());
	}

	void UpdateStat() override
	{
		stat.GetStat().lastFreeVbCnt = GeFreeVbCnt();
		stat.GetStat().lastDataVbCnt = GeDataVbCnt();
	}
	const Int64 UTC_DIFFERENCE = 11644473600000000;
protected:
	virtual InitCmd GetRawTrace() = 0;
	queue<InitCmd> traceCq;

	void EnqTraceCq()
	{
		while ((traceCq.size() < WORKLOAD_CQ_CAPACITY) && (!stop))
		{
			traceCq.push(GetRawTrace());
		}
	}

	void PushHostCmd(const Int64& time, const Opcode& cmd, const OcAddr& addr, const Int32& size) override
	{
		hostCq.push(HostCmd(time, cmd, addr, size));
	}

	void EnqueHostCmd()
	{
		EnqTraceCq();

		while (!traceCq.empty() && (hostCq.size() < WORKLOAD_CQ_CAPACITY))
		{
			InitCmd initCmd = traceCq.front();
			traceCq.pop();

			if (IGNORE_CMD == 1 && initCmd.op == Opcode::READ)
				continue;
			else if (IGNORE_CMD == 2 && initCmd.op == Opcode::WRITE)
				continue;


			initCmd.lba = initCmd.lba % LBA_PER_GROUP;
			Int64 puLba = (initCmd.lba / LBA_PER_PU) * LBA_PER_PU;
			Int64 offLba = (initCmd.lba % LBA_PER_PU) % (USER_LBA_PER_PU - initCmd.size);
			initCmd.lba = puLba + offLba;

			stat.GetStat().wkdWriteSize += initCmd.size;

			switch (initCmd.op)
			{
			case Opcode::READ:
				ReadOperation(initCmd);
				break;
			case Opcode::WRITE:
				ChunkMngOperation(initCmd);
				WriteOperation(initCmd);
				break;
			default:
				break;
			}
		}
	}

	void CheckReplay(Int64 time)
	{
		if (feof(importFile))
		{
			if (INFINITE_PLAY == 0)
				stop = true;
			else
			{
				if (playCycle == 0)
					totalPlayTimeStamp = time;
				InitFilePointer();
				SetReplayBaseTimestamp(totalPlayTimeStamp);
				//printf("Infinite trace play base time %lld, cycle %d \n", GetBaseTimeStamp(), ++playCycle);
			}
		}
	}
	virtual void InitFilePointer() { fseek(importFile, 0, SEEK_SET); }; //TPC MSN 의 특이성 때문에 재정의를 요함.
	void SetBaseTimestamp(Int64 timestamp) { baseTimestamp = timestamp; }
	void SetReplayBaseTimestamp(Int64 lastTimeStamp) { baseTimestamp -= lastTimeStamp; }
	Int64 NormalizeTimestamp(Int64 timestamp) { return timestamp - baseTimestamp; }
	const Int64 GetBaseTimeStamp() { return baseTimestamp; }

	bool INFINITE_PLAY;
	Int64 baseTimestamp;
	Int64 totalPlayTimeStamp;
	Int32 playCycle;
	FILE* importFile;

	Int32 IGNORE_CMD;
};



class SeqWorkload : public WorkloadInterface, HostFtlModule
{
public:
	SeqWorkload(const class Configurer& config, class OcChunkMngInterface& ocIf, class StatArchiver& stat)
		: WorkloadInterface(config, stat), HostFtlModule(config, ocIf)
		, IGNORE_CMD(config.GetWkdConfig().realConfig.IGNORE_CMD)
	{
		lastLbaSeq = 0;
	}

	void UpdateStat() override
	{
		stat.GetStat().lastFreeVbCnt = GeFreeVbCnt();
		stat.GetStat().lastDataVbCnt = GeDataVbCnt();
	}

	Int64 lastLbaSeq;
protected:
	InitCmd GetRawTrace()
	{
		Int64 tempLastLbaSeq = lastLbaSeq;
		lastLbaSeq += 32;
		lastLbaSeq %= LBA_PER_GROUP;
		return InitCmd(0, Opcode::WRITE, tempLastLbaSeq, 32);
	}
	queue<InitCmd> traceCq;

	void EnqTraceCq()
	{
		while ((traceCq.size() < WORKLOAD_CQ_CAPACITY) && (!stop))
		{
			traceCq.push(GetRawTrace());
		}
	}

	void PushHostCmd(const Int64& time, const Opcode& cmd, const OcAddr& addr, const Int32& size) override
	{
		hostCq.push(HostCmd(time, cmd, addr, size));
	}

	void EnqueHostCmd()
	{
		EnqTraceCq();

		while (!traceCq.empty() && (hostCq.size() < WORKLOAD_CQ_CAPACITY))
		{
			InitCmd initCmd = traceCq.front();
			traceCq.pop();

			if (IGNORE_CMD == 1 && initCmd.op == Opcode::READ)
				continue;
			else if (IGNORE_CMD == 2 && initCmd.op == Opcode::WRITE)
				continue;


			initCmd.lba = initCmd.lba % LBA_PER_GROUP;
			Int64 puLba = (initCmd.lba / LBA_PER_PU) * LBA_PER_PU;
			Int64 offLba = (initCmd.lba % LBA_PER_PU) % (USER_LBA_PER_PU - initCmd.size);
			initCmd.lba = puLba + offLba;

			stat.GetStat().wkdWriteSize += initCmd.size;

			switch (initCmd.op)
			{
			case Opcode::READ:
				ReadOperation(initCmd);
				break;
			case Opcode::WRITE:
				ChunkMngOperation(initCmd);
				WriteOperation(initCmd);
				break;
			default:
				break;
			}
		}
	}

	Int32 IGNORE_CMD;
};
