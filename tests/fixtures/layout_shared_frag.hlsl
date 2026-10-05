cbuffer sharedBuffer : register(b2, space1) { float4 sharedValue; };
float4 main() : SV_TARGET { return sharedValue; }
