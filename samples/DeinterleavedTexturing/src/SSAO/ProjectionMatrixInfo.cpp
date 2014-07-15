//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO/ProjectionMatrixInfo.cpp
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

#include "ProjectionMatrixInfo.h"
#include "MatrixView.h"

//--------------------------------------------------------------------------------
GFSDK::SSAO::ProjectionMatrixInfo::ProjectionMatrixInfo(const GFSDK_SSAO_InputDepthData *pDepthData)
{
    MatrixView m(pDepthData->pProjectionMatrix, pDepthData->ProjectionMatrixLayout);

    float A = m(2,2);
    float B = m(3,2);
    bool IsLeftHanded = (m(2,3) == 1.0f);
    if (IsLeftHanded) A = -A;

    m_ZNear = B / A;
    m_ZFar = (A != -1.0f) ? B / (A + 1.0f) : FLT_MAX;
    m_TanHalfFovy = 1.0f / m(1,1);
}

//--------------------------------------------------------------------------------
bool GFSDK::SSAO::ProjectionMatrixInfo::IsValid(const GFSDK_SSAO_InputDepthData *pDepthData)
{
    MatrixView m(pDepthData->pProjectionMatrix, pDepthData->ProjectionMatrixLayout);

    const bool AllowReverseInfiniteProjection = true;
    bool m22Pass = AllowReverseInfiniteProjection ? true : m(2,2) != 0.0f;

    return (m(0,0) != 0.0f && m(0,1) == 0.0f && m(0,2) == 0.0f && m(0,3) == 0.0f &&
            m(1,0) == 0.0f && m(1,1) != 0.0f && m(1,2) == 0.0f && m(1,3) == 0.0f &&
            m(2,0) == 0.0f && m(2,1) == 0.0f && m22Pass && fabs(m(2,3)) == 1.0f &&
            m(3,0) == 0.0f && m(3,1) == 0.0f && m(3,2) != 0.0f && m(3,3) == 0.0f);
}
