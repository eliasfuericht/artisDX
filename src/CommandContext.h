#pragma once

#include "pch.h"

#include "D3D12Core.h"
#include "CommandQueue.h"
#include <unordered_map>

class CommandContext
{
public:
	CommandContext() = default;
	~CommandContext() = default;
	CommandContext(const CommandContext&) = delete;
	CommandContext& operator=(const CommandContext&) = delete;

	void InitializeCommandContext(QUEUETYPE queueType);

	void Reset();

	void SetPipelineState(MSWRL::ComPtr<ID3D12PipelineState> pipelineState);
	void SetGraphicsRootSignature(MSWRL::ComPtr<ID3D12RootSignature> rootSignature);
	// Pin every owner/object whose commands outlive its CPU reference.
	void KeepAlive(MSWRL::ComPtr<IUnknown> object);
	void KeepAlive(std::shared_ptr<void> owner);
	// Whole-resource states for the existing sequential graphics pass cycle.
	void DeclareResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES initialState,
		D3D12_RESOURCE_STATES finalState, const std::string& name);
	bool UseResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state);

	MSWRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return _commandList.Get(); }

	CommandCompletion Finish(bool waitForExecution);
	const CommandCompletion& GetLastSubmission() const { return _lastSubmission; }

private:
	QUEUETYPE _queueType = QUEUE_INVALID;

	MSWRL::ComPtr<ID3D12CommandAllocator> _allocator;
	MSWRL::ComPtr<ID3D12GraphicsCommandList> _commandList;
	CommandCompletion _lastSubmission;
	bool _recording = false;
	bool _submissionFailed = false;
	SubmissionResources _recordedResources;
	struct ResourceUse
	{
		D3D12_RESOURCE_STATES state;
		D3D12_RESOURCE_STATES finalState;
		std::string name;
	};
	std::unordered_map<ID3D12Resource*, ResourceUse> _resourceUses;
};
