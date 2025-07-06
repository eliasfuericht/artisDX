// Texture and sampler bound from root signature
Texture2D albedoTexture             : register(t0);
Texture2D metallicRoughnessTexture  : register(t1);
Texture2D normalTexture             : register(t2);
Texture2D emissiveTexture           : register(t3);
Texture2D occlusionTexture          : register(t4);

SamplerState mySampler              : register(s0);

struct StageInput
{
    float4 position : SV_Position;
    float2 inUV : TEXCOORD;
    float3 inNormal : NORMAL;
    float4 inTangent : TANGENT;
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
