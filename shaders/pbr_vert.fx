cbuffer viewProjMatrixBuffer : register(b0)
{
    row_major float4x4 c_viewProjectionMatrix : packoffset(c0);
};

cbuffer modelMatrixBuffer : register(b1)
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
    float4 outPosition : SV_Position;
    float3 outWorldPos : TEXCOORD0;
    float2 outUV : TEXCOORD1;
    float3 outNormal : NORMAL;
    float4 outTangent : TANGENT;
};

StageOutput main(StageInput stageInput)
{
    StageOutput output;
    
    float4 worldPos = mul(float4(stageInput.inPos, 1.0f), c_modelMatrix);
    output.outWorldPos = worldPos.xyz;
    
    output.outPosition = mul(worldPos, c_viewProjectionMatrix);
    
    output.outNormal = normalize(mul(float4(stageInput.inNormal, 0.0f), c_modelMatrix).xyz);
    
    float3 tangentWorld = normalize(mul(float4(stageInput.inTangent.xyz, 0.0f), c_modelMatrix).xyz);
    output.outTangent = float4(tangentWorld, stageInput.inTangent.w);

    output.outUV = stageInput.inUV;

    return output;
}
