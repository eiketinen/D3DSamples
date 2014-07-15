//----------------------------------------------------------------------------------
// File:        SoftShadows\media/ContactHardeningShadows.fxh
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

//--------------------------------------------------------------------------------------

// Change to 1 to reuse the blocker search gather in the filter
#define REUSE_GATHER 0

#define FS  11       // Filter size
#define FS2 (FS / 2) // Filter size / 2
#define BMS (FS + 2) // Bezier matrix size

//--------------------------------------------------------------------------------------

// Matrices of bezier control points used to dynamically generate a filter matrix.
// The generated matrix is sized FS x FS; the bezier control matrices are zero padded
// to simplify the dynamic filter loop.
static const float P0[BMS][BMS] = 
{
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 },  
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.8f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 1.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.8f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
};

static const float P1[BMS][BMS] = 
{ 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 	
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 },  
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
};

static const float P2[BMS][BMS] = 
{
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 },  
	{ 0.0f, 0.0f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.2f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.2f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }, 
};

static const float P3[BMS][BMS] = 
{
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 },  
	{ 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0 }, 
	{ 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0 }, 
	{ 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0 }, 
	{ 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0 }, 
	{ 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0 }, 
	{ 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0 }, 
	{ 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0 }, 
	{ 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0 }, 
	{ 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0 }, 
	{ 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0 }, 
	{ 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0 }, 
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 }
};

//--------------------------------------------------------------------------------------

// Compute dynamic weight at a certain row, column of the matrix
float BezierFilter(int row, int col, float t)
{
	int r = row + 1;
	int c = col + 1;
	return (
		(1.0f - t) * (1.0f - t) * (1.0f - t) * P0[r][c] +
		3.0f * (1.0 - t) * (1.0f - t) * t * P1[r][c] +
		3.0f * t * t * (1.0f - t) * P2[r][c] +
		t * t * t * P3[r][c]);
}

//--------------------------------------------------------------------------------------

// Compute shadow using a dynamically generated contact hardening shadow filter
float CHS_Shadow(float3 tc)
{
	// Calculate filter center and offset by the filter center
	float2 rc = (g_shadowMapDimensions.xy * tc.xy) + float2(0.5f, 0.5f);
	float2 fc = rc - floor(rc);
	tc.xy -= fc * g_shadowMapDimensions.zw;

	// Find number of blockers and average blocker depth
	float avgBlockerDepth = 0.0f;
	float numBlockers = 0.0f;
	float maxBlockers = 0.0f;
	[unroll(FS)]for (int row = -FS2; row <= FS2; row += 2 )
	{
		[unroll(FS)]for (int col = -FS2; col <= FS2; col += 2 )
		{
			float4 d4 = g_shadowMap.GatherRed(PointSamplerCHS, tc.xy, int2(col, row));
			float4 b4  = (tc.zzzz <= d4) ? (0.0f).xxxx : (1.0f).xxxx;

			numBlockers += dot(b4, (1.0f).xxxx);
			avgBlockerDepth += dot(d4, b4);
			maxBlockers += 4.0f;
		}
	}

	// Compute PCSS ratio used to drive Bezier interpolator
	float t = 0.0f;
	if (numBlockers == 0)
	{
		return 1.0f;
	}
	else if (numBlockers == maxBlockers)
	{
		return 0.0f;
	}
	else if (numBlockers > 0.0)
	{
		avgBlockerDepth /= numBlockers;
		t = saturate(((tc.z - avgBlockerDepth) * g_lightWidth) / avgBlockerDepth);
		t *= t;
	}	

	// Sum the weights of the dynamic filter matrix
	float  w = 0.0f;
	[unroll(FS)]for (int row = 0; row < FS; ++row)
	{
		[unroll(FS)]for (int col = 0; col < FS; ++col)
		{
			w += BezierFilter(row, col, t);
		}
	}

	// Filter the shadow map using the dynamically generated matrix
	float  s = 0.0f;
	float2 prevRow[FS2 + 1];
	[unroll(FS)]for (int row = -FS2; row <= FS2; row += 2)
	{
		[unroll(FS)]for (int col = -FS2; col <= FS2; col += 2)
		{
#if (REUSE_GATHER)
			float4 d4 = g_shadowMap.GatherRed(PointSamplerCHS, tc.xy, int2(col, row));
			float4 sm4 = (tc.zzzz <= d4) ? (1.0).xxxx : (0.0).xxxx;
#else
			float4 sm4 = g_shadowMap.GatherCmpRed(CmpLessEqualSamplerCHS, tc.xy, tc.z, int2(col, row));
#endif			

#define SM(_row, _col) ((_row) == 0 ? ((_col) == 0 ? sm4.w : sm4.z) \
                                    : ((_col) == 0 ? sm4.x : sm4.y))

			float f10 = BezierFilter(row + FS2, col + FS2 - 1, t);
			float f11 = BezierFilter(row + FS2, col + FS2, t);
			float f12 = BezierFilter(row + FS2, col + FS2 + 1, t);

			s += (1.0f - fc.y) * (SM(0, 0) * (fc.x * (f10 - f11) + f11) + SM(0, 1) * (fc.x * (f11 - f12) + f12));
			s +=         fc.y  * (SM(1, 0) * (fc.x * (f10 - f11) + f11) + SM(1, 1) * (fc.x * (f11 - f12) + f12));

			if (row > -FS2)
			{
				float f00 = BezierFilter(row + FS2 - 1, col + FS2 - 1, t);
				float f01 = BezierFilter(row + FS2 - 1, col + FS2, t);
				float f02 = BezierFilter(row + FS2 - 1, col + FS2 + 1, t);
				s += (1.0f - fc.y) * (prevRow[(col + FS2) / 2].x * (fc.x * (f00 - f01) + f01) + prevRow[(col + FS2) / 2].y * (fc.x * (f01 - f02) + f02));
				s +=         fc.y  * (SM(0, 0) * (fc.x * (f00 - f01) + f01) + SM(0, 1) * (fc.x * (f01 - f02) + f02));
			}

#undef SM

			prevRow[(col + FS2) / 2] = sm4.xy;
		}
	}

	return (s / w);
}
