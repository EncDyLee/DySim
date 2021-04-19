#pragma once

#include "../Framework/Configurer.h"
#include "../Framework/HwRsrcInterface.h"
#include "../Framework/Command.h"

class NandModelInterface
{
public:
	NandModelInterface(const Configurer& config)
		:conf(config.GetHwConfig()) {}

	inline const HwConfig& GetConfig() { return conf; }
	inline const HwRsrc& GetWayRsrc(Int32 wayNo) { return way[wayNo]; }


	virtual Int64 SetRead(Int64 time, PhyAddr& addr) = 0;
	virtual Int64 SetRead(Int64 time, PhyAddrMul& addr) = 0;

	virtual Int64 SetProgram(Int64 time, PhyAddrMul& addr) = 0;

	virtual Int64 SetErase(Int64 time, PhyAddr& addr) = 0;
	virtual Int64 SetErase(Int64 time, PhyAddrMul& addr) = 0;

protected:
	vector<HwRsrc> way;
	const HwConfig conf;
};

class SimpleNandModel : public NandModelInterface
{
public:
	SimpleNandModel(const Configurer& config)
		:NandModelInterface(config)
	{	
		for (Int32 i = 0; i < conf.WAY_PER_CHANNEL; i++)
		{		
			way.push_back(HwRsrc(i));
		}
	}

	Int64 SetRead(Int64 time, PhyAddr& addr)
	{
		if (way[addr.wayNo].readyTime > time)
		{
			way[addr.wayNo].readyTime += conf.READ_TIME_PAGE;
			way[addr.wayNo].sumBusyTime += conf.READ_TIME_PAGE;
		}
		else
		{
			way[addr.wayNo].sumIdleTime += (time - way[addr.wayNo].readyTime);
			way[addr.wayNo].readyTime = time + conf.READ_TIME_PAGE;
		}

		return way[addr.wayNo].readyTime;
	}

	Int64 SetRead(Int64 time, PhyAddrMul& addr)
	{
		if (way[addr.wayNo].readyTime < time)
		{
			way[addr.wayNo].sumIdleTime += (time - way[addr.wayNo].readyTime);
			way[addr.wayNo].readyTime = time;
		}

		way[addr.wayNo].readyTime += conf.READ_TIME_PAGE;
		way[addr.wayNo].sumBusyTime += conf.READ_TIME_PAGE;

		return way[addr.wayNo].readyTime;
	}

	Int64 SetProgram(Int64 time, PhyAddrMul& addr)
	{
		if (way[addr.wayNo].readyTime < time)
		{
			way[addr.wayNo].sumIdleTime += (time - way[addr.wayNo].readyTime);
			way[addr.wayNo].readyTime = time;
		}

		if ((addr.pageNo % conf.BIT_PER_CEL) == 0)
		{
			way[addr.wayNo].readyTime += conf.PROGRAM_TIME_SLC_PAGE;
			way[addr.wayNo].sumBusyTime += conf.PROGRAM_TIME_SLC_PAGE;
		}
		else if ((addr.pageNo % conf.BIT_PER_CEL) == 1)
		{
			way[addr.wayNo].readyTime += conf.PROGRAM_TIME_MLC_PAGE;
			way[addr.wayNo].sumBusyTime += conf.PROGRAM_TIME_MLC_PAGE;
		}
		else
		{
			way[addr.wayNo].readyTime += conf.PROGRAM_TIME_TLC_PAGE;
			way[addr.wayNo].sumBusyTime += conf.PROGRAM_TIME_TLC_PAGE;
		}

		return way[addr.wayNo].readyTime;
	}

	Int64 SetErase(Int64 time, PhyAddr& addr)
	{
		if (way[addr.wayNo].readyTime < time)
		{
			way[addr.wayNo].sumIdleTime += (time - way[addr.wayNo].readyTime);
			way[addr.wayNo].readyTime = time;
		}

		way[addr.wayNo].readyTime += conf.ERASE_TIME_BLOCK;
		way[addr.wayNo].sumBusyTime += conf.ERASE_TIME_BLOCK;

		return way[addr.wayNo].readyTime;
	}

	Int64 SetErase(Int64 time, PhyAddrMul& addr)
	{
		if (way[addr.wayNo].readyTime < time)
		{
			way[addr.wayNo].sumIdleTime += (time - way[addr.wayNo].readyTime);
			way[addr.wayNo].readyTime = time;
		}

		way[addr.wayNo].readyTime += conf.ERASE_TIME_BLOCK;
		way[addr.wayNo].sumBusyTime += conf.ERASE_TIME_BLOCK;

		return way[addr.wayNo].readyTime;
	}

private:
	
	
};
