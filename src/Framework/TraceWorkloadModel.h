#pragma once

#include "WorkloadInterface.h"

class BrickSim : public TraceWorkload
{
public:
	BrickSim(const class Configurer& config, class OcChunkMngInterface& ocIf, class StatArchiver& stat)
		: TraceWorkload(config, ocIf, stat)
	{
		InitCmd genCmd;
		char tempOp;

		if (fscanf(importFile, "%lld,%c,%lld,%d ", &genCmd.time, &tempOp, &genCmd.lba, &genCmd.size) != 4)
		{
			ErrMsg("Initial Workload fetch fail (format doesn't match) \n");
		}

		SetBaseTimestamp(genCmd.time / 1000);
		InitFilePointer();
	}

protected:
	InitCmd GetRawTrace() override
	{
		InitCmd genCmd;
		char tempOp;

		if (fscanf(importFile, "%lld,%c,%lld,%d ", &genCmd.time, &tempOp, &genCmd.lba, &genCmd.size) != 4)
			ErrMsg("Workload format doesn't match\n");

		genCmd.time = genCmd.time / 1000;
		genCmd.op = (Opcode)tempOp;
		return genCmd;
	}
};

class Spc : public TraceWorkload
{
public:
	Spc(const class Configurer& config, class OcChunkMngInterface& ocIf, class StatArchiver& stat)
		: TraceWorkload(config, ocIf, stat)
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

		SetBaseTimestamp(Int64(genCmd.time * 1000000));
		InitFilePointer();
	}
protected:
	InitCmd GetRawTrace() override
	{
		InitCmd genCmd;
		char opCode;
		Int64 byteOffset;
		Int32 byteSize;
		double timestamp;

		if (fscanf(importFile, "%*d,%lld,%d,%c,%lf ", &byteOffset, &byteSize, &opCode, &timestamp) != 4)
		{
			ErrMsg("Initial Workload fetch fail (format doesn't match) \n");
		}

		genCmd.time = Int64(genCmd.time * 1000000);	// unit of workload time is [s] wherease unit of simulation time is [ns]
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

};

class Msr : public TraceWorkload
{
public:
	Msr(const class Configurer& config, class OcChunkMngInterface& ocIf, class StatArchiver& stat)
		: TraceWorkload(config, ocIf, stat)
	{
		InitCmd genCmd;
		char opCode[8];
		Int64 byteOffset;
		Int32 byteSize;

		if (fscanf(importFile, "%lld,%*[^,],%*d,%[^,],%lld,%d,%*d ", &genCmd.time, opCode, &byteOffset, &byteSize) != 4)
		{
			ErrMsg("Initial Workload fetch fail (format doesn't match) \n");
		}
		SimTimeArrange(genCmd.time);

		SetBaseTimestamp(genCmd.time);
		lastTime = NormalizeTimestamp(genCmd.time);

		InitFilePointer();
	}

protected:
	InitCmd GetRawTrace()
	{
		InitCmd genCmd;
		char opCode[8];
		Int64 byteOffset;
		Int32 byteSize;

		if (fscanf(importFile, "%lld,%*[^,],%*d,%[^,],%lld,%d,%*d ", &genCmd.time, opCode, &byteOffset, &byteSize) != 4)
		{
			ErrMsg("Initial Workload fetch fail (format doesn't match) \n");
		}
		SimTimeArrange(genCmd.time);

		if (opCode[0] == 'R')
			genCmd.op = Opcode::READ;
		else if (opCode[0] == 'W')
			genCmd.op = Opcode::WRITE;
		else
			ErrMsg("Workload CMD doesn't match\n");

		genCmd.size = KbToSect(byteSize / 1024.0);
		genCmd.size = (genCmd.size == 0) ? 1 : genCmd.size;

		genCmd.lba = KbToSect(byteOffset / 1024.0);
		genCmd.time = NormalizeTimestamp(genCmd.time);

		if (lastTime > genCmd.time)
			genCmd.time = lastTime;
		else
			lastTime = genCmd.time;

		CheckReplay(genCmd.time);

		return genCmd;
	}

	void SimTimeArrange(Int64& time)
	{
		time /= 10;
		time -= UTC_DIFFERENCE;
	}
};
