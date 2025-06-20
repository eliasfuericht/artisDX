// Texture and sampler bound from root signature
Texture2D myTexture : register(t0);
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

    // Sample the texture using interpolated UV coordinates
    float4 texColor = myTexture.Sample(mySampler, stage_input.inUV);

    // You can output the color directly or modify it
    stage_output.outFragColor = texColor;

    return stage_output;
}
