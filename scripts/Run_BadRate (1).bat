echo off

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD0_ST WKD_USR60_SEQ %%~ni RST_0416_BADRATE_MLC_SEQ

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD2_ST WKD_USR60_SEQ %%~ni RST_0416_BADRATE_MLC_SEQ

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD4_ST WKD_USR60_SEQ %%~ni RST_0416_BADRATE_MLC_SEQ

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD6_ST WKD_USR60_SEQ %%~ni RST_0416_BADRATE_MLC_SEQ


pause

