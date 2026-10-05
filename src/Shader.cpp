#include "Shader.h"

Shader::Shader(const std::filesystem::path& path, SHADERTYPE shaderType)
{
	if (!D3D12Core::ShaderCompiler::utils || !D3D12Core::ShaderCompiler::compiler || !D3D12Core::ShaderCompiler::includeHandler)
		D3D12Core::ShaderCompiler::InitializeShaderCompiler();

	_shaderType = shaderType;

	std::vector<LPCWSTR> compilationArguments { L"-E", L"main", DXC_ARG_PACK_MATRIX_ROW_MAJOR, DXC_ARG_WARNINGS_ARE_ERRORS,DXC_ARG_ALL_RESOURCES_BOUND };

	compilationArguments.push_back(L"-T");

	switch (shaderType)
	{
	case SHADERTYPE::SHADER_VERTEX:
		compilationArguments.push_back(L"vs_6_7");
		break;
	case SHADERTYPE::SHADER_PIXEL:
		compilationArguments.push_back(L"ps_6_7");
		break;
	case SHADERTYPE::SHADER_COMPUTE:
		compilationArguments.push_back(L"cs_6_7");
		break;
	default:
		ThrowException("Invalid shader stage " + std::to_string(shaderType) + " for " + path.string());
	}
	std::string profile;
	// The shader profiles above contain only ASCII characters.
	for (const wchar_t character : std::wstring(compilationArguments.back()))
		profile.push_back(static_cast<char>(character));
	const std::string context = "Shader " + path.string() + " [" + profile + "]";

#if defined(_DEBUG)
	compilationArguments.push_back(DXC_ARG_DEBUG);
	compilationArguments.push_back(L"-Zi");           
	compilationArguments.push_back(L"-Qembed_debug"); 
	compilationArguments.push_back(L"-Od");
#else
	compilationArguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif

	// Load the shader source file to a blob.
	MSWRL::ComPtr<IDxcBlobEncoding> sourceBlob{};
	ThrowIfFailed(D3D12Core::ShaderCompiler::utils->LoadFile(path.c_str(), nullptr, &sourceBlob), context + ": source loading failed");

	DxcBuffer sourceBuffer = {};
	sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
	sourceBuffer.Size = sourceBlob->GetBufferSize();
	sourceBuffer.Encoding = 0u;

	ThrowIfFailed(D3D12Core::ShaderCompiler::compiler->Compile(&sourceBuffer,
		compilationArguments.data(),
		static_cast<uint32_t>(compilationArguments.size()),
		D3D12Core::ShaderCompiler::includeHandler.Get(),
		IID_PPV_ARGS(&_compiledShaderBuffer)), context + ": compiler invocation failed");
	if (!_compiledShaderBuffer)
		ThrowException(context + ": compiler returned no result");

	HRESULT compilationStatus{};
	ThrowIfFailed(_compiledShaderBuffer->GetStatus(&compilationStatus), context + ": GetStatus failed");

	MSWRL::ComPtr<IDxcBlobUtf8> errors{};
	ThrowIfFailed(_compiledShaderBuffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr), context + ": diagnostic retrieval failed");
	const std::string diagnostics = errors && errors->GetStringLength() > 0 ? errors->GetStringPointer() : "";
	ThrowIfFailed(compilationStatus, context + ": compilation failed\n" + diagnostics);
	if (!diagnostics.empty())
		PRINT(context, ": ", diagnostics);

	ThrowIfFailed(_compiledShaderBuffer->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&_shaderBlob), nullptr), context + ": bytecode retrieval failed");
	if (!_shaderBlob || _shaderBlob->GetBufferSize() == 0)
		ThrowException(context + ": compiler returned empty bytecode");
}
