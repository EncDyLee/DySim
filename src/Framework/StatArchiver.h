#pragma once

#include "Common.h"
#include "Command.h"
#include "Time.h"

struct StatDataStruct
{
public:
	Int64 hostReadCnt;
	Int64 hostWriteCnt;
	Int64 hostCopyCnt;
	Int64 hostEraseCnt;

	Int64 hostReadSize;
	Int64 hostWriteSize;
	Int64 hostCopySize;

	Int64 hostReadLatencyUsSum;
	Int64 hostWriteLatencyUsSum;
	Int64 hostCopyLatencyUsSum;
	Int64 hostEraseLatencyUsSum;




	Int64 wkdWriteSize;
	Int64 lastFreeVbCnt;
	Int64 lastDataVbCnt;

	double stdDev;
	Int32 maxEc;
	Int32 minEc;
};

class StatArchiver
{
public:
	StatArchiver();
	void InitStat();

	struct StatDataStruct& GetStat() { return data; }
	
private:

	struct StatDataStruct data;
};
