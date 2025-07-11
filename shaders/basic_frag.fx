// Texture and sampler bound from root signature
Texture2D albedoTexture             : register(t0);

SamplerState mySampler              : register(s0);

struct StageInput
{
    float4 position : SV_Position;
    float2 inUV : TEXCOORD;
};

struct StageOutput
{
    float4 outFragColor : SV_Target0;
};

StageOutput main(StageInput stageInput)
{
    StageOutput stageOutput;
    
    stageOutput.outFragColor = albedoTexture.Sample(mySampler, stageInput.inUV);
    
    return stageOutput;
}
