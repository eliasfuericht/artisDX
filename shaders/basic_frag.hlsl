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

    // Convert XY from NDC [-1,1] to UV [0,1], flip Y for DX texture coords
    float2 shadowUV;
    shadowUV.x = projCoords.x * 0.5f + 0.5f;
    shadowUV.y = projCoords.y * -0.5f + 0.5f;

    // Z is already [0,1] in DirectX orthographic projection - don't remap
    float currentDepth = projCoords.z;

    float depthFromShadowMap = dShadowMap.Sample(mySampler, shadowUV).r;

    float bias = 0.001f;
    float shadow = (currentDepth - bias) > depthFromShadowMap ? 1.0f : 0.0f;
    
    stageOutput.outFragColor = float4(albedo.rgb * (1.0f - shadow + 0.2f), albedo.a);
    return stageOutput;
}
