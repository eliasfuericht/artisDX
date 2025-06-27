#include "Texture.h"

Texture::Texture(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, Texture::TEXTURETYPE texType, ScratchImage& texture)
{
	ScratchImage mipChain;
	ThrowIfFailed(GenerateMipMaps(
		texture.GetImages(),
		texture.GetImageCount(),
		texture.GetMetadata(),
		TEX_FILTER_DEFAULT,
		0,
		mipChain
	));

	_image = std::move(mipChain);
	_textureType = texType;

	CreateBuffers(commandList);
}

void Texture::CreateBuffers(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	const TexMetadata& metadata = _image.GetMetadata();
	_mipCount = metadata.mipLevels;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	textureDesc.Width = metadata.width;
	textureDesc.Height = metadata.height;
	textureDesc.DepthOrArraySize = metadata.arraySize;
	textureDesc.MipLevels = _mipCount;
	textureDesc.Format = metadata.format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	ThrowIfFailed(D3D12Core::GetDevice()->CreateCommittedResource(
		&defaultHeap,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&_textureResource)));

	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(_textureResource.Get(), 0, _mipCount);

	D3D12_HEAP_PROPERTIES defaultUploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	// Create the GPU upload buffer.
	ThrowIfFailed(D3D12Core::GetDevice()->CreateCommittedResource(
		&defaultUploadHeap,
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_textureUploadHeap)));

	std::vector<D3D12_SUBRESOURCE_DATA> subresources(_mipCount);

	for (UINT i = 0; i < _mipCount; ++i) {
		const Image* img = _image.GetImage(i, 0, 0);
		subresources[i].pData = img->pixels;
		subresources[i].RowPitch = img->rowPitch;
		subresources[i].SlicePitch = img->slicePitch;
	}

	UpdateSubresources(commandList.Get(), _textureResource.Get(), _textureUploadHeap.Get(), 0, 0, _mipCount, subresources.data());
	
	D3D12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_textureResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->ResourceBarrier(1, &transitionBarrier);

	_srvCpuHandle = DescriptorAllocator::Instance().Allocate();

	// Describe and create a SRV for the texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = _mipCount;
	srvDesc.Texture2D.MostDetailedMip = 0;
	D3D12Core::GetDevice()->CreateShaderResourceView(_textureResource.Get(), &srvDesc, _srvCpuHandle);
}

void Texture::BindTexture(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	ID3D12DescriptorHeap* heaps[] = { DescriptorAllocator::Instance().GetHeap() };
	commandList->SetDescriptorHeaps(1, heaps);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = DescriptorAllocator::Instance().GetGPUHandle(_srvCpuHandle);
	commandList->SetGraphicsRootDescriptorTable(_textureType + 2, gpuHandle);
}