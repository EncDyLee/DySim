#pragma once

#include "Common.h"

class HwRsrc
{
public:
	HwRsrc() :index(INV_VALUE), readyTime(0), sumIdleTime(0), sumBusyTime(0) {}
	HwRsrc(Int32 index) :index(index), readyTime(0), sumIdleTime(0), sumBusyTime(0) {}

	Int32 index;
	Int64 readyTime;

	Int64 sumIdleTime;
	Int64 sumBusyTime;

};


static bool TimeFreeComp(const HwRsrc lhs, const HwRsrc rhs) { return lhs.readyTime < rhs.readyTime; }
