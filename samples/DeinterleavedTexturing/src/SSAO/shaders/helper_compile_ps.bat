call set_env_vars.bat

if "%2"=="" goto d0
if "%4"=="" goto d1
goto d2

:d0
%FXC_CMD% /T ps_5_0 src/%1_PS.hlsl /E %1_PS /DMAIN_FUNCTION=%1_PS /Fh "bin/%1_PS.h"
if errorlevel 1 goto error
echo #include "bin/%1_PS.h" >> %SHADER_INCLUDE%
goto :eof

:d1
%FXC_CMD% /T ps_5_0 src/%1_PS.hlsl /E %1_%2_%3_PS /D%2=%3 /DMAIN_FUNCTION=%1_%2_%3_PS /Fh "bin\%1_%2_%3_PS.h"
if errorlevel 1 goto error
echo #include "bin/%1_%2_%3_PS.h" >> %SHADER_INCLUDE%
goto :eof

:d2
%FXC_CMD% /T ps_5_0 src/%1_PS.hlsl /E %1_%2_%3_%4_%5_PS /D%2=%3 /D%4=%5 /DMAIN_FUNCTION=%1_%2_%3_%4_%5_PS /Fh "bin\%1_%2_%3_%4_%5_PS.h"
if errorlevel 1 goto error
echo #include "bin/%1_%2_%3_%4_%5_PS.h" >> %SHADER_INCLUDE%
goto :eof

:error
pause
exit /b 1