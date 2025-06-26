// Texture and sampler bound from root signature
Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
SamplerState mySampler : register(s0);

struct SPIRV_Cross_Input
{
    float2 inUV : TEXCOORD;
    float4 inPos : SV_Position;
};

struct SPIRV_Cross_Output
{
    float4 outFragColor : SV_Target0;
};

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    SPIRV_Cross_Output stage_output;

    float4 texColor = albedoTexture.Sample(mySampler, stage_input.inUV);
    //texColor += normalTexture.Sample(mySampler, stage_input.inUV) * 0.5f;
    stage_output.outFragColor = texColor;

    return stage_output;
}
