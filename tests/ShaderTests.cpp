#include "TestCases.h"
#include "TestSupport.h"
#include "Shader.h"

namespace Testing
{
    void TestShader(const std::filesystem::path& path, bool optimized)
    {
        // Use the engine compiler/DLL and both configurations regardless of the C++ build configuration.
        D3D12Core::ShaderCompiler::InitializeShaderCompiler();
        MSWRL::ComPtr<IDxcBlobEncoding> source;
        ThrowIfFailed(D3D12Core::ShaderCompiler::utils->LoadFile(path.c_str(), nullptr, &source));
        DxcBuffer buffer{source->GetBufferPointer(), source->GetBufferSize(), 0};
        const auto profile = path.stem().string().ends_with("_vert") ? L"vs_6_7" : L"ps_6_7";
        std::vector<LPCWSTR> arguments{L"-E", L"main", L"-T", profile, L"-Zpr", L"-WX", L"-all_resources_bound"};
        if (optimized)
            arguments.push_back(L"-O3");
        else
            arguments.insert(arguments.end(), {L"-Zi", L"-Qembed_debug", L"-Od"});
        MSWRL::ComPtr<IDxcResult> result;
        ThrowIfFailed(D3D12Core::ShaderCompiler::compiler->Compile(&buffer, arguments.data(),
            static_cast<uint32_t>(arguments.size()), D3D12Core::ShaderCompiler::includeHandler.Get(), IID_PPV_ARGS(&result)));
        MSWRL::ComPtr<IDxcBlobUtf8> diagnostics;
        ThrowIfFailed(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&diagnostics), nullptr));
        if (diagnostics && diagnostics->GetStringLength())
            std::cerr << diagnostics->GetStringPointer();
        HRESULT status;
        ThrowIfFailed(result->GetStatus(&status));
        ThrowIfFailed(status, "Shader compilation failed: " + path.string());
        MSWRL::ComPtr<IDxcBlob> object;
        ThrowIfFailed(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&object), nullptr));
        Require(object && object->GetBufferSize() > 0, "Shader compiler must produce bytecode");
    }
}
