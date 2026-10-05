Texture2D<float4> unboundedTextures[] : register(t0);
float4 main(uint index : TEXCOORD0) : SV_TARGET { return unboundedTextures[index].Load(int3(0, 0, 0)); }
