#pragma once

#include "pch.h"
#include "D3D12Core.h"

enum QUEUETYPE : int32_t
{
	QUEUE_INVALID = NOTOK,
	QUEUE_GRAPHICS = 0,
	QUEUE_COMPUTE = 1,
	QUEUE_UPLOAD = 2
};

class CommandCompletion
{
	friend class CommandQueue;
public:
	CommandCompletion() = default;
	ID3D12Fence* GetFence() const { return fence.Get(); }
	uint64_t GetValue() const { return value; }
private:
	CommandCompletion(MSWRL::ComPtr<ID3D12Fence> submittedFence, uint64_t submittedValue)
		: fence(std::move(submittedFence)), value(submittedValue) {}
	MSWRL::ComPtr<ID3D12Fence> fence;
	uint64_t value = 0;
};

struct SubmissionResources
{
	std::vector<MSWRL::ComPtr<IUnknown>> objects;
	std::vector<std::shared_ptr<void>> owners;
};

class CommandQueue
{
public:
	CommandQueue() = default;
	~CommandQueue();
	CommandQueue(const CommandQueue&) = delete;
	CommandQueue& operator=(const CommandQueue&) = delete;
	void InitializeCommandQueue(D3D12_COMMAND_LIST_TYPE queuetype);

	CommandCompletion Submit(ID3D12CommandList* list, SubmissionResources resources = {});
	bool IsComplete(const CommandCompletion& completion) const;
	void WaitCPU(const CommandCompletion& completion);
	void WaitGPU(const CommandCompletion& producer);
	void Flush();
	void CollectCompleted();
	size_t GetPendingSubmissionCount() const { return _pendingSubmissions.size(); }
	// Compatibility: this signals and flushes, rather than waiting on an old submission.
	void WaitForFence() { Flush(); }

	MSWRL::ComPtr<ID3D12CommandQueue> _commandQueue;
	MSWRL::ComPtr<ID3D12Fence> _fence;

	HANDLE _fenceEvent = nullptr;
	uint64_t _fenceValue = 0;

private:
	struct PendingSubmission
	{
		CommandCompletion completion;
		SubmissionResources resources;
	};
	std::vector<PendingSubmission> _pendingSubmissions;
	bool _submissionFailed = false;
	CommandCompletion Signal();
	void ValidateCompletion(const CommandCompletion& completion) const;
};

namespace CommandQueueManager
{
	extern void InitializeCommandQueueManager();
	
	extern CommandQueue& GetCommandQueue(QUEUETYPE queueType);
	
	extern CommandQueue commandQueues[3];
}
