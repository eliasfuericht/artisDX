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
    float2 uv : TEXCOORD;
};

struct DirectionalLightData
{
    float3 direction;
    uint shadowsEnabled;
    float intensity;
    float3 color;
    uint shadowMapIndex;
    float3 padding;
    float4x4 lightViewProjection;
};

cbuffer modelMatrixBuffer : register(b1)
{
    row_major float4x4 c_modelMatrix : packoffset(c0);
};

StructuredBuffer<DirectionalLightData> directionalLights : register(t10);

StageOutput main(StageInput stageInput)
{
    StageOutput stageOutput;

    float4 worldPos = mul(float4(stageInput.inPos, 1.0f), c_modelMatrix);
    // for shadowpass we only bind 1 dLight at location 0
    stageOutput.position = mul(worldPos, directionalLights[0].lightViewProjection);
    stageOutput.uv = stageInput.inUV;
    
    return stageOutput;
}
