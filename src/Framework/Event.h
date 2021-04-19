#pragma once

#include "Common.h"
#include "ModelingInterface.h"

class Event
{
public:
	Event(HostCmd& cmd, LogReport& log, StatArchiver& stat, bool& SHUT_DOWN)
		: cmd(cmd), time(cmd.GetInitTime()), step(0), stop(false), log(log), stat(stat), SHUT_DOWN(SHUT_DOWN) {}

	virtual ~Event() {}

	const Int64& GetTime() { return time; }
	bool IsEnd() { return stop; }

	void Handle() {	SetNext( ExcuteStep(step) ); }
	virtual void UpdateStat() = 0;

protected:
	virtual Int64 ExcuteStep(const Int32& step) = 0;

	void SetStop() { stop = true; }
	void SetNext(Int64 nextTime)
	{
		time = nextTime;
		step++;
	}

	LogReport& log;
	StatArchiver& stat;

	HostCmd cmd;
	bool& SHUT_DOWN;
private:
	Int64 time;
	Int32 step;
	bool stop;

	
};

static bool EventTimeCmp(Event* first, Event* second)
{
	if (first->GetTime() < second->GetTime())
		return true;
	else
		return false;
}

class ReadEvent : public Event
{
public:
	ReadEvent(HostCmd& cmd, LogReport& log, StatArchiver& stat, bool& SHUT_DOWN, SsdModel& ssd)
		: Event(cmd, log, stat, SHUT_DOWN), ssd(ssd) {}

	void UpdateStat() override
	{
		stat.GetStat().hostReadCnt++;
		stat.GetStat().hostReadSize += cmd.GetSize();
		stat.GetStat().hostReadLatencyUsSum += (GetTime() - cmd.GetInitTime());
	}

protected:
	Int64 ExcuteStep(const Int32& step) override
	{
		switch (step)
		{
		case 0:
			return AddrTrans();
			break;
		case 1:
			return NandRead(); 
			break;
		case 2:
			SetStop();
			return DataTransfer();
			break;
		default: 
			ErrMsg("Read Event Excution step cnt error %d \n", step);	break;
			break;
		}

		return GetTime();
	}

private:
	SsdModel& ssd;
	queue<PhyAddrMul> multiPlnCq;
	Int64 AddrTrans()
	{
		//1. Address Translation
		vector<PhyAddr> pAddr(cmd.GetSize());
		for (Int32 i = 0; i < cmd.GetSize(); i++)
		{
			ssd.ocFtl->TransPhyLbaAddr(cmd.GetAddr().pu, cmd.GetAddr().chunk, (cmd.GetAddr().lb + i), pAddr[i]);
		}

		queue<PhyAddrMul> multiPlnCq;
		for (Int32 i = 0; i < pAddr.size(); i++)
		{
			if ((multiPlnCq.empty()) || (multiPlnCq.back().wayNo != pAddr[i].wayNo) || (multiPlnCq.back().plnBitMap[pAddr[i].planeNo] == true))
			{
				PhyAddrMul multiPlnCmd(ssd.nand->GetConfig().PLANE_PER_WAY);;
				multiPlnCmd.wayNo = pAddr[i].wayNo;
				multiPlnCmd.pageNo = pAddr[i].pageNo;

				multiPlnCmd.plnBitMap[pAddr[i].planeNo] = true;
				multiPlnCmd.blkNo[pAddr[i].planeNo] = pAddr[i].blkNo;

				multiPlnCq.push(multiPlnCmd);
			}
			else
			{
				multiPlnCq.back().plnBitMap[pAddr[i].planeNo] = true;
				multiPlnCq.back().blkNo[pAddr[i].planeNo] = pAddr[i].blkNo;
			}
		}

		return (GetTime() + 1);
	}
	Int64 NandRead()
	{
		//2. NAND read		
		Int64 nandCmplTime = GetTime();
		while (!multiPlnCq.empty())
		{
			Int64 wayCmplTime = ssd.nand->SetRead(GetTime(), multiPlnCq.front());
			nandCmplTime = (wayCmplTime > nandCmplTime) ? wayCmplTime : nandCmplTime;

			multiPlnCq.pop();
		}

		return nandCmplTime;
	}

	Int64 DataTransfer()
	{
		return ssd.bus.DataTrans(GetTime(), cmd.GetSize());
	}

};

class WriteEvent : public Event
{
public:
	WriteEvent(HostCmd& cmd, LogReport& log, StatArchiver& stat, bool& SHUT_DOWN, SsdModel& ssd)
		: Event(cmd, log, stat, SHUT_DOWN), ssd(ssd) {}

	void UpdateStat() override
	{
		stat.GetStat().hostWriteCnt++;
		stat.GetStat().hostWriteSize += cmd.GetSize();
		stat.GetStat().hostWriteLatencyUsSum += (GetTime() - cmd.GetInitTime());
	}

protected:
	Int64 ExcuteStep(const Int32& step) override
	{
		switch (step)
		{
		case 0:
			return AddrTrans();
			break;
		case 1:
			return DataTransfer();
			break;
		case 2:
			SetStop();
			return NandProgram();
			break;
		default:
			ErrMsg("Write Event Excution step cnt error %d \n", step);	break;
		}

		return GetTime();
	}


private:
	SsdModel& ssd;

	queue<PhyAddrMul> multiPlnCq;
	Int64 AddrTrans()
	{
		//1. Address Translation
		vector<PhyAddr> pAddr(cmd.GetSize());
		for (Int32 i = 0; i < cmd.GetSize(); i++)
		{
			ssd.ocFtl->WriteUpdate(cmd.GetAddr().pu, cmd.GetAddr().chunk, (cmd.GetAddr().lb + i));
			ssd.ocFtl->TransPhyLbaAddr(cmd.GetAddr().pu, cmd.GetAddr().chunk, (cmd.GetAddr().lb + i), pAddr[i]);

			if (pAddr[i].pageNo == 0)
			{
				Int32 rber = ssd.err.GetErrBitCnt(pAddr[i].wayNo, pAddr[i].planeNo, pAddr[i].blkNo);
				ssd.ocFtl->RberUpdate(cmd.GetAddr().pu, cmd.GetAddr().chunk, (cmd.GetAddr().lb + i), rber);
			}
		}

		
		for (Int32 i = 0; i < pAddr.size(); i++)
		{
			if ((multiPlnCq.empty()) || (multiPlnCq.back().wayNo != pAddr[i].wayNo) || (multiPlnCq.back().plnBitMap[pAddr[i].planeNo] == true))
			{
				PhyAddrMul multiPlnCmd(ssd.nand->GetConfig().PLANE_PER_WAY);;
				multiPlnCmd.wayNo = pAddr[i].wayNo;
				multiPlnCmd.pageNo = pAddr[i].pageNo;

				multiPlnCmd.plnBitMap[pAddr[i].planeNo] = true;
				multiPlnCmd.blkNo[pAddr[i].planeNo] = pAddr[i].blkNo;

				multiPlnCq.push(multiPlnCmd);
			}
			else
			{
				multiPlnCq.back().plnBitMap[pAddr[i].planeNo] = true;
				multiPlnCq.back().blkNo[pAddr[i].planeNo] = pAddr[i].blkNo;
			}
		}

		return (GetTime() + 1);
	}
	Int64 DataTransfer()
	{
		return ssd.bus.DataTrans(GetTime(), cmd.GetSize());
	}

	Int64 NandProgram()
	{	
		Int64 nandCmplTime = GetTime();
		while (!multiPlnCq.empty())
		{
			Int64 wayCmplTime = ssd.nand->SetProgram(GetTime(), multiPlnCq.front());
			nandCmplTime = (wayCmplTime > nandCmplTime) ? wayCmplTime : nandCmplTime;

			multiPlnCq.pop();
		}

		return nandCmplTime;
	}
};

class CopyEvent : public Event
{
public:
	CopyEvent(HostCmd& cmd, LogReport& log, StatArchiver& stat, bool& SHUT_DOWN, SsdModel& ssd)
		: Event(cmd, log, stat, SHUT_DOWN), ssd(ssd) {}

	void UpdateStat() override
	{
		stat.GetStat().hostCopyCnt++;
		stat.GetStat().hostCopySize += cmd.GetSize();
		stat.GetStat().hostCopyLatencyUsSum += (GetTime() - cmd.GetInitTime());
	}

protected:
	Int64 ExcuteStep(const Int32& step) override
	{
		switch (step)
		{
		case 0:
			return AddrTrans();
			break;
		case 1:
			return NandRead();
			break;
		case 2:
			SetStop();
			return NandProgram();
			break;
		default:
			ErrMsg("Write Event Excution step cnt error %d \n", step);	break;
		}

		return GetTime();
	}


private:
	SsdModel& ssd;

	queue<PhyAddrMul> multiPlnCq;
	Int64 AddrTrans()
	{
		//1. Address Translation
		vector<PhyAddr> pAddr(cmd.GetSize());
		for (Int32 i = 0; i < cmd.GetSize(); i++)
		{
			ssd.ocFtl->WriteUpdate(cmd.GetAddr().pu, cmd.GetAddr().chunk, (cmd.GetAddr().lb + i));
			ssd.ocFtl->TransPhyLbaAddr(cmd.GetAddr().pu, cmd.GetAddr().chunk, (cmd.GetAddr().lb + i), pAddr[i]);

			if (pAddr[i].pageNo == 0)
			{
				Int32 rber = ssd.err.GetErrBitCnt(pAddr[i].wayNo, pAddr[i].planeNo, pAddr[i].blkNo);
				ssd.ocFtl->RberUpdate(cmd.GetAddr().pu, cmd.GetAddr().chunk, (cmd.GetAddr().lb + i), rber);
			}
		}


		for (Int32 i = 0; i < pAddr.size(); i++)
		{
			if ((multiPlnCq.empty()) || (multiPlnCq.back().wayNo != pAddr[i].wayNo) || (multiPlnCq.back().plnBitMap[pAddr[i].planeNo] == true))
			{
				PhyAddrMul multiPlnCmd(ssd.nand->GetConfig().PLANE_PER_WAY);;
				multiPlnCmd.wayNo = pAddr[i].wayNo;
				multiPlnCmd.pageNo = pAddr[i].pageNo;

				multiPlnCmd.plnBitMap[pAddr[i].planeNo] = true;
				multiPlnCmd.blkNo[pAddr[i].planeNo] = pAddr[i].blkNo;

				multiPlnCq.push(multiPlnCmd);
			}
			else
			{
				multiPlnCq.back().plnBitMap[pAddr[i].planeNo] = true;
				multiPlnCq.back().blkNo[pAddr[i].planeNo] = pAddr[i].blkNo;
			}
		}

		return (GetTime() + 1);
	}

	Int64 NandRead()
	{
		
		return GetTime();
	}

	Int64 NandProgram()
	{
		Int64 nandCmplTime = GetTime();
		while (!multiPlnCq.empty())
		{
			Int64 wayCmplTime = ssd.nand->SetProgram(GetTime(), multiPlnCq.front());
			nandCmplTime = (wayCmplTime > nandCmplTime) ? wayCmplTime : nandCmplTime;

			multiPlnCq.pop();
		}

		return nandCmplTime;
	}
};
class EraseEvent : public Event
{
public:
	EraseEvent(HostCmd& cmd, LogReport& log, StatArchiver& stat, bool& SHUT_DOWN, SsdModel& ssd)
		: Event(cmd, log, stat, SHUT_DOWN), ssd(ssd) {}

	void UpdateStat() override
	{
		stat.GetStat().hostEraseCnt++;
		stat.GetStat().hostEraseLatencyUsSum += (GetTime() - cmd.GetInitTime());
	}
protected:
	Int64 ExcuteStep(const Int32& step) override
	{
		switch (step)
		{
		case 0:
			return AddrTrans();
			break;
		case 1:
			SetStop();
			return NandErase();
			break;
		default:
			ErrMsg("Erase Event Excution step cnt error %d, %d (MAX) \n", step);	break;
		}

		return GetTime();
	}


private:
	SsdModel& ssd;

	queue<PhyAddrMul> multiPlnCq;
	Int64 AddrTrans()
	{
		//1. Address Translation
		vector<PhyAddr> pAddr;
		ssd.ocFtl->TransPhyChunkAddr(cmd.GetAddr().pu, cmd.GetAddr().chunk, pAddr);

		for (Int32 i = 0; i < pAddr.size(); i++)
		{
			if ((multiPlnCq.empty()) || (multiPlnCq.back().wayNo != pAddr[i].wayNo) || (multiPlnCq.back().plnBitMap[pAddr[i].planeNo] == true))
			{
				PhyAddrMul multiPlnCmd(ssd.nand->GetConfig().PLANE_PER_WAY);;
				multiPlnCmd.wayNo = pAddr[i].wayNo;
				multiPlnCmd.pageNo = pAddr[i].pageNo;

				multiPlnCmd.plnBitMap[pAddr[i].planeNo] = true;
				multiPlnCmd.blkNo[pAddr[i].planeNo] = pAddr[i].blkNo;

				multiPlnCq.push(multiPlnCmd);
			}
			else
			{
				multiPlnCq.back().plnBitMap[pAddr[i].planeNo] = true;
				multiPlnCq.back().blkNo[pAddr[i].planeNo] = pAddr[i].blkNo;
			}
		}


		//3. Chunk Stat update
		bool isOk = ssd.ocFtl->EraseUpdate(cmd.GetAddr().pu, cmd.GetAddr().chunk);
		if ((isOk == false))
			SHUT_DOWN = true;
		//	if (isOk == false)
		//		printf("BAD PU %d, CHK %d\n", cmd.GetAddr().pu, cmd.GetAddr().chunk);


		//2. Error stat update
		for (Int32 i = 0; i < pAddr.size(); i++)
		{
			ssd.err.EraseErrBitUpdate(pAddr[i].wayNo, pAddr[i].planeNo, pAddr[i].blkNo);
		}

		return (GetTime() + 1);
	}

	Int64 NandErase()
	{
		//2. NAND Erase	
		Int64 nandCmplTime = GetTime();
		while (!multiPlnCq.empty())
		{
			Int64 wayCmplTime = ssd.nand->SetErase(GetTime(), multiPlnCq.front());
			nandCmplTime = (wayCmplTime > nandCmplTime) ? wayCmplTime : nandCmplTime;

			multiPlnCq.pop();
		}

		return nandCmplTime;
	}
};
