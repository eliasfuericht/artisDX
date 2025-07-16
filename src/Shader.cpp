#include "Shader.h"

Shader::Shader(std::filesystem::path path, SHADERTYPE shaderType)
{
	if (!D3D12Core::ShaderCompiler::utils || !D3D12Core::ShaderCompiler::compiler || !D3D12Core::ShaderCompiler::includeHandler)
		D3D12Core::ShaderCompiler::InitializeShaderCompiler();

	_shaderType = shaderType;

	std::vector<LPCWSTR> compilationArguments { L"-E", L"main", DXC_ARG_PACK_MATRIX_ROW_MAJOR, DXC_ARG_WARNINGS_ARE_ERRORS,DXC_ARG_ALL_RESOURCES_BOUND };

	compilationArguments.push_back(L"-T");

	switch (shaderType)
	{
	case SHADERTYPE::VERTEX:
		compilationArguments.push_back(L"vs_6_7");
		break;
	case SHADERTYPE::PIXEL:
		compilationArguments.push_back(L"ps_6_7");
		break;
	case SHADERTYPE::COMPUTE:
		compilationArguments.push_back(L"cs_6_7");
		break;
	default:
		break;
	}

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
	ThrowIfFailed(D3D12Core::ShaderCompiler::utils->LoadFile(path.c_str(), nullptr, &sourceBlob), "Failed to load shader with path: " + path.string());

	DxcBuffer sourceBuffer = {};
	sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
	sourceBuffer.Size = sourceBlob->GetBufferSize();
	sourceBuffer.Encoding = 0u;

	ThrowIfFailed(D3D12Core::ShaderCompiler::compiler->Compile(&sourceBuffer,
		compilationArguments.data(),
		static_cast<uint32_t>(compilationArguments.size()),
		D3D12Core::ShaderCompiler::includeHandler.Get(),
		IID_PPV_ARGS(&_compiledShaderBuffer)), "Failed to compile shader with path: " + path.string());

	MSWRL::ComPtr<IDxcBlobUtf8> errors{};
	ThrowIfFailed(_compiledShaderBuffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr), "Failed to retrieve Shader Compilation Errors!");
	if (errors && errors->GetStringLength() > 0)
	{
		const LPCSTR errorMessage = errors->GetStringPointer();
		ThrowException(errorMessage);
	}

	ThrowIfFailed(_compiledShaderBuffer->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&_shaderBlob), nullptr), "Failed to retrieve Shader Blob!");
}