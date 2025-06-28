cbuffer viewProjBuffer : register(b0)
{
    row_major float4x4 c_viewProjectionMatrix : packoffset(c0);
};

cbuffer ModelBuffer : register(b1)
{
    row_major float4x4 c_modelMatrix : packoffset(c0);
};

struct SPIRV_Cross_Input
{
    float3 inPos : POSITION;
    float3 inNormal : NORMAL;
    float4 inTangent : TANGENT;
    float2 inUV : TEXCOORD;
};

struct SPIRV_Cross_Output
{
    float4 position : SV_Position;
    float2 outUV : TEXCOORD;
    float3 outNormal : NORMAL;
    float4 outTangent : TANGENT;
};

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    SPIRV_Cross_Output stage_output;

    float4 worldPos = mul(float4(stage_input.inPos, 1.0f), c_modelMatrix);
    stage_output.position = mul(worldPos, c_viewProjectionMatrix);
    
    float3x3 normalMatrix = (float3x3) c_modelMatrix; 
    stage_output.outNormal = normalize(mul(stage_input.inNormal, normalMatrix));
    
    float3 worldTangent = normalize(mul(stage_input.inTangent.xyz, normalMatrix));
    stage_output.outTangent = float4(worldTangent, stage_input.inTangent.w);

    stage_output.outUV = stage_input.inUV;
    return stage_output;
}
