#include "WorkloadInterface.h"



RealWorkload::RealWorkload(const class Configurer& config)
	: WorkloadInterface(), realConfig(config.GetWkdConfig().realConfig)
{
	baseTimestampInt = 0;
	baseTimestampDouble = 0;
	lastTimestampInt = 0;
	lastTimestampDouble = 0;
	curPlayCycle = 1;

	string filePath;

	if (realConfig.WORKLOAD_NAME.size() != 4)
		filePath = (realConfig.WORKLOAD_PATH + realConfig.WORKLOAD_NAME);
	else
	{
		string multiWorkloadName = realConfig.WORKLOAD_PATH + "\\*" + realConfig.WORKLOAD_NAME;
		multiFileHandler = _findfirst(multiWorkloadName.c_str(), &fd);

		if (multiFileHandler == -1)
		{
			printf("Multi workload file Initial open fail, config : %s\n", multiWorkloadName.c_str());
			abort();
		}
		else
			printf("Multi workload file open first, Name: %s\n", fd.name);

		filePath = (realConfig.WORKLOAD_PATH + "\\" + fd.name);
	}

	if (realConfig.REAL_FORMAT == 3) //2GB 이상의 파일 포인터를 다루기 위해서 쓰기 모드 사용
		importFile = fopen(filePath.c_str(), "wb");
	else
		importFile = fopen(filePath.c_str(), "rb");

	ChkOpenedFile(importFile, filePath.c_str());

	MAX_WORKLOAD_VOLUME_SECTOR = (config.GetWkdConfig().MAX_WORKLOAD_VOLUME_GB == 0) ? INV_VALUE : GbToSect(config.GetWkdConfig().MAX_WORKLOAD_VOLUME_GB);
}

Int64 RealWorkload::NormalizeTimestamp(Int64 timestamp)
{
	if (lastTimestampInt > timestamp)
		SetBaseTimestamp((timestamp - lastTimestampInt));

	lastTimestampInt = timestamp - baseTimestampInt;
	return lastTimestampInt;
}
double RealWorkload::NormalizeTimestamp(double timestamp)
{
	if (lastTimestampDouble > timestamp)
		SetBaseTimestamp((timestamp - lastTimestampDouble));

	lastTimestampDouble = timestamp - baseTimestampDouble;
	return lastTimestampDouble;
}
void RealWorkload::SetBaseTimestamp(Int64 timestamp) { baseTimestampInt = timestamp; }
void RealWorkload::SetBaseTimestamp(double timestamp) { baseTimestampDouble = timestamp; }
bool RealWorkload::NeedMorePlayCycle()
{
	if (realConfig.REPLAY_CYCLE == 0)
		return true;
	else if (curPlayCycle < realConfig.REPLAY_CYCLE)
	{
		curPlayCycle++;
		printf("Workload play cycle update : %d / %d \n", curPlayCycle, realConfig.REPLAY_CYCLE);
		return true;
	}
	else
		return false;
}


bool RealWorkload::IsFetchableWorkload()
{
	if ((feof(importFile)))
	{
		if (realConfig.WORKLOAD_NAME.size() != 4)
		{
			if (NeedMorePlayCycle())
			{
				InitFilePointer();
				return true;
			}
			else
				return false;
		}
		else
		{
			if (_findnext(multiFileHandler, &fd) != -1)
			{
				string filePath(realConfig.WORKLOAD_PATH + "\\" + fd.name);
				importFile = fopen(filePath.c_str(), "rb");
				ChkOpenedFile(importFile, filePath.c_str());
				printf("Multi workload file open: %s\n", fd.name);
				InitFilePointer();
				return true;
			}
			else if (NeedMorePlayCycle())
			{
				string multiWorkloadName = realConfig.WORKLOAD_PATH + "\\*" + realConfig.WORKLOAD_NAME;
				multiFileHandler = _findfirst(multiWorkloadName.c_str(), &fd);

				if (multiFileHandler == -1)
				{
					printf("Multi workload file Initial open fail, config : %s\n", multiWorkloadName.c_str());
					abort();
				}

				string filePath = (realConfig.WORKLOAD_PATH + "\\" + fd.name);
				importFile = fopen(filePath.c_str(), "rb");
				ChkOpenedFile(importFile, filePath.c_str());
				printf("Multi workload file (Init) open: %s\n", fd.name);
				InitFilePointer();
				return true;
			}
			else
				return false;
		}

	}
	else
		return true;
}



BrickSim::BrickSim(const class Configurer& config)
	: RealWorkload(config)
{
	InitCmd genCmd;
	char tempOp;

	if (fscanf(importFile, "%lld,%c,%d,%d ", &genCmd.time, &tempOp, &genCmd.lba, &genCmd.size) != 4)
	{
		ErrMsg("Initial Workload fetch fail (format doesn't match) \n");
	}

	SetBaseTimestamp(genCmd.time);
	InitFilePointer();

	START_TIME = genCmd.time;
}

InitCmd BrickSim::GetInitCmd()
{
	InitCmd genCmd;
	char tempOp;

	if (!IsFetchableWorkload())
		return genCmd;

	if (fscanf(importFile, "%lld,%c,%d,%d ", &genCmd.time, &tempOp, &genCmd.lba, &genCmd.size) != 4)
		ErrMsg("Workload format doesn't match\n");

	genCmd.time = NormalizeTimestamp(genCmd.time);
	genCmd.op = (Opcode)tempOp;
	return genCmd;
}

Spc::Spc(const class Configurer& config) : RealWorkload(config)
	
{
	InitCmd genCmd;
	char opCode;
	Int32 byteOffset;
	Int32 byteSize;
	double timestamp;

	if (fscanf(importFile, "%*d,%d,%d,%c,%lf ", &byteOffset, &byteSize, &opCode, &timestamp) != 4)
	{
		ErrMsg("Initial Workload fetch fail (format doesn't match) \n");
	}

	genCmd.time = (Int64)(NormalizeTimestamp(timestamp) * 1000000000);	// unit of workload time is [s] wherease unit of simulation time is [ns]
	SetBaseTimestamp(genCmd.time);
	InitFilePointer();

	START_TIME = Int64(timestamp) * 1000000000;
}

InitCmd Spc::GetInitCmd()
{
	InitCmd genCmd;
	char opCode;
	Int32 byteOffset;
	Int32 byteSize;
	double timestamp;

	if (!IsFetchableWorkload())
		return genCmd;

	if (fscanf(importFile, "%*d,%d,%d,%c,%lf ", &byteOffset, &byteSize, &opCode, &timestamp) != 4)
	{
		ErrMsg("Initial Workload fetch fail (format doesn't match) \n");
	}

	genCmd.time = (Int64)(NormalizeTimestamp(timestamp) * 1000000000);	// unit of workload time is [s] wherease unit of simulation time is [ns]
	genCmd.lba = KbToSect(byteOffset / 2.0);
	genCmd.size = KbToSect(byteSize / 1024.0);
	genCmd.size = (genCmd.size == 0) ? 1 : genCmd.size;

	if ((opCode == 'R') || (opCode == 'r'))
		genCmd.op = Opcode::READ;
	else if ((opCode == 'W') || (opCode == 'w'))
		genCmd.op = Opcode::WRITE;
	else
		ErrMsg("Workload CMD doesn't match\n");

	return genCmd;
}

Msr::Msr(const class Configurer& config) : RealWorkload(config)
{
	InitCmd genCmd;
	char opCode[8];
	Int64 byteOffset;
	Int32 byteSize;

	if (fscanf(importFile, "%lld,%*[^,],%*d,%[^,],%lld,%d,%*d ", &genCmd.time, opCode, &byteOffset, &byteSize) != 4)
	{
		ErrMsg("Initial Workload fetch fail (format doesn't match) \n");
	}

	SetBaseTimestamp(genCmd.time);
	InitFilePointer();

	START_TIME = genCmd.time * 100;
}

InitCmd Msr::GetInitCmd()
{
	InitCmd genCmd;
	char opCode[8];
	Int64 byteOffset;
	Int32 byteSize;

	if (!IsFetchableWorkload())
		return genCmd;

	if (fscanf(importFile, "%lld,%*[^,],%*d,%[^,],%lld,%d,%*d ", &genCmd.time, opCode, &byteOffset, &byteSize) != 4)
		ErrMsg("Workload format doesn't match\n");

	genCmd.time = NormalizeTimestamp(genCmd.time) * 100;	// unit of workload time is [100 ns] wherease unit of simulation time is [ns]

	string typeStr(opCode);
	if (typeStr == "Read")
		genCmd.op = Opcode::READ;
	else if (typeStr == "Write")
		genCmd.op = Opcode::WRITE;
	else
		ErrMsg("Workload CMD doesn't match\n");

	genCmd.lba = KbToSect(byteOffset / 1024.0);
	genCmd.size = KbToSect(byteSize / 1024.0);
	genCmd.size = (genCmd.size == 0) ? 1 : genCmd.size;
	return genCmd;
}



TpcMsn::TpcMsn(const class Configurer& config) : RealWorkload(config), IsEndWorkload(false), headerPosition(0)
{
	char type[35];
	const Int32 lineSize = 400;
	char str[lineSize];
	Int32 headerByteCnt = 1;
	while (headerByteCnt++)
	{
		if (fgets(str, lineSize, importFile) == NULL)
			ErrMsg("Workload format doesn't match\n");

		sscanf(str, " %[^,] ", type);
		string typeStr(type);
		if (typeStr == "EndHeader\r\n") //헤더의 존재 유무로 포멧은 맞는 것으로 판단. TPC MSN은 포멧 매칭을 기준으로 파일 끝을 판단함.
		{
			fgetpos(importFile, &headerPosition); // 헤더 마지막위치 기록
			break;
		}
		else if ((headerByteCnt > 1024 * 1024)) //헤더가 1MB 이상일리 없다고 판단.
			ErrMsg("Workload format doesn't match\n");
	}


	while ((fgets(str, lineSize, importFile) != NULL))
	{
		sscanf(str, " %[^,] ", type);
		string typeStr(type);
		if (typeStr == "DiskRead" || typeStr == "DiskWrite")
		{
			Int64 timestamp;
			Int64 byteOffset;
			Int32 byteSize;
			if (sscanf(str, "%s%lld,%*[^,],%*d,%*llx,%llx,%x", type, &timestamp, &byteOffset, &byteSize) != 4)
				ErrMsg("Workload format doesn't match\n");

			SetBaseTimestamp(timestamp);
			START_TIME = timestamp * 1000;
			break;
		}
	}

	InitFilePointer();
}

void TpcMsn::InitFilePointer() { fsetpos(importFile, &headerPosition); } //헤더의 끝으로 포인터 이동};

InitCmd TpcMsn::GetInitCmd()
{
	InitCmd genCmd;

	if (!IsFetchableWorkload())
		return genCmd;

	const Int32 lineSize = 400;
	char str[lineSize];
	char type[35];
	while (true)
	{
		if (fgets(str, lineSize, importFile) == NULL)
		{
			IsEndWorkload = true;
			return genCmd;
		}

		sscanf(str, " %[^,] ", type);
		string typeStr(type);
		if (typeStr == "DiskRead" || typeStr == "DiskWrite")
			break;
	}

	Int64 timestamp;
	Int64 byteOffset;
	Int32 byteSize;

	if (sscanf(str, "%s%lld,%*[^,],%*d,%*llx,%llx,%x", type, &timestamp, &byteOffset, &byteSize) != 4)
		ErrMsg("Workload format doesn't match\n");


	genCmd.time = NormalizeTimestamp(timestamp) * 1000;// unit of workload time is [us] wherease unit of simulation time is [ns]

	string typeStr(type);
	typeStr.pop_back();
	if (typeStr == "DiskRead")
		genCmd.op = Opcode::READ;
	else if (typeStr == "DiskWrite")
		genCmd.op = Opcode::WRITE;
	else
		assert(false && "op importing error in TPC/MSN workload");

	genCmd.lba = KbToSect(byteOffset / 1024.0);
	genCmd.size = KbToSect(byteSize / 1024.0);
	genCmd.size = (genCmd.size == 0) ? 1 : genCmd.size;


	return genCmd;
}



bool TpcMsn::IsFetchableWorkload()
{
	if (IsEndWorkload)
	{
		if (realConfig.WORKLOAD_NAME.size() != 4)
		{
			if (NeedMorePlayCycle())
			{
				InitFilePointer();
				IsEndWorkload = false;
				return true;
			}
			else
				return false;
		}
		else
		{
			if (_findnext(multiFileHandler, &fd) != -1)
			{
				string filePath(realConfig.WORKLOAD_PATH + "\\" + fd.name);
				importFile = fopen(filePath.c_str(), "rb");
				ChkOpenedFile(importFile, filePath.c_str());
				printf("Multi workload file open: %s\n", fd.name);
				InitFilePointer();
				IsEndWorkload = false;
				return true;
			}
			else if (NeedMorePlayCycle())
			{
				string multiWorkloadName = realConfig.WORKLOAD_PATH + "\\*" + realConfig.WORKLOAD_NAME;
				multiFileHandler = _findfirst(multiWorkloadName.c_str(), &fd);

				if (multiFileHandler == -1)
				{
					printf("Multi workload file Initial open fail, config : %s\n", multiWorkloadName.c_str());
					abort();
				}

				string filePath = (realConfig.WORKLOAD_PATH + "\\" + fd.name);
				importFile = fopen(filePath.c_str(), "rb");
				ChkOpenedFile(importFile, filePath.c_str());
				printf("Multi workload file (Init) open: %s\n", fd.name);
				InitFilePointer();
				IsEndWorkload = false;
				return true;
			}
			else
				return false;
		}

	}
	else
		return true;
}
