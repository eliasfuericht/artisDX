// Texture and sampler bound from root signature
Texture2D albedoTexture             : register(t0);
Texture2D normalTexture             : register(t1);
Texture2D metallicRoughnessTexture  : register(t2);
Texture2D emissiveTexture           : register(t3);
Texture2D occlusionTexture          : register(t4);
SamplerState mySampler              : register(s0);

struct SPIRV_Cross_Input
{
    float4 position : SV_Position;
    float2 inUV : TEXCOORD;
    float3 inNormal : NORMAL;
    float4 inTangent : TANGENT;
};

struct SPIRV_Cross_Output
{
    float4 outFragColor : SV_Target0;
};

SPIRV_Cross_Output main(SPIRV_Cross_Input input)
{
    SPIRV_Cross_Output stage_output;
    
    stage_output.outFragColor = albedoTexture.Sample(mySampler, input.inUV);
    
    return stage_output;
}
