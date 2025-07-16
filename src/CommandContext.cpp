#include "CommandContext.h"

void CommandContext::InitializeCommandContext(QUEUETYPE queueType)
{
	_queueType = queueType;
	std::wstring allocatorName;
	std::wstring listName;

	D3D12_COMMAND_LIST_TYPE listType{};
	switch (_queueType)
	{
		case QUEUETYPE::GRAPHICS:
			listType = D3D12_COMMAND_LIST_TYPE_DIRECT;
			allocatorName = L"GraphicsCommandAllocator";
			listName = L"GraphicsCommandList";
			break;
		case QUEUETYPE::COMPUTE:
			listType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			allocatorName = L"ComputeCommandAllocator";
			listName = L"ComputeCommandList";
			break;
		case QUEUETYPE::UPLOAD:
			listType = D3D12_COMMAND_LIST_TYPE_COPY;
			allocatorName = L"UploadCommandAllocator";
			listName = L"UploadCommandList";
			break;
		default:
			break;
	}

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommandAllocator(listType, IID_PPV_ARGS(&_allocator)), "Failed to create CommandAllocator!");
	_allocator->SetName(allocatorName.c_str());
	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommandList(0, listType, _allocator.Get(), nullptr, IID_PPV_ARGS(&_commandList)), "Failed to create CommandList!");
	_commandList->SetName(listName.c_str());
}

void CommandContext::SetPipelineState(MSWRL::ComPtr<ID3D12PipelineState> pipelineState)
{
	_commandList->SetPipelineState(pipelineState.Get());
}

void CommandContext::Reset()
{
	ThrowIfFailed(_allocator->Reset(), "Failed to reset Allocator!");
	ThrowIfFailed(_commandList->Reset(_allocator.Get(), nullptr), "Failed to reset CommandList!");
}

void CommandContext::Finish(BOOL waitForExecution)
{
	_commandList->Close();

	ID3D12CommandList* cmdLists[] = { _commandList.Get() };
	CommandQueueManager::GetCommandQueue(_queueType)._commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	if (waitForExecution)
		CommandQueueManager::GetCommandQueue(_queueType).WaitForFence();
}