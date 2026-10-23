cbuffer sharedBuffer : register(b0) { float4 sharedValue; };
float4 main() : SV_TARGET { return sharedValue; }
