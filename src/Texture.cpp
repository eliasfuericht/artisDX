#include "Texture.h"

Texture::Texture(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, Texture::TEXTURETYPE texType, ScratchImage& scratchImage)
{
	_image = std::move(scratchImage);

	// convert image if needed
	if (_image.GetMetadata().format != DXGI_FORMAT_R8G8B8A8_UNORM)
	{
		ScratchImage converted;
		ThrowIfFailed(Convert(
			_image.GetImages(),
			_image.GetImageCount(),
			_image.GetMetadata(),
			DXGI_FORMAT_R8G8B8A8_UNORM,
			TEX_FILTER_DEFAULT,
			TEX_THRESHOLD_DEFAULT,
			converted
		));

		_image = std::move(converted);
	}

	// dont generate mipmaps for fallbacktextures -> throws error
	if (_image.GetMetadata().width > 1 && _image.GetMetadata().height > 1)
	{
		ScratchImage mipChain;
		ThrowIfFailed(GenerateMipMaps(
			_image.GetImages(),
			_image.GetImageCount(),
			_image.GetMetadata(),
			TEX_FILTER_DEFAULT,
			0,
			mipChain
		));

		_image = std::move(mipChain);
	}

	_textureType = texType;

	CreateBuffers(commandList);
}

void Texture::CreateBuffers(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	const TexMetadata& metadata = _image.GetMetadata();
	_mipCount = static_cast<uint32_t>(metadata.mipLevels);

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	textureDesc.Width = static_cast<uint32_t>(metadata.width);
	textureDesc.Height = static_cast<uint32_t>(metadata.height);
	textureDesc.DepthOrArraySize = static_cast<uint16_t>(metadata.arraySize);
	textureDesc.MipLevels = static_cast<uint16_t>(_mipCount);
	textureDesc.Format = metadata.format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(
		&defaultHeap,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&_textureResource)));

	const uint64_t uploadBufferSize = GetRequiredIntermediateSize(_textureResource.Get(), 0, _mipCount);

	D3D12_HEAP_PROPERTIES defaultUploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	// Create the GPU upload buffer.
	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(
		&defaultUploadHeap,
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_textureUploadHeap)));

	std::vector<D3D12_SUBRESOURCE_DATA> subresources(_mipCount);

	for (size_t i = 0; i < _mipCount; ++i) {
		const Image* img = _image.GetImage(i, 0, 0);
		subresources[i].pData = img->pixels;
		subresources[i].RowPitch = img->rowPitch;
		subresources[i].SlicePitch = img->slicePitch;
	}

	UpdateSubresources(commandList.Get(), _textureResource.Get(), _textureUploadHeap.Get(), 0, 0, _mipCount, subresources.data());

	_srvCpuHandle = DescriptorAllocator::CBVSRVUAV::Allocate();

	// Describe and create a SRV for the texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = _mipCount;
	srvDesc.Texture2D.MostDetailedMip = 0;
	D3D12Core::GraphicsDevice::device->CreateShaderResourceView(_textureResource.Get(), &srvDesc, _srvCpuHandle);
}

void Texture::BindTexture(const ShaderPass& shaderPass, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = DescriptorAllocator::CBVSRVUAV::GetGPUHandle(_srvCpuHandle);
	switch (_textureType)
	{
	case TEXTURE_ALBEDO:
		if (auto slot = shaderPass.GetRootParameterIndex("albedoTexture"))
			commandList->SetGraphicsRootDescriptorTable(slot.value(), gpuHandle);
		break;
	case TEXTURE_METALLICROUGHNESS:
		if (auto slot = shaderPass.GetRootParameterIndex("metallicRoughnessTexture"))
			commandList->SetGraphicsRootDescriptorTable(slot.value(), gpuHandle);
		break;
	case TEXTURE_NORMAL:
		if (auto slot = shaderPass.GetRootParameterIndex("normalTexture"))
			commandList->SetGraphicsRootDescriptorTable(slot.value(), gpuHandle);
		break;
	case TEXTURE_EMISSIVE:
		if (auto slot = shaderPass.GetRootParameterIndex("emissiveTexture"))
			commandList->SetGraphicsRootDescriptorTable(slot.value(), gpuHandle);
		break;
	case TEXTURE_OCCLUSION:
		if (auto slot = shaderPass.GetRootParameterIndex("occlusionTexture"))
			commandList->SetGraphicsRootDescriptorTable(slot.value(), gpuHandle);
		break;
	default:
		break;
	}
}