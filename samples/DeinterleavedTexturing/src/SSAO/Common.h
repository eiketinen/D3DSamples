//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/Common.h
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
#include "Types.h"
#include "Shaders/Src/SharedDefines.h"
#include <algorithm>
#include <math.h>
#include <float.h>
#include <assert.h>

#undef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }

#undef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }

#undef SAFE_RELEASE_ARRAY
#define SAFE_RELEASE_ARRAY(A)\
    for (UINT i = 0; i < ARRAYSIZE(A); ++i)\
    {\
        SAFE_RELEASE(A[i]);\
    }

#undef SAFE_RELEASE_ARRAY_2D
#define SAFE_RELEASE_ARRAY_2D(A)\
    for (UINT i = 0; i < ARRAYSIZE(A); ++i)\
    for (UINT j = 0; j < ARRAYSIZE(A[i]); ++j)\
    {\
        SAFE_RELEASE(A[i][j]);\
    }

#undef SAFE_D3D_CALL
#define SAFE_D3D_CALL(x)     { if (x != S_OK) { assert(0); } }

#undef ZERO_ARRAY
#define ZERO_ARRAY(A)       memset(A, 0, sizeof(A));

#undef ZERO_STRUCT
#define ZERO_STRUCT(S)      memset(&S, 0, sizeof(S));

#undef clamp
#define clamp(x,a,b)        (min(max(x, a), b))

#undef iDivUp
#define iDivUp(a,b)         ((a + b-1) / b)

#undef D3DX_PI
#define D3DX_PI 3.141592654f

#undef EPSILON
#define EPSILON 1.e-8f
