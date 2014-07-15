call set_env_vars.bat

%FXC_CMD% /T vs_5_0 src/%1_VS.hlsl /E %1_VS /Fh "bin/%1_VS.h"
echo #include "bin/%1_VS.h" >> %SHADER_INCLUDE%
