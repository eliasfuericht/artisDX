#include "ShaderPass.h"

void ShaderPass::AddShader(std::filesystem::path path, SHADERTYPE shaderType)
{
	_shaders.try_emplace(shaderType, Shader(path, shaderType));
}

void ShaderPass::GenerateRootSignature()
{
	std::vector<D3D12_DESCRIPTOR_RANGE1> ranges;
	std::vector<D3D12_ROOT_PARAMETER1> rootParams;

	for (const auto& shader : _shaders)
	{
		MSWRL::ComPtr<IDxcBlob> reflectionBlob{};
		ThrowIfFailed(shader.second._compiledShaderBuffer->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), 0), "Failed to retrieve Shader Reflection Data!");

		DxcBuffer reflectionBuffer{};
		reflectionBuffer.Ptr = reflectionBlob->GetBufferPointer();
		reflectionBuffer.Size = reflectionBlob->GetBufferSize();
		reflectionBuffer.Encoding = 0;

		MSWRL::ComPtr<ID3D12ShaderReflection> shaderReflection{};
		D3D12Core::ShaderCompiler::_utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&shaderReflection));
		D3D12_SHADER_DESC shaderDesc{};
		shaderReflection->GetDesc(&shaderDesc);

		for (UINT i = 0; i < shaderDesc.BoundResources; i++)
		{
			D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
			shaderReflection->GetResourceBindingDesc(i, &bindDesc);

			if (bindDesc.Type == D3D_SIT_CBUFFER)
			{
				D3D12_DESCRIPTOR_RANGE1 cbvRange{};
				cbvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
				cbvRange.NumDescriptors = bindDesc.BindCount;
				cbvRange.BaseShaderRegister = bindDesc.BindPoint;
				cbvRange.RegisterSpace = bindDesc.Space;
				cbvRange.OffsetInDescriptorsFromTableStart = 0;
				ranges.push_back(cbvRange);
			}
			else if (bindDesc.Type == D3D_SIT_TEXTURE)
			{
				// SRV table entry
				D3D12_DESCRIPTOR_RANGE1 srvRange{};
				srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				srvRange.NumDescriptors = bindDesc.BindCount;
				srvRange.BaseShaderRegister = bindDesc.BindPoint;
				srvRange.RegisterSpace = bindDesc.Space;
				srvRange.OffsetInDescriptorsFromTableStart = 0;
				ranges.push_back(srvRange);
			}
			else if (bindDesc.Type == D3D_SIT_SAMPLER)
			{
				// Sampler table entry
				D3D12_DESCRIPTOR_RANGE1 samplerRange{};
				samplerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
				samplerRange.NumDescriptors = bindDesc.BindCount;
				samplerRange.BaseShaderRegister = bindDesc.BindPoint;
				samplerRange.RegisterSpace = bindDesc.Space;
				samplerRange.OffsetInDescriptorsFromTableStart = 0;
				ranges.push_back(samplerRange);
			}
			else if (bindDesc.Type == D3D_SIT_UAV_RWTYPED)
			{
				// UAV table entry
				D3D12_DESCRIPTOR_RANGE1 uavRange{};
				uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				uavRange.NumDescriptors = bindDesc.BindCount;
				uavRange.BaseShaderRegister = bindDesc.BindPoint;
				uavRange.RegisterSpace = bindDesc.Space;
				uavRange.OffsetInDescriptorsFromTableStart = 0;
				ranges.push_back(uavRange);
			}
		}
	}

	for (const auto& range : ranges)
	{
		D3D12_ROOT_PARAMETER1 param{};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		param.DescriptorTable.NumDescriptorRanges = (UINT)range.NumDescriptors;
		param.DescriptorTable.pDescriptorRanges = &range;
		rootParams.push_back(param);
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootDesc{};
	rootDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootDesc.Desc_1_1.NumParameters = (UINT)rootParams.size();
	rootDesc.Desc_1_1.pParameters = rootParams.data();
	rootDesc.Desc_1_1.NumStaticSamplers = 0;
	rootDesc.Desc_1_1.pStaticSamplers = nullptr;
	rootDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	MSWRL::ComPtr<ID3DBlob> sigBlob;
	MSWRL::ComPtr<ID3DBlob> errorBlob;

	ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootDesc, &sigBlob, &errorBlob));

	ThrowIfFailed(D3D12Core::GraphicsDevice::_device->CreateRootSignature(
		0,
		sigBlob->GetBufferPointer(),
		sigBlob->GetBufferSize(),
		IID_PPV_ARGS(&_rootSignature)), "Root Signature creation failed!");
}