static float4 outFragColor;
static float2 inUV;

struct SPIRV_Cross_Input
{
    float2 inUV : TEXCOORD;
    float4 inPos : SV_Position;
};

struct SPIRV_Cross_Output
{
    float4 outFragColor : SV_Target0;
};

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    SPIRV_Cross_Output stage_output;

    // Gradient effect using UV coordinates
    float2 uv = stage_input.inUV;
    float4 pos = stage_input.inPos;

    // Basic gradient: Horizontal (Red) and Vertical (Green)
    stage_output.outFragColor = float4(uv.x, uv.y, 1.0f - uv.x, 1.0f);
    // draws depthbuffer how it should be
    //stage_output.outFragColor = float4(pos.w / 1000.0f, pos.w / 1000.0f, pos.w / 1000.0f, 1.0f);

    return stage_output;
}
