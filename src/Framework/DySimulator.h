#pragma once

#include "Event.h"

#include "ModelingInterface.h"
#include "WorkloadInterface.h"

#include "SyntheticWorkloadModel.h"
#include "TraceWorkloadModel.h"

class DySimulator
{
public:
	DySimulator(const class Configurer& config, class LogReport& log)
		:log(log), config(config), ssd(config, stat), SHUT_DOWN(false)
	{
		SetupWkdModel(config, stat);

		srand(config.GetSimConfig().RAND_SEED);

		MIN_QUEUE_SIZE = config.GetWkdConfig().WORKLOAD_CQ_CAPACITY;
		LOG_PRINT_SEC_TERM = config.GetSimConfig().LOG_PRINT_SEC_TERM;
		OUT_STAND_CQ_CAP = config.GetHwConfig().OUT_STAND_CQ_CAP;

	}

	void RunSimulation()
	{
		PrintBoth(log.GetLogFile(), "==========================================================================================\n");
		PrintBoth(log.GetLogFile(), "Simulation Start \n\n");

		while (!SHUT_DOWN)
		{
			EnqueueEvent();
			ExcuteEvent();
			
			RuntimeLog();
		}


		PrintBoth(log.GetLogFile(), "==========================================================================================\n");
		PrintBoth(log.GetLogFile(), "Simulation Terminate: RealTime: %s, SimTime: %s \n", realTime.TimeProgress().c_str(), simTime.TimeProgress().c_str());

		ResultPrintOut();
	}

	void PrintOutCsv(ResultReport& rst)
	{
		string algorithmName;
		switch (config.GetSwConfig().CHUNK_MAPPING_MODE)
		{
		case 0:
			algorithmName = "Static";
			break;
		case 1:
			algorithmName = "Replace";
			break;
		case 2:
			algorithmName = "Automatic";
			break;
		case 3:
			algorithmName = "Dynamic";
			break;
		default:

			break;
		}
		rst.BufferItemValue("ALGO.", algorithmName);
		rst.BufferItemValue("OP_BLK (%)", to_string(config.GetHwConfig().OVER_PROVISION_BLOCK_RATIO));
		rst.BufferItemValue("GC_TR (%)", to_string(config.GetSwConfig().GC_TRIGGER_FREE_CHUNK_RATIO));
		rst.BufferItemValue("PLN_PER_CHK", to_string(config.GetSwConfig().PLANE_PER_CHUNK));
		rst.BufferItemValue("USER_RATE(%)", to_string(config.GetWkdConfig().USER_ADDR_RATE));
		rst.BufferItemValue("BLK_MANAGE", to_string(config.GetSwConfig().BLOCK_MANAGEMENT));
		rst.BufferItemValue("WL_TH", to_string(config.GetSwConfig().WL_THRESHOLD));
		rst.BufferItemValue("DBG_ERASE_CNT_SCALE", to_string(config.GetSimConfig().DBG_ERASE_COUNT_SCALE));

		const auto totalReadSize = SectToKb(stat.GetStat().hostReadSize); //KB
		const auto totalWriteSize = SectToKb(stat.GetStat().hostWriteSize); //KB
		const auto totalCopySize = SectToKb(stat.GetStat().hostCopySize); //KB

		rst.BufferItemValue("Read Vol. (MB)", to_string(totalReadSize / 1024));
		rst.BufferItemValue("Write Vol. (MB)", to_string(totalWriteSize / 1024));
		rst.BufferItemValue("Copy Vol. (MB)", to_string(totalCopySize / 1024));

		const auto totalReadCnt = stat.GetStat().hostReadCnt;
		const auto totalWriteCnt = stat.GetStat().hostWriteCnt;
		const auto totalEraseCnt = stat.GetStat().hostEraseCnt;

		const double readAvgSize = (totalReadCnt == 0) ? -1 : ((double)stat.GetStat().hostReadSize / totalReadCnt);
		const double writeAvgSize = (totalWriteCnt == 0) ? -1 : ((double)stat.GetStat().hostWriteSize / totalWriteCnt);
		rst.BufferItemValue("Read Avg. (KB)", to_string(readAvgSize));
		rst.BufferItemValue("Write Avg. (KB)", to_string(writeAvgSize));

		const double readLatency = (totalReadCnt == 0) ? -1 : ((double)stat.GetStat().hostReadLatencyUsSum / totalReadCnt);
		const double writeLatency = (totalWriteCnt == 0) ? -1 : ((double)stat.GetStat().hostWriteLatencyUsSum / totalWriteCnt);
		const double eraseLatency = (totalEraseCnt == 0) ? -1 : ((double)stat.GetStat().hostEraseLatencyUsSum / totalEraseCnt);

		rst.BufferItemValue("Read Lat. (us)", to_string(readLatency));
		rst.BufferItemValue("Write Lat. (us)", to_string(writeLatency));
		rst.BufferItemValue("Erase Lat. (us)", to_string(eraseLatency));
		
		wkdIf->UpdateStat();

		rst.BufferItemValue("Free VB Total", to_string(stat.GetStat().lastFreeVbCnt));
		rst.BufferItemValue("Data VB Total", to_string(stat.GetStat().lastDataVbCnt));

		rst.BufferItemValue("Erase (Chunk) Total", to_string(totalEraseCnt));
	
		string ecPerPu;
		Int64 sumOfEc = 0;
		for (Int32 i = 0; i < config.GetOcConfig().PU_PER_GROUP; i++)
		{
			if (!ecPerPu.empty())
				ecPerPu += "_";

			Int64 sumOfEcOnPu = 0;
			for (Int32 j = 0; j < config.GetOcConfig().CHUNK_PER_PU; j++)
			{
				sumOfEc += ssd.ocFtl->GetEraseCnt(i, j);
				sumOfEcOnPu += ssd.ocFtl->GetEraseCnt(i, j);
			}
			ecPerPu += to_string(sumOfEcOnPu);

		}
	
		rst.BufferItemValue("Erase (Chunk) perPu", ecPerPu);

		double avgOfEc = double(sumOfEc) / config.GetOcConfig().CHUNK_PER_PU;
		rst.BufferItemValue("Erase (Chunk) Cnt. Avg.", to_string(avgOfEc));
		double stdDevEc = GetChunkEcStdDev(avgOfEc);
		rst.BufferItemValue("Erase (Chunk) Std.Dev", to_string(stdDevEc));
		
		if (true)
		{
			double sumOfRber = 0;
			double sumOfErrStd = 0;
			Int32 dataChunkCnt = 0;
			for (Int32 i = 0; i < config.GetOcConfig().PU_PER_GROUP; i++)
			{
				for (Int32 j = 0; j < config.GetOcConfig().CHUNK_PER_PU; j++)
				{
					if (ssd.ocFtl->GetWritePtr(i, j) > 0)
					{
						dataChunkCnt++;
						double rberChk = GetBlockRberAverage(i, j);
						sumOfRber += rberChk;
						sumOfErrStd += GetBlockRberStdDeviation(i, j, rberChk);
					}
				}
			}

			double stdDevAvg = double(sumOfErrStd) / dataChunkCnt;
			double rberAvg = double(sumOfRber) / dataChunkCnt;
			rst.BufferItemValue("RBER (Chunk) Std.Dev.", to_string(stdDevAvg));
			rst.BufferItemValue("RBER (Chunk) Avg.", to_string(rberAvg));
		}



		double sumOfRbec = 0;
		double sumOfErrcStd = 0;
		Int32 dataChunkCnt = 0;
		for (Int32 i = 0; i < config.GetOcConfig().PU_PER_GROUP; i++)
		{
			for (Int32 j = 0; j < config.GetOcConfig().CHUNK_PER_PU; j++)
			{
				if (ssd.ocFtl->GetWritePtr(i, j) > 0)
				{
					dataChunkCnt++;
					double rbecChk = GetBlockRbecAverage(i, j);
					sumOfRbec += rbecChk;
					sumOfErrcStd += GetBlockRbecStdDeviation(i, j, rbecChk);
				}
			}
		}

		double stdDevAvgC = double(sumOfErrcStd) / dataChunkCnt;
		double rbecAvg = double(sumOfRbec) / dataChunkCnt;
		rst.BufferItemValue("RBEC (Chunk) Std.Dev.", to_string(stdDevAvgC));
		rst.BufferItemValue("RBEC (Chunk) Avg.", to_string(rbecAvg));

		rst.BufferItemValue("Data Chunk Total Rate (%).", to_string(double(dataChunkCnt) / config.GetOcConfig().CHUNK_PER_PU / config.GetOcConfig().PU_PER_GROUP * 100.0));
		string dataChunRatekInPu;

		for (Int32 i = 0; i < config.GetOcConfig().PU_PER_GROUP; i++)
		{
			double dataChkCntInpu = 0;
			if (!dataChunRatekInPu.empty())
				dataChunRatekInPu += "_";

			for (Int32 j = 0; j < config.GetOcConfig().CHUNK_PER_PU; j++)
			{
				if (ssd.ocFtl->GetWritePtr(i, j) > 0)
				{
					dataChkCntInpu++;
				}
			}
			dataChunRatekInPu += to_string(dataChkCntInpu / config.GetOcConfig().CHUNK_PER_PU);
		}
		rst.BufferItemValue("Data Chunk Total Rate in pu(%).", dataChunRatekInPu);

		rst.BufferItemValue("Data Chunk Rate (%).", to_string(double(dataChunkCnt) / config.GetOcConfig().CHUNK_PER_PU / config.GetOcConfig().PU_PER_GROUP * 100.0));

		rst.ReportItemValue();

		PrintBoth(log.GetLogFile(), "CSV File out success \n");
	}

	void PrintOutBlockStat(LogReport& logRpt)
	{
		///START make header //////////////////////////////////////////////////////////////////////

		fprintf(logRpt.GetLogFile(), ",");
		fprintf(logRpt.GetLogFile(), "Erase count stat ->>");
		for (Int32 i = 0; i < config.GetOcConfig().BLOCK_PER_CHUNK; i++)
		{
			fprintf(logRpt.GetLogFile(), ",");
		}
		fprintf(logRpt.GetLogFile(), ",,");

		fprintf(logRpt.GetLogFile(), "RBER stat ->>");
		for (Int32 i = 0; i < config.GetOcConfig().BLOCK_PER_CHUNK; i++)
		{
			fprintf(logRpt.GetLogFile(), ",");
		}
		fprintf(logRpt.GetLogFile(), ",,");

		fprintf(logRpt.GetLogFile(), "\n");

		fprintf(logRpt.GetLogFile(), "ChunkNo");
		//For erase count
		for (Int32 i = 0; i < config.GetOcConfig().BLOCK_PER_CHUNK; i++)
		{
			fprintf(logRpt.GetLogFile(), ",plane %d", i);
		}
		fprintf(logRpt.GetLogFile(), ", Avg, St.dev");

		//For Bit error
		for (Int32 i = 0; i < config.GetOcConfig().BLOCK_PER_CHUNK; i++)
		{
			fprintf(logRpt.GetLogFile(), ",plane %d", i);
		}
		fprintf(logRpt.GetLogFile(), ", Avg, St.dev");

		fprintf(logRpt.GetLogFile(), "\n");

		///END make header //////////////////////////////////////////////////////////////////////

		for (Int32 i = 0; i < config.GetOcConfig().PU_PER_GROUP; i++)
		{
			for (Int32 j = 0; j < config.GetOcConfig().CHUNK_PER_PU; j++)
			{
				//if (ssd.ocFtl->GetWritePtr(i, j) <= 0)
				//	continue;
				if (ssd.ocFtl->GetWritePtr(i, j) == 0)
					ssd.ocFtl->WriteUpdate(i, j, 0);

				fprintf(logRpt.GetLogFile(), "%d", (i * config.GetOcConfig().CHUNK_PER_PU) + j);
				vector<PhyAddr> phyAddr;

				ssd.ocFtl->TransPhyChunkAddr(i, j, phyAddr);
				if (true)
				{
					Int32 sum = 0;
					for (Int32 k = 0; k < config.GetOcConfig().BLOCK_PER_CHUNK; k++)
					{
						sum += ssd.err.GetEraseCnt(phyAddr[k].wayNo, phyAddr[k].planeNo, phyAddr[k].blkNo);
						fprintf(logRpt.GetLogFile(), ",%d", ssd.err.GetEraseCnt(phyAddr[k].wayNo, phyAddr[k].planeNo, phyAddr[k].blkNo));
					}

					double avg = (double)sum / config.GetOcConfig().BLOCK_PER_CHUNK;
					fprintf(logRpt.GetLogFile(), ",%lf", avg);

					double devSum = 0;
					for (Int32 k = 0; k < config.GetOcConfig().BLOCK_PER_CHUNK; k++)
					{
						devSum += (ssd.err.GetEraseCnt(phyAddr[k].wayNo, phyAddr[k].planeNo, phyAddr[k].blkNo) - avg)
							* (ssd.err.GetEraseCnt(phyAddr[k].wayNo, phyAddr[k].planeNo, phyAddr[k].blkNo) - avg);
					}

					double var = devSum / config.GetOcConfig().BLOCK_PER_CHUNK;
					fprintf(logRpt.GetLogFile(), ",%lf", sqrt(var));
				}
	
				if (true)
				{
					Int32 sum = 0;
					for (Int32 k = 0; k < config.GetOcConfig().BLOCK_PER_CHUNK; k++)
					{
						sum += ssd.err.GetErrBitCnt(phyAddr[k].wayNo, phyAddr[k].planeNo, phyAddr[k].blkNo);
						fprintf(logRpt.GetLogFile(), ",%d", ssd.err.GetErrBitCnt(phyAddr[k].wayNo, phyAddr[k].planeNo, phyAddr[k].blkNo));
					}

					double avg = (double)sum / config.GetOcConfig().BLOCK_PER_CHUNK;
					fprintf(logRpt.GetLogFile(), ",%lf", avg);

					double devSum = 0;
					for (Int32 k = 0; k < config.GetOcConfig().BLOCK_PER_CHUNK; k++)
					{
						devSum += ((ssd.err.GetErrBitCnt(phyAddr[k].wayNo, phyAddr[k].planeNo, phyAddr[k].blkNo) - avg)
							* (ssd.err.GetErrBitCnt(phyAddr[k].wayNo, phyAddr[k].planeNo, phyAddr[k].blkNo) - avg));
					}

					double var = devSum / config.GetOcConfig().BLOCK_PER_CHUNK;
					fprintf(logRpt.GetLogFile(), ",%lf", sqrt(var));
				}

				fprintf(logRpt.GetLogFile(), "\n");
			}
		}
	}

private:
	list<Event*> eventList;
	Int32 OUT_STAND_CQ_CAP;

	WorkloadInterface* wkdIf;
	LogReport& log;


	StatArchiver stat;				//statistics archiver

	RealTime realTime;				//real time for simulator user
	SimTime simTime;				//simulation time for simulation

	SsdModel ssd;

	const class Configurer& config;

	Int64 LOG_PRINT_SEC_TERM;

	Int32 MIN_QUEUE_SIZE;
	bool SHUT_DOWN;

	void EnqueueEvent()
	{
		if (!wkdIf->IsEnd())
			wkdIf->EnqueTrace();
		else if (wkdIf->wkdCq.empty() && eventList.empty())
			SHUT_DOWN = true;


		if ((!wkdIf->wkdCq.empty()) && eventList.empty() && (simTime.GetTime() < wkdIf->wkdCq.front().GetInitTime()))	
			simTime.SetTime(wkdIf->wkdCq.front().GetInitTime());


		while ((eventList.size() < OUT_STAND_CQ_CAP) && (!wkdIf->wkdCq.empty()) && (simTime.GetTime() >= wkdIf->wkdCq.front().GetInitTime()))  //&& (!eventList.empty())  && (eventList.front()->GetTime() > wkdIf->wkdCq.front().GetInitTime()))
		{
			HostCmd& hostCmd = wkdIf->wkdCq.front();
			
			if (simTime.GetTime() > hostCmd.GetInitTime())
				hostCmd.SetInitTime(simTime.GetTime());

			switch (hostCmd.GetOpcode())
			{
			case Opcode::READ:
			{
				ReadEvent* readEv = new ReadEvent(hostCmd, log, stat, SHUT_DOWN, ssd);
				eventList.push_back(readEv);
				eventList.sort(EventTimeCmp);
				wkdIf->wkdCq.pop();
				break;
			}
			case Opcode::WRITE:
			{
				WriteEvent* writeEv = new WriteEvent(hostCmd, log, stat, SHUT_DOWN, ssd);
				eventList.push_back(writeEv);
				eventList.sort(EventTimeCmp);
				wkdIf->wkdCq.pop();
				break;
			}
			case Opcode::COPY:
			{
				CopyEvent* copyEv = new CopyEvent(hostCmd, log, stat, SHUT_DOWN, ssd);
				eventList.push_back(copyEv);
				eventList.sort(EventTimeCmp);
				wkdIf->wkdCq.pop();
				break;
			}
			case Opcode::ERASE:
			{
				EraseEvent* eraseEv = new EraseEvent(hostCmd, log, stat, SHUT_DOWN, ssd);
				eventList.push_back(eraseEv);
				eventList.sort(EventTimeCmp);
				wkdIf->wkdCq.pop();
				break;
			}
			default:
				ErrMsg("Host Cmd opcode error \n");
				break;
			}
		}
	}
	
	void ExcuteEvent()
	{
		if (!eventList.empty())
		{
			simTime.SetTime(eventList.front()->GetTime());
			eventList.front()->Handle();

			if (eventList.front()->IsEnd())
			{
				eventList.front()->UpdateStat();
				delete eventList.front();
				eventList.pop_front();
			}else
				eventList.sort(EventTimeCmp);
		}
	}
	
	enum class WkdType
	{
		REAL_MSR = 0,
		SEQ = 1
	};

	void SetupWkdModel(const class Configurer& config, class StatArchiver& stat)
	{
		switch (WkdType(config.GetWkdConfig().WORKLOAD_TYPE))
		{
		case WkdType::REAL_MSR:
			wkdIf = new Msr(config, *ssd.ocFtl, stat);
			break;

		case WkdType::SEQ:
			wkdIf = new SeqWorkload(config, *ssd.ocFtl, stat);
			break;

		default:
			printf("Workload initializer fail (real type error)");
			abort();
			break;
		}
	
	}

	void ResultPrintOut()
	{
		const auto totalReadCnt = stat.GetStat().hostReadCnt;
		const auto totalWriteCnt = stat.GetStat().hostWriteCnt;
		const auto totalEraseCnt = stat.GetStat().hostEraseCnt;

		const auto totalReadSize = SectToKb(stat.GetStat().hostReadSize); //KB
		const auto totalWriteSize = SectToKb(stat.GetStat().hostWriteSize); //KB

		PrintBoth(log.GetLogFile(), "%-36s%20s\n", "Read Size Sum [MB]"
			, SeparatedNumber(Int64(totalReadSize * config.GetHwConfig().PAGE_SIZE / 1024)).c_str());
		PrintBoth(log.GetLogFile(), "%-36s%20s\n", "Write Size Sum [MB]"
			, SeparatedNumber(Int64(totalWriteSize * config.GetHwConfig().PAGE_SIZE / 1024)).c_str());
		PrintBoth(log.GetLogFile(), "%-36s%20s\n", "Erase Cnt [Chunk]"
			, SeparatedNumber(Int64(totalEraseCnt)).c_str());


		const auto avgEraseCnt = (totalEraseCnt * config.GetSwConfig().PLANE_PER_CHUNK) / (double(config.GetHwConfig().BLOCK_PER_PLANE) * config.GetHwConfig().PLANE_PER_WAY * config.GetHwConfig().WAY_PER_CHANNEL);
		PrintBoth(log.GetLogFile(), "%-36s%20s\n", "Erase Avg [Block]"
			, SeparatedNumber(avgEraseCnt).c_str());


		const double readLatency = (totalReadCnt == 0) ? -1 : ((double)stat.GetStat().hostReadLatencyUsSum / totalReadCnt);
		const double writeLatency = (totalWriteCnt == 0) ? -1 : ((double)stat.GetStat().hostWriteLatencyUsSum / totalWriteCnt);
		const double eraseLatency = (totalEraseCnt == 0) ? -1 : ((double)stat.GetStat().hostEraseLatencyUsSum / totalEraseCnt);

		PrintBoth(log.GetLogFile(), "%-36s%20s\n", "Read Latency avg. [us]"
			, SeparatedNumber(Int64(readLatency)).c_str());
		PrintBoth(log.GetLogFile(), "%-36s%20s\n", "Write Latency avg. [us]"
			, SeparatedNumber(Int64(writeLatency)).c_str());
		PrintBoth(log.GetLogFile(), "%-36s%20s\n", "Erase Latency avg. [us]"
			, SeparatedNumber(Int64(eraseLatency)).c_str());


		const double readBandwidth = (totalReadSize == 0) ? -1 : ((double)totalReadSize / 1024.0 / (simTime.GetTime() / 1000000.0));
		const double writeBandwidth = (totalWriteSize == 0) ? -1 : ((double)totalWriteSize / 1024.0 / (simTime.GetTime() / 1000000.0));

		PrintBoth(log.GetLogFile(), "%-36s%20s\n", "Read Bandwidth avg. [MB/s]"
			, SeparatedNumber(Int64(readBandwidth)).c_str());
		PrintBoth(log.GetLogFile(), "%-36s%20s\n", "Write Bandwidth avg. [MB/s]"
			, SeparatedNumber(Int64(writeBandwidth)).c_str());

	}

	void RuntimeLog()
	{
		if ((IS_CTRL_HANDLER_CALL == true))				// Ctrl+C Printout
			IS_CTRL_HANDLER_CALL = false;
		else if (realTime.GetTimeSec() > log.GetLastLogTime())	 // Time term logRpt printout (logRpt per "LOG_PRINT_SEC_TERM" second)
			log.UpdateLastLogTime();
		else
			return;

		PrintBoth(log.GetLogFile(), "[Time] Sim %s Real %s [Stat] Read %.3f GB, Write %.3f GB, Erase %d CHK, Top Rber/ECC %d/%d \n"
			, simTime.TimeProgress().c_str()
			, realTime.TimeProgress().c_str()
			, (stat.GetStat().hostReadSize * 4 / 1024.0 / 1024.0)
			, (stat.GetStat().hostWriteSize * 4 /1024.0 / 1024.0)
			, stat.GetStat().hostEraseCnt
			, ssd.err.GetTopErrBitCnt()
			, config.GetHwConfig().ECC_BIT_MAX);
	}

	double GetChunkEcStdDev(double mean)
	{
		double devSum = 0;
		for (Int32 i = 0; i < config.GetOcConfig().PU_PER_GROUP; i++)
		{
			for (Int32 j = 0; j < config.GetOcConfig().CHUNK_PER_PU; j++)
			{
				devSum += ((ssd.ocFtl->GetEraseCnt(i, j) - mean) * (ssd.ocFtl->GetEraseCnt(i, j) - mean));
			}
		}

		double var = devSum / (double(config.GetOcConfig().PU_PER_GROUP) * config.GetOcConfig().CHUNK_PER_PU);

		return sqrt(var);
	}


	double GetBlockRbecAverage(Int32 pu, Int32 chunk)
	{
		vector<PhyAddr> phyAddr;
		ssd.ocFtl->TransPhyChunkAddr(pu, chunk, phyAddr);

		Int32 sum = 0;
		for (Int32 i = 0; i < config.GetOcConfig().BLOCK_PER_CHUNK; i++)
		{
			sum += ssd.err.GetErrBitCnt(phyAddr[i].wayNo, phyAddr[i].planeNo, phyAddr[i].blkNo);
		}

		return (double)sum / config.GetOcConfig().BLOCK_PER_CHUNK;
	}

	double GetBlockRbecStdDeviation(Int32 pu, Int32 chunk, double mean)
	{
		vector<PhyAddr> phyAddr;
		ssd.ocFtl->TransPhyChunkAddr(pu, chunk, phyAddr);
		
		double devSum = 0;
		for (Int32 i = 0; i < config.GetOcConfig().BLOCK_PER_CHUNK; i++)
			devSum += (ssd.err.GetErrBitCnt(phyAddr[i].wayNo, phyAddr[i].planeNo, phyAddr[i].blkNo) - mean)
			* (ssd.err.GetErrBitCnt(phyAddr[i].wayNo, phyAddr[i].planeNo, phyAddr[i].blkNo) - mean);
		double var = devSum / config.GetOcConfig().BLOCK_PER_CHUNK;

		return sqrt(var);
	}

	double GetBlockRbecStdDeviation(Int32 pu, Int32 chunk)
	{
		vector<PhyAddr> phyAddr;
		ssd.ocFtl->TransPhyChunkAddr(pu, chunk, phyAddr);

		Int32 sum = 0;
		for (Int32 i = 0; i < config.GetOcConfig().BLOCK_PER_CHUNK; i++)
		{
			sum += ssd.err.GetErrBitCnt(phyAddr[i].wayNo, phyAddr[i].planeNo, phyAddr[i].blkNo);
		}
		double mean = (double)sum / config.GetOcConfig().BLOCK_PER_CHUNK;

		double devSum = 0;
		for (Int32 i = 0; i < config.GetOcConfig().BLOCK_PER_CHUNK; i++)
			devSum += (ssd.err.GetErrBitCnt(phyAddr[i].wayNo, phyAddr[i].planeNo, phyAddr[i].blkNo) - mean) 
			* (ssd.err.GetErrBitCnt(phyAddr[i].wayNo, phyAddr[i].planeNo, phyAddr[i].blkNo) - mean);
		double var = devSum / config.GetOcConfig().BLOCK_PER_CHUNK;

		return sqrt(var);
	}


	double GetBlockRberAverage(Int32 pu, Int32 chunk)
	{
		vector<PhyAddr> phyAddr;
		ssd.ocFtl->TransPhyChunkAddr(pu, chunk, phyAddr);

		double sum = 0;
		for (Int32 i = 0; i < config.GetOcConfig().BLOCK_PER_CHUNK; i++)
		{
			sum += (ssd.err.GetErrBitCnt(phyAddr[i].wayNo, phyAddr[i].planeNo, phyAddr[i].blkNo) / 32768.0);
		}

		return (double)sum / config.GetOcConfig().BLOCK_PER_CHUNK;
	}

	double GetBlockRberStdDeviation(Int32 pu, Int32 chunk, double mean)
	{
		vector<PhyAddr> phyAddr;
		ssd.ocFtl->TransPhyChunkAddr(pu, chunk, phyAddr);

		double devSum = 0;
		for (Int32 i = 0; i < config.GetOcConfig().BLOCK_PER_CHUNK; i++)
			devSum += ((ssd.err.GetErrBitCnt(phyAddr[i].wayNo, phyAddr[i].planeNo, phyAddr[i].blkNo) / 32768.0) - mean)
			* ((ssd.err.GetErrBitCnt(phyAddr[i].wayNo, phyAddr[i].planeNo, phyAddr[i].blkNo) / 32768.0) - mean);
		double var = devSum / config.GetOcConfig().BLOCK_PER_CHUNK;

		return sqrt(var);
	}

	double GetBlockRberStdDeviation(Int32 pu, Int32 chunk)
	{
		vector<PhyAddr> phyAddr;
		ssd.ocFtl->TransPhyChunkAddr(pu, chunk, phyAddr);

		double sum = 0;
		for (Int32 i = 0; i < config.GetOcConfig().BLOCK_PER_CHUNK; i++)
		{
			sum += (ssd.err.GetErrBitCnt(phyAddr[i].wayNo, phyAddr[i].planeNo, phyAddr[i].blkNo) / 32768.0);
		}
		double mean = (double)sum / config.GetOcConfig().BLOCK_PER_CHUNK;

		double devSum = 0;
		for (Int32 i = 0; i < config.GetOcConfig().BLOCK_PER_CHUNK; i++)
			devSum += ((ssd.err.GetErrBitCnt(phyAddr[i].wayNo, phyAddr[i].planeNo, phyAddr[i].blkNo) - mean)
			* (ssd.err.GetErrBitCnt(phyAddr[i].wayNo, phyAddr[i].planeNo, phyAddr[i].blkNo) - mean));
		double var = devSum / config.GetOcConfig().BLOCK_PER_CHUNK;

		return sqrt(var);
	}
};