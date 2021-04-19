#pragma once

#include "../Framework/Configurer.h"
#include "../Framework/HwRsrcInterface.h"
#include "../Framework/Command.h"

class CleanDirtyBufferModel
{
public:
	CleanDirtyBufferModel(const class Configurer &config)
	{
		Int32 BUF_SLOT_CNT = config.GetHwConfig().DATA_BUF_SIZE / config.GetHwConfig().PAGE_SIZE;

		for (Int32 i = 0; i < BUF_SLOT_CNT; i++)
		{
			HwRsrc temp(i);
			cleanBuf.push_back(temp);
		}
	}

	Int32 GetCleanBufRsrcCnt() { return Int32(cleanBuf.size()); }

	Int64 GetCleanBufRsrc(vector<HwRsrc>& bufSlot)
	{
		for (Int32 i = 0; i < bufSlot.size(); i++)
		{
			bufSlot[i] = cleanBuf.front();
			cleanBuf.pop_front();
		}

		return bufSlot.back().readyTime;
	}

	void SetCleanBufRsrc(HwRsrc& bufSlot)
	{
		cleanBuf.push_back(bufSlot);
		cleanBuf.sort(TimeFreeComp);
	}

	Int64 GetDirtyBufRsrc(vector<HwRsrc>& bufSlot)
	{
		for (Int32 i = 0; i < bufSlot.size(); i++)
		{
			bufSlot[i] = dirtyBuf.front();
			dirtyBuf.pop_front();
		}

		return bufSlot.back().readyTime;
	}

	void SetDirtyBufRsrc(HwRsrc& bufSlot, const OcAddr& ocAddr)
	{
		dirtyBuf.push_back(bufSlot);
		bufMap[bufSlot.index] = ocAddr;
	}

	const OcAddr& GetDirtyBufOcAddr(HwRsrc& bufSlot)
	{
		return bufMap[bufSlot.index];
	}
	

protected:

	vector<OcAddr> bufMap;

	list<HwRsrc> dirtyBuf;
	list<HwRsrc> cleanBuf;
};


class FifoBufModel
{
public:
	FifoBufModel(const class Configurer& config)
	{
		Int32 BUF_SLOT_CNT = config.GetHwConfig().DATA_BUF_SIZE / config.GetHwConfig().PAGE_SIZE;

		for (Int32 i = 0; i < BUF_SLOT_CNT; i++)
		{
			HwRsrc temp(i);
			buf.push_back(temp);
		}
		
	}//buffer size need to increase because it cant proc racking memory resoruce

	
	Int64 AllocBufRsrc(vector<HwRsrc>& bufSlot)
	{
		for (Int32 i = 0; i < bufSlot.size(); i++)
		{
			bufSlot[i] = buf.front();
			buf.pop_front();
		}

		return bufSlot.back().readyTime;
	}

	void FreeBufRsrc(HwRsrc& bufSlot)
	{
		buf.push_back(bufSlot);
		buf.sort(TimeFreeComp);
	}


protected:

	list<HwRsrc> buf;
};
