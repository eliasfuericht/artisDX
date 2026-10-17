Texture2D albedoTexture : register(t0);
SamplerState dShadowMapSampler : register(s0);

cbuffer pbrFactors : register(b5)
{
    float4 c_baseColorFactor;
    float c_metallicFactor;
    float c_roughnessFactor;
    float c_alphaCutoff;
    uint c_alphaMode;
};

struct StageInput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

void main(StageInput input)
{
    if (c_alphaMode == 1)
        clip(albedoTexture.Sample(dShadowMapSampler, input.uv).a * c_baseColorFactor.a - c_alphaCutoff);
}
