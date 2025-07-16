// Texture and sampler bound from root signature
Texture2D albedoTexture             : register(t0);
Texture2D metallicRoughnessTexture  : register(t1);
Texture2D normalTexture             : register(t2);
Texture2D emissiveTexture           : register(t3);
Texture2D occlusionTexture          : register(t4);

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

StageOutput main(StageInput stageInput)
{
    StageOutput stageOutput;
    
    float4 albedoalpha = pow(albedoTexture.Sample(mySampler, stageInput.inUV), 2.2);
    float3 albedo = albedoalpha.rgb;

    float4 mr = metallicRoughnessTexture.Sample(mySampler, stageInput.inUV);
    float metallic = mr.b; // Assume metallic stored in blue channel
    float roughness = mr.g; // Assume roughness stored in green channel

    float3 emissive = emissiveTexture.Sample(mySampler, stageInput.inUV).rgb;
    float ao = occlusionTexture.Sample(mySampler, stageInput.inUV).r;
    
    // Decode tangent-space normal from normal map
    float3 tangentNormal = normalTexture.Sample(mySampler, stageInput.inUV).rgb;
    tangentNormal = normalize(tangentNormal * 2.0f - 1.0f); // [-1,1] range

    // Prepare TBN matrix
    float3 N = normalize(stageInput.inNormal);
    float3 T = normalize(stageInput.inTangent.xyz);
    float3 B = normalize(stageInput.inBiTangent);

    float3x3 TBN = float3x3(T, B, N);

    // Transform from tangent space to world space
    float3 worldNormal = normalize(mul(tangentNormal, TBN));

    // Lighting (simple Lambert diffuse for demonstration)
    float3 lightDir = normalize(c_dLightPosition - stageInput.inWorldPos);
    float NdotL = max(dot(worldNormal, lightDir), 0.0);

    // Camera and lighting vectors
    float3 viewDir = normalize(c_camPos - stageInput.inWorldPos); // View vector
    float3 H = normalize(viewDir + lightDir); // Halfway vector

    // Calculate reflectance at normal incidence; if metallic use albedo else use dielectric F0
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(worldNormal, H, roughness);
    float G = GeometrySmith(worldNormal, viewDir, lightDir, roughness);
    float3 F = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(worldNormal, viewDir), 0.0) * max(dot(worldNormal, lightDir), 0.0) + 0.001;
    float3 specular = numerator / denominator;

    // kS is specular reflectance, kD is diffuse component
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    
    float3 diffuse = kD * albedo / 3.14159265;
    
    float3 ambient = float3(0.03, 0.03, 0.03) * albedo * ao;

    float3 color = ambient + (diffuse + specular) * lightColor * NdotL + emissive;
    
    // gammacorrection
    color = pow(color, 1.0 / 2.2);
    
    stageOutput.outFragColor = float4(color, albedoalpha.a);
    return stageOutput;
}