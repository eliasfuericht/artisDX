// Intentional compiler error for the production Shader constructor regression.
float4 main() : SV_TARGET
{
    return missing_regression_symbol;
}
