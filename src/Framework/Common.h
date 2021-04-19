#pragma once

#include <cassert>
#include <cstdint>
#include <cstdarg>
#include <chrono>
#include <ctime>
#include <direct.h>
#include <deque>
#include <fstream>
#include <io.h>
#include <iostream>
#include <list>
#include <forward_list>
#include <math.h>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>
#include <windows.h>
#include <signal.h>
#include <set>

#define INV_VALUE (-1)
#define SECTOR_SIZE (4)

#define RANDOM(__min__, __max__) ((int)(((double)((rand()<<15) | rand())) / ((RAND_MAX<<15 | RAND_MAX) + 1) * (((__max__) + 1) - (__min__))) + (__min__))

using namespace std;

// 정수 자료형 따로 정의하여 사용
typedef int8_t		Int8;
typedef uint8_t		UInt8;
typedef int16_t		Int16;
typedef uint16_t	UInt16;
typedef int32_t		Int32;
typedef uint32_t	UInt32;
typedef int64_t		Int64;
typedef uint64_t	UInt64;


inline Int64 SectToKb(Int64 sectSize)
{
	return sectSize * SECTOR_SIZE;
}
inline double SectToMb(Int64 sectSize)
{
	return SectToKb(sectSize) / 1024.0;
}
inline double SectToGb(Int64 sectSize)
{
	return SectToMb(sectSize) / 1024;
}
inline double SectToTb(Int64 sectSize)
{
	return SectToGb(sectSize) / 1024;
}

inline Int32 KbToSect(double kbSize)
{
	return (Int32)(kbSize / SECTOR_SIZE + 0.5);
}
inline Int32 MbToSect(double mbSize)
{
	return KbToSect(mbSize * 1024);
}
inline Int32 GbToSect(double gbSize)
{
	return MbToSect(gbSize * 1024);
}
inline Int32 TbToSect(double tbSize)
{
	return GbToSect(tbSize * 1024);
}

inline double UsToSec(Int64 us)
{
	return (us / 1000.0 / 1000.0);
}
inline Int64 SecToUs(double sec)
{
	return Int64(sec * 1000.0 * 1000.0);
}

void PrintBoth(FILE* stream, const char* format, ...);

void ErrMsg(const char* format, ...);
void ErrMsg(FILE* stream, const char* format, ...);

void ChkOpenedFile(const FILE* file, const char* format);

void fclose_with_chk(FILE* file, const char* format);

void _mkdir_with_chk(const char* dirName);

string MapIntToStr(Int32 target, ...);


// 숫자에 콤마를 찍음
string SeparatedNumber(Int32 num);
string SeparatedNumber(Int64 num);
string SeparatedNumber(double num);


struct ListedIndex
{
	ListedIndex() :index(INV_VALUE), value(INV_VALUE) {}

	ListedIndex(Int32 index, Int32 value)
		: index(index), value(value) {}

	Int32 index;
	Int32 value;
};

static bool CmpValue(ListedIndex& first, ListedIndex& second)
{
	if (first.value < second.value)
		return true;
	else
		return false;
}

static bool CmpIndex(ListedIndex& first, ListedIndex& second)
{
	if (first.index < second.index)
		return true;
	else
		return false;
}
