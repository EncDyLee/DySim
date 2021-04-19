#pragma once
#include "Common.h"

enum class Opcode
{
	READ = 'R',
	WRITE = 'W',
	COPY = 'C',
	TRIM = 'T',
	FLUSH = 'F',
	INVALID = 'I',
	ERASE = 'E',
	GC = 'G',
	WL = 'L'
};

struct OcAddr
{
	OcAddr() : pu(INV_VALUE), chunk(INV_VALUE), lb(INV_VALUE) {}

	OcAddr(Int32 pu, Int32 chunk, Int32 lb) : pu(pu), chunk(chunk), lb(lb) {}

	Int32 pu;
	Int32 chunk;
	Int32 lb;

	vector<Int64> vectLbaMap;
};

struct InitCmd
{
	InitCmd() : time(INV_VALUE), op(Opcode::INVALID), lba(INV_VALUE), size(INV_VALUE) {}
	InitCmd(Int64 time, Opcode op, Int64 lba, Int32 size) : time(time), op(op), lba(lba), size(size) {}

	Int64 time;
	Opcode op;
	Int64 lba;
	Int32 size;
};


class HostCmd
{
public:
	HostCmd(Int64 initTime, Opcode op, OcAddr addr, Int32 size)
		: initTime(initTime), op(op), addr(addr), size(size) {}

	void SetInitTime(Int64 time) { initTime = time; }
	const Int64& GetInitTime() { return initTime; }

	const Opcode& GetOpcode() { return op; }
	const OcAddr& GetAddr() { return addr; }
	Int32& GetSize() { return size; }

protected:

	Int64 initTime;
	Opcode op;
	OcAddr addr;
	Int32 size;
};


struct PhyAddr
{
	PhyAddr() : wayNo(INV_VALUE), planeNo(INV_VALUE), blkNo(INV_VALUE), pageNo(INV_VALUE) {}

	PhyAddr(Int32 wayNo, Int32 planeNo, Int32 blkNo, Int32 pageNo) : wayNo(wayNo), planeNo(planeNo), blkNo(blkNo), pageNo(pageNo) {}

	Int32 wayNo;
	Int32 planeNo;
	Int32 blkNo;
	Int32 pageNo;
};

struct PhyAddrMul
{
	PhyAddrMul(Int32 PLANE_PER_WAY)
		: wayNo(INV_VALUE), plnBitMap(PLANE_PER_WAY, false), blkNo(PLANE_PER_WAY, INV_VALUE), pageNo(INV_VALUE) {}

	Int32 wayNo;
	vector<bool> plnBitMap;
	vector<Int32> blkNo;
	Int32 pageNo;
};
