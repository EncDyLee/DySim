#include "Framework\Common.h"
#include "Framework\Configurer.h"
#include "Framework\DySimulator.h"

bool IS_CTRL_HANDLER_CALL = false;

BOOL CtrlcHandler(DWORD ctrlType) //For ctrl + c key print out
{
	if (ctrlType == CTRL_C_EVENT)
	{
		IS_CTRL_HANDLER_CALL = true;
		return TRUE;
	}
	else
		return FALSE;
}

int main(int argc, char* argv[])
{
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlcHandler, TRUE))
		cout << "Could not set control handler" << endl;

	Configurer config(argc, argv);			//Configuation Setup

	LogReport log(config);					//report log file
	config.PrintConfig(log.GetLogFile());

	DySimulator dySim(config, log);
	dySim.RunSimulation();


	ResultReport rstDefault(config);				//report csv file 1
	dySim.PrintOutCsv(rstDefault);

	if (config.GetSimConfig().PRINT_BLOCK_STAT)
	{
		LogReport rstBlkStat(config, "_blk");		//report csv file 2
		dySim.PrintOutBlockStat(rstBlkStat);
	}

	
	return 0;
}