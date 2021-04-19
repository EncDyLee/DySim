#pragma once

#include "Common.h"

void PrintBoth(FILE* stream, const char* format, ...)
{
	va_list list;

	va_start(list, format);
	vprintf(format, list);
	va_end(list);

	if (stream != stdout)
	{
		va_start(list, format);
		vfprintf(stream, format, list);
		va_end(list);
	}
}

void ErrMsg(const char* format, ...)
{
	va_list list;

	va_start(list, format);
	vprintf(format, list);
	va_end(list);

	abort();
}

void ErrMsg(FILE* stream, const char* format, ...)
{
	va_list list;

	va_start(list, format);
	vprintf(format, list);
	va_end(list);

	if (stream != stdout)
	{
		va_start(list, format);
		vfprintf(stream, format, list);
		va_end(list);
		fclose(stream);
	}

	abort();
}

void ChkOpenedFile(const FILE* file, const char* format)
{
	if (file == nullptr)
	{
		string fileName(format);
		cout << format << " file open error." << endl;

		abort();
	}
}

void fclose_with_chk(FILE* file, const char* format)
{
	if (fclose(file) == EOF)
	{
		string fileName(format);
		cout << format << " file close error." << endl;

		abort();
	}
}

void _mkdir_with_chk(const char* dirName)
{
	_mkdir(dirName);
	if (errno == ENOENT)
	{
		printf("%s directory make error.\n", dirName);
		abort();
	}
}

string MapIntToStr(Int32 target, ...)
{
	va_list list;
	va_start(list, target);

	while (target != va_arg(list, Int32))
		va_arg(list, const char*);
	string str = va_arg(list, const char*);

	va_end(list);

	return str;
}


// 숫자에 콤마를 찍음
string SeparatedNumber(Int32 num)
{
	if (num == -1)
	{
		string str("NaN");

		return str;
	}
	else
	{
		// create string with integer number
		char strBuf[10 + 1];
		sprintf(strBuf, "%d", num);

		// separate the string by comma
		string str(strBuf);
		Int32 triple = (str.size() % 3 == 0) ? 1 : 0;
		Int32 length = (Int32)str.size() / 3 - triple;

		string::iterator it = str.end();
		for (Int32 i = 0; i < length; i++)
			it = str.insert(it - 3, ',');

		return str;
	}
}

string SeparatedNumber(Int64 num)
{
	if (num == -1)
	{
		string str("NaN");

		return str;
	}
	else
	{
		// create string with integer number
		char strBuf[19 + 1];
		sprintf(strBuf, "%lld", num);

		// separate the string by comma
		string str(strBuf);
		Int32 triple = (str.size() % 3 == 0) ? 1 : 0;
		Int32 length = (Int32)str.size() / 3 - triple;

		string::iterator it = str.end();
		for (Int32 i = 0; i < length; i++)
			it = str.insert(it - 3, ',');

		return str;
	}
}

string SeparatedNumber(double num)
{
	if (num == -1)
	{
		string str("NaN");

		return str;
	}
	else
	{
		// create string with floating number
		char strBuf[22 + 1];
		sprintf(strBuf, "%.2lf", num);

		// separate the string by comma
		string str(strBuf);
		Int32 effectiveSize = (Int32)str.size() - 3;
		Int32 triple = (effectiveSize % 3 == 0) ? 1 : 0;
		Int32 length = effectiveSize / 3 - triple;

		string::iterator it = str.end() - 3;
		for (Int32 i = 0; i < length; i++)
			it = str.insert(it - 3, ',');

		return str;
	}
}

