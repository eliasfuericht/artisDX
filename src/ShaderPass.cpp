#include "ShaderPass.h"

ShaderPass::ShaderPass(const std::string& name)
{
	_name = name;
}

void ShaderPass::AddShader(const std::filesystem::path& path, SHADERTYPE shaderType)
{
	_shaders.try_emplace(shaderType, Shader(path, shaderType));
}

void ShaderPass::GenerateGraphicsRootSignature()
{
	std::vector<std::pair<SHADERTYPE, D3D12_DESCRIPTOR_RANGE1>> ranges;
	std::vector<D3D12_ROOT_PARAMETER1> rootParams;

	uint32_t incrementor = 0;
	for (const auto& shader : _shaders)
	{
		MSWRL::ComPtr<IDxcBlob> reflectionBlob{};
		ThrowIfFailed(shader.second._compiledShaderBuffer->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), nullptr), "Failed to retrieve Shader Reflection Data!");

		DxcBuffer reflectionBuffer{};
		reflectionBuffer.Ptr = reflectionBlob->GetBufferPointer();
		reflectionBuffer.Size = reflectionBlob->GetBufferSize();
		reflectionBuffer.Encoding = 0;

		MSWRL::ComPtr<ID3D12ShaderReflection> shaderReflection{};
		D3D12Core::ShaderCompiler::utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&shaderReflection));
		D3D12_SHADER_DESC shaderDesc{};
		shaderReflection->GetDesc(&shaderDesc);

		for (size_t i = 0; i < shaderDesc.BoundResources; i++)
		{
			D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
			shaderReflection->GetResourceBindingDesc(i, &bindDesc);

			_bindingMap.try_emplace(bindDesc.Name, incrementor++);

			if (bindDesc.Type == D3D_SIT_CBUFFER)
			{
				D3D12_DESCRIPTOR_RANGE1 cbvRange{};
				cbvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
				cbvRange.NumDescriptors = bindDesc.BindCount;
				cbvRange.BaseShaderRegister = bindDesc.BindPoint;
				cbvRange.RegisterSpace = bindDesc.Space;
				cbvRange.OffsetInDescriptorsFromTableStart = 0;
				ranges.emplace_back(shader.first, cbvRange);
			}
			else if (bindDesc.Type == D3D_SIT_TEXTURE)
			{
				D3D12_DESCRIPTOR_RANGE1 srvRange{};
				srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				srvRange.NumDescriptors = bindDesc.BindCount;
				srvRange.BaseShaderRegister = bindDesc.BindPoint;
				srvRange.RegisterSpace = bindDesc.Space;
				srvRange.OffsetInDescriptorsFromTableStart = 0;
				ranges.emplace_back(shader.first, srvRange);
			}
			else if (bindDesc.Type == D3D_SIT_SAMPLER)
			{
				D3D12_DESCRIPTOR_RANGE1 samplerRange{};
				samplerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
				samplerRange.NumDescriptors = bindDesc.BindCount;
				samplerRange.BaseShaderRegister = bindDesc.BindPoint;
				samplerRange.RegisterSpace = bindDesc.Space;
				samplerRange.OffsetInDescriptorsFromTableStart = 0;
				ranges.emplace_back(shader.first, samplerRange);
			}
			else if (bindDesc.Type == D3D_SIT_UAV_RWTYPED)
			{
				D3D12_DESCRIPTOR_RANGE1 uavRange{};
				uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				uavRange.NumDescriptors = bindDesc.BindCount;
				uavRange.BaseShaderRegister = bindDesc.BindPoint;
				uavRange.RegisterSpace = bindDesc.Space;
				uavRange.OffsetInDescriptorsFromTableStart = 0;
				ranges.emplace_back(shader.first, uavRange);
			}
		}
	}

	for (const std::pair<SHADERTYPE, D3D12_DESCRIPTOR_RANGE1>& range : ranges)
	{
		D3D12_ROOT_PARAMETER1 param{};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

		switch (range.first)
		{
		case SHADERTYPE::SHADER_VERTEX:
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
			break;
		case SHADERTYPE::SHADER_PIXEL:
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			break;
		default:
			break;
		}
		param.DescriptorTable.NumDescriptorRanges = range.second.NumDescriptors;
		param.DescriptorTable.pDescriptorRanges = &range.second;
		rootParams.push_back(param);
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootDesc{};
	rootDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootDesc.Desc_1_1.NumParameters = rootParams.size();
	rootDesc.Desc_1_1.pParameters = rootParams.data();
	rootDesc.Desc_1_1.NumStaticSamplers = 0;
	rootDesc.Desc_1_1.pStaticSamplers = nullptr;
	rootDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	MSWRL::ComPtr<ID3DBlob> sigBlob;
	MSWRL::ComPtr<ID3DBlob> errorBlob;

	ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootDesc, &sigBlob, &errorBlob), "idk what this is tbh");

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateRootSignature(
		0,
		sigBlob->GetBufferPointer(),
		sigBlob->GetBufferSize(),
		IID_PPV_ARGS(&_rootSignature)), "RootSignature creation failed!");
}

void ShaderPass::GeneratePipeLineStateObjectForwardPass(D3D12_FILL_MODE fillMode, D3D12_CULL_MODE cullMode, bool alphaBlending)
{
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = _rootSignature.Get();

	D3D12_SHADER_BYTECODE vsBytecode;
	D3D12_SHADER_BYTECODE psBytecode;

	vsBytecode.pShaderBytecode = _shaders.find(SHADERTYPE::SHADER_VERTEX)->second._shaderBlob->GetBufferPointer();
	vsBytecode.BytecodeLength = _shaders.find(SHADERTYPE::SHADER_VERTEX)->second._shaderBlob->GetBufferSize();
	psoDesc.VS = vsBytecode;

	psBytecode.pShaderBytecode = _shaders.find(SHADERTYPE::SHADER_PIXEL)->second._shaderBlob->GetBufferPointer();
	psBytecode.BytecodeLength = _shaders.find(SHADERTYPE::SHADER_PIXEL)->second._shaderBlob->GetBufferSize();
	psoDesc.PS = psBytecode;

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FillMode = fillMode;
	psoDesc.RasterizerState.CullMode = cullMode;
	
	if (alphaBlending)
	{
		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = true;
		blendDesc.RenderTarget[0].LogicOpEnable = false;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		psoDesc.BlendState = blendDesc;
	}
	else
	{
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	}

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pipelineState)), "PipelineStateObject creation failed!");
}

std::optional<uint32_t> ShaderPass::GetRootParameterIndex(const std::string& name) const {
	auto it = _bindingMap.find(name);
	if (it == _bindingMap.end())
		return std::nullopt;
	return it->second;
}

void ShaderPass::DrawGUI()
{
	GUI::Begin(_name.c_str());
	GUI::Checkbox("Enable Pass", &_usePass);
	GUI::End();
}