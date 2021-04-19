#include "Configurer.h"

#define MIN_ARGC_BRICKSIM 4

Configurer::Configurer(Int32 argc, char* argv[])
{
	ParseCmdLine(argc, argv);
	ParseHwConfig();
	ParseSwConfig();
	ParseWorkloadConfig();
	ParseSimConfig();
	ParseOcConfig();
}

void Configurer::ParseCmdLine(Int32 argc, char* argv[])
{
	// print parsed command line
	PrintBoth(stdout, "... parsed command line ...\n");
	PrintBoth(stdout, "==========================================================================================\n");
	for (int i = 1; i < argc; i++)
		PrintBoth(stdout, "%s ", argv[i]);
	PrintBoth(stdout, "\n");
	PrintBoth(stdout, "==========================================================================================\n\n");

	if (argc < MIN_ARGC_BRICKSIM)
	{
		printf("==================================================================================================\n");
		printf("BrickSim <HW_CONFIG_FILE> <SW_CONFIG_FILE> <WKD_CONFIG_FILE> <INPUT_FILE> <OUT_FILE> \n");
		printf("<HW_CONFIG_FILE>: HW configuration file name (ending WITHOUT .txt) \n");
		printf("<SW_CONFIG_FILE>: SW configuration file name (ending WITHOUT .txt) \n");
		printf("<WKD_CONFIG_FILE>: WKD configuration file name (ending WITHOUT .txt) \n");
		printf("<INPUT_TAG>: -I Input trace file name or syn Tag \n");
		printf("<OUT_TAG>: OUTPUT File name tag (ending WITHOUT .csv) \n");
		printf("==================================================================================================\n");

		printf("==================================================================================================\n");
		printf("BrickSim CMD line Extension -e <EXAMPLE>\n");
		printf("--------------------------------------------------------------------------------------------------\n");

		printf("==================================================================================================\n");

		ErrMsg(stdout, "Invalid command line\n");
	}
	else
	{
		tokenConfig.hw = argv[1];
		tokenConfig.sw = argv[2];
		tokenConfig.wkd = argv[3];
		tokenConfig.trace = argv[4];
		tokenConfig.out = argv[5];

		time_t rawTime;
		struct tm* timeInfo;
		time(&rawTime);
		timeInfo = localtime(&rawTime);
		tokenConfig.date = to_string((timeInfo->tm_year + 1900)/* % 100*/) + "-" + to_string(timeInfo->tm_mon + 1) + "-" + to_string(timeInfo->tm_mday) + "-" +
			to_string(timeInfo->tm_hour) + "-" + to_string(timeInfo->tm_min) + "-" + to_string(timeInfo->tm_sec);


		pathConfig.PROJ_PATH = "./";
		pathConfig.CONFIG_PATH = "configs/";

		Int32 maxCheckCnt = 0;
		int32_t isCreated = _access((pathConfig.PROJ_PATH + pathConfig.CONFIG_PATH).c_str(), 0);	// 리턴값 0 이면 존재 -1이면 미존재
		while (isCreated == -1)
		{
			if (maxCheckCnt > 5)
				ErrMsg("Can not found project path\n");

			pathConfig.PROJ_PATH += "../";
			maxCheckCnt++;
			isCreated = _access((pathConfig.PROJ_PATH + pathConfig.CONFIG_PATH).c_str(), 0);	// 리턴값 0 이면 존재 -1이면 미존재
		}


		//for (Int32 i = (MIN_ARGC_BRICKSIM + 1); i < argc; i += 2)
		//{
		//	string exCmd = string(argv[i]).substr(0, 2);

		//	if ((exCmd == "-i") || (exCmd == "-I"))
		//		tokenConfig.csvResult = argv[i + 1];
		//	else if ((exCmd == "-t") || (exCmd == "-T"))
		//		tokenConfig.csvTime = argv[i + 1];
		//	else if ((exCmd == "-v") || (exCmd == "-V"))
		//		tokenConfig.csvValue = argv[i + 1];
		//	else
		//		ErrMsg(stdout, "Invalid command line (extension commnad) \n");
		//}
	}

}


void Configurer::ParseHwConfig()
{
	string pathConfigHw = pathConfig.PROJ_PATH + pathConfig.CONFIG_PATH + tokenConfig.hw + ".txt";
	FILE* configFile = fopen(pathConfigHw.c_str(), "rt");

	ChkOpenedFile(configFile, pathConfigHw.c_str());

	const Int32 lineSize = 200;
	char str[lineSize];


	for (int i = 1; fgets(str, lineSize, configFile) != NULL; i++)
	{
		if (str[0] == '/' || isspace(str[0]))	// ignore comments or blank lines
			continue;

		char item[lineSize / 4];
		Int32 value;
		if (sscanf(str, "%s %*c %d", item, &value) == 0)
			ErrMsg("Scanf Zero return\n");

		if (strcmp(item, "DATA_BUF_SIZE") == 0)
			hwConfig.DATA_BUF_SIZE = value;
		else if (strcmp(item, "OUT_STAND_CQ_CAP") == 0)
			hwConfig.OUT_STAND_CQ_CAP = value;
		else if (strcmp(item, "MAX_SINGLE_REQ_SIZE") == 0)
			hwConfig.MAX_SINGLE_REQ_SIZE = value;

		//BUS array model
		else if (strcmp(item, "NVME_BANDWIDTH") == 0)
			hwConfig.NVME_BANDWIDTH = value;
		else if (strcmp(item, "NVME_LANE_CNT") == 0)
			hwConfig.NVME_LANE_CNT = value;
		else if (strcmp(item, "WAY_BANDWIDTH") == 0)
			hwConfig.WAY_BANDWIDTH = value;
		

		// NAND flash memory array model
		else if (strcmp(item, "CHANNEL_CNT") == 0)
			hwConfig.CHANNEL_CNT = value;
		else if (strcmp(item, "WAY_PER_CHANNEL") == 0)
			hwConfig.WAY_PER_CHANNEL = value;
		else if (strcmp(item, "PLANE_PER_WAY") == 0)
			hwConfig.PLANE_PER_WAY = value;
		else if (strcmp(item, "BLOCK_PER_PLANE") == 0)
			hwConfig.BLOCK_PER_PLANE = value;
		else if (strcmp(item, "PAGE_PER_BLOCK") == 0)
			hwConfig.PAGE_PER_BLOCK = value;
		else if (strcmp(item, "PAGE_SIZE") == 0)
			hwConfig.PAGE_SIZE = value;
		else if (strcmp(item, "BIT_PER_CEL") == 0)
			hwConfig.BIT_PER_CEL = value;

		else if (strcmp(item, "OVER_PROVISION_BLOCK_RATIO") == 0)
			hwConfig.OVER_PROVISION_BLOCK_RATIO = value;

		// NAND flash memory performance model
		else if (strcmp(item, "READ_TIME_PAGE") == 0)
			hwConfig.READ_TIME_PAGE = value;
		else if (strcmp(item, "PROGRAM_TIME_SLC_PAGE") == 0)
			hwConfig.PROGRAM_TIME_SLC_PAGE = value;
		else if (strcmp(item, "PROGRAM_TIME_MLC_PAGE") == 0)
			hwConfig.PROGRAM_TIME_MLC_PAGE = value;
		else if (strcmp(item, "PROGRAM_TIME_TLC_PAGE") == 0)
			hwConfig.PROGRAM_TIME_TLC_PAGE = value;
		else if (strcmp(item, "ERASE_TIME_BLOCK") == 0)
			hwConfig.ERASE_TIME_BLOCK = value;

		else if (strcmp(item, "A_MIN") == 0)
			hwConfig.A_MIN = value;
		else if (strcmp(item, "A_MAX") == 0)
			hwConfig.A_MAX = value;
		else if (strcmp(item, "B_MIN") == 0)
			hwConfig.B_MIN = value;
		else if (strcmp(item, "B_MAX") == 0)
			hwConfig.B_MAX = value;
		else if (strcmp(item, "A_ERROR_MIN") == 0)
			hwConfig.A_ERROR_MIN = value;
		else if (strcmp(item, "A_ERROR_MAX") == 0)
			hwConfig.A_ERROR_MAX = value;
		else if (strcmp(item, "B_ERROR_MIN") == 0)
			hwConfig.B_ERROR_MIN = value;
		else if (strcmp(item, "B_ERROR_MAX") == 0)
			hwConfig.B_ERROR_MAX = value;

		else if (strcmp(item, "ECC_BIT_MAX") == 0)
			hwConfig.ECC_BIT_MAX = value;


		else
			ErrMsg(stdout, "HW configuration file parsing error on line %d\n", i);
	}
	fclose_with_chk(configFile, "HW config");
}
void Configurer::ParseSwConfig()
{
	string pathConfigSw = pathConfig.PROJ_PATH + pathConfig.CONFIG_PATH + tokenConfig.sw + ".txt";
	FILE* configFile = fopen(pathConfigSw.c_str(), "rt");
	ChkOpenedFile(configFile, "SW config");

	const Int32 lineSize = 200;
	char str[lineSize];


	for (int i = 1; fgets(str, lineSize, configFile) != NULL; i++)
	{
		if (str[0] == '/' || isspace(str[0]))	// ignore comments or blank lines
			continue;

		char item[lineSize / 4];
		Int32 value;
		if (sscanf(str, "%s %*c %d", item, &value) == 0)
			ErrMsg("Scanf Zero return\n");

		//Host FTL
		if (strcmp(item, "BLOCK_MANAGEMENT") == 0)
			swConfig.BLOCK_MANAGEMENT = value;
		else if (strcmp(item, "WL_THRESHOLD") == 0)
			swConfig.WL_THRESHOLD = value;
		else if (strcmp(item, "GC_TRIGGER_FREE_CHUNK_RATIO") == 0)
			swConfig.GC_TRIGGER_FREE_CHUNK_RATIO = value;
		else if (strcmp(item, "LIMIT_BAD_CHUNK_RATIO") == 0)
			swConfig.LIMIT_BAD_CHUNK_RATIO = value;

		
		//OCSSD firmware
		else if (strcmp(item, "CHUNK_MAPPING_MODE") == 0)
			swConfig.CHUNK_MAPPING_MODE = value;
		else if (strcmp(item, "PLANE_PER_CHUNK") == 0)
			swConfig.PLANE_PER_CHUNK = value;


		else
			ErrMsg(stdout, "SW configuration file parsing error on line %d\n", i);
	}
	fclose_with_chk(configFile, "SW config");
}
void Configurer::ParseWorkloadConfig()
{
	string pathConfigWkd = pathConfig.PROJ_PATH + pathConfig.CONFIG_PATH + tokenConfig.wkd + ".txt";
	FILE* configFile = fopen(pathConfigWkd.c_str(), "rt");
	ChkOpenedFile(configFile, "Workload config");

	const Int32 lineSize = 400;
	char str[lineSize];


	for (int i = 1; fgets(str, lineSize, configFile) != NULL; i++)
	{
		if (str[0] == '/' || isspace(str[0]))	// ignore comments or blank lines
			continue;

		char item[lineSize / 4];
		Int32 value;
		if (sscanf(str, "%s %*c %d", item, &value) == 0)
			ErrMsg("Scanf Zero return\n");


		if (strcmp(item, "WORKLOAD_CQ_CAPACITY") == 0)
			wkdConfig.WORKLOAD_CQ_CAPACITY = value;
		else if (strcmp(item, "WORKLOAD_TYPE") == 0)
			wkdConfig.WORKLOAD_TYPE = value;

		else if (strcmp(item, "MAX_WORKLOAD_VOLUME") == 0)
			wkdConfig.MAX_WORKLOAD_VOLUME_GB = value;
		else if (strcmp(item, "USER_ADDR_RATE") == 0)
			wkdConfig.USER_ADDR_RATE = value;
		else if (strcmp(item, "PRE_CONDITION_RATE") == 0)
			wkdConfig.PRE_CONDITION_RATE = value;

		else if (strcmp(item, "TRACE_FILE_PATH") == 0)
		{
			char workloadDir[lineSize];
			sscanf(str, "%*s %*c %s", workloadDir);
			wkdConfig.realConfig.TRACE_FILE_PATH = workloadDir;
		}

		else if (strcmp(item, "TIME_DELAY_RATE") == 0)
			wkdConfig.realConfig.TIME_DELAY_RATE = value;
		else if (strcmp(item, "IGNORE_IDLE_TIME") == 0)
			wkdConfig.realConfig.IGNORE_IDLE_TIME = (value == 0) ? false : true;
		else if (strcmp(item, "IGNORE_CMD") == 0)
			wkdConfig.realConfig.IGNORE_CMD = value;
		else if (strcmp(item, "INFINITE_PLAY") == 0)
			wkdConfig.realConfig.INFINITE_PLAY = value;


		//MICRO BENCHMARK
		else if (strcmp(item, "ADDR_SEQUENCE") == 0)
			wkdConfig.synConfig.ADDR_SEQUENCE = value;
		else if (strcmp(item, "GAUSSIAN_STD_DEV") == 0)
			wkdConfig.synConfig.GAUSSIAN_STD_DEV = value;

		//CMD CONFIG
		else if (strcmp(item, "WRITE_RATIO") == 0)
			wkdConfig.synConfig.WRITE_RATIO = value;
		else if (strcmp(item, "READ_RATIO") == 0)
			wkdConfig.synConfig.READ_RATIO = value;
		else if (strcmp(item, "FIXED_CMD_SIZE") == 0)
			wkdConfig.synConfig.FIXED_CMD_SIZE = value;
		else if (strcmp(item, "TIME_INTERVAL") == 0)
			wkdConfig.synConfig.TIME_INTERVAL = value;
	
		else
			ErrMsg(stdout, "Workload configuration file parsing error on line %d\n", i);

	}
	fclose_with_chk(configFile, "Workload config");

}

void Configurer::ParseSimConfig()
{
	string pathLog = pathConfig.PROJ_PATH + pathConfig.CONFIG_PATH + "SIM.txt";
	FILE* configFile = fopen(pathLog.c_str(), "rt");
	ChkOpenedFile(configFile, "SIM config");

	const Int32 lineSize = 400;
	char str[lineSize];

	for (int i = 1; fgets(str, lineSize, configFile) != NULL; i++)
	{
		if (str[0] == '/' || isspace(str[0]))	// ignore comments or blank lines
			continue;

		char item[lineSize / 4];
		Int32 value;
		if (sscanf(str, "%s %*c %d", item, &value) == 0)
			ErrMsg("Scanf Zero return\n");

		// COMMON
		if (strcmp(item, "LOG_PRINT_SEC_TERM") == 0)
			simConfig.LOG_PRINT_SEC_TERM = value;
		else if (strcmp(item, "RAND_SEED") == 0)
			simConfig.RAND_SEED = value;
		else if (strcmp(item, "PRINT_BLOCK_STAT") == 0)
			simConfig.PRINT_BLOCK_STAT = bool(value);
		else if (strcmp(item, "DBG_ERASE_COUNT_SCALE") == 0)
			simConfig.DBG_ERASE_COUNT_SCALE = value;
		else
			ErrMsg(stdout, "Log configuration file parsing error on line %d\n", i);

	}
	fclose_with_chk(configFile, "Log config");


	pathConfig.RESULT_PATH = pathConfig.PROJ_PATH + "results/";;
	pathConfig.LOG_PATH = pathConfig.PROJ_PATH + "results/log/";;

	// logRpt file path setting
	_mkdir_with_chk((pathConfig.RESULT_PATH).c_str());
	_mkdir_with_chk((pathConfig.LOG_PATH).c_str());


}

void Configurer::ParseOcConfig()
{
	ocConfig.PLANE_PER_PU = swConfig.PLANE_PER_CHUNK;
	ocConfig.PLANE_PER_CHUNK = swConfig.PLANE_PER_CHUNK;
	ocConfig.BLOCK_PER_CHUNK = swConfig.PLANE_PER_CHUNK;


	ocConfig.GROUP_CNT = hwConfig.CHANNEL_CNT;
	ocConfig.PU_PER_GROUP = hwConfig.PLANE_PER_WAY * hwConfig.WAY_PER_CHANNEL / ocConfig.PLANE_PER_CHUNK;
	ocConfig.WAY_PER_PU = hwConfig.CHANNEL_CNT * hwConfig.WAY_PER_CHANNEL / ocConfig.PU_PER_GROUP;
	if (ocConfig.WAY_PER_PU == 0)
		ErrMsg("ocConfig.WAY_PER_PU is 0 (청크는 최소 WAY 단위로 구성됨) \n", ocConfig.WAY_PER_PU);

	ocConfig.CHUNK_PER_PU = hwConfig.BLOCK_PER_PLANE;
	ocConfig.LBA_PER_CHUNK = ocConfig.PAGE_PER_LBA * hwConfig.PAGE_PER_BLOCK * ocConfig.BLOCK_PER_CHUNK;
}

void Configurer::PrintConfig(FILE* stream)
{
	PrintBoth(stream, "... parsed config file ...\n");
	PrintBoth(stream, "==========================================================================================\n");

	PrintBoth(stream, "\nNAND flash memory array model\n");
	PrintBoth(stream, "\t\t%-50s%12d\n", "CHANNEL_CNT:", hwConfig.CHANNEL_CNT);
	PrintBoth(stream, "\t\t%-50s%12d\n", "WAY_PER_CHANNEL:", hwConfig.WAY_PER_CHANNEL);
	PrintBoth(stream, "\t\t%-50s%12d\n", "PLANE_PER_WAY:", hwConfig.PLANE_PER_WAY);
	PrintBoth(stream, "\t\t%-50s%12d\n", "BLOCK_PER_PLANE:", hwConfig.BLOCK_PER_PLANE);
	PrintBoth(stream, "\t\t%-50s%12d\n", "PAGE_PER_BLOCK:", hwConfig.PAGE_PER_BLOCK);
	PrintBoth(stream, "\t\t%-50s%12d KB\n", "PAGE_SIZE:", hwConfig.PAGE_SIZE);
	PrintBoth(stream, "\t\t%-50s%12d KB\n", "SECTOR_SIZE:", SECTOR_SIZE);

	PrintBoth(stream, "\nNAND flash memory time model\n");
	PrintBoth(stream, "\t\t%-50s%12lld us\n", "READ_TIME_PAGE:", hwConfig.READ_TIME_PAGE);
	PrintBoth(stream, "\t\t%-50s%12lld us\n", "PROGRAM_TIME_SLC_PAGE:", hwConfig.PROGRAM_TIME_SLC_PAGE);
	PrintBoth(stream, "\t\t%-50s%12lld us\n", "PROGRAM_TIME_MLC_PAGE:", hwConfig.PROGRAM_TIME_MLC_PAGE);
	PrintBoth(stream, "\t\t%-50s%12lld us\n", "PROGRAM_TIME_TLC_PAGE:", hwConfig.PROGRAM_TIME_TLC_PAGE);
	PrintBoth(stream, "\t\t%-50s%12lld us\n", "ERASE_TIME_BLOCK:", hwConfig.ERASE_TIME_BLOCK);

	PrintBoth(stream, "\nNAND flash memory error model\n");
	PrintBoth(stream, "\t\t%-50s%12d\n", "WORKLOAD_CQ_CAPACITY:", hwConfig.ECC_BIT_MAX);
	

	PrintBoth(stream, "\nSimulation model\n");
	PrintBoth(stream, "\t\t%-50s%12d\n", "WORKLOAD_CQ_CAPACITY:", wkdConfig.WORKLOAD_CQ_CAPACITY);


	PrintBoth(stream, "\nFTL model\n");
	PrintBoth(stream, "\t\t%-50s%12d %%\n", "GC_TRIGGER_FREE_CHUNK_RATIO (Greedy only):", swConfig.GC_TRIGGER_FREE_CHUNK_RATIO);



	PrintBoth(stream, "\nWorkload model\n");
	PrintBoth(stream, "\t\t%-30s%32s\n", "WORKLOAD_TYPE:"
		, MapIntToStr(wkdConfig.WORKLOAD_TYPE, 0, "MSR", 1, "SYN", 2, "GAUS").c_str());
	PrintBoth(stream, "\t\t%-50s%12d GB\n", "MAX_WORKLOAD_VOLUME:", wkdConfig.MAX_WORKLOAD_VOLUME_GB);
	PrintBoth(stream, "\t\t%-50s%12d GB\n", "USER_ADDR_RATE %:", wkdConfig.USER_ADDR_RATE);



	if (wkdConfig.WORKLOAD_TYPE == 0)
	{
		PrintBoth(stream, "\nReal Workload\n");
		PrintBoth(stream, "\t\t%-50s\n", "TRACE_FILE_PATH:");
		PrintBoth(stream, "\t\t\t%s\n\n", wkdConfig.realConfig.TRACE_FILE_PATH.c_str());
		PrintBoth(stream, "\t\t%-30s%32s\n", "WORKLOAD_NAME:", tokenConfig.trace.c_str());

		PrintBoth(stream, "\t\t%-30s%32s\n", "IGNORE_IDLE_TIME:"
			, MapIntToStr(wkdConfig.realConfig.IGNORE_IDLE_TIME, 0, "Involve", 1, "Ignore").c_str());

		PrintBoth(stream, "\t\t%-50s%12d\n", "INFINITE_PLAY:", wkdConfig.realConfig.INFINITE_PLAY);

	}
	else
	{
		PrintBoth(stream, "\nSynthetic Workload\n");

		PrintBoth(stream, "\t\t%-30s%32s\n", "ADDR_SEQUENCE:"
			, MapIntToStr(wkdConfig.synConfig.ADDR_SEQUENCE, 0, "Sequential", 1, "Random").c_str());
		
		PrintBoth(stream, "\t\t%-50s%12d %%\n", "WRITE_RATIO:", wkdConfig.synConfig.WRITE_RATIO);
		PrintBoth(stream, "\t\t%-50s%12d %%\n", "READ_RATIO:", wkdConfig.synConfig.READ_RATIO);

	}

	PrintBoth(stream, "\nSim parameter\n");

	PrintBoth(stream, "\t\t%-50s%12d\n", "RAND_SEED:", simConfig.RAND_SEED);


	PrintBoth(stream, "\nOCSSD model\n");
	PrintBoth(stream, "\t\t%-30s%32s\n", "CHUNK_MAPPING_MODE:", MapIntToStr(swConfig.CHUNK_MAPPING_MODE, 0, "Static", 1, "Replace", 2, "Auto", 3, "Dynamic").c_str());
	PrintBoth(stream, "\t\t%-50s%12d\n", "PLANE_PER_CHUNK:", swConfig.PLANE_PER_CHUNK);

	PrintBoth(stream, "\t\t%-50s%12d\n", "GROUP_CNT (cal.):", ocConfig.GROUP_CNT);
	PrintBoth(stream, "\t\t%-50s%12d\n", "PU_PER_GROUP (cal.):", ocConfig.PU_PER_GROUP);
	PrintBoth(stream, "\t\t%-50s%12d\n", "CHUNK_PER_PU (cal.):", ocConfig.CHUNK_PER_PU);

	PrintBoth(stream, "\nHW Model\n");
	Int64 standardBlockCnt = Int64(hwConfig.BLOCK_PER_PLANE) * hwConfig.PLANE_PER_WAY * hwConfig.WAY_PER_CHANNEL* hwConfig.CHANNEL_CNT;
	PrintBoth(stream, "\t\t%-50s%12d\n", "STANDARD_BLOCK_CNT (cal.):", standardBlockCnt);
	Int64 totalBlockCnt = standardBlockCnt + (standardBlockCnt * (hwConfig.OVER_PROVISION_BLOCK_RATIO / 100.0));
	PrintBoth(stream, "\t\t%-50s%12d\n", "TOTAL_BLOCK_CNT (cal.):", totalBlockCnt);
	PrintBoth(stream, "\t\t%-50s%12d %%\n", "OVER_PROVISION_BLOCK_RATIO:", hwConfig.OVER_PROVISION_BLOCK_RATIO);
	
	Int64 opBlockCnt = standardBlockCnt * (hwConfig.OVER_PROVISION_BLOCK_RATIO / 100.0);
	PrintBoth(stream, "\t\t%-50s%12d\n", "- OVER_PROVISION_BLOCK (cal.):", opBlockCnt);
	PrintBoth(stream, "\t\t%-50s%12d\n", "- PROVISION_BLOCK (cal.):", standardBlockCnt + opBlockCnt);
	PrintBoth(stream, "\t\t%-50s%12d\n", "TOTAL_CHUNK_CNT (cal.):", ocConfig.CHUNK_PER_PU * ocConfig.PU_PER_GROUP);


	PrintBoth(stream, "\t\t%-50s%12d MB\n", "BLOCK_SIZE (cal.):", hwConfig.PAGE_SIZE * hwConfig.PAGE_PER_BLOCK / 1024);
	PrintBoth(stream, "\t\t%-50s%12d MB\n", "CHUNK_SIZE (cal.):", hwConfig.PAGE_SIZE * hwConfig.PAGE_PER_BLOCK * swConfig.PLANE_PER_CHUNK / 1024);

	PrintBoth(stream, "\t\t%-50s%12d GB\n", "HOST_VOLUME (cal.):"
		, hwConfig.PAGE_SIZE * hwConfig.PAGE_PER_BLOCK * swConfig.PLANE_PER_CHUNK * ocConfig.CHUNK_PER_PU * ocConfig.PU_PER_GROUP / 1024 / 1024);
	PrintBoth(stream, "\t\t%-50s%12d GB\n", "DEVICE_VOLUME (cal.):"
		, totalBlockCnt * hwConfig.PAGE_SIZE * hwConfig.PAGE_PER_BLOCK / 1024 / 1024);

}