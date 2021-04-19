#pragma once

class SimpleBusModel
{
public:
	SimpleBusModel(const class Configurer& config) 
	{
		BUS_TRANS_TIME_PER_PAGE = SecToUs((config.GetHwConfig().PAGE_SIZE) / (config.GetHwConfig().NVME_BANDWIDTH * 1024.0));
	
		for (Int32 i = 0; i < config.GetHwConfig().NVME_LANE_CNT; i++)
		{
			nvmeLane.push_back(HwRsrc(i));
		}
	}

	const Int64 DataTrans(Int64 time, Int32 pageCnt) 
	{ 
		nvmeLane.sort(TimeFreeComp);

		if (time > nvmeLane.front().readyTime)
		{
			nvmeLane.front().sumIdleTime += (time - nvmeLane.front().readyTime);
			nvmeLane.front().readyTime = time;
		}

		nvmeLane.front().sumBusyTime += (BUS_TRANS_TIME_PER_PAGE * pageCnt);
		nvmeLane.front().readyTime += (BUS_TRANS_TIME_PER_PAGE * pageCnt);
		
		return nvmeLane.front().readyTime;
	}

private:
	list<HwRsrc> nvmeLane;
	Int64 BUS_TRANS_TIME_PER_PAGE;
};
