#include "Texture.h"

Texture::Texture(MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, ScratchImage& texture)
{
	_image = std::move(texture);

	CreateBuffers(device, commandList);
}

void Texture::CreateBuffers(MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&_srvHeap)));

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

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeap,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&_textureResource)));

	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(_textureResource.Get(), 0, 1);

	D3D12_HEAP_PROPERTIES defaultUploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	// Create the GPU upload buffer.
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultUploadHeap,
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_textureUploadHeap)));

	const Image* img = _image.GetImage(0, 0, 0);
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = img->pixels;
	textureData.RowPitch = img->rowPitch;
	textureData.SlicePitch = img->slicePitch;

	D3D12_RESOURCE_BARRIER transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_textureResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	UpdateSubresources(commandList.Get(), _textureResource.Get(), _textureUploadHeap.Get(), 0, 0, 1, &textureData);
	commandList->ResourceBarrier(1, &transitionBarrier);

	// Describe and create a SRV for the texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(_textureResource.Get(), &srvDesc, _srvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Texture::CreateGPUHandle(MSWRL::ComPtr<ID3D12Device> device)
{
	_srvHandle = _srvHeap->GetGPUDescriptorHandleForHeapStart();
}

void Texture::BindTexture(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	ID3D12DescriptorHeap* heaps[] = { _srvHeap.Get() };
	commandList->SetDescriptorHeaps(1, heaps);

	// Then bind to root signature (depends on your root param setup)
	commandList->SetGraphicsRootDescriptorTable(2, _srvHandle);
}