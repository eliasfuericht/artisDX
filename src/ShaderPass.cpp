#include "ShaderPass.h"

ShaderPass::ShaderPass(const std::string& name)
{
	_name = name;
}

void ShaderPass::AddShader(const std::filesystem::path& path, SHADERTYPE shaderType)
{
	_shaders.try_emplace(shaderType, Shader(path, shaderType));
}

MSWRL::ComPtr<ID3DBlob> ShaderPass::GenerateGraphicsRootSignature()
{
	struct ReflectedBinding
	{
		D3D12_DESCRIPTOR_RANGE1 range;
		D3D12_SHADER_VISIBILITY visibility;
		D3D_SHADER_INPUT_TYPE type;
		D3D_RESOURCE_RETURN_TYPE returnType;
		D3D_SRV_DIMENSION dimension;
		UINT numSamples;
	};
	std::vector<ReflectedBinding> ranges;
	std::vector<D3D12_ROOT_PARAMETER1> rootParams;
	std::unordered_map<std::string, uint32_t> bindings;
	std::unordered_map<std::string, std::string> bindingDescriptions;
	D3D12_ROOT_SIGNATURE_FLAGS rootFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// Fix stage order independently of unordered_map iteration and insertion order.
	std::vector<SHADERTYPE> stages;
	for (const auto& shader : _shaders)
		stages.push_back(shader.first);
	std::sort(stages.begin(), stages.end());
	for (const SHADERTYPE stage : stages)
	{
		const std::string context = "Shader pass '" + _name + "', stage " + std::to_string(stage);
		if (stage != SHADER_VERTEX && stage != SHADER_PIXEL)
			ThrowException(context + ": unsupported graphics stage");
		const Shader& shader = _shaders.at(stage);
		if (!shader._compiledShaderBuffer)
			ThrowException(context + ": no compilation result for reflection");
		MSWRL::ComPtr<IDxcBlob> reflectionBlob;
		ThrowIfFailed(shader._compiledShaderBuffer->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), nullptr), context + ": reflection output retrieval failed");
		if (!reflectionBlob || reflectionBlob->GetBufferSize() == 0)
			ThrowException(context + ": missing reflection output");
		DxcBuffer reflectionBuffer{reflectionBlob->GetBufferPointer(), reflectionBlob->GetBufferSize(), 0};
		MSWRL::ComPtr<ID3D12ShaderReflection> reflection;
		ThrowIfFailed(D3D12Core::ShaderCompiler::utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflection)), context + ": CreateReflection failed");
		D3D12_SHADER_DESC shaderDesc{};
		ThrowIfFailed(reflection->GetDesc(&shaderDesc), context + ": reflection GetDesc failed");
		if (reflection->GetRequiresFlags() & D3D_SHADER_REQUIRES_RESOURCE_DESCRIPTOR_HEAP_INDEXING)
			rootFlags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
		for (uint32_t i = 0; i < shaderDesc.BoundResources; ++i)
		{
			D3D12_SHADER_INPUT_BIND_DESC binding{};
			ThrowIfFailed(reflection->GetResourceBindingDesc(i, &binding), context + ": GetResourceBindingDesc failed at resource " + std::to_string(i));
			const std::string description = context + ", resource '" + binding.Name + "', kind " + std::to_string(binding.Type) +
				", register " + std::to_string(binding.BindPoint) + ", space " + std::to_string(binding.Space) + ", count " + std::to_string(binding.BindCount);
			if (binding.BindCount == 0 || binding.BindCount == UINT32_MAX)
				ThrowException(description + ": unbounded descriptor ranges are unsupported");
			D3D12_DESCRIPTOR_RANGE1 range{};
			switch (binding.Type)
			{
			case D3D_SIT_CBUFFER: range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; break;
			case D3D_SIT_TEXTURE:
			case D3D_SIT_STRUCTURED: range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; break;
			case D3D_SIT_SAMPLER: range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER; break;
			case D3D_SIT_UAV_RWTYPED: range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; break;
			default: ThrowException(description + ": unsupported binding kind");
			}
			if (auto previous = bindings.find(binding.Name); previous != bindings.end())
			{
				ReflectedBinding& existing = ranges[previous->second];
				if (existing.type != binding.Type || existing.returnType != binding.ReturnType ||
					existing.dimension != binding.Dimension || existing.numSamples != binding.NumSamples ||
					existing.range.BaseShaderRegister != binding.BindPoint || existing.range.RegisterSpace != binding.Space ||
					existing.range.NumDescriptors != binding.BindCount)
					ThrowException(description + ": conflicting binding; first binding: " + bindingDescriptions.at(binding.Name));
				existing.visibility = D3D12_SHADER_VISIBILITY_ALL;
				continue;
			}
			bindingDescriptions.emplace(binding.Name, description);
			bindings.emplace(binding.Name, static_cast<uint32_t>(ranges.size()));
			range.NumDescriptors = binding.BindCount;
			range.BaseShaderRegister = binding.BindPoint;
			range.RegisterSpace = binding.Space;
			range.OffsetInDescriptorsFromTableStart = 0;
			ranges.push_back({range, stage == SHADER_VERTEX ? D3D12_SHADER_VISIBILITY_VERTEX : D3D12_SHADER_VISIBILITY_PIXEL,
				binding.Type, binding.ReturnType, binding.Dimension, binding.NumSamples});
		}
	}
	for (const auto& range : ranges)
	{
		D3D12_ROOT_PARAMETER1 param{};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param.ShaderVisibility = range.visibility;
		param.DescriptorTable = {1, &range.range};
		rootParams.push_back(param);
	}
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootDesc{};
	rootDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootDesc.Desc_1_1.NumParameters = static_cast<uint32_t>(rootParams.size());
	rootDesc.Desc_1_1.pParameters = rootParams.data();
	rootDesc.Desc_1_1.Flags = rootFlags;
	auto serialized = CreateRootSignature(rootDesc);
	_bindingMap.swap(bindings);
	return serialized;
}

MSWRL::ComPtr<ID3DBlob> ShaderPass::CreateRootSignature(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& rootDesc)
{
	MSWRL::ComPtr<ID3DBlob> sigBlob;
	MSWRL::ComPtr<ID3DBlob> errorBlob;

	const HRESULT serializationResult = D3D12SerializeVersionedRootSignature(&rootDesc, &sigBlob, &errorBlob);
	std::string diagnostics;
	if (errorBlob && errorBlob->GetBufferSize() > 0)
	{
		diagnostics.assign(static_cast<const char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
		if (diagnostics.back() == '\0')
			diagnostics.pop_back();
	}
	ThrowIfFailed(serializationResult, "Shader pass '" + _name + "': root signature serialization failed\n" + diagnostics);
	if (!sigBlob || sigBlob->GetBufferSize() == 0)
		ThrowException("Shader pass '" + _name + "': serializer returned empty root signature");

	MSWRL::ComPtr<ID3D12RootSignature> rootSignature;
	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateRootSignature(
		0,
		sigBlob->GetBufferPointer(),
		sigBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)), "Shader pass '" + _name + "': RootSignature creation failed");
	_rootSignature.Swap(rootSignature);
	return sigBlob;
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
	psoDesc.RasterizerState.FrontCounterClockwise = true;
	
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pipelineState)), "PipelineStateObject creation failed!");
	const std::wstring name(_name.begin(), _name.end());
	ThrowIfFailed(_pipelineState->SetName((name + L" opaque").c_str()));

	_transparentPipelineState.Reset();
	if (alphaBlending)
	{
		auto& blend = psoDesc.BlendState.RenderTarget[0];
		blend.BlendEnable = true;
		blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blend.SrcBlendAlpha = D3D12_BLEND_ONE;
		blend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_transparentPipelineState)), "Transparent PipelineStateObject creation failed!");
		ThrowIfFailed(_transparentPipelineState->SetName((name + L" transparent").c_str()));
	}
}

std::optional<uint32_t> ShaderPass::GetRootParameterIndex(const std::string& name) const {
	auto it = _bindingMap.find(name);
	if (it == _bindingMap.end())
		return std::nullopt;
	return it->second;
}

void ShaderPass::DrawGUI()
{
	const std::string windowName = std::to_string(_id) + "_" + _name + "###ShaderPass_" + std::to_string(_id);
	ImGui::Begin(windowName.c_str());
	ImGui::Checkbox("Enable Pass", &_usePass);
	ImGui::End();
}
