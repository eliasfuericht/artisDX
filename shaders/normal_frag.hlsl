// Texture and sampler bound from root signature
Texture2D albedoTexture             : register(t0);
Texture2D normalTexture             : register(t2);

SamplerState mySampler              : register(s0);

cbuffer cameraPosBuffer : register(b2)
{
    float3 c_camPos : packoffset(c0);
};

cbuffer lightPosBuffer : register(b3)
{
    float3 c_dLightPosition : packoffset(c0);
};

struct StageInput
{
    float4 inPosition : SV_Position;
    float3 inWorldPos : TEXCOORD0;
    float2 inUV : TEXCOORD1;
    float3 inNormal : NORMAL;
    float4 inTangent : TANGENT;
    float3 inBiTangent : BITANGENT;
};

struct StageOutput
{
    float4 outFragColor : SV_Target0;
};

static const float3 lightColor = float3(1.0, 1.0, 1.0);

StageOutput main(StageInput stageInput)
{
    StageOutput output;
    
    float4 texColor = albedoTexture.Sample(mySampler, stageInput.inUV);
    
    float3 tangentNormal = normalTexture.Sample(mySampler, stageInput.inUV).rgb;
    tangentNormal = normalize(tangentNormal * 2.0f - 1.0f);
    
    float3 N = normalize(stageInput.inNormal);
    float3 T = normalize(stageInput.inTangent.xyz);
    float3 B = normalize(stageInput.inBiTangent);

    float3x3 TBN = float3x3(T, B, N);
    
    float3 worldNormal = normalize(mul(tangentNormal, TBN));
    
    float3 lightDir = normalize(c_dLightPosition - stageInput.inWorldPos);
    float NdotL = max(dot(worldNormal, lightDir), 0.0);

    float3 finalColor = texColor.rgb * lightColor * NdotL;

    output.outFragColor = float4(finalColor, texColor.a);
    return output;
}
