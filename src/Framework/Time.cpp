#include "Time.h"

string RealTime::TimeProgress() const
{
	duration<double> realTime = duration_cast<duration<double>>(steady_clock::now() - startTime);
	string realTimeStr = SeparatedNumber(realTime.count());
	return string(realTimeStr + " s");
}

Int32 RealTime::GetTimeSec()
{
	duration<Int32> realTime = duration_cast<duration<Int32>>(steady_clock::now() - startTime);
	return realTime.count();
};

string SimTime::TimeProgress() const
{
	double simTimeSec = UsToSec(simTime);

	if (simTimeSec < (60 * 60)) //1Hour (3600s) under
	{
		string simTimeStr = SeparatedNumber(simTimeSec);
		return string(simTimeStr + " s");
	}
	else if (simTimeSec < (60 * 60 * 24) ) // 1Dat under
	{
		string simTimeStr = SeparatedNumber(simTimeSec / 60);
		return string(simTimeStr + " min");
	}
	else if (simTimeSec < (60 * 60 * 24*30)) // 1Month under
	{
		string simTimeStr = SeparatedNumber(simTimeSec / 60 / 60);
		return string(simTimeStr + " Hour");
	}
	else
	{
		string simTimeStr = SeparatedNumber(simTimeSec / 60 / 60 / 24);
		return string(simTimeStr + " Day");
	}

}

