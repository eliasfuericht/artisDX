cbuffer viewProjBuffer : register(b0)
{
    row_major float4x4 c_viewProjectionMatrix : packoffset(c0);
};

cbuffer ModelBuffer : register(b1)
{
    row_major float4x4 c_modelMatrix : packoffset(c0);
};

struct StageInput
{
    float3 inPos : POSITION;
    float3 inNormal : NORMAL;
    float4 inTangent : TANGENT;
    float2 inUV : TEXCOORD;
};

struct StageOutput
{
    float4 position : SV_Position;
    float2 outUV : TEXCOORD;
    float3 outNormal : NORMAL;
    float4 outTangent : TANGENT;
};

StageOutput main(StageInput stageInput)
{
    StageOutput stageOutput;

    float4 worldPos = mul(float4(stageInput.inPos, 1.0f), c_modelMatrix);
    stageOutput.position = mul(worldPos, c_viewProjectionMatrix);

    // Transform normal and tangent to world space
    float3 worldNormal = mul(stageInput.inNormal, (float3x3) c_modelMatrix);
    float3 worldTangent = mul(stageInput.inTangent.xyz, (float3x3) c_modelMatrix);

    stageOutput.outNormal = normalize(worldNormal);
    stageOutput.outTangent = float4(normalize(worldTangent), stageInput.inTangent.w);

    stageOutput.outUV = stageInput.inUV;
    return stageOutput;
}
