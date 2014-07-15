@echo off

del /F /Q bin\*

set SHADER_INCLUDE=Bin.h
del /F %SHADER_INCLUDE%

call helper_compile_vs FullScreenTriangle

call helper_compile_ps LinearizeDepthNoMSAA
call helper_compile_ps ResolveAndLinearizeDepthMSAA
call helper_compile_ps DeinterleaveDepth
call helper_compile_ps ReconstructNormal

call helper_compile_ps SeparableAO USE_RANDOM_TEXTURE 0
call helper_compile_ps SeparableAO USE_RANDOM_TEXTURE 1

call helper_compile_ps NonSeparableAO ENABLE_BLUR 0 USE_RANDOM_TEXTURE 0 
call helper_compile_ps NonSeparableAO ENABLE_BLUR 0 USE_RANDOM_TEXTURE 1 

call helper_compile_ps NonSeparableAO ENABLE_BLUR 1 USE_RANDOM_TEXTURE 0 
call helper_compile_ps NonSeparableAO ENABLE_BLUR 1 USE_RANDOM_TEXTURE 1 

call helper_compile_ps ReinterleaveAO ENABLE_BLUR 0
call helper_compile_ps ReinterleaveAO ENABLE_BLUR 1

call helper_compile_ps BlurX
call helper_compile_ps BlurY

pause
