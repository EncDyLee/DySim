echo off

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD0_ST WKD_USR60 %%~ni RST_0416_BADRATE_MLC

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD2_ST WKD_USR60 %%~ni RST_0416_BADRATE_MLC

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD4_ST WKD_USR60 %%~ni RST_0416_BADRATE_MLC

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD6_ST WKD_USR60 %%~ni RST_0416_BADRATE_MLC


for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD8_ST WKD_USR60 %%~ni RST_0416_BADRATE_MLC

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD10_ST WKD_USR60 %%~ni RST_0416_BADRATE_MLC

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD12_ST WKD_USR60 %%~ni RST_0416_BADRATE_MLC


for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD0_DY WKD_USR60 %%~ni RST_0416_BADRATE_MLC

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD2_DY WKD_USR60 %%~ni RST_0416_BADRATE_MLC

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD4_DY WKD_USR60 %%~ni RST_0416_BADRATE_MLC

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD6_DY WKD_USR60 %%~ni RST_0416_BADRATE_MLC


for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD8_DY WKD_USR60 %%~ni RST_0416_BADRATE_MLC

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD10_DY WKD_USR60 %%~ni RST_0416_BADRATE_MLC

for %%i in (D:\MSR_WO\*.csv) do ..\x64\Release\DySim.exe HW_PL4_MLC_OP0 SW_WL_PL4_BAD12_DY WKD_USR60 %%~ni RST_0416_BADRATE_MLC

pause

