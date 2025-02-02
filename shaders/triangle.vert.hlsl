cbuffer ubo : register(b0)
{
    row_major float4x4 ubo_projectionMatrix : packoffset(c0);
    row_major float4x4 ubo_modelMatrix : packoffset(c4);
    row_major float4x4 ubo_viewMatrix : packoffset(c8);
};

static float4 position;
static float2 outUV;
static float2 inUV;
static float4 inTangent;
static float3 inNormal;
static float3 inPos;

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

    inUV = stage_input.inUV;
    inPos = stage_input.inPos;

    stage_output.position = mul(float4(inPos, 1.0f), mul(ubo_modelMatrix, mul(ubo_viewMatrix, ubo_projectionMatrix)));
    stage_output.outUV = inUV;

    return stage_output;
}
