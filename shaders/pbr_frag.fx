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

// Constants for light
static const float3 lightDir = normalize(float3(0.3, 0.5, 0.8));
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

float3 getNormalFromMap(float2 uv, float3 normal, float4 tangent)
{
    // Sample normal map in tangent space
    float3 tangentNormal = normalTexture.Sample(mySampler, uv).xyz * 2.0 - 1.0;

    // Reconstruct tangent space basis
    float3 T = normalize(tangent.xyz);
    float3 N = normalize(normal);
    float3 B = cross(N, T) * tangent.w;

    // Transform tangent space normal to world space
    float3x3 TBN = float3x3(T, B, N);
    return normalize(mul(TBN, tangentNormal));
}

SPIRV_Cross_Output main(SPIRV_Cross_Input input)
{
    SPIRV_Cross_Output stage_output;
    
    // Sample textures
    float3 albedo = pow(albedoTexture.Sample(mySampler, input.inUV).rgb, 2.2); // Gamma to linear
    float3 normal = getNormalFromMap(input.inUV, input.inNormal, input.inTangent);

    float4 mr = metallicRoughnessTexture.Sample(mySampler, input.inUV);
    float metallic = mr.b; // Assume metallic stored in blue channel
    float roughness = mr.g; // Assume roughness stored in green channel

    float3 emissive = emissiveTexture.Sample(mySampler, input.inUV).rgb;
    float ao = occlusionTexture.Sample(mySampler, input.inUV).r;

    // Camera and lighting vectors
    float3 N = normal;
    float3 V = normalize(c_camPos - input.position.xyz); // View vector
    float3 L = normalize(-lightDir); // Light vector, inverse because directional light points TO surface
    float3 H = normalize(V + L); // Halfway vector

    // Calculate reflectance at normal incidence; if metallic use albedo else use dielectric F0
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    float3 specular = numerator / denominator;

    // kS is specular reflectance, kD is diffuse component
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);

    // Lambert diffuse
    float3 diffuse = kD * albedo / 3.14159265;

    // Final color
    float3 ambient = float3(0.03, 0.03, 0.03) * albedo * ao;

    float3 color = ambient + (diffuse + specular) * lightColor * NdotL + emissive;

    // Gamma correction
    color = pow(color, 1.0 / 2.2);

    stage_output.outFragColor = float4(color, 1.0);
    return stage_output;
}