#include "CommandContext.h"

void CommandContext::InitializeCommandContext(QUEUETYPE queueType)
{
	if (_commandList) ThrowException("Command context already initialized");
	_queueType = queueType;
	std::wstring allocatorName;
	std::wstring listName;

	D3D12_COMMAND_LIST_TYPE listType{};
	switch (_queueType)
	{
		case QUEUETYPE::QUEUE_GRAPHICS:
			listType = D3D12_COMMAND_LIST_TYPE_DIRECT;
			allocatorName = L"GraphicsCommandAllocator";
			listName = L"GraphicsCommandList";
			break;
		case QUEUETYPE::QUEUE_COMPUTE:
			listType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			allocatorName = L"ComputeCommandAllocator";
			listName = L"ComputeCommandList";
			break;
		case QUEUETYPE::QUEUE_UPLOAD:
			listType = D3D12_COMMAND_LIST_TYPE_COPY;
			allocatorName = L"UploadCommandAllocator";
			listName = L"UploadCommandList";
			break;
		default:
			ThrowException("Invalid command context queue type: " + std::to_string(queueType));
	}

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommandAllocator(listType, IID_PPV_ARGS(&_allocator)), "Failed to create CommandAllocator!");
	_allocator->SetName(allocatorName.c_str());
	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommandList(0, listType, _allocator.Get(), nullptr, IID_PPV_ARGS(&_commandList)), "Failed to create CommandList!");
	_commandList->SetName(listName.c_str());
	_recording = true;
}

void CommandContext::SetPipelineState(MSWRL::ComPtr<ID3D12PipelineState> pipelineState)
{
	KeepAlive(pipelineState);
	_commandList->SetPipelineState(pipelineState.Get());
}

void CommandContext::SetGraphicsRootSignature(MSWRL::ComPtr<ID3D12RootSignature> rootSignature)
{
	KeepAlive(rootSignature);
	_commandList->SetGraphicsRootSignature(rootSignature.Get());
}

void CommandContext::KeepAlive(MSWRL::ComPtr<IUnknown> object)
{
	if (!_recording || !object) ThrowException("KeepAlive requires a recording context and a valid object");
	const auto duplicate = std::find_if(_recordedResources.objects.begin(), _recordedResources.objects.end(),
		[&](const auto& existing) { return existing.Get() == object.Get(); });
	if (duplicate == _recordedResources.objects.end()) _recordedResources.objects.push_back(std::move(object));
}

void CommandContext::KeepAlive(std::shared_ptr<void> owner)
{
	if (!_recording || !owner) ThrowException("KeepAlive requires a recording context and a valid owner");
	if (std::find(_recordedResources.owners.begin(), _recordedResources.owners.end(), owner) == _recordedResources.owners.end())
		_recordedResources.owners.push_back(std::move(owner));
}

void CommandContext::DeclareResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES initialState,
	D3D12_RESOURCE_STATES finalState, const std::string& name)
{
	if (!_recording || _queueType != QUEUE_GRAPHICS || !resource)
		ThrowException("Resource declaration requires a recording graphics context and a valid resource");
	if (_resourceUses.contains(resource)) ThrowException("Resource already declared: " + name);
	KeepAlive(MSWRL::ComPtr<IUnknown>(resource));
	_resourceUses.emplace(resource, ResourceUse{initialState, finalState, name});
}

bool CommandContext::UseResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state)
{
	if (!_recording) ThrowException("Resource use requires a recording context");
	auto use = _resourceUses.find(resource);
	if (use == _resourceUses.end()) ThrowException("Resource use was not declared for this command list");
	if (use->second.state == state) return false;
	const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, use->second.state, state);
	_commandList->ResourceBarrier(1, &barrier);
	use->second.state = state;
	return true;
}

void CommandContext::Reset()
{
	if (_submissionFailed) ThrowException("Cannot reset a context after a failed submission");
	if (_recording) ThrowException("Cannot reset a recording command context; finish it first");
	CommandQueueManager::GetCommandQueue(_queueType).WaitCPU(_lastSubmission);
	ThrowIfFailed(_allocator->Reset(), "Failed to reset Allocator!");
	ThrowIfFailed(_commandList->Reset(_allocator.Get(), nullptr), "Failed to reset CommandList!");
	_resourceUses.clear();
	_recording = true;
}

CommandCompletion CommandContext::Finish(bool waitForExecution)
{
	for (const auto& [resource, use] : _resourceUses)
	{
		(void)resource;
		if (use.state != use.finalState)
			ThrowException("Resource did not reach declared final state: " + use.name);
	}
	KeepAlive(_allocator);
	KeepAlive(_commandList);
	ThrowIfFailed(_commandList->Close(), "Failed to Close command list for queue " + std::to_string(_queueType));
	_recording = false;
	auto& queue = CommandQueueManager::GetCommandQueue(_queueType);
	// Signal failure after Execute must never make this context reusable.
	_submissionFailed = true;
	_lastSubmission = queue.Submit(_commandList.Get(), std::move(_recordedResources));
	_submissionFailed = false;
	if (waitForExecution) queue.WaitCPU(_lastSubmission);
	return _lastSubmission;
}
