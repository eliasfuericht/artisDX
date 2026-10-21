// Texture and sampler bound from root signature
Texture2D albedoTexture             : register(t0);
Texture2D metallicRoughnessTexture  : register(t1);
Texture2D normalTexture             : register(t2);
Texture2D emissiveTexture           : register(t3);
Texture2D occlusionTexture          : register(t4);
Texture2D dShadowMap : register(t5);

SamplerState mySampler              : register(s0);

cbuffer cameraBuffer : register(b2)
{
    float3 c_camPos : packoffset(c0);
};

cbuffer plightBuffer : register(b3)
{
    float3 c_pLightPosition : packoffset(c0);
};

cbuffer dlightBuffer : register(b4)
{
    float3 c_dLightDirection : packoffset(c0);
    float c_shadowsEnabled : packoffset(c0.w);
};

cbuffer pbrFactors : register(b5)
{
    float4 c_baseColorFactor;
    float c_metallicFactor;
    float c_roughnessFactor;
    float c_alphaCutoff;
    uint c_alphaMode; // fastgltf::AlphaMode: Opaque=0, Mask=1, Blend=2.
};

struct StageInput
{
    float4 inPosition : SV_Position;
    float3 inWorldPos : WORLDPOS;
    float2 inUV : TEXCOORD1;
    float3 inNormal : NORMAL;
    float4 inTangent : TANGENT;
    float3 inBiTangent : BITANGENT;
    float4 inFragPosLightSpace : FRAGPOSLIGHTSPACE;
};

struct StageOutput
{
    float4 outFragColor : SV_Target0;
};

static const float3 lightColor = float3(1.0, 1.0, 1.0);

// Helper functions for PBR
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;

    return a2 / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float denom = NdotV * (1.0 - k) + k;
    return NdotV / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float PercentageCloserFiltering(Texture2D shadowMap, float2 locationUV, float currentDepth, float bias)
{
    if (any(locationUV < 0.0f) || any(locationUV >= 1.0f) || currentDepth < 0.0f || currentDepth > 1.0f)
        return 0.0f;

    uint width, height;
    shadowMap.GetDimensions(width, height);

    float2 texelSize = 1.0f / float2(width, height);
    float shadow = 0.0f;

    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            float2 sampleUV = locationUV + float2(x, y) * texelSize;

            int2 samplePixel = int2(floor(sampleUV * float2(width, height)));
            if (any(samplePixel < 0) || any(samplePixel >= int2(width, height)))
                continue;

            float sampledDepth = shadowMap.Load(int3(samplePixel, 0)).r;
            shadow += currentDepth - bias > sampledDepth ? 1.0f : 0.0f;
        }
    }

    return shadow / 9.0f;
}

StageOutput main(StageInput stageInput)
{
    StageOutput stageOutput;
    
    float4 albedoalpha = albedoTexture.Sample(mySampler, stageInput.inUV);
    albedoalpha.rgb = pow(albedoalpha.rgb, 2.2);
    albedoalpha *= c_baseColorFactor;
    if (c_alphaMode == 1)
    {
        clip(albedoalpha.a - c_alphaCutoff);
    }
    
    if (c_alphaMode != 2)
    {
        albedoalpha.a = 1.0;
    }
    
    float3 albedo = albedoalpha.rgb;

    float4 mr = metallicRoughnessTexture.Sample(mySampler, stageInput.inUV);
    float metallic = mr.b * c_metallicFactor; 
    float roughness = mr.g * c_roughnessFactor;

    float3 emissive = emissiveTexture.Sample(mySampler, stageInput.inUV).rgb;
    float ao = occlusionTexture.Sample(mySampler, stageInput.inUV).r;
    
    float3 tangentNormal = normalTexture.Sample(mySampler, stageInput.inUV).rgb;
    tangentNormal = normalize(tangentNormal * 2.0f - 1.0f); 
    
    float3 N = normalize(stageInput.inNormal);
    float3 T = normalize(stageInput.inTangent.xyz);
    float3 B = normalize(stageInput.inBiTangent);

    float3x3 TBN = float3x3(T, B, N);
    
    float3 worldNormal = normalize(mul(tangentNormal, TBN));
    
    //float3 lightDir = normalize(c_dLightDirection);
    float3 lightDir = normalize(c_pLightPosition - stageInput.inWorldPos);
    float NdotL = max(dot(worldNormal, lightDir), 0.0);
    
    float3 viewDir = normalize(c_camPos - stageInput.inWorldPos); 
    float3 H = normalize(viewDir + lightDir); 
    
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    
    float NDF = DistributionGGX(worldNormal, H, roughness);
    float G = GeometrySmith(worldNormal, viewDir, lightDir, roughness);
    float3 F = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(worldNormal, viewDir), 0.0) * max(dot(worldNormal, lightDir), 0.0) + 0.001;
    float3 specular = numerator / denominator;
    
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    
    float3 diffuse = kD * albedo / 3.14159265;
    
    float3 ambient = float3(0.03, 0.03, 0.03) * albedo * ao;

    float shadowFactor = 1.0f;
    [branch]
    if (c_shadowsEnabled != 0.0f)
    {
        float3 projCoords = stageInput.inFragPosLightSpace.xyz / stageInput.inFragPosLightSpace.w;

        float2 shadowUV;
        shadowUV.x = projCoords.x * 0.5f + 0.5f;
        shadowUV.y = projCoords.y * -0.5f + 0.5f;

        float currentDepth = projCoords.z;

        float bias = max(0.01f * (1.0f - dot(N, lightDir)), 0.001f);
        float shadow = PercentageCloserFiltering(dShadowMap, shadowUV, currentDepth, bias);
        shadowFactor = 1.0f - shadow;
    }
    
    float3 direct = (diffuse + specular) * lightColor * NdotL;
    float3 color = ambient + shadowFactor * direct + emissive;
    color = pow(color, 1.0 / 2.2);

    stageOutput.outFragColor = float4(color, albedoalpha.a);
    return stageOutput;
}
