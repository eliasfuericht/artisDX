cbuffer sharedBuffer : register(b0) { float4 sharedValue; };
float4 main(float4 position : POSITION) : SV_POSITION { return position + sharedValue; }
