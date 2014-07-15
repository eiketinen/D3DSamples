//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/Viewports.h
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

#pragma once
#include "Common.h"

namespace GFSDK
{
namespace SSAO
{

//--------------------------------------------------------------------------------
class Viewports
{
public:
    Viewports()
    {
        ZERO_STRUCT(FullRes);
        ZERO_STRUCT(QuarterRes);
    }

    void SetFullResolution(UINT FullWidth, UINT FullHeight)
    {
        FullRes.TopLeftX = 0.f;
        FullRes.TopLeftY = 0.f;
        FullRes.Width    = FLOAT(FullWidth);
        FullRes.Height   = FLOAT(FullHeight);
        FullRes.MinDepth = 0.f;
        FullRes.MaxDepth = 1.f;

        QuarterRes           = FullRes;
        QuarterRes.Width     = FLOAT(iDivUp(FullWidth,4));
        QuarterRes.Height    = FLOAT(iDivUp(FullHeight,4));
    }

    D3D11_VIEWPORT FullRes;
    D3D11_VIEWPORT QuarterRes;
};

} // namespace SSAO
} // namespace GFSDK
