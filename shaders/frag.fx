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

    // Sample base color
    float4 texColor = albedoTexture.Sample(mySampler, input.inUV);

    // Sample and decode normal map (Tangent space)
    float3 normalSample = normalTexture.Sample(mySampler, input.inUV).xyz;
    float3 tangentNormal = normalize(normalSample * 2.0f - 1.0f);

    // Normalize inputs (already transformed to world space in vertex shader)
    float3 N = normalize(input.inNormal);
    float3 T = normalize(input.inTangent.xyz);

    // Re-orthogonalize tangent to normal
    T = normalize(T - dot(T, N) * N);

    // Calculate bitangent using handedness (inTangent.w)
    float3 B = cross(N, T) * input.inTangent.w;

    // Construct TBN matrix and transform tangent space normal to world space
    float3x3 TBN = float3x3(T, B, N);
    float3 worldNormal = normalize(mul(TBN, tangentNormal));

    // Simple directional light
    float angle = input.position.x * 0.01f; // Pseudo-time based on pixel x-position
    float3 lightDir = normalize(float3(sin(angle), 1.0f, cos(angle)));
    float3 lightColor = float3(1.0f, 1.0f, 1.0f); // White light

    // Diffuse Lambertian lighting
    float NdotL = max(dot(worldNormal, -lightDir), 0.0f); // -lightDir = direction *from* light
    float3 diffuse = texColor.rgb * (lightColor * NdotL);

    stage_output.outFragColor = float4(diffuse, texColor.a);
    stage_output.outFragColor = texColor;
    return stage_output;
}
