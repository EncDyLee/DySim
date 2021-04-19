#pragma once
#include "Common.h"

using namespace chrono;

class RealTime
{
public:
	RealTime(): startTime(steady_clock::now()) {}
	string TimeProgress() const;
	Int32 GetTimeSec();
private:
	steady_clock::time_point startTime;	// 시뮬레이션 실제 수행 시간을 측정
};

class SimTime
{
public:
	SimTime(): simTime(0) {}

	string TimeProgress() const;

	inline Int64 GetTime() const { return simTime; }

	inline void ResetSimTime() { simTime = 0; }
	inline void InitilizeSimTime(Int64 initTime) { simTime = initTime; }
	inline void SetTime(Int64 time) 
	{ 
		if (simTime > time)
			printf("RVS TIME cur  %lld, nex %lld\n", simTime, time);
		simTime = time; 
	}
	inline void AddTime(Int64 time) { simTime += time; }
private:
	Int64 simTime;	// 현 시뮬레이션 시간 [us]
};
