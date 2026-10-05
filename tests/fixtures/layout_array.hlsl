Texture2D<float4> arrayTextures[2] : register(t3, space2);
float4 main(float2 uv : TEXCOORD0) : SV_TARGET
{
    return arrayTextures[0].Load(int3(0, 0, 0)) + arrayTextures[1].Load(int3(0, 0, 0));
}
