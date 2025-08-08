// Texture and sampler bound from root signature
Texture2D albedoTexture             : register(t0);
Texture2D dShadowMap                : register(t1);

SamplerState mySampler              : register(s0);

struct StageInput
{
    float4 position : SV_Position;
    float2 inUV : TEXCOORD;
    float4 inFragPosLightSpace : FRAGPOSLIGHTSPACE;
};

struct StageOutput
{
    float4 outFragColor : SV_Target0;
};

StageOutput main(StageInput stageInput)
{
    StageOutput stageOutput;
    
    float4 albedo = albedoTexture.Sample(mySampler, stageInput.inUV);
    
    float3 projCoords = stageInput.inFragPosLightSpace.xyz / stageInput.inFragPosLightSpace.w;
    projCoords = projCoords * 0.5f + 0.5f; 
    
    float depthFromShadowMap = dShadowMap.Sample(mySampler, projCoords.xy).r;
    
    float shadow = projCoords.z > depthFromShadowMap ? 1.0f : 0.0f;
    
    stageOutput.outFragColor = float4(albedo.rgb * (1.0f - shadow + 0.2f), albedo.a);
    return stageOutput;
}
