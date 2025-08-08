cbuffer viewProjMatrixBuffer : register(b0)
{
    row_major float4x4 c_viewProjectionMatrix : packoffset(c0);
};

cbuffer modelMatrixBuffer : register(b1)
{
    row_major float4x4 c_modelMatrix : packoffset(c0);
};

cbuffer lightViewProjMatrixBuffer : register(b2)
{
    row_major float4x4 c_lightViewProjectionMatrix : packoffset(c0);
};

struct StageInput
{
    float3 inPos : POSITION;
    float3 inNormal : NORMAL;
    float2 inUV : TEXCOORD;
    float4 inTangent : TANGENT;
    float3 inBiTangent : BITANGENT;
};

struct StageOutput
{
    float4 position : SV_Position;
    float2 outUV : TEXCOORD;
    float4 outFragPosLightSpace : FRAGPOSLIGHTSPACE;
};

StageOutput main(StageInput stageInput)
{
    StageOutput stageOutput;

    float4 worldPos = mul(float4(stageInput.inPos, 1.0f), c_modelMatrix);
    stageOutput.position = mul(worldPos, c_viewProjectionMatrix);
    
    stageOutput.outFragPosLightSpace = mul(worldPos, c_lightViewProjectionMatrix);

    stageOutput.outUV = stageInput.inUV;
    
    return stageOutput;
}
