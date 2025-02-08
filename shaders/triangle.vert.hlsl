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
    float2 outUV : TEXCOORD;
    float4 position : SV_Position;
};

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    SPIRV_Cross_Output stage_output;

    float2 inUV = stage_input.inUV;
    float3 inPos = stage_input.inPos;

    stage_output.position = mul(float4(inPos, 1.0f), mul(c_modelMatrix, c_viewProjectionMatrix));
    stage_output.outUV = inUV;

    return stage_output;
}
