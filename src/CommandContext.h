#pragma once

#include "pch.h"

#include "D3D12Core.h"
#include "CommandQueue.h"

class CommandContext
{
public:
	CommandContext() {};

	void InitializeCommandContext(QUEUETYPE queueType);

	void Reset();

	void SetPipelineState(MSWRL::ComPtr<ID3D12PipelineState> pipelineState);

	MSWRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return _commandList.Get(); }

	void Finish(bool waitForExecution = false);

private:
	QUEUETYPE _queueType;

	MSWRL::ComPtr<ID3D12CommandAllocator> _allocator;
	MSWRL::ComPtr<ID3D12GraphicsCommandList> _commandList;
};