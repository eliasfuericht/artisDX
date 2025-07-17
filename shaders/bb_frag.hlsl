struct StageInput
{
    float4 position : SV_Position;
    float2 inUV : TEXCOORD;
};

struct StageOutput
{
    float4 outFragColor : SV_Target0;
};

StageOutput main(StageInput stageInput)
{
    StageOutput stageOutput;
    
    stageOutput.outFragColor = float4(0.0f, 1.0f, 0.0f, 1.0f);
    
    return stageOutput;
}