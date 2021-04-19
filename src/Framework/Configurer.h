#pragma once

#include "Common.h"

enum class WorkloadType
{
	REAL,
	SYNTHETIC
};

enum class RealFormat
{
	BRICKSIM,
	SPC,
	MSR,
	TPC_MSN
};

enum class SyntheticFormat
{
	BENCHMARK,
	GAUSSIAN
};

struct WkdConfig
{
	Int32 WORKLOAD_CQ_CAPACITY;


	Int32 WORKLOAD_TYPE;
	Int32 MAX_WORKLOAD_VOLUME_GB;

	Int32 USER_ADDR_RATE;
	Int32 PRE_CONDITION_RATE;

	struct RealWorkloadConfig
	{
		string TRACE_FILE_PATH;
		string TRACE_FILE_NAME;

		Int32 TIME_DELAY_RATE;
		bool IGNORE_IDLE_TIME;
		Int32 IGNORE_CMD;
		Int32 INFINITE_PLAY;
	}realConfig;

	struct SyntheticWorkloadConfig
	{
		Int32 ADDR_SEQUENCE;
		Int32 GAUSSIAN_STD_DEV;

		Int32 WRITE_RATIO;
		Int32 READ_RATIO;

		Int32 FIXED_CMD_SIZE;
		Int64 TIME_INTERVAL;
	}synConfig;

};

struct HwConfig
{
	//Memory model
	Int32 DATA_BUF_SIZE;
	Int32 OUT_STAND_CQ_CAP;
	Int32 MAX_SINGLE_REQ_SIZE;

	// Host interface model
	Int32 NVME_BANDWIDTH;
	Int32 NVME_LANE_CNT;
	Int32 WAY_BANDWIDTH;

	// NAND flash memory array model
	Int32 CHANNEL_CNT;
	Int32 WAY_PER_CHANNEL;
	Int32 PLANE_PER_WAY;
	Int32 BLOCK_PER_PLANE;
	Int32 PAGE_PER_BLOCK;
	Int32 PAGE_SIZE;
	Int32 BIT_PER_CEL;

	Int32 OVER_PROVISION_BLOCK_RATIO;

	// NAND flash memory performance model
	Int64 READ_TIME_PAGE;
	Int64 PROGRAM_TIME_SLC_PAGE;
	Int64 PROGRAM_TIME_MLC_PAGE;
	Int64 PROGRAM_TIME_TLC_PAGE;
	Int64 ERASE_TIME_BLOCK;

	// NAND error model
	Int32 A_MAX;
	Int32 A_MIN;
	Int32 B_MAX;
	Int32 B_MIN;

	Int32 A_ERROR_MIN;
	Int32 A_ERROR_MAX;
	Int32 B_ERROR_MIN;
	Int32 B_ERROR_MAX;

	Int32 ECC_BIT_MAX;

};

struct OcConfig
{
	Int32 GROUP_CNT;
	Int32 PU_PER_GROUP;
	Int32 CHUNK_PER_PU;
	Int32 LBA_PER_CHUNK;
	const Int32 PAGE_PER_LBA = 1;

	Int32 WKD_CHUNK_PER_PU;

	Int32 PLANE_PER_CHUNK;
	Int32 BLOCK_PER_CHUNK;

	Int32 PLANE_PER_PU;
	Int32 WAY_PER_PU;
};

struct SwConfig
{
	//Host FTL
	Int32 BLOCK_MANAGEMENT;
	Int32 WL_THRESHOLD;
	Int32 GC_TRIGGER_FREE_CHUNK_RATIO; //Minimum
	Int32 LIMIT_BAD_CHUNK_RATIO;

	//OCSSD Firmware
	Int32 CHUNK_MAPPING_MODE;
	Int32 PLANE_PER_CHUNK;
};

struct SimConfig
{
	Int32 RAND_SEED;
	Int64 LOG_PRINT_SEC_TERM;

	bool PRINT_BLOCK_STAT;
	Int32 DBG_ERASE_COUNT_SCALE = 1;
};

struct ConfigToken
{
	string hw;
	string sw;
	string wkd;
	string out;
	string trace;
	string date;

};

struct ConfigPath
{
	string PROJ_PATH;
	string CONFIG_PATH;
	string RESULT_PATH;
	string LOG_PATH;
};


class Configurer
{
public:
	Configurer(Int32 argc, char* argv[]);

	const struct OcConfig GetOcConfig() const { return ocConfig; }
	const struct HwConfig GetHwConfig() const { return hwConfig; }
	const struct SwConfig GetSwConfig() const { return swConfig; }
	const struct WkdConfig GetWkdConfig() const { return wkdConfig; }
	const struct SimConfig GetSimConfig() const { return simConfig; }
	const struct ConfigPath GetConfigPath() const { return pathConfig; }
	const struct ConfigToken GetConfigToken() const { return tokenConfig; }

	void PrintConfig(FILE* stream);
private:
	void ParseCmdLine(Int32 argc, char* argv[]);
	void ParseHwConfig();
	void ParseSwConfig();
	void ParseWorkloadConfig();
	void ParseSimConfig();
	void ParseOcConfig();



	struct OcConfig ocConfig;
	struct HwConfig hwConfig;
	struct SwConfig swConfig;
	struct WkdConfig wkdConfig;
	struct SimConfig simConfig;


	struct ConfigPath pathConfig;
	struct ConfigToken tokenConfig;
};