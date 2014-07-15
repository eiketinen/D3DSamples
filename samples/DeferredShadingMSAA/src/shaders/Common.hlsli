// TAGRELEASE: PUBLIC

#define SAMPLE_COUNT 4

cbuffer ConfigBuffer : register(b1)
{
	int UseDiscontinuity;
	int AdaptiveShading;
	int SeperateEdgePass;
	int ShowComplexPixels;
	int UseOrenNayar;
	int3 pad;
};