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

    //float4 texColor = myTexture.Sample(mySampler, stage_input.inUV);
    //stage_output.outFragColor = texColor;

    float2 uv = stage_input.inUV;
    stage_output.outFragColor = float4(uv.x, uv.y, 1.0f - uv.x, 1.0f);


    return stage_output;
}
