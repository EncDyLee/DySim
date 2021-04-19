
#include "StatArchiver.h"


StatArchiver::StatArchiver()
{
	InitStat();
}

void StatArchiver::InitStat()
{
	memset(&data, 0, sizeof(StatDataStruct));
}
