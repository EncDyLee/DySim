#pragma once
#include "..\Framework\Common.h"
#include "../Framework/Configurer.h"

class ResultReport
{
public:
	ResultReport(const Configurer& config);
	ResultReport(const Configurer& config, string addToken);
	void BufferItemValue(const string& item, const string& value);
	void ReportItemValue() const;

private:

	string csvFilePath;

	string item;
	string value;
};

class LogReport
{
public:
	LogReport(const Configurer& config);
	LogReport(const Configurer& config, string addToken);
	~LogReport();

	inline FILE* GetLogFile() { return logFile; }
	inline const Int64& GetLastLogTime() { return lastLogTime; }
	void UpdateLastLogTime() { lastLogTime += LOG_TIME_TERM; }
private:
	FILE* logFile;
	Int64 lastLogTime;
	Int64 LOG_TIME_TERM;
};