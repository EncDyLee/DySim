#pragma once

#include "WorkloadInterface.h"
#include "LogReport.h"
#include "Time.h"
#include "StatArchiver.h"

extern bool IS_CTRL_HANDLER_CALL;
extern Int32 LOG_PRINT_SEC_TERM;

//Model Sample Simple ~
#include "../Models/SimpleBufferModel.h"
#include "../Models/SimpleFtlModel.h"
#include "../Models/SimpleNandModel.h"
#include "../Models/SimpleBusModel.h"


class SsdModel
{
public:
	SsdModel(const class Configurer& config, class StatArchiver& stat)
		: buf(config), bus(config), err(config), stat(stat), busyTime(0)
	{

		switch (config.GetSwConfig().CHUNK_MAPPING_MODE)
		{
		case 0:
			ocFtl = new MultiPlaneOcStaticChunkModel(config);
			break;
		case 1:
			ocFtl = new MultiPlaneOcStaticReplaceChunkModel(config);
			break;
		case 2:
			ocFtl = new MultiPlaneOcAutoChunkModel(config);
			break;
		case 3:
			ocFtl = new MultiPlaneOcDynamicChunkModel(config);
			break;
		default:
			ErrMsg("OC 펌웨어 모드 선택 에러 \n");
			abort();
			break;
		}

		nand = new SimpleNandModel(config);
	}

	
	Int64 busyTime;

	StatArchiver& stat;

	OcChunkMngInterface* ocFtl;
	NandModelInterface* nand;

	SimpleBusModel bus;
	FifoBufModel buf;

	NandErrorModel err;
protected:
	
};