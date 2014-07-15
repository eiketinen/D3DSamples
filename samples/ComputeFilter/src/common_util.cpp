//----------------------------------------------------------------------------------
// File:        ComputeFilter\src/common_util.cpp
// SDK Version: v1.2 
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2014, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------
#include "common_util.h"

#include <D3DX11.h>
#include <d3dcompiler.h>
#include <string>
#include <cstdarg>

HRESULT get_shader_resource(const wchar_t * id, void ** bytecode, UINT & bytecode_length)
{
      *bytecode = nullptr;
      HMODULE module = GetModuleHandle(NULL);
      HRSRC resource = FindResource(module, id, RT_RCDATA);
      if (resource)
      {
            bytecode_length = SizeofResource(module, resource);
            HGLOBAL global_id = LoadResource(module, resource);
            if (global_id)
            {
                  *bytecode = LockResource(global_id);
                  if (*bytecode)
                        return S_OK;
            }
      }
      return E_INVALIDARG;
}

HRESULT messagebox_printf(const char * caption, UINT mb_type, const char * format, ...) {
    va_list args;
    va_start(args, format);
    char formatted_text[512];
    _vsnprintf(formatted_text, 512, format, args);
    return MessageBoxA(nullptr, formatted_text, caption, mb_type);
}