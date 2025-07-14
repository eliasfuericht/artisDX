#include "Shader.h"

namespace D3D12Core::ShaderCompiler
{
	Microsoft::WRL::ComPtr<IDxcUtils> _utils;
	Microsoft::WRL::ComPtr<IDxcCompiler3> _compiler;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> _includeHandler;

	void InitializeShaderCompiler()
	{
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&_utils)));
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&_compiler)));
		ThrowIfFailed(_utils->CreateDefaultIncludeHandler(&_includeHandler));
	}
}

Shader::Shader(std::filesystem::path path, SHADERTYPE shaderType)
{
	if (!D3D12Core::ShaderCompiler::_utils || !D3D12Core::ShaderCompiler::_compiler || !D3D12Core::ShaderCompiler::_includeHandler)
		D3D12Core::ShaderCompiler::InitializeShaderCompiler();

	std::vector<LPCWSTR> compilationArguments { L"-E", L"main", DXC_ARG_PACK_MATRIX_ROW_MAJOR, DXC_ARG_WARNINGS_ARE_ERRORS,DXC_ARG_ALL_RESOURCES_BOUND };

	compilationArguments.push_back(L"-T");

	switch (shaderType)
	{
	case VERTEX:
		compilationArguments.push_back(L"vs_6_7");
		break;
	case PIXEL:
		compilationArguments.push_back(L"ps_6_7");
		break;
	case COMPUTE:
		compilationArguments.push_back(L"cs_6_7");
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
	ThrowIfFailed(D3D12Core::ShaderCompiler::_utils->LoadFile(path.c_str(), nullptr, &sourceBlob), "Failed to load shader with path: " + path.string());

	DxcBuffer sourceBuffer = {};
	sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
	sourceBuffer.Size = sourceBlob->GetBufferSize();
	sourceBuffer.Encoding = 0u;

	Microsoft::WRL::ComPtr<IDxcResult> compiledShaderBuffer{};

	ThrowIfFailed(D3D12Core::ShaderCompiler::_compiler->Compile(&sourceBuffer,
		compilationArguments.data(),
		static_cast<uint32_t>(compilationArguments.size()),
		D3D12Core::ShaderCompiler::_includeHandler.Get(),
		IID_PPV_ARGS(&compiledShaderBuffer)), "Failed to compile shader with path: " + path.string());

	MSWRL::ComPtr<IDxcBlobUtf8> errors{};
	ThrowIfFailed(compiledShaderBuffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr), "Failed to retrieve Shader Compilation Errors!");
	if (errors && errors->GetStringLength() > 0)
	{
		const LPCSTR errorMessage = errors->GetStringPointer();
		ThrowException(errorMessage);
	}

	MSWRL::ComPtr<IDxcBlob> reflectionBlob{};
	ThrowIfFailed(compiledShaderBuffer->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), nullptr) , "Failed to retrieve Shader Reflection Data!");

	DxcBuffer reflectionBuffer {};
	reflectionBuffer.Ptr = reflectionBlob->GetBufferPointer();
	reflectionBuffer.Size = reflectionBlob->GetBufferSize();
	reflectionBuffer.Encoding = 0;

	MSWRL::ComPtr<ID3D12ShaderReflection> shaderReflection{};
	D3D12Core::ShaderCompiler::_utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&shaderReflection));
	D3D12_SHADER_DESC shaderDesc{};
	shaderReflection->GetDesc(&shaderDesc);
	
	std::vector<D3D12_DESCRIPTOR_RANGE1> srvRanges;
	std::vector<D3D12_DESCRIPTOR_RANGE1> samplerRanges;
	std::vector<D3D12_ROOT_PARAMETER1> rootParams;

	for (UINT i = 0; i < shaderDesc.BoundResources; i++)
	{
		D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
		shaderReflection->GetResourceBindingDesc(i, &bindDesc);

		if (bindDesc.Type == D3D_SIT_CBUFFER)
		{
			// Root CBV at register bX
			D3D12_ROOT_PARAMETER1 param{};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			param.Descriptor.ShaderRegister = bindDesc.BindPoint;
			param.Descriptor.RegisterSpace = bindDesc.Space;
			rootParams.push_back(param);
		}
		else if (bindDesc.Type == D3D_SIT_TEXTURE)
		{
			// SRV table entry
			D3D12_DESCRIPTOR_RANGE1 range{};
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			range.NumDescriptors = bindDesc.BindCount;
			range.BaseShaderRegister = bindDesc.BindPoint;
			range.RegisterSpace = bindDesc.Space;
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			srvRanges.push_back(range);
		}
		else if (bindDesc.Type == D3D_SIT_SAMPLER)
		{
			// Sampler table entry
			D3D12_DESCRIPTOR_RANGE1 range{};
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			range.NumDescriptors = bindDesc.BindCount;
			range.BaseShaderRegister = bindDesc.BindPoint;
			range.RegisterSpace = bindDesc.Space;
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			samplerRanges.push_back(range);
		}
		else if (bindDesc.Type == D3D_SIT_UAV_RWTYPED)
		{
			// UAV table entry
			D3D12_DESCRIPTOR_RANGE1 range{};
			range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			range.NumDescriptors = bindDesc.BindCount;
			range.BaseShaderRegister = bindDesc.BindPoint;
			range.RegisterSpace = bindDesc.Space;
			range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			srvRanges.push_back(range);
		}
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootDesc{};
	rootDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootDesc.Desc_1_1.NumParameters = (UINT)rootParams.size();
	rootDesc.Desc_1_1.pParameters = rootParams.data();
	rootDesc.Desc_1_1.NumStaticSamplers = 0;
	rootDesc.Desc_1_1.pStaticSamplers = nullptr;
	rootDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// Serialize & create
	MSWRL::ComPtr<ID3DBlob> sigBlob;
	MSWRL::ComPtr<ID3DBlob> errorBlob;

	ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootDesc, &sigBlob, &errorBlob));

	MSWRL::ComPtr<ID3D12RootSignature> rootSignature;
	ThrowIfFailed(D3D12Core::GraphicsDevice::_device->CreateRootSignature(
		0,
		sigBlob->GetBufferPointer(),
		sigBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)), "Root Signature creation failed!");

}