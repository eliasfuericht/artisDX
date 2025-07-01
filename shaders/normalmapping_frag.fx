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

static const float3 lightDir = normalize(float3(-0.5, -1.0, -0.3));
static const float3 lightColor = float3(1.0, 1.0, 1.0);

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

    // Sample base color
    float4 texColor = albedoTexture.Sample(mySampler, input.inUV);

    // Sample and decode normal map (Tangent space)
    float3 normal = getNormalFromMap(input.inUV, input.inNormal, input.inTangent);

    // Diffuse Lambertian lighting
    float NdotL = max(dot(normal, -lightDir), 0.0f); // -lightDir = direction *from* light
    float3 diffuse = texColor.rgb * (lightColor * NdotL);

    stage_output.outFragColor = float4(diffuse, texColor.a);
    return stage_output;
}
