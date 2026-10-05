StructuredBuffer<float4> unsupportedBuffer : register(t0);
float4 main() : SV_TARGET { return unsupportedBuffer[0]; }
