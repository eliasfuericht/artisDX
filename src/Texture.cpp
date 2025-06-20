#include "Texture.h"

Texture::Texture(MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, DirectX::ScratchImage& texture)
{
	_image = std::move(texture);
	const DirectX::TexMetadata& metadata = _image.GetMetadata();
	DirectX::GenerateMipMaps(*_image.GetImages(), DirectX::TEX_FILTER_BOX, 0, _mipMaps);

	CreateBuffers(device);
	UploadBuffers(commandList);
}

void Texture::CreateGPUHandle(MSWRL::ComPtr<ID3D12Device> device)
{
	// 1. Create SRV Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = 1;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_srvHeap)));

	// 2. Get descriptor handle
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(_srvHeap->GetCPUDescriptorHandleForHeapStart());

	// 3. Describe the SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = _textureResource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = _mipCount;  // or 1 if no mipmaps
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	// 4. Create the SRV
	device->CreateShaderResourceView(_textureResource.Get(), &srvDesc, cpuHandle);

	// 5. Store GPU handle if needed (for shader binding)
	_srvHandle = _srvHeap->GetGPUDescriptorHandleForHeapStart();
}

void Texture::BindTexture(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	ID3D12DescriptorHeap* heaps[] = { _srvHeap.Get() };
	commandList->SetDescriptorHeaps(1, heaps);

	// Then bind to root signature (depends on your root param setup)
	commandList->SetGraphicsRootDescriptorTable(2, _srvHandle);
}

void Texture::CreateBuffers(MSWRL::ComPtr<ID3D12Device> device)
{
	const auto& metadata = _mipMaps.GetMetadata();

	// Texture (default heap)
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	textureDesc.Width = static_cast<UINT>(metadata.width);
	textureDesc.Height = static_cast<UINT>(metadata.height);
	textureDesc.DepthOrArraySize = static_cast<UINT16>(std::max<size_t>(1, metadata.arraySize));
	textureDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
	textureDesc.Format = metadata.format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES defaultHeap = { D3D12_HEAP_TYPE_DEFAULT };

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeap,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&_textureResource)));

	// Upload heap
	UINT64 uploadBufferSize = 0;
	device->GetCopyableFootprints(&textureDesc, 0, metadata.mipLevels, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

	D3D12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	D3D12_HEAP_PROPERTIES uploadHeap = { D3D12_HEAP_TYPE_UPLOAD };

	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeap,
		D3D12_HEAP_FLAG_NONE,
		&uploadDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_uploadHeap)));
}

void Texture::UploadBuffers(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	std::vector<D3D12_SUBRESOURCE_DATA> subresourceData;
	_mipCount = _mipMaps.GetImageCount();
	subresourceData.reserve(_mipCount);

	for (size_t i = 0; i < _mipCount; ++i) {
		const DirectX::Image* img = _mipMaps.GetImage(i, 0, 0);
		D3D12_SUBRESOURCE_DATA subresource = {};
		subresource.pData = img->pixels;
		subresource.RowPitch = static_cast<LONG_PTR>(img->rowPitch);
		subresource.SlicePitch = static_cast<LONG_PTR>(img->slicePitch);
		subresourceData.push_back(subresource);
	}

	UpdateSubresources(commandList.Get(), _textureResource.Get(), _uploadHeap.Get(), 0, 0, (UINT)subresourceData.size(), subresourceData.data());

	// Transition to PIXEL_SHADER_RESOURCE
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_textureResource.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	commandList->ResourceBarrier(1, &barrier);
}