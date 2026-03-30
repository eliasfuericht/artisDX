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
    projCoords.y = 1.0f - projCoords.y;

    float shadow = 0.0f;
    if (projCoords.x >= 0.0f && projCoords.x <= 1.0f &&
        projCoords.y >= 0.0f && projCoords.y <= 1.0f &&
        projCoords.z >= 0.0f && projCoords.z <= 1.0f)
    {
        float depthFromShadowMap = dShadowMap.Sample(mySampler, projCoords.xy).r;
        float bias = 0.005f;
        shadow = projCoords.z - bias > depthFromShadowMap ? 1.0f : 0.0f;
    }
    
    stageOutput.outFragColor = float4(albedo.rgb * (1.0f - shadow + 0.2f), albedo.a);
    return stageOutput;
}
