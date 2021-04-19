echo off

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_MULTI_PL4_MLC_OP0 SW_WL_PL4_BAD12_ST WKD_USR68 %%~ni RST_0416_PLN_MLC

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_MULTI_PL4_MLC_OP0 SW_WL_PL4_BAD12_DY WKD_USR68 %%~ni RST_0416_PLN_MLC




pause

