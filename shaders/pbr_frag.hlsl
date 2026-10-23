// Texture and sampler bound from root signature
Texture2D albedoTexture             : register(t0);
Texture2D metallicRoughnessTexture  : register(t1);
Texture2D normalTexture             : register(t2);
Texture2D emissiveTexture           : register(t3);
Texture2D occlusionTexture          : register(t4);
Texture2D shadowMaps[1] : register(t5);

SamplerState mySampler              : register(s0);

struct StageInput
{
    float4 inPosition : SV_Position;
    float3 inWorldPos : WORLDPOS;
    float2 inUV : TEXCOORD1;
    float3 inNormal : NORMAL;
    float4 inTangent : TANGENT;
    float3 inBiTangent : BITANGENT;
};

struct StageOutput
{
    float4 outFragColor : SV_Target0;
};

cbuffer cameraBuffer : register(b2)
{
    float3 c_camPos : packoffset(c0);
};

cbuffer pbrFactors : register(b5)
{
    float4 c_baseColorFactor;
    float c_metallicFactor;
    float c_roughnessFactor;
    float c_alphaCutoff;
    uint c_alphaMode;
};

struct DirectionalLightData
{
    float3 direction;
    uint shadowsEnabled;
    float intensity;
    float3 color;
    uint shadowMapIndex;
    float3 padding;
    float4x4 lightViewProjection;
};

struct PointLightData
{
    float3 position;
    float intensity;
    float3 color;
    float padding;
};

StructuredBuffer<DirectionalLightData> directionalLights : register(t10);
StructuredBuffer<PointLightData> pointLights : register(t11);

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

float3 EvaluateDirectLight(float3 N, float3 V, float3 L, float3 albedo, float metallic, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    if (NdotV <= 0.0f || NdotL <= 0.0f)
        return 0.0f;

    float3 H = normalize(V + L);
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(saturate(dot(H, V)), F0);
    float3 specular = D * G * F / max(4.0f * NdotV * NdotL, 1e-6f);
    float3 diffuse = (1.0f - F) * (1.0f - metallic) * albedo / 3.14159265f;
    return (diffuse + specular) * NdotL;
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
    float metallic = saturate(mr.b * c_metallicFactor);
    float roughness = clamp(mr.g * c_roughnessFactor, 0.045f, 1.0f);

    float3 emissive = emissiveTexture.Sample(mySampler, stageInput.inUV).rgb;
    float ao = occlusionTexture.Sample(mySampler, stageInput.inUV).r;
    
    float3 tangentNormal = normalTexture.Sample(mySampler, stageInput.inUV).rgb;
    tangentNormal = normalize(tangentNormal * 2.0f - 1.0f); 
    
    float3 N = normalize(stageInput.inNormal);
    float3 T = normalize(stageInput.inTangent.xyz);
    float3 B = normalize(stageInput.inBiTangent);

    float3x3 TBN = float3x3(T, B, N);
    
    float3 worldNormal = normalize(mul(tangentNormal, TBN));
    
    float3 viewDir = normalize(c_camPos - stageInput.inWorldPos); 
    float3 direct = 0.0f;
    uint plightCount, plightStride;
    pointLights.GetDimensions(plightCount, plightStride);
    for (uint i = 0; i < plightCount; ++i)
    {
        PointLightData light = pointLights[i];
        float3 toLight = light.position - stageInput.inWorldPos;
        float distanceSquared = dot(toLight, toLight);
        if (light.intensity <= 0.0f || distanceSquared <= 1e-8f)
            continue;

        float3 L = toLight * rsqrt(distanceSquared);
        direct += EvaluateDirectLight(worldNormal, viewDir, L, albedo, metallic, roughness)
            * light.color * light.intensity / distanceSquared;
    }

    float3 ambient = float3(0.03, 0.03, 0.03) * albedo * ao;

    uint dlightCount, dlightStride;
    directionalLights.GetDimensions(dlightCount, dlightStride);
    for (uint i = 0; i < dlightCount; ++i)
    {
        float shadowFactor = 1.0f;
        [branch]
        if (directionalLights[i].shadowsEnabled != 0.0f)
        {
            float4 lightSpace = mul(float4(stageInput.inWorldPos, 1.0f), directionalLights[i].lightViewProjection);
            float3 projCoords = lightSpace.xyz / lightSpace.w;

            float2 shadowUV;
            shadowUV.x = projCoords.x * 0.5f + 0.5f;
            shadowUV.y = projCoords.y * -0.5f + 0.5f;

            float currentDepth = projCoords.z;

            float bias = max(0.01f * (1.0f - dot(N, normalize(directionalLights[i].direction))), 0.001f);
            float shadow = PercentageCloserFiltering(shadowMaps[i], shadowUV, currentDepth, bias);
            shadowFactor = 1.0f - shadow;
        }
    
        if (dot(directionalLights[i].direction, directionalLights[i].direction) > 1e-8f)
        {
            direct += shadowFactor * directionalLights[i].color * EvaluateDirectLight(worldNormal, viewDir,
                normalize(directionalLights[i].direction), albedo, metallic, roughness);
        }
    }
    float3 color = ambient + direct + emissive;
    color = pow(color, 1.0 / 2.2);

    stageOutput.outFragColor = float4(color, albedoalpha.a);
    return stageOutput;
}
