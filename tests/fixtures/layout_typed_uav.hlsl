RWTexture2D<float4> outputTexture : register(u0);
float4 main() : SV_TARGET { outputTexture[int2(0, 0)] = float4(1, 0, 0, 1); return float4(1, 0, 0, 1); }
