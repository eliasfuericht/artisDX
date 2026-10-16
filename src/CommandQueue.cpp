#include "CommandQueue.h"

void CommandQueue::InitializeCommandQueue(D3D12_COMMAND_LIST_TYPE queuetype)
{
	if (_commandQueue)
	{
		ThrowException("Command queue already initialized");
	}
	
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = queuetype;

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue)), "CommandQueue creation failed!");

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));

	_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	if (_fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

CommandQueue::~CommandQueue()
{
	try
	{
		if (_commandQueue && _fence && _fenceEvent) Flush();
	}
	catch (const std::exception& error) 
	{ 
		PRINT("Queue teardown: ", error.what()); 
	}

	if (_fenceEvent)
	{
		CloseHandle(_fenceEvent);
	}
}

CommandCompletion CommandQueue::Signal()
{
	const std::string context = "Queue type " + std::to_string(_commandQueue->GetDesc().Type);

	const uint64_t value = _fenceValue + 1;
	ThrowIfFailed(_commandQueue->Signal(_fence.Get(), value), context + ": Signal failed");
	_fenceValue = value;

	return {_fence, value};
}

CommandCompletion CommandQueue::Submit(ID3D12CommandList* list, SubmissionResources resources)
{
	if (!list)
	{
		ThrowException("Cannot submit a null command list");
	}
	
	if (list->GetType() != _commandQueue->GetDesc().Type)
	{
		ThrowException("Command list type does not match its submission queue");
	}
		
	if (_submissionFailed)
	{
		ThrowException("Queue needs a successful flush after a failed submission");
	}

	if (_fenceValue >= UINT64_MAX - 1)
	{
		ThrowException("Fence values exhausted before submission");
	}

	CollectCompleted();
	
	_pendingSubmissions.push_back({{}, std::move(resources)});
	_commandQueue->ExecuteCommandLists(1, &list);
	try
	{
		const CommandCompletion completion = Signal();
		_pendingSubmissions.back().completion = completion;
		return completion;
	}
	catch (...) 
	{ 
		_submissionFailed = true;
		throw;
	}
}

void CommandQueue::ValidateCompletion(const CommandCompletion& completion) const
{
	if (!completion.fence && completion.value == 0) return;
	if (completion.fence.Get() != _fence.Get() || completion.value == 0 || completion.value > _fenceValue)
		ThrowException("Completion does not belong to this queue or was never signaled");
}

bool CommandQueue::IsComplete(const CommandCompletion& completion) const
{
	ValidateCompletion(completion);
	if (!completion.fence) return true;
	return ReadCompletedValue() >= completion.value;
}

uint64_t CommandQueue::ReadCompletedValue() const
{
	const uint64_t value = _fence->GetCompletedValue();
	if (value == UINT64_MAX)
	{
		MSWRL::ComPtr<ID3D12Device> device;
		ThrowIfFailed(_fence->GetDevice(IID_PPV_ARGS(&device)));
		ThrowIfFailed(device->GetDeviceRemovedReason(), "Device removed during fence query");
		ThrowException("Fence reported device removal");
	}
	return value;
}

void CommandQueue::WaitCPU(const CommandCompletion& completion)
{
	ValidateCompletion(completion);
	if (!completion.fence) { CollectCompleted(); return; }
	uint64_t completedValue = ReadCompletedValue();
	if (completedValue < completion.value)
	{
		ThrowIfFailed(_fence->SetEventOnCompletion(completion.value, _fenceEvent), "SetEventOnCompletion failed");
		const DWORD waitResult = WaitForSingleObjectEx(_fenceEvent, INFINITE, false);
		if (waitResult == WAIT_FAILED)
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), "Fence event wait failed");
		if (waitResult != WAIT_OBJECT_0) ThrowException("Unexpected fence event wait result " + std::to_string(waitResult));
		completedValue = ReadCompletedValue();
		if (completedValue < completion.value) ThrowException("Fence event signaled before completion");
	}
	CollectCompleted(completedValue);
}

void CommandQueue::WaitGPU(const CommandCompletion& producer)
{
	if (!producer.fence || producer.value == 0) ThrowException("GPU wait requires a submitted producer completion");
	if (producer.fence->GetCompletedValue() == UINT64_MAX) ThrowException("Producer fence reported device removal");
	ThrowIfFailed(_commandQueue->Wait(producer.fence.Get(), producer.value), "GPU queue dependency failed");
}

void CommandQueue::Flush()
{
	if (_fenceValue >= UINT64_MAX - 1)
		ThrowException("Fence values exhausted before flush");
	const auto completion = Signal();
	for (auto& pending : _pendingSubmissions)
		if (!pending.completion.fence) pending.completion = completion;
	_submissionFailed = false;
	WaitCPU(completion);
}

void CommandQueue::CollectCompleted()
{
	if (_pendingSubmissions.empty()) return;
	CollectCompleted(ReadCompletedValue());
}

void CommandQueue::CollectCompleted(uint64_t completedValue)
{
	std::erase_if(_pendingSubmissions, [completedValue](const PendingSubmission& pending)
	{
		return pending.completion.fence && pending.completion.value <= completedValue;
	});
}

namespace CommandQueueManager
{
	CommandQueue commandQueues[3];

	void InitializeCommandQueueManager()
	{
		CommandQueueManager::commandQueues[QUEUETYPE::QUEUE_GRAPHICS].InitializeCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
		CommandQueueManager::commandQueues[QUEUETYPE::QUEUE_COMPUTE].InitializeCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
		CommandQueueManager::commandQueues[QUEUETYPE::QUEUE_UPLOAD].InitializeCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	}

	CommandQueue& GetCommandQueue(QUEUETYPE queueType)
	{
		if (queueType < QUEUE_GRAPHICS || queueType > QUEUE_UPLOAD)
			ThrowException("Invalid command queue type: " + std::to_string(queueType));
		return CommandQueueManager::commandQueues[queueType];
	}
}
