#include "LogReport.h"

ResultReport::ResultReport(const Configurer& config)
{
	csvFilePath = config.GetConfigPath().RESULT_PATH + config.GetConfigToken().out + ".csv";

	if (_access(csvFilePath.c_str(), 0) != 0)
	{
		FILE* csvFile = fopen(csvFilePath.c_str(), "wt");
		ChkOpenedFile(csvFile, csvFilePath.c_str());
		fclose_with_chk(csvFile, csvFilePath.c_str());
	}

	item = "Date,HW,SW,WKLD,Trace";
	value = config.GetConfigToken().date + "," + config.GetConfigToken().hw + "," + config.GetConfigToken().sw + "," + config.GetConfigToken().wkd + "," + config.GetConfigToken().trace;

}

ResultReport::ResultReport(const Configurer& config, string addToken)
{
	csvFilePath = config.GetConfigPath().RESULT_PATH  + config.GetConfigToken().out + addToken + ".csv";

	if (_access(csvFilePath.c_str(), 0) != 0)
	{
		FILE* csvFile = fopen(csvFilePath.c_str(), "wt");
		ChkOpenedFile(csvFile, csvFilePath.c_str());
		fclose_with_chk(csvFile, csvFilePath.c_str());
	}

	item = "Date,HW,SW,WKLD,Trace";
	value = config.GetConfigToken().date + "," + config.GetConfigToken().hw + "," + config.GetConfigToken().sw + "," + config.GetConfigToken().wkd + "," + config.GetConfigToken().trace;

}

void ResultReport::BufferItemValue(const string& item, const string& value)
{
	if (!csvFilePath.empty())
	{
		this->item += "," + item;
		this->value += "," + value;
	}
}

void ResultReport::ReportItemValue() const
{
	if (!csvFilePath.empty())
	{
		FILE* csvFile = fopen(csvFilePath.c_str(), "a+t");
		ChkOpenedFile(csvFile, csvFilePath.c_str());

		fscanf(csvFile, " ");
		auto eof = feof(csvFile);
		rewind(csvFile);	// update("+")로 fopen()한 파일에서 읽기/쓰기 전환하는데 필요

		if (eof != 0)	// 파일 끝인 경우 최초 report
		{
			fprintf(csvFile, "%s", item.c_str());
		}
		fprintf(csvFile, "\n%s", value.c_str());

		fclose_with_chk(csvFile, csvFilePath.c_str());
	}
}

LogReport::LogReport(const Configurer& config)
{
	string commonFileToken = config.GetConfigToken().hw + "_" + config.GetConfigToken().sw + "_" + config.GetConfigToken().wkd + "_" + config.GetConfigToken().trace + "_" + config.GetConfigToken().date;

	lastLogTime = 0;
	LOG_TIME_TERM = config.GetSimConfig().LOG_PRINT_SEC_TERM;
	logFile = fopen((config.GetConfigPath().LOG_PATH + commonFileToken + ".txt").c_str(), "wt");
	ChkOpenedFile(logFile, "LogFile");
}

LogReport::LogReport(const Configurer& config, string addToken)
{
	string commonFileToken = config.GetConfigToken().hw + "_" + config.GetConfigToken().sw + "_" + config.GetConfigToken().wkd + "_" + config.GetConfigToken().trace + "_" + config.GetConfigToken().date;

	lastLogTime = 0;
	LOG_TIME_TERM = config.GetSimConfig().LOG_PRINT_SEC_TERM;
	logFile = fopen((config.GetConfigPath().LOG_PATH + commonFileToken + addToken + ".csv").c_str(), "wt");
	ChkOpenedFile(logFile, "LogFile");
}

LogReport::~LogReport()
{
	fclose_with_chk(logFile, "LogFile");
}
