#include "TestCases.h"
#include "TestSupport.h"
#include "Renderer.h"
#include "FrameCoordinator.h"
#include "Model.h"
#include "GLTFLoader.h"

#include <array>
#include <cstring>
#include <future>
#include <type_traits>

namespace Testing
{
namespace
{
    constexpr uint32_t ViewportSize = 96;

    struct GpuUnavailable : std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    struct ComApartment
    {
        ComApartment() { ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED)); }
        ~ComApartment() { CoUninitialize(); }
    };

    MSWRL::ComPtr<ID3D12InfoQueue> infoQueue;

    void InitializeScene(Renderer& renderer, const std::filesystem::path& scene)
    {
        renderer.InitializeResources();
        renderer._modelManager.ClearModels();
        renderer._modelManager.LoadModel(scene);
    }

    struct SceneFrames
    {
        FrameCoordinator coordinator;
        SceneFrames() { coordinator.Initialize(2); }
        void Render(Renderer& renderer, float dt)
        {
            std::shared_ptr<CommandContext> graphics = coordinator.BeginFrame(0);
            renderer.PrepareFrame(dt);
            renderer.RecordScene(graphics);
            coordinator.SubmitFrame(0);
            coordinator.Drain();
        }
        void Shutdown() { coordinator.Shutdown(); }
    };


    struct QueueGate
    {
        MSWRL::ComPtr<ID3D12Fence> fence;
        explicit QueueGate(CommandQueue& queue)
        {
            ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
            ThrowIfFailed(queue._commandQueue->Wait(fence.Get(), 1));
        }
        // Release even on assertion failure so queue teardown cannot hang.
        ~QueueGate() { fence->Signal(1); }
        QueueGate(const QueueGate&) = delete;
        QueueGate& operator=(const QueueGate&) = delete;
        void Release() { ThrowIfFailed(fence->Signal(1)); }
    };

    void RequirePending(const CommandCompletion& completion, std::string_view message)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
        do
        {
            Require(completion.GetFence()->GetCompletedValue() < completion.GetValue(), message);
            SwitchToThread();
        } while (std::chrono::steady_clock::now() < deadline);
    }

    void InitializeGpu(bool hardware, bool validateShaders)
    {
        MSWRL::ComPtr<ID3D12Debug> debug;
        if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
            throw GpuUnavailable("D3D12 debug layer unavailable. Install Windows Graphics Tools and rerun the GPU tests.");
        debug->EnableDebugLayer();
        if (validateShaders)
        {
            MSWRL::ComPtr<ID3D12Debug1> validation;
            ThrowIfFailed(debug.As(&validation));
            validation->SetEnableGPUBasedValidation(true);
        }
        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&D3D12Core::GraphicsDevice::factory)));
        if (hardware)
        {
            D3D12Core::GraphicsDevice::InitializeDevice();
            DXGI_ADAPTER_DESC1 selected{};
            ThrowIfFailed(D3D12Core::GraphicsDevice::adapter->GetDesc1(&selected));
            const LUID actual = D3D12Core::GraphicsDevice::device->GetAdapterLuid();
            Require(actual.HighPart == selected.AdapterLuid.HighPart && actual.LowPart == selected.AdapterLuid.LowPart,
                "Hardware device must use the selected adapter");
            std::cout << "Hardware adapter vendor=" << selected.VendorId << " device=" << selected.DeviceId
                << " dedicated bytes=" << selected.DedicatedVideoMemory << " (device LUID matches selection)\n";
        }
        else
        {
            MSWRL::ComPtr<IDXGIAdapter> warp;
            if (FAILED(D3D12Core::GraphicsDevice::factory->EnumWarpAdapter(IID_PPV_ARGS(&warp))) ||
                FAILED(D3D12CreateDevice(warp.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&D3D12Core::GraphicsDevice::device))))
                throw GpuUnavailable("A D3D12 WARP device is unavailable; GPU coverage was skipped.");
        }
        ThrowIfFailed(D3D12Core::GraphicsDevice::device.As(&infoQueue));
        // Keep errors in the queue so CTest can report them instead of stopping in a debugger.
        ThrowIfFailed(infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false));
        ThrowIfFailed(infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false));
        GUI::viewportWidth = GUI::viewportHeight = ViewportSize;
    }

    void CheckGpuDiagnostics(std::optional<D3D12_MESSAGE_ID> expectedError = std::nullopt,
        std::optional<D3D12_MESSAGE_ID> expectedWarning = std::nullopt, uint32_t expectedWarnings = 1)
    {
        ThrowIfFailed(D3D12Core::GraphicsDevice::device->GetDeviceRemovedReason(), "GPU device was removed");
        bool errors = false;
        uint32_t expectedCount = 0;
        uint32_t warningCount = 0;
        for (uint64_t i = 0; i < infoQueue->GetNumStoredMessages(); ++i)
        {
            SIZE_T size = 0;
            ThrowIfFailed(infoQueue->GetMessage(i, nullptr, &size));
            std::vector<uint8_t> storage(size);
            auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
            ThrowIfFailed(infoQueue->GetMessage(i, message, &size));
            const bool errorExpected = expectedError && message->ID == *expectedError && message->Severity == D3D12_MESSAGE_SEVERITY_ERROR;
            const bool warningExpected = expectedWarning && message->ID == *expectedWarning && message->Severity == D3D12_MESSAGE_SEVERITY_WARNING;
            const bool expected = errorExpected || warningExpected;
            if (message->Severity <= D3D12_MESSAGE_SEVERITY_WARNING)
                std::cerr << (expected ? "Expected D3D12 [" : "D3D12 [") << message->ID << "]: " << message->pDescription << '\n';
            if (errorExpected)
                ++expectedCount;
            else if (warningExpected)
                ++warningCount;
            else
                errors |= message->Severity <= D3D12_MESSAGE_SEVERITY_WARNING;
        }
        Require(!errors, "D3D12 debug layer reported unexpected warnings/errors; see the messages above");
        if (expectedError)
            Require(expectedCount == 1, "Negative case must produce exactly its one expected D3D12 error");
        if (expectedWarning)
            Require(warningCount == expectedWarnings, "GPU diagnostics must contain exactly the expected warning count");
        infoQueue->ClearStoredMessages();
    }

    void TestCommandFailures()
    {
        CommandQueueManager::InitializeCommandQueueManager();
        for (const QUEUETYPE type : {QUEUE_INVALID, static_cast<QUEUETYPE>(3)})
        {
            ExpectFailure([&] { CommandQueueManager::GetCommandQueue(type); }, "Invalid command queue type");
            CommandContext invalid;
            ExpectFailure([&] { invalid.InitializeCommandContext(type); }, "Invalid command context queue type");
            Require(!invalid.GetCommandList(), "Invalid queue type must not create a command list");
        }

        CommandContext closed;
        closed.InitializeCommandContext(QUEUE_GRAPHICS);
        ThrowIfFailed(closed.GetCommandList()->Close());
        const uint64_t before = CommandQueueManager::GetCommandQueue(QUEUE_GRAPHICS)._fenceValue;
        ExpectFailure([&] { closed.Finish(true); }, "Close command list");
        Require(CommandQueueManager::GetCommandQueue(QUEUE_GRAPHICS)._fenceValue == before,
            "Failed Close must stop before submission and its completion signal");

        auto& queue = CommandQueueManager::GetCommandQueue(QUEUE_GRAPHICS);
        {
            struct RestoreFenceValue
            {
                CommandQueue& queue;
                uint64_t value;
                ~RestoreFenceValue() { queue._fenceValue = value; }
            } restore{queue, queue._fenceValue};
            const auto pending = queue.GetPendingSubmissionCount();
            queue._fenceValue = UINT64_MAX - 1;
            // Both public signal paths must reject exhaustion before executing work
            // or changing retained submissions; Signal() itself assumes this check.
            ExpectFailure([&] { queue.Submit(closed.GetCommandList().Get()); }, "Fence values exhausted before submission");
            ExpectFailure([&] { queue.Flush(); }, "Fence values exhausted before flush");
            Require(queue.GetPendingSubmissionCount() == pending && queue._fenceValue == UINT64_MAX - 1,
                "Exhausted fence must leave submission ownership and signal value unchanged");
        }
    }

    void TestRootSignatureFailures()
    {
        ShaderPass missing("missing-reflection-regression");
        missing._shaders.emplace(SHADERTYPE::SHADER_VERTEX, Shader{});
        ExpectFailure([&] { missing.GenerateGraphicsRootSignature(); }, "no compilation result for reflection");
        Require(!missing._rootSignature, "Missing reflection must not create a root signature");

        // A real serializer error, independently of the pending reflection layout repairs.
        D3D12_DESCRIPTOR_RANGE1 range{};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = 0; // Invalid: a range needs at least one descriptor.
        D3D12_ROOT_PARAMETER1 parameter{};
        parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameter.DescriptorTable = {1, &range};
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC description{};
        description.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        description.Desc_1_1.NumParameters = 1;
        description.Desc_1_1.pParameters = &parameter;
        ShaderPass invalid("invalid-root-regression");
        try
        {
            invalid.CreateRootSignature(description);
            Require(false, "Zero-descriptor range must fail serialization");
        }
        catch (const std::runtime_error& error)
        {
            const std::string_view diagnostic(error.what());
            Require(diagnostic.find("invalid-root-regression': root signature serialization failed") != std::string_view::npos &&
                diagnostic.find("NumDescriptors cannot be 0") != std::string_view::npos,
                "Serialization failure must include pass name and the descriptor diagnostic: " + std::string(diagnostic));
            std::cout << "Expected serialization failure: " << error.what() << '\n';
        }
        Require(!invalid._rootSignature, "Failed serialization must stop before root signature creation");
    }

    void TestRootLayout(const std::filesystem::path& fixtures)
    {
        ShaderPass pass("layout-regression");
        pass.AddShader(fixtures / "layout_array.hlsl", SHADER_PIXEL);
        const auto blob = pass.GenerateGraphicsRootSignature();
        MSWRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> deserializer;
        ThrowIfFailed(D3D12CreateVersionedRootSignatureDeserializer(blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&deserializer)));
        const auto& desc = deserializer->GetUnconvertedRootSignatureDesc()->Desc_1_1;
        Require(desc.NumParameters == 1 && desc.pParameters[0].DescriptorTable.NumDescriptorRanges == 1,
            "Texture array must produce one table containing one range record");
        const auto& range = desc.pParameters[0].DescriptorTable.pDescriptorRanges[0];
        Require(range.NumDescriptors == 2 && range.BaseShaderRegister == 3 && range.RegisterSpace == 2,
            "Array range must retain reflected descriptor count, register and space");
        Require(pass.GetRootParameterIndex("arrayTextures") == 0, "Array name must address the actual table");
        const auto originalBindings = pass._bindingMap;
        const auto repeated = pass.GenerateGraphicsRootSignature();
        Require(pass._bindingMap == originalBindings && repeated->GetBufferSize() == blob->GetBufferSize() &&
            std::memcmp(repeated->GetBufferPointer(), blob->GetBufferPointer(), blob->GetBufferSize()) == 0,
            "Regeneration must produce identical metadata and serialized layout");

        for (const auto& fixture : {"layout_structured.hlsl", "layout_unbounded.hlsl"})
        {
            pass._shaders.at(SHADER_PIXEL) = Shader(fixtures / fixture, SHADER_PIXEL);
            auto* previous = pass._rootSignature.Get();
            ExpectFailure([&] { pass.GenerateGraphicsRootSignature(); }, fixture == std::string_view("layout_structured.hlsl") ? "unsupported binding kind" : "unbounded descriptor ranges");
            Require(pass._rootSignature.Get() == previous && pass._bindingMap == originalBindings,
                "Rejected layout must preserve the previous complete signature and bindings");
        }
        pass._shaders.clear();
        pass._shaders.emplace(SHADER_COMPUTE, Shader{});
        ExpectFailure([&] { pass.GenerateGraphicsRootSignature(); }, "unsupported graphics stage");
        Require(pass._bindingMap == originalBindings, "Unsupported stage must not publish slots");
        pass._shaders.clear();
        pass.AddShader(fixtures / "layout_shared_frag.hlsl", SHADER_PIXEL);
        pass.GenerateGraphicsRootSignature();
        Require(!pass.GetRootParameterIndex("arrayTextures") && pass.GetRootParameterIndex("sharedBuffer") == 0,
            "Changed layout must discard stale names");
        pass.AddShader(fixtures / "layout_shared_vert.hlsl", SHADER_VERTEX);
        auto* previous = pass._rootSignature.Get();
        const auto validBindings = pass._bindingMap;
        ExpectFailure([&] { pass.GenerateGraphicsRootSignature(); }, "duplicate binding name; first binding:");
        Require(pass._rootSignature.Get() == previous && pass._bindingMap == validBindings,
            "Ambiguous cross-stage names must preserve the last valid layout");

        ShaderPass forward("forward-insertion"), reverse("reverse-insertion");
        forward.AddShader("../shaders/bb_vert.hlsl", SHADER_VERTEX);
        forward.AddShader(fixtures / "layout_array.hlsl", SHADER_PIXEL);
        reverse.AddShader(fixtures / "layout_array.hlsl", SHADER_PIXEL);
        reverse.AddShader("../shaders/bb_vert.hlsl", SHADER_VERTEX);
        const auto first = forward.GenerateGraphicsRootSignature();
        const auto second = reverse.GenerateGraphicsRootSignature();
        Require(forward._bindingMap == reverse._bindingMap && first->GetBufferSize() == second->GetBufferSize() &&
            std::memcmp(first->GetBufferPointer(), second->GetBufferPointer(), first->GetBufferSize()) == 0,
            "Shader insertion order must not change root slots or serialized layout");
        ShaderPass typed("supported-typed-uav");
        typed.AddShader(fixtures / "layout_typed_uav.hlsl", SHADER_PIXEL);
        typed.GenerateGraphicsRootSignature();
        Require(typed.GetRootParameterIndex("outputTexture") == 0, "Existing typed UAV support must remain valid");
    }

    MSWRL::ComPtr<ID3D12Resource> ReadbackBuffer(uint64_t size)
    {
        MSWRL::ComPtr<ID3D12Resource> buffer;
        const CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_READBACK);
        const auto description = CD3DX12_RESOURCE_DESC::Buffer(size);
        ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE,
            &description, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&buffer)));
        return buffer;
    }

    void TestCommandCompletion()
    {
        static_assert(!std::is_copy_constructible_v<CommandQueue>);
        static_assert(!std::is_copy_constructible_v<CommandContext>);
        static_assert(!std::is_constructible_v<CommandCompletion, MSWRL::ComPtr<ID3D12Fence>, uint64_t>);
        CommandQueueManager::InitializeCommandQueueManager();
        auto& graphics = CommandQueueManager::GetCommandQueue(QUEUE_GRAPHICS);
        auto& copy = CommandQueueManager::GetCommandQueue(QUEUE_UPLOAD);
        CommandContext context;
        context.InitializeCommandContext(QUEUE_GRAPHICS);
        ExpectFailure([&] { copy.Submit(context.GetCommandList().Get()); }, "does not match its submission queue");
        ExpectFailure([&] { graphics.WaitGPU({}); }, "requires a submitted producer completion");
        ExpectFailure([&] { context.Reset(); }, "recording command context");
        // Declare the future before its gate: unwind releases GPU work before joining.
        std::promise<void> started;
        auto resetStarted = started.get_future();
        std::future<void> reset;
        QueueGate gate(graphics);
        const auto first = context.Finish(false);
        Require(first.GetFence() == graphics._fence.Get() && first.GetValue() > 0 && !graphics.IsComplete(first),
            "Nonblocking submission must signal its own identifiable, pending completion");
        ExpectFailure([&] { copy.WaitCPU(first); }, "does not belong to this queue");
        const auto signaled = graphics._fenceValue;
        reset = std::async(std::launch::async, [&] { started.set_value(); context.Reset(); });
        Require(resetStarted.wait_for(std::chrono::seconds(2)) == std::future_status::ready, "Reset worker must start");
        const auto blocked = reset.wait_for(std::chrono::milliseconds(50));
        gate.Release();
        reset.get();
        Require(blocked == std::future_status::timeout, "Reset must remain blocked while its allocator is in flight");
        Require(graphics.IsComplete(first) && graphics._fenceValue == signaled,
            "Reset must wait on its own submission without creating a fresh flush signal");
        const auto second = context.Finish(true);
        Require(second.GetValue() == first.GetValue() + 1 && graphics.IsComplete(second), "Repeated submissions need distinct completed values");

        MSWRL::ComPtr<ID3D12Resource> upload, destination;
        const auto buffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32_t));
        const CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD), defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
        auto* device = D3D12Core::GraphicsDevice::device.Get();
        ThrowIfFailed(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &buffer,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload)));
        ThrowIfFailed(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &buffer,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&destination)));
        uint32_t* mapped = nullptr;
        const D3D12_RANGE noRead{0, 0};
        ThrowIfFailed(upload->Map(0, &noRead, reinterpret_cast<void**>(&mapped)));
        *mapped = 0x12345678;
        upload->Unmap(0, nullptr);
        CommandContext producer;
        producer.InitializeCommandContext(QUEUE_UPLOAD);
        QueueGate copyGate(copy);
        producer.GetCommandList()->CopyBufferRegion(destination.Get(), 0, upload.Get(), 0, sizeof(uint32_t));
        const auto copied = producer.Finish(false);
        Require(!copy.IsComplete(copied), "Copy producer must still be held by its gate");
        graphics.WaitGPU(copied);
        auto readback = ReadbackBuffer(sizeof(uint32_t));
        context.Reset();
        context.GetCommandList()->CopyBufferRegion(readback.Get(), 0, destination.Get(), 0, sizeof(uint32_t));
        const auto consumed = context.Finish(false);
        RequirePending(consumed, "Graphics consumption must stay pending until the copy producer is released");
        copyGate.Release();
        graphics.WaitCPU(consumed);
        Require(consumed.GetFence() != copied.GetFence() && copy.IsComplete(copied), "Queue identity and copy-to-graphics dependency must be preserved");
        ThrowIfFailed(readback->Map(0, nullptr, reinterpret_cast<void**>(&mapped)));
        Require(*mapped == 0x12345678, "Consumer must read the producer's completed copy");
        readback->Unmap(0, &noRead);
        producer.Reset();
        producer.Finish(true); // Copy allocator reuse is covered here, not by a second lifecycle loop.
    }

    void TestFrameLifecycle()
    {
        CommandQueueManager::InitializeCommandQueueManager();
        auto& queue = CommandQueueManager::GetCommandQueue(QUEUE_GRAPHICS);
        constexpr UINT index = 0; // This isolated lifecycle test has no swapchain.
        FrameCoordinator frames;
        ExpectFailure([&] { frames.BeginFrame(0); }, "not running");
        frames.Initialize(2);
        Require(queue._fenceValue == 0, "Frame initialization must not submit an empty list");
        ExpectFailure([&] { frames.Initialize(2); }, "already initialized");
        ExpectFailure([&] { frames.SubmitFrame(index); }, "active frame");
        std::shared_ptr<CommandContext> graphics = frames.BeginFrame(0);
        ExpectFailure([&] { frames.BeginFrame(0); }, "overlapping frame");
        auto owner = std::make_shared<int>(42);
        const std::weak_ptr<int> retained = owner;
        graphics->KeepAlive(owner);
        owner.reset();
        std::promise<void> started;
        const auto beginning = started.get_future();
        std::future<void> nextFrame;
        QueueGate gate(queue); // Unwind releases the GPU before joining the worker.
        const auto completion = frames.SubmitFrame(index);
        const auto signaled = queue._fenceValue;
        Require(!queue.IsComplete(completion) && !retained.expired(), "Submission must return with pending GPU work and retained owners");
        nextFrame = std::async(std::launch::async, [&]
        {
            started.set_value();
            frames.BeginFrame(0);
        });
        Require(beginning.wait_for(std::chrono::seconds(2)) == std::future_status::ready, "Frame acquire worker must start");
        const auto blocked = nextFrame.wait_for(std::chrono::milliseconds(50));
        gate.Release();
        nextFrame.get();
        Require(blocked == std::future_status::timeout, "Frame acquisition must wait before allowing data preparation");
        Require(queue._fenceValue == signaled && retained.expired(), "Acquisition must use the prior completion and retire its owners without a flush");
        frames.SubmitFrame(index);
        frames.Shutdown();
        frames.Shutdown();
        ExpectFailure([&] { frames.BeginFrame(0); }, "not running");

        FrameCoordinator failed;
        failed.Initialize(2);
        std::shared_ptr<CommandContext> unfinished = failed.BeginFrame(0);
        const auto buffer = ReadbackBuffer(4);
        unfinished->DeclareResource(buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE, "unfinished frame");
        ExpectFailure([&] { failed.SubmitFrame(index); }, "did not reach declared final state");
        ExpectFailure([&] { failed.BeginFrame(0); }, "after a frame failure");
        ExpectFailure([&] { failed.SubmitFrame(index); }, "after a frame failure");
        failed.Shutdown();
    }

    template<typename T>
    T ReadConstant(ID3D12Resource* source)
    {
        auto readback = ReadbackBuffer(sizeof(T));
        CommandContext context;
        context.InitializeCommandContext(QUEUE_GRAPHICS);
        context.GetCommandList()->CopyBufferRegion(readback.Get(), 0, source, 0, sizeof(T));
        context.Finish(true);
        const D3D12_RANGE range{0, sizeof(T)};
        void* mapped = nullptr;
        ThrowIfFailed(readback->Map(0, &range, &mapped));
        T result;
        std::memcpy(&result, mapped, sizeof(T));
        const D3D12_RANGE noWrites{0, 0};
        readback->Unmap(0, &noWrites);
        return result;
    }

    void TestConstants()
    {
        Renderer renderer;
        renderer.InitializeRenderer();
        SceneFrames frames;
        GUI::viewportWidth = GUI::viewportHeight = 0;
        renderer.CreateConstantBuffers();
        Require(std::isfinite(renderer._projectionMatrix._11) && renderer._projectionMatrix._11 > 0 &&
            renderer._projectionMatrix._11 == renderer._projectionMatrix._22, "Invalid startup extent must use a valid square projection");
        GUI::viewportWidth = GUI::viewportHeight = ViewportSize;
        renderer.UpdateBuffers(0);
        Window::keys[KEYCODE_W] = true;
        for (int frame = 1; frame <= 3; ++frame)
        {
            renderer.UpdateBuffers(0.1f);
            const auto position = ReadConstant<XMFLOAT3>(renderer._camPosBufferResource.Get());
            Near(position, {0, 0, 5.0f - 0.25f * frame}, "GPU must see the latest camera position every update");
            const auto matrix = ReadConstant<XMFLOAT4X4>(renderer._VPBufferResource.Get());
            XMFLOAT3 eyeInClip;
            XMStoreFloat3(&eyeInClip, XMVector3Transform(XMLoadFloat3(&position), XMLoadFloat4x4(&matrix)));
            Near(eyeInClip.x, 0, "View-projection must use the matching camera position (X)");
            Near(eyeInClip.y, 0, "View-projection must use the matching camera position (Y)");
            Require(eyeInClip.z < 0, "The eye must be behind the near plane in clip space");
        }
        Window::keys[KEYCODE_W] = false;
        const auto before = ReadConstant<XMFLOAT4X4>(renderer._VPBufferResource.Get());
        GUI::viewportWidth = ViewportSize * 2;
        GUI::viewportResized = true;
        renderer.UpdateBuffers(0);
        const auto resized = ReadConstant<XMFLOAT4X4>(renderer._VPBufferResource.Get());
        Near(resized._11, before._11 * 0.5f, "Doubling viewport width must halve horizontal projection scale");
        GUI::viewportResized = false;
        Window::HandleMouse(100, 50);
        renderer.UpdateBuffers(0);
        const auto after = ReadConstant<XMFLOAT4X4>(renderer._VPBufferResource.Get());
        Require(std::memcmp(&resized, &after, sizeof(after)) != 0, "Rotation must reach the VP buffer");
        for (const auto& row : after.m)
            for (float value : row)
                Require(std::isfinite(value), "Updated view-projection values must be finite");
        // Exercise the production panel-size conversion, not only direct integer globals.
        const auto lastProjection = renderer._projectionMatrix;
        const auto lastVP = ReadConstant<XMFLOAT4X4>(renderer._VPBufferResource.Get());
        for (const ImVec2 extent : {ImVec2(0, 96), ImVec2(96, 0), ImVec2(-5, 96), ImVec2(96, -5),
            ImVec2(0.5f, 96), ImVec2(96, 0.5f), ImVec2(0, 0)})
        {
            GUI::SetViewportExtent(extent);
            Require(GUI::viewportWidth == 192 && GUI::viewportHeight == 96 && !GUI::viewportResized,
                "Invalid/subpixel panel extent must retain the last valid integer dimensions");
            renderer.UpdateBuffers(0);
            Require(std::memcmp(&renderer._projectionMatrix, &lastProjection, sizeof(lastProjection)) == 0,
                "Invalid panel extent must retain the last valid projection");
        }
        const auto rejectedPanel = ReadConstant<XMFLOAT4X4>(renderer._VPBufferResource.Get());
        Require(std::memcmp(&rejectedPanel, &lastVP, sizeof(lastVP)) == 0, "Rejected panel extents must preserve GPU VP bytes");
        GUI::SetViewportExtent(ImVec2(192.9f, 96.9f));
        Require(!GUI::viewportResized, "Unchanged integer extent must not repeatedly update projection");
        // Renderer must also defend against invalid dimensions supplied outside the GUI helper.
        for (const auto extent : {std::pair{0, 96}, std::pair{96, 0}, std::pair{-1, 96}, std::pair{96, -1}})
        {
            GUI::viewportWidth = extent.first;
            GUI::viewportHeight = extent.second;
            GUI::viewportResized = true;
            renderer.UpdateBuffers(0);
            Require(std::memcmp(&renderer._projectionMatrix, &lastProjection, sizeof(lastProjection)) == 0,
                "Invalid renderer extents must not change projection");
        }
        const auto rejectedRenderer = ReadConstant<XMFLOAT4X4>(renderer._VPBufferResource.Get());
        Require(std::memcmp(&rejectedRenderer, &lastVP, sizeof(lastVP)) == 0, "Rejected renderer extents must preserve GPU VP bytes");
        Window::keys[KEYCODE_W] = true;
        renderer.UpdateBuffers(0.1f);
        Window::keys[KEYCODE_W] = false;
        const auto moved = ReadConstant<XMFLOAT4X4>(renderer._VPBufferResource.Get());
        Require(std::memcmp(&moved, &lastVP, sizeof(moved)) != 0, "Camera must keep updating while panel extent is invalid");
        for (const auto& row : moved.m)
            for (const float value : row) Require(std::isfinite(value), "Collapsed-panel camera updates must stay finite");
        for (const ImVec2 extent : {ImVec2(192, 96), ImVec2(96, 192), ImVec2(1, 1), ImVec2(96, 96)})
        {
            GUI::SetViewportExtent(extent);
            renderer.UpdateBuffers(0);
            const auto matrix = ReadConstant<XMFLOAT4X4>(renderer._VPBufferResource.Get());
            const auto expected = XMMatrixPerspectiveFovRH(XMConvertToRadians(45.0f), extent.x / extent.y, 0.1f, 100.0f);
            XMFLOAT4X4 expectedVP;
            XMStoreFloat4x4(&expectedVP, XMLoadFloat4x4(&renderer._viewMatrix) * expected);
            for (int row = 0; row < 4; ++row)
                for (int col = 0; col < 4; ++col)
                    Near(matrix.m[row][col], expectedVP.m[row][col], "Restored panel must upload the correct wide/tall VP");
        }
        frames.Shutdown();
    }

    void TestBounds()
    {
        const auto vertices = Triangle(false);
        AABB bounds(vertices);
        Near(bounds.GetMin(), {0, 0, 0}, "Bounds must contain the original triangle (min)");
        Near(bounds.GetMax(), {1, 1, 0}, "Bounds must contain the original triangle (max)");
        XMFLOAT4X4 transform;
        XMStoreFloat4x4(&transform, XMMatrixScaling(-2, 3, 1) * XMMatrixTranslation(4, -5, 2));
        bounds.Recompute(transform);
        Near(bounds.GetMin(), {2, -5, 2}, "Negative scale must reorder transformed minima");
        Near(bounds.GetMax(), {4, -2, 2}, "Translated/scaled maxima must enclose all corners");
        AABB rotated(vertices);
        XMStoreFloat4x4(&transform, XMMatrixRotationZ(XM_PIDIV2));
        rotated.Recompute(transform);
        Near(rotated.GetMin(), {-1, 0, 0}, "Rotated bounds must include all transformed corners (min)");
        Near(rotated.GetMax(), {0, 1, 0}, "Rotated bounds must include all transformed corners (max)");
    }

    void TestPipeline(const std::string& name)
    {
        const std::filesystem::path directory("../shaders");
        ShaderPass pass(name);
        pass.AddShader(directory / (name + "_vert.hlsl"), SHADERTYPE::SHADER_VERTEX);
        pass.AddShader(directory / (name + "_frag.hlsl"), SHADERTYPE::SHADER_PIXEL);
        pass.GenerateGraphicsRootSignature();
        pass.GeneratePipeLineStateObjectForwardPass(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, name == "pbr");
        Require(pass._rootSignature && pass._pipelineState, "Reflected shaders must produce a root signature and PSO");
        const auto matrixBinding = name == "dShadowMap" ? "lightViewProjMatrixBuffer" : "viewProjMatrixBuffer";
        Require(pass.GetRootParameterIndex(matrixBinding).has_value(), "The pass must expose its scene-matrix binding");
        Require(pass.GetRootParameterIndex("modelMatrixBuffer").has_value(), "The pass must expose its model binding");
        Require(!pass.GetRootParameterIndex("nonexistentBinding").has_value(), "Missing bindings must return no index");
    }

    std::vector<uint8_t> CaptureTexture(ID3D12Resource* texture,
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
    {
        const auto description = texture->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        uint64_t size = 0;
        D3D12Core::GraphicsDevice::device->GetCopyableFootprints(&description, 0, 1, 0, &footprint, nullptr, nullptr, &size);
        auto readback = ReadbackBuffer(size);
        CommandContext context;
        context.InitializeCommandContext(QUEUE_GRAPHICS);
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture,
            initialState, D3D12_RESOURCE_STATE_COPY_SOURCE);
        context.GetCommandList()->ResourceBarrier(1, &barrier);
        const CD3DX12_TEXTURE_COPY_LOCATION destination(readback.Get(), footprint);
        const CD3DX12_TEXTURE_COPY_LOCATION source(texture, 0);
        context.GetCommandList()->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
        std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
        context.GetCommandList()->ResourceBarrier(1, &barrier);
        context.Finish(true);
        const D3D12_RANGE range{0, static_cast<SIZE_T>(size)};
        void* mapped = nullptr;
        ThrowIfFailed(readback->Map(0, &range, &mapped));
        const auto rowSize = static_cast<size_t>(description.Width) * 4;
        std::vector<uint8_t> pixels(rowSize * description.Height);
        for (uint32_t y = 0; y < description.Height; ++y)
            std::memcpy(pixels.data() + y * rowSize,
                static_cast<const uint8_t*>(mapped) + footprint.Offset + y * footprint.Footprint.RowPitch, rowSize);
        const D3D12_RANGE noWrites{0, 0};
        readback->Unmap(0, &noWrites);
        return pixels;
    }

    void TestRtvDescriptors()
    {
        auto* device = D3D12Core::GraphicsDevice::device.Get();
        const auto rtvIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        const auto resourceIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        std::cout << "Descriptor increments: RTV=" << rtvIncrement << ", CBV/SRV/UAV=" << resourceIncrement << '\n';

        DescriptorAllocator::RTV::InitializeDescriptorAllocator(3);
        Require(DescriptorAllocator::RTV::descriptorSize == rtvIncrement,
            "RTV allocator must use the device's RTV increment");
        const auto heapStart = DescriptorAllocator::RTV::GetHeap()->GetCPUDescriptorHandleForHeapStart();

        CommandQueueManager::InitializeCommandQueueManager();
        CommandContext context;
        context.InitializeCommandContext(QUEUE_GRAPHICS);
        const float colors[3][4] = {{1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}};
        std::array<MSWRL::ComPtr<ID3D12Resource>, 3> textures;
        const CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_DEFAULT);
        auto description = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 8, 8, 1, 1);
        description.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        for (size_t i = 0; i < textures.size(); ++i)
        {
            const auto handle = DescriptorAllocator::RTV::Allocate();
            Require(handle.ptr == heapStart.ptr + i * rtvIncrement,
                "Each RTV must occupy the next slot at the device's RTV stride");
            D3D12_CLEAR_VALUE clear{};
            clear.Format = description.Format;
            std::memcpy(clear.Color, colors[i], sizeof(clear.Color));
            ThrowIfFailed(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &description,
                D3D12_RESOURCE_STATE_RENDER_TARGET, &clear, IID_PPV_ARGS(&textures[i])));
            device->CreateRenderTargetView(textures[i].Get(), nullptr, handle);
            context.GetCommandList()->ClearRenderTargetView(handle, colors[i], 0, nullptr);
            const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(textures[i].Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            context.GetCommandList()->ResourceBarrier(1, &barrier);
        }
        context.Finish(true);
        for (size_t i = 0; i < textures.size(); ++i)
        {
            const auto pixels = CaptureTexture(textures[i].Get());
            for (size_t pixel = 0; pixel < pixels.size(); pixel += 4)
                for (size_t channel = 0; channel < 4; ++channel)
                    Require(pixels[pixel + channel] == static_cast<uint8_t>(colors[i][channel] * 255),
                        "Clearing each RTV must write its own texture without overwriting another target");
        }
    }

    size_t DrawnPixels(const std::vector<uint8_t>& pixels)
    {
        size_t drawn = 0;
        for (size_t i = 0; i < pixels.size(); i += 4)
        {
            const auto background = [](uint8_t channel) { return std::abs(static_cast<int>(channel) - 51) <= 1; };
            drawn += !(background(pixels[i]) && background(pixels[i + 1]) && background(pixels[i + 2]));
        }
        return drawn;
    }

    struct GuiFrameFixture
    {
        Renderer renderer;
        FrameCoordinator frames;
        ~GuiFrameFixture()
        {
            try { frames.Shutdown(); }
            catch (const std::exception& error) { std::cerr << "Frame fixture cleanup: " << error.what() << '\n'; }
            if (ImGui::GetCurrentContext()) GUI::Shutdown();
            for (auto& target : D3D12Core::Swapchain::renderTargets) target.Reset();
            D3D12Core::Swapchain::swapchain.Reset();
            Window::Shutdown();
        }
        void Initialize(const std::filesystem::path& scene)
        {
            renderer.InitializeRenderer();
            frames.Initialize(2);
            Window::InitializeWindow("artisDX hidden frame test", 256, 256, false);
            Require(Window::hWindow != nullptr, "Frame integration needs a hidden window");
            ReleaseCapture();
            ShowCursor(true); // Balance the editor window's startup capture without showing it.
            Window::captureMouse = false;
            D3D12Core::Swapchain::InitializeSwapchain();
            InitializeScene(renderer, scene);
            GUI::InitializeGUI();
            GUI::imguiIO->IniFilename = nullptr;
            GUI::imguiIO->ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
        }
    };

    struct SceneImageOverlay : IGUIComponent
    {
        Renderer& renderer;
        MSWRL::ComPtr<ID3D12Resource> readback;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        ID3D12Resource* captureTarget = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
        bool detached = false;
        ImGuiID detachedViewport = 0;
        explicit SceneImageOverlay(Renderer& scene) : renderer(scene) {}
        void DrawGUI() override
        {
            const auto index = D3D12Core::Swapchain::swapchain->GetCurrentBackBufferIndex();
            captureTarget = D3D12Core::Swapchain::renderTargets[index].Get();
            const auto description = captureTarget->GetDesc();
            width = static_cast<uint32_t>(description.Width);
            height = description.Height;
            uint64_t bytes = 0;
            D3D12Core::GraphicsDevice::device->GetCopyableFootprints(&description, 0, 1, 0,
                &footprint, nullptr, nullptr, &bytes);
            if (!readback || readback->GetDesc().Width != bytes) readback = ReadbackBuffer(bytes);
            const ImVec2 origin = ImGui::GetMainViewport()->Pos + ImVec2(13, 17);
            auto* foreground = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
            foreground->AddImage(static_cast<ImTextureID>(renderer._viewportSRV_GPU.ptr),
                origin, origin + ImVec2(ViewportSize, ViewportSize));
            // Capture in the submitted list before flip-discard Present can discard contents.
            foreground->AddCallback([](const ImDrawList*, const ImDrawCmd* command)
            {
                auto& image = *static_cast<SceneImageOverlay*>(command->UserCallbackData);
                const auto* state = static_cast<ImGui_ImplDX12_RenderState*>(ImGui::GetPlatformIO().Renderer_RenderState);
                D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(image.captureTarget,
                    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
                state->CommandList->ResourceBarrier(1, &barrier);
                const CD3DX12_TEXTURE_COPY_LOCATION destination(image.readback.Get(), image.footprint);
                const CD3DX12_TEXTURE_COPY_LOCATION source(image.captureTarget, 0);
                state->CommandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
                std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
                state->CommandList->ResourceBarrier(1, &barrier);
            }, this);
            foreground->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
            if (detached)
            {
                ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos + ImVec2(500, 50), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(240, 280), ImGuiCond_Always);
                ImGui::Begin("Hidden detached scene and shadow");
                detachedViewport = ImGui::GetWindowViewport()->ID;
                ImGui::Image(static_cast<ImTextureID>(renderer._viewportSRV_GPU.ptr), ImVec2(96, 96));
                ImGui::Image(static_cast<ImTextureID>(DescriptorAllocator::CBVSRVUAV::GetGPUHandle(
                    renderer._dLights.back()->_directionalShadowMapSRVCPUHandle).ptr), ImVec2(96, 96));
                ImGui::End();
            }
        }
        std::vector<uint8_t> ReadPixels()
        {
            const D3D12_RANGE range{ 0, static_cast<SIZE_T>(readback->GetDesc().Width) };
            void* mapped = nullptr;
            ThrowIfFailed(readback->Map(0, &range, &mapped));
            std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
            for (uint32_t y = 0; y < height; ++y)
                std::memcpy(pixels.data() + y * width * 4,
                    static_cast<const uint8_t*>(mapped) + footprint.Offset + y * footprint.Footprint.RowPitch, width * 4);
            const D3D12_RANGE noWrites{ 0, 0 };
            readback->Unmap(0, &noWrites);
            return pixels;
        }
    };

    void TestFrameGui(const std::filesystem::path& scene)
    {
        GuiFrameFixture runtime;
        runtime.Initialize(scene);
        const auto checkIds = []<typename T>()
        {
            const T first{};
            const T second{};
            Require(second.GetID() == first.GetID() + 1,
                "New objects must receive consecutive IDs from their own class");
        };
        checkIds.operator()<Camera>();
        checkIds.operator()<DirectionalLight>();
        checkIds.operator()<PointLight>();
        checkIds.operator()<ShaderPass>();
        checkIds.operator()<Model>();
        checkIds.operator()<ModelNode>();
        checkIds.operator()<Mesh>();
        checkIds.operator()<Primitive>();
        checkIds.operator()<Material>();
        checkIds.operator()<Texture>();
        std::vector<Mesh> meshes(1);
        const uint64_t meshId = meshes.front().GetID();
        meshes.reserve(8);
        Require(meshes.front().GetID() == meshId, "Container relocation must preserve logical asset IDs");
        const Mesh copied = meshes.front();
        Require(copied.GetID() == meshId, "Copying a shared asset must preserve its logical ID");

        auto duplicatePass = std::make_shared<ShaderPass>(runtime.renderer._mainPass->_name);
        duplicatePass->RegisterWithGUI();
        auto overlay = std::make_shared<SceneImageOverlay>(runtime.renderer);
        overlay->RegisterWithGUI();
        auto& queue = CommandQueueManager::GetCommandQueue(QUEUE_GRAPHICS);
        const auto before = queue._fenceValue;
        runtime.frames.RenderFrame(runtime.renderer, 0);
        Require(queue._fenceValue == before + 1, "Scene and main-window GUI must use exactly one submission");
        const std::string settings = ImGui::SaveIniSettingsToMemory();
        const std::string passKey = "[Window][###ShaderPass_" + std::to_string(runtime.renderer._mainPass->GetID()) + "]";
        const std::string duplicateKey = "[Window][###ShaderPass_" + std::to_string(duplicatePass->GetID()) + "]";
        Require(passKey != duplicateKey && settings.find(passKey) != std::string::npos && settings.find(duplicateKey) != std::string::npos,
            "Passes with the same display name must have separate saved windows");
        const std::string lightId = std::to_string(runtime.renderer._dLights.back()->GetID());
        Require(settings.find("[Window][###DirectionalLight_" + lightId + "]") != std::string::npos &&
            settings.find("[Window][###DirectionalLightShadowMap_" + lightId + "]") != std::string::npos,
            "A light and its shadow map must have separate saved windows");
        runtime.renderer._mainPass->_name = "Renamed main pass";
        runtime.frames.RenderFrame(runtime.renderer, 0);
        const std::string renamedSettings = ImGui::SaveIniSettingsToMemory();
        Require(renamedSettings.find(passKey) != std::string::npos &&
            renamedSettings.find(passKey, renamedSettings.find(passKey) + passKey.size()) == std::string::npos,
            "Renaming a pass must preserve its single saved window identity");
        const auto viewport = CaptureTexture(runtime.renderer._viewportTexture.Get());
        Require(DrawnPixels(viewport) > 100, "GUI composition must exercise a rendered scene");
        const auto backbuffer = overlay->ReadPixels(); // Scene readback waited for the combined submission.
        const auto rowWidth = overlay->width;
        for (uint32_t y = 2; y < ViewportSize - 2; ++y)
            for (uint32_t x = 2; x < ViewportSize - 2; ++x)
                for (uint32_t channel = 0; channel < 3; ++channel)
                {
                    const auto source = (y * ViewportSize + x) * 4;
                    const float alpha = viewport[source + 3] / 255.0f;
                    const auto expected = static_cast<int>(std::lround(viewport[source + channel] * alpha + 51 * (1 - alpha)));
                    const auto actual = backbuffer[((y + 17) * rowWidth + x + 13) * 4 + channel];
                    Require(std::abs(static_cast<int>(actual) - expected) <= 2,
                        "GUI sample mismatch x=" + std::to_string(x) + " y=" + std::to_string(y) +
                        " channel=" + std::to_string(channel) + " actual=" + std::to_string(actual) +
                        " expected=" + std::to_string(expected));
                }

        const auto rtv0 = D3D12Core::Swapchain::rtvCPUHandle[0].ptr;
        const auto rtv1 = D3D12Core::Swapchain::rtvCPUHandle[1].ptr;
        // Change presentation parity so resize also exercises a nonzero backbuffer index.
        runtime.frames.RenderFrame(runtime.renderer, 0);
        runtime.frames.Drain();
        QueueGate gate(queue);
        runtime.frames.RenderFrame(runtime.renderer, 0);
        const auto submitted = queue._fenceValue;
        SendMessage(Window::hWindow, WM_SIZE, SIZE_RESTORED, MAKELPARAM(200, 160));
        SendMessage(Window::hWindow, WM_SIZE, SIZE_RESTORED, MAKELPARAM(224, 176));
        SendMessage(Window::hWindow, WM_SIZE, SIZE_MINIMIZED, 0);
        SendMessage(Window::hWindow, WM_SIZE, SIZE_RESTORED, 0);
        Require(queue._fenceValue == submitted && queue._fence->GetCompletedValue() < submitted,
            "Window resize messages must return without waiting for pending rendering or signaling a flush");
        Require(D3D12Core::Swapchain::width == 256 && D3D12Core::Swapchain::height == 256,
            "Backbuffers must remain intact until the next frame boundary");
        gate.Release();
        runtime.frames.RenderFrame(runtime.renderer, 0);
        Require(D3D12Core::Swapchain::width == 224 && D3D12Core::Swapchain::height == 176,
            "Frame boundary must consume the latest valid resize and ignore minimized/zero extents");
        Require(D3D12Core::Swapchain::renderTargets[0]->GetDesc().Width == 224 &&
            D3D12Core::Swapchain::renderTargets[1]->GetDesc().Height == 176 &&
            D3D12Core::Swapchain::rtvCPUHandle[0].ptr == rtv0 && D3D12Core::Swapchain::rtvCPUHandle[1].ptr == rtv1,
            "Resizing must reacquire both backbuffers without allocating new RTV slots");
        runtime.frames.Drain();
        const auto drained = queue._fenceValue;
        runtime.frames.RenderFrame(runtime.renderer, 0);
        Require(queue._fenceValue == drained + 1, "Consumed resize must not repeat on subsequent frames");

        // Exercise the pinned secondary-window queues without opening visible test windows.
        runtime.frames.Drain();
        CheckGpuDiagnostics(); // Keep the backend's known Wait(0) outside main-frame checks.
        GUI::imguiIO->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        auto& platform = ImGui::GetPlatformIO();
        Require(platform.Renderer_CreateWindow && platform.Renderer_RenderWindow && platform.Renderer_SwapBuffers,
            "Detached coverage requires the DX12 backend callbacks, not only native windows");
        struct BackendWindows
        {
            decltype(platform.Renderer_CreateWindow) create;
            uint32_t count = 0;
        } backendWindows{ platform.Renderer_CreateWindow };
        const auto userData = GUI::imguiIO->UserData;
        GUI::imguiIO->UserData = &backendWindows;
        platform.Renderer_CreateWindow = [](ImGuiViewport* viewport)
        {
            auto& windows = *static_cast<BackendWindows*>(ImGui::GetIO().UserData);
            windows.create(viewport);
            ++windows.count;
        };
        const auto showWindow = platform.Platform_ShowWindow;
        platform.Platform_ShowWindow = [](ImGuiViewport* viewport)
        {
            ShowWindow(static_cast<HWND>(viewport->PlatformHandle), SW_HIDE);
        };
        // Finish the previously disabled platform phase before toggling into a new frame.
        ImGui::UpdatePlatformWindows();
        overlay->detached = true;
        for (int frame = 0; frame < 3; ++frame)
            runtime.frames.RenderFrame(runtime.renderer, 0);
        Require(overlay->detachedViewport != ImGui::GetMainViewport()->ID,
            "A panel outside the main viewport must create a secondary OS window");
        Require(queue._fence->GetCompletedValue() >= queue._fenceValue,
            "The detached-window stage must wait for this frame's texture producer");
        runtime.renderer._dLights.back()->_position = { -1, 2, 1 };
        runtime.frames.RenderFrame(runtime.renderer, 0);
        overlay->detached = false;
        for (int frame = 0; frame < 4; ++frame)
            runtime.frames.RenderFrame(runtime.renderer, 0);
        Require(std::none_of(platform.Viewports.begin(), platform.Viewports.end(),
            [&](const auto* viewport) { return viewport->ID == overlay->detachedViewport; }),
            "Closing a detached panel must release its secondary window");
        overlay->detached = true;
        for (int frame = 0; frame < 2; ++frame)
            runtime.frames.RenderFrame(runtime.renderer, 0);
        Require(platform.Viewports.Size > 1, "Shutdown coverage needs a live detached window");
        runtime.frames.Shutdown();
        // Each new backend queue first waits on its zero-valued fence. The SDK warns
        // about this no-op; accept only this ID and exactly one per created queue.
        Require(backendWindows.count > 0, "Detached coverage must create renderer-owned queues");
        CheckGpuDiagnostics(std::nullopt, D3D12_MESSAGE_ID_FENCE_ZERO_WAIT, backendWindows.count);
        platform.Renderer_CreateWindow = backendWindows.create;
        GUI::imguiIO->UserData = userData;
        platform.Platform_ShowWindow = showWindow;
        GUI::Shutdown();
    }

    void WriteCapture(const std::vector<uint8_t>& pixels, const std::string& name)
    {
        const DWORD capacity = GetEnvironmentVariableW(L"ARTISDX_TEST_CAPTURE_DIR", nullptr, 0);
        if (!capacity) return;
        std::wstring directory(capacity, L'\0');
        const DWORD length = GetEnvironmentVariableW(L"ARTISDX_TEST_CAPTURE_DIR", directory.data(), capacity);
        if (!length || length >= capacity) return;
        directory.resize(length);
        try
        {
            std::filesystem::create_directories(directory);
            ScratchImage capture;
            ThrowIfFailed(capture.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, ViewportSize, ViewportSize, 1, 1));
            const auto* image = capture.GetImage(0, 0, 0);
            for (size_t row = 0; row < ViewportSize; ++row)
                std::memcpy(image->pixels + row * image->rowPitch, pixels.data() + row * ViewportSize * 4, ViewportSize * 4);
            const auto path = std::filesystem::path(directory) / (name + ".png");
            ThrowIfFailed(SaveToWICFile(*image, WIC_FLAGS_NONE, GetWICCodec(WIC_CODEC_PNG), path.c_str()),
                "Could not save render capture");
        }
        catch (const std::exception& error) { std::cerr << "Capture skipped: " << error.what() << '\n'; }
    }

    void ClearShadowDepth(Renderer& renderer, float depth)
    {
        CheckGpuDiagnostics(); // Do not allow unrelated warnings into the injection scope.
        CommandContext context;
        context.InitializeCommandContext(QUEUE_GRAPHICS);
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderer._dLights.back()->_directionalShadowMapBuffer.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        context.GetCommandList()->ResourceBarrier(1, &barrier);
        context.GetCommandList()->ClearDepthStencilView(renderer._dLights.back()->_directionalShadowMapDSVCPUHandle,
            D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
        std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
        context.GetCommandList()->ResourceBarrier(1, &barrier);
        context.Finish(true);
        CheckGpuDiagnostics(std::nullopt, depth == 1 ? std::nullopt :
            std::optional{D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE});
    }

    std::vector<uint8_t> TestShadowToggle(Renderer& renderer, SceneFrames& frames)
    {
        renderer._depthPass->_usePass = false;
        frames.Render(renderer, 0);
        const auto disabled = CaptureTexture(renderer._viewportTexture.Get());
        WriteCapture(disabled, "shadows_off");
        Require(DrawnPixels(disabled) > 100, "Main must render before the first shadow frame, with DepthPass off");

        // Force stale depth to fully occluding and fully visible values.
        ClearShadowDepth(renderer, 0);
        frames.Render(renderer, 0);
        Require(CaptureTexture(renderer._viewportTexture.Get()) == disabled,
            "Shadows off must ignore fully occluding stale depth");

        renderer._dLights.back()->_position = {-1, 0.7f, 1};
        frames.Render(renderer, 0);
        const auto movedLight = CaptureTexture(renderer._viewportTexture.Get());
        Require(DrawnPixels(movedLight) > 100 && movedLight != disabled,
            "Moving the light with shadows off must still update directional lighting");
        ClearShadowDepth(renderer, 1);
        frames.Render(renderer, 0);
        Require(CaptureTexture(renderer._viewportTexture.Get()) == movedLight,
            "Shadows off must ignore stale depth after moving the light");

        const auto lightMatrix = renderer._dLights.back()->_lightViewProjMatrix;
        renderer._dLights.back()->_orthoWidth = 10;
        renderer._dLights.back()->_sceneCenter = {0.4f, 0.2f, 0};
        frames.Render(renderer, 0);
        Require(std::memcmp(&lightMatrix, &renderer._dLights.back()->_lightViewProjMatrix, sizeof(lightMatrix)) != 0,
            "The test must change the shadow projection");
        Require(CaptureTexture(renderer._viewportTexture.Get()) == movedLight,
            "Shadow projection changes must not affect Main with shadows off");

        renderer._dLights.back()->_position = {1, 1, 1};
        renderer._dLights.back()->_orthoWidth = 7.5f;
        renderer._dLights.back()->_sceneCenter = {0, 0, 0};
        frames.Render(renderer, 0);
        Require(CaptureTexture(renderer._viewportTexture.Get()) == disabled,
            "Restoring the light with shadows off must restore full visibility");

        renderer._depthPass->_usePass = true;
        frames.Render(renderer, 0);
        const auto enabled = CaptureTexture(renderer._viewportTexture.Get());
        size_t shadowedPixels = 0;
        for (size_t i = 0; i < enabled.size(); i += 4)
        {
            const int enabledColor = enabled[i] + enabled[i + 1] + enabled[i + 2];
            const int disabledColor = disabled[i] + disabled[i + 1] + disabled[i + 2];
            shadowedPixels += disabledColor - enabledColor > 20;
        }
        Require(shadowedPixels > 5, "Enabling DepthPass must produce visible shadows on the receiver quad");
        renderer._depthPass->_usePass = false;
        frames.Render(renderer, 0);
        Require(CaptureTexture(renderer._viewportTexture.Get()) == disabled,
            "Disabling an already rendered shadow map must restore full visibility");
        renderer._depthPass->_usePass = true;
        frames.Render(renderer, 0);
        Require(CaptureTexture(renderer._viewportTexture.Get()) == enabled,
            "Re-enabling DepthPass must regenerate the same shadows at the unchanged pose");
        std::cout << "PASS: shadow toggle checks (first frame, stale depth, moving light, off/on)\n";
        return enabled;
    }

    void TestTextureUpload(const std::filesystem::path& scene)
    {
        const ScratchImage fallback = GLTFLoader::LoadFallbackAlbedoTexture();
        const Image* fallbackImage = fallback.GetImage(0, 0, 0);
        Require(fallbackImage && fallbackImage->pixels[0] == 255 && fallbackImage->pixels[1] == 255 &&
            fallbackImage->pixels[2] == 255 && fallbackImage->pixels[3] == 255,
            "A missing base-color texture must preserve the material factor with opaque white");
        Renderer renderer;
        renderer.InitializeRenderer();
        SceneFrames frames;
        InitializeScene(renderer, scene);
        CheckGpuDiagnostics();
        frames.Render(renderer, 0);
        const auto pixels = CaptureTexture(renderer._viewportTexture.Get());
        Require(DrawnPixels(pixels) > 100, "Copy-queue uploads must be usable by the first graphics draw");

        // The textured quad's center must retain the fixture's red albedo.
        XMFLOAT3 projectedCenter;
        // Textured child at (-0.9, 0, 0), then parent's scale and translation.
        XMStoreFloat3(&projectedCenter, XMVector3TransformCoord(XMVectorSet(-0.42f, 0.25f, 0, 1),
            XMLoadFloat4x4(&renderer._viewProjectionMatrix)));
        const int x = static_cast<int>((projectedCenter.x * 0.5f + 0.5f) * ViewportSize);
        const int y = static_cast<int>((0.5f - projectedCenter.y * 0.5f) * ViewportSize);
        Require(x >= 0 && x < static_cast<int>(ViewportSize) && y >= 0 && y < static_cast<int>(ViewportSize),
            "The uploaded texture's test pixel must be inside the viewport");
        const auto offset = (static_cast<size_t>(y) * ViewportSize + x) * 4;
        Require(pixels[offset] > pixels[offset + 1] + 10 && pixels[offset] > pixels[offset + 2] + 10,
            "The first graphics draw must sample the uploaded red albedo texture");
        frames.Shutdown();
    }

    // Record an actual manager draw without the synchronous scene-test helper.
    // This deliberately tests manager retention, rather than implementing another frame path.
    void RecordMainDraw(Renderer& renderer, const std::shared_ptr<CommandContext>& contextOwner)
    {
        auto& context = *contextOwner;
        context.DeclareResource(renderer._viewportTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, "retirement viewport");
        context.DeclareResource(renderer._depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, "retirement depth");
        context.KeepAlive(renderer._VPBufferResource);
        context.KeepAlive(renderer._camPosBufferResource);
        for (const auto& light : renderer._pLights) context.KeepAlive(light);
        context.KeepAlive(renderer._dLights.back());
        context.KeepAlive(MSWRL::ComPtr<IUnknown>(DescriptorAllocator::CBVSRVUAV::GetHeap()));
        context.KeepAlive(MSWRL::ComPtr<IUnknown>(DescriptorAllocator::Sampler::GetHeap()));
        auto list = context.GetCommandList();
        ID3D12DescriptorHeap* heaps[] = {DescriptorAllocator::CBVSRVUAV::GetHeap(), DescriptorAllocator::Sampler::GetHeap()};
        list->SetDescriptorHeaps(_countof(heaps), heaps);
        context.SetPipelineState(renderer._mainPass->_pipelineState);
        context.SetGraphicsRootSignature(renderer._mainPass->_rootSignature);
        list->RSSetViewports(1, &renderer._vp);
        list->RSSetScissorRects(1, &renderer._scissor);
        context.UseResource(renderer._viewportTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        list->OMSetRenderTargets(1, &renderer._viewportRTV, false, &renderer._dsvCPUHandle);
        const float clear[] = {0.2f, 0.2f, 0.2f, 1};
        list->ClearRenderTargetView(renderer._viewportRTV, clear, 0, nullptr);
        list->ClearDepthStencilView(renderer._dsvCPUHandle, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
        const auto bind = [&](const char* name, D3D12_CPU_DESCRIPTOR_HANDLE handle)
        {
            const auto slot = renderer._mainPass->GetRootParameterIndex(name);
            Require(slot.has_value(), std::string("Main draw requires binding: ") + name);
            list->SetGraphicsRootDescriptorTable(*slot, DescriptorAllocator::CBVSRVUAV::GetGPUHandle(handle));
        };
        bind("viewProjMatrixBuffer", renderer._VPBufferDescriptor);
        bind("cameraBuffer", renderer._camPosBufferDescriptor);
        // The current PBR shader does not use point lighting; optimized reflection drops it.
        if (auto slot = renderer._mainPass->GetRootParameterIndex("plightBuffer"); slot && !renderer._pLights.empty())
            list->SetGraphicsRootDescriptorTable(*slot, DescriptorAllocator::CBVSRVUAV::GetGPUHandle(renderer._pLights.back()->_cbvpLightCPUHandle));
        bind("dlightBuffer", renderer._dLights.back()->_dLightDirectionCPUHandle);
        bind("lightViewProjMatrixBuffer", renderer._dLights.back()->_dLightLVPCPUHandle);
        bind("dShadowMap", renderer._dLights.back()->_directionalShadowMapSRVCPUHandle);
        const auto sampler = renderer._mainPass->GetRootParameterIndex("mySampler");
        Require(sampler.has_value(), "Main draw requires its sampler");
        list->SetGraphicsRootDescriptorTable(*sampler, DescriptorAllocator::Sampler::GetGPUHandle(renderer._samplerCPUHandle));
        renderer._modelManager.DrawAll(renderer._mainPass, contextOwner, renderer._viewMatrix);
        context.UseResource(renderer._viewportTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    void TestRetirement(const std::filesystem::path& scene)
    {
        Renderer renderer;
        renderer.InitializeRenderer();
        SceneFrames frames;
        auto& graphics = CommandQueueManager::GetCommandQueue(QUEUE_GRAPHICS);
        auto& copy = CommandQueueManager::GetCommandQueue(QUEUE_UPLOAD);
        auto* device = D3D12Core::GraphicsDevice::device.Get();
        MSWRL::ComPtr<ID3D12Resource> upload;
        const CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_UPLOAD);
        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32_t));
        ThrowIfFailed(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload)));
        uint32_t* mapped = nullptr;
        ThrowIfFailed(upload->Map(0, nullptr, reinterpret_cast<void**>(&mapped)));
        *mapped = 0xABCDEF42;
        upload->Unmap(0, nullptr);
        auto owner = std::make_shared<MSWRL::ComPtr<ID3D12Resource>>(upload);
        std::weak_ptr<MSWRL::ComPtr<ID3D12Resource>> weak = owner;
        auto readback = ReadbackBuffer(sizeof(uint32_t));
        auto context = std::make_shared<CommandContext>();
        context->InitializeCommandContext(QUEUE_GRAPHICS);
        context->KeepAlive(owner);
        context->KeepAlive(readback);
        context->GetCommandList()->CopyBufferRegion(readback.Get(), 0, upload.Get(), 0, sizeof(uint32_t));
        owner.reset();
        upload.Reset();
        Require(!weak.expired(), "Recorded-but-unsubmitted work must retain its source owner");
        QueueGate gate(graphics);
        const auto pending = context->Finish(false);
        context.reset(); // Queue also retains the list and allocator; destruction need not wait.
        graphics.CollectCompleted();
        Require(!graphics.IsComplete(pending) && !weak.expired() && graphics.GetPendingSubmissionCount() > 0,
            "Destroying the CPU context must preserve every pending object and owner");
        gate.Release();
        graphics.WaitCPU(pending);
        Require(weak.expired() && graphics.GetPendingSubmissionCount() == 0, "Completed ownership must be collected");
        ThrowIfFailed(readback->Map(0, nullptr, reinterpret_cast<void**>(&mapped)));
        Require(*mapped == 0xABCDEF42, "GPU copy must survive deletion of its CPU source owner and context");
        const D3D12_RANGE noWrites{0, 0};
        readback->Unmap(0, &noWrites);

        QueueGate copyGate(copy);
        auto shared = std::make_shared<int>(42);
        std::weak_ptr<int> sharedWeak = shared;
        CommandContext copyUse, graphicsUse;
        copyUse.InitializeCommandContext(QUEUE_UPLOAD);
        graphicsUse.InitializeCommandContext(QUEUE_GRAPHICS);
        copyUse.KeepAlive(shared);
        graphicsUse.KeepAlive(shared);
        shared.reset();
        const auto copyPending = copyUse.Finish(false);
        graphicsUse.Finish(true);
        Require(!sharedWeak.expired(), "Completing one queue must not release an owner still used by another");
        copyGate.Release();
        copy.WaitCPU(copyPending);
        Require(sharedWeak.expired(), "Owner must retire after all queue uses complete");

        // Actual pass replacement while its old native objects are already recorded.
        ShaderPass pass("retained-pass");
        pass.AddShader("../shaders/bb_vert.hlsl", SHADER_VERTEX);
        pass.AddShader("../shaders/bb_frag.hlsl", SHADER_PIXEL);
        pass.GenerateGraphicsRootSignature();
        pass.GeneratePipeLineStateObjectForwardPass(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, false);
        graphicsUse.Reset();
        graphicsUse.SetPipelineState(pass._pipelineState);
        graphicsUse.SetGraphicsRootSignature(pass._rootSignature);
        pass.GenerateGraphicsRootSignature();
        pass.GeneratePipeLineStateObjectForwardPass(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, false);
        graphicsUse.Finish(true);

        InitializeScene(renderer, scene);
        renderer._depthPass->_usePass = false;
        frames.Render(renderer, 0);
        const auto expectedImage = CaptureTexture(renderer._viewportTexture.Get());
        Require(DrawnPixels(expectedImage) > 100, "Scene must draw before unload");
        std::weak_ptr<Model> modelOwner;
        for (const auto& component : GUI::guiComponents)
            if (auto model = std::dynamic_pointer_cast<Model>(component.lock())) modelOwner = model;
        Require(!modelOwner.expired(), "The fixture model must expose an observable weak owner");
        std::vector<Model::DrawItem> borrowedDrawItems;
        {
            const std::shared_ptr<Model> model = modelOwner.lock();
            model->AppendDrawItems(borrowedDrawItems);
        }
        Require(!borrowedDrawItems.empty(), "Loaded geometry must produce borrowed draw items");
        context = std::make_shared<CommandContext>();
        context->InitializeCommandContext(QUEUE_GRAPHICS);
        RecordMainDraw(renderer, context);
        renderer._modelManager.ClearModels();
        Require(!modelOwner.expired(), "Recorded model commands must survive manager unload before submission");
        QueueGate modelGate(graphics);
        const auto modelPending = context->Finish(false);
        context.reset();
        RequirePending(modelPending, "Model draw must remain in flight while its queue is gated");
        Require(!modelOwner.expired(), "Submitted model draw must retain its owner after manager and context destruction");
        modelGate.Release();
        graphics.WaitCPU(modelPending);
        Require(modelOwner.expired(), "Model owner must retire after its submitted draw completes");
        for (const Model::DrawItem& item : borrowedDrawItems)
            Require(!item.model.lock(), "Surviving draw items must expire without retaining or accessing the unloaded model");
        Require(CaptureTexture(renderer._viewportTexture.Get()) == expectedImage,
            "An in-flight draw must preserve the model's expected image after unload");
        frames.Render(renderer, 0);
        Require(DrawnPixels(CaptureTexture(renderer._viewportTexture.Get())) == 0, "Clearing model ownership must produce an empty scene safely");
        frames.Shutdown();
    }

    void TestPassStateAndTransforms(const std::filesystem::path& scene)
    {
        Renderer renderer;
        renderer.InitializeRenderer();
        SceneFrames frames;
        InitializeScene(renderer, scene);
        auto& queue = CommandQueueManager::GetCommandQueue(QUEUE_GRAPHICS);
        auto* viewport = renderer._viewportTexture.Get();
        CommandContext context;
        context.InitializeCommandContext(QUEUE_GRAPHICS);
        ExpectFailure([&] { context.UseResource(viewport, D3D12_RESOURCE_STATE_COPY_SOURCE); }, "not declared");
        context.DeclareResource(viewport, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, "test viewport");
        ExpectFailure([&] { context.DeclareResource(viewport, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, "duplicate viewport"); }, "already declared");
        Require(!context.UseResource(viewport, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE), "Unchanged use must emit no transition");
        Require(context.UseResource(viewport, D3D12_RESOURCE_STATE_COPY_SOURCE) &&
            !context.UseResource(viewport, D3D12_RESOURCE_STATE_COPY_SOURCE), "Only the first changed use needs a transition");
        const auto before = queue._fenceValue;
        ExpectFailure([&] { context.Finish(false); }, "did not reach declared final state: test viewport");
        Require(queue._fenceValue == before, "Missing final use must reject before submission/signaling");
        Require(context.UseResource(viewport, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE), "Caller must be able to finish the rejected recording");
        context.Finish(true);
        ExpectFailure([&] { context.UseResource(viewport, D3D12_RESOURCE_STATE_COPY_SOURCE); }, "requires a recording context");
        context.Reset();
        context.DeclareResource(viewport, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, "new recording");
        context.Finish(true);
        CommandContext upload;
        upload.InitializeCommandContext(QUEUE_UPLOAD);
        ExpectFailure([&] { upload.DeclareResource(viewport, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON, "copy texture"); },
            "requires a recording graphics context");
        upload.Finish(true);

        renderer._mainPass->_usePass = renderer._depthPass->_usePass = renderer._bbPass->_usePass = false;
        frames.Render(renderer, 0);
        Require(DrawnPixels(CaptureTexture(renderer._viewportTexture.Get())) == 0,
            "All disabled passes must leave the color target clear");

        // No ordinary drawing pass has run: boxes must already have the authored hierarchy pose.
        renderer._bbPass->_usePass = true;
        frames.Render(renderer, 0);
        const auto boxesOnly = CaptureTexture(renderer._viewportTexture.Get());
        Require(DrawnPixels(boxesOnly) > 10, "Boxes must have valid transforms before Main or Depth ever draws");
        // Independent authored-position oracle: child bounds [-1.7,-0.1] x [-0.8,0.8],
        // followed by parent scale (0.8,1.1,1) and translation (0.3,0.25,0).
        for (const float worldX : {-1.06f, 0.22f})
        {
            XMFLOAT3 projected;
            XMStoreFloat3(&projected, XMVector3TransformCoord(XMVectorSet(worldX, -0.63f, 0, 1),
                XMLoadFloat4x4(&renderer._viewProjectionMatrix)));
            const int x = static_cast<int>((projected.x * 0.5f + 0.5f) * ViewportSize);
            const int y = static_cast<int>((0.5f - projected.y * 0.5f) * ViewportSize);
            bool greenCorner = false;
            for (int row = std::max(0, y - 1); row <= std::min(static_cast<int>(ViewportSize) - 1, y + 1); ++row)
                for (int column = std::max(0, x - 1); column <= std::min(static_cast<int>(ViewportSize) - 1, x + 1); ++column)
                {
                    const size_t offset = (static_cast<size_t>(row) * ViewportSize + column) * 4;
                    greenCorner |= boxesOnly[offset] == 0 && boxesOnly[offset + 1] == 255 && boxesOnly[offset + 2] == 0;
                }
            Require(greenCorner, "Cold boxes must occupy the authored child-plus-parent world bounds");
        }
        for (int cycle = 0; cycle < 2; ++cycle)
            for (unsigned mask = 0; mask < 8; ++mask)
            {
                renderer._mainPass->_usePass = (mask & 1) != 0;
                renderer._depthPass->_usePass = (mask & 2) != 0;
                renderer._bbPass->_usePass = (mask & 4) != 0;
                frames.Render(renderer, 0);
                const auto pixels = CaptureTexture(renderer._viewportTexture.Get());
                const std::string scenario = "Pass mask " + std::to_string(mask) + ", cycle " + std::to_string(cycle) + ": ";
                Require((DrawnPixels(pixels) > 0) == ((mask & 5) != 0), scenario + "color target contract");
                if (mask == 4)
                    Require(pixels == boxesOnly, scenario + "must preserve the cold boxes-only pose");
                if (mask == 1)
                    Require(DrawnPixels(pixels) > 100, scenario + "Main must draw the scene");
                CheckGpuDiagnostics();
            }
        const auto shadow = CaptureTexture(renderer._dLights.back()->_directionalShadowMapBuffer.Get());
        Require(!shadow.empty(), "Shadow texture must remain readable after the pass cycles");
        frames.Shutdown();
    }

    void TestTransparency(Renderer& renderer, SceneFrames& frames)
    {
        struct Fixture
        {
            std::filesystem::path path = std::filesystem::temp_directory_path() /
                ("artisDX-transparency-" + std::to_string(GetCurrentProcessId()) + ".gltf");
            ~Fixture() { std::error_code error; std::filesystem::remove(path, error); }
        } fixture;
        // A quad plus an RGBA PNG with alpha=128, embedded to avoid new fixture files.
        const auto load = [&](const char* mode, const char* color, float z, float cutoff = 0.5f, bool textured = false)
        {
            std::ofstream output;
            output.exceptions(std::ios::failbit | std::ios::badbit);
            output.open(fixture.path);
            output << R"({"asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0]}],
"nodes":[{"translation":[0,0,)" << z << R"(],"children":[1]},{"mesh":0}],
"meshes":[{"primitives":[{"attributes":{"POSITION":0,"NORMAL":1,"TEXCOORD_0":2},"indices":3,"material":0}]}],
"materials":[{"alphaMode":")" << mode << R"(","alphaCutoff":)" << cutoff << R"(,"pbrMetallicRoughness":{
"baseColorFactor":)" << color << R"(,"metallicFactor":0,"roughnessFactor":1)";
            if (textured) output << R"(,"baseColorTexture":{"index":0})";
            output << R"(}}],"textures":[{"source":0}],"images":[{"bufferView":4,"mimeType":"image/png"}],
"accessors":[{"bufferView":0,"componentType":5126,"count":4,"type":"VEC3","min":[-0.8,-0.8,0],"max":[0.8,0.8,0]},
{"bufferView":1,"componentType":5126,"count":4,"type":"VEC3"},
{"bufferView":2,"componentType":5126,"count":4,"type":"VEC2"},
{"bufferView":3,"componentType":5123,"count":6,"type":"SCALAR"}],
"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":48},{"buffer":0,"byteOffset":48,"byteLength":48},
{"buffer":0,"byteOffset":96,"byteLength":32},{"buffer":0,"byteOffset":128,"byteLength":12},
{"buffer":0,"byteOffset":140,"byteLength":76}],"buffers":[{"byteLength":216,"uri":
"data:application/octet-stream;base64,zcxMv83MTL8AAAAAzcxMP83MTL8AAAAAzcxMP83MTD8AAAAAzcxMv83MTD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAgD8AAIA/AAAAAAAAgD8AAAEAAgAAAAIAAwCJUE5HDQoaCgAAAA1JSERSAAAACAAAAAgIBgAAAMQPvosAAAATSURBVHicY/j//38DPswwMhQAAFRl30EREFkEAAAAAElFTkSuQmCC"}]})";
            output.close();
            renderer._modelManager.LoadModel(fixture.path);
        };
        const auto pixel = [&]
        {
            frames.Render(renderer, 0);
            const auto pixels = CaptureTexture(renderer._viewportTexture.Get());
            std::array<uint8_t, 4> result;
            std::memcpy(result.data(), pixels.data() + (ViewportSize / 2 * ViewportSize + ViewportSize / 2) * 4, 4);
            return result;
        };
        const auto depthAtCenter = [&]
        {
            const auto bytes = CaptureTexture(renderer._depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
            float depth;
            std::memcpy(&depth, bytes.data() + (ViewportSize / 2 * ViewportSize + ViewportSize / 2) * 4, 4);
            return depth;
        };
        const auto shadowWritten = [&]
        {
            const auto bytes = CaptureTexture(renderer._dLights.back()->_directionalShadowMapBuffer.Get());
            for (size_t i = 0; i < bytes.size(); i += sizeof(float))
            {
                float depth;
                std::memcpy(&depth, bytes.data() + i, sizeof(depth));
                if (depth < 1) return true;
            }
            return false;
        };

        renderer._modelManager.ClearModels();
        renderer._depthPass->_usePass = false;
        load("OPAQUE", "[1,0,0,1]", 1);
        const auto red = pixel();
        renderer._modelManager.ClearModels();
        load("OPAQUE", "[0,0,1,1]", 0);
        const auto blue = pixel();
        Require(red[0] > red[2] + 30 && blue[2] > blue[0] + 30, "Transparency reference quads must cover the test pixel");

        renderer._modelManager.ClearModels();
        load("BLEND", "[1,0,0,0.5]", 1, 0.5f, true); // Deliberately load near first.
        load("BLEND", "[0,0,1,0.5]", 0);
        const auto blended = pixel();
        const float nearAlpha = 0.5f * 128.0f / 255.0f;
        for (size_t channel = 0; channel < 3; ++channel)
        {
            const float expected = red[channel] * nearAlpha +
                (blue[channel] * 0.5f + 51 * 0.5f) * (1 - nearAlpha);
            Require(std::abs(blended[channel] - expected) <= 2, "Global back-to-front blending, channel " + std::to_string(channel) +
                ": expected " + std::to_string(expected) + ", got " + std::to_string(blended[channel]));
        }
        Require(blended[3] == 255 && depthAtCenter() == 1, "Blending must preserve viewport alpha and leave scene depth untouched");
        renderer._modelManager.ClearModels();
        load("BLEND", "[0,0,1,0.5]", 0);
        load("BLEND", "[1,0,0,0.5]", 1, 0.5f, true);
        Require(pixel() == blended, "Blended output must be independent of model insertion order");

        renderer._modelManager.ClearModels();
        load("BLEND", "[1,0,0,0.5]", 1, 0.5f, true);
        load("OPAQUE", "[0,0,1,1]", 0); // Solid geometry loaded after the transparent model.
        const auto overOpaque = pixel();
        for (size_t channel = 0; channel < 3; ++channel)
            Require(std::abs(overOpaque[channel] - (red[channel] * nearAlpha + blue[channel] * (1 - nearAlpha))) <= 2,
                "Every model's solid surfaces must draw before any blended surfaces");

        renderer._modelManager.ClearModels();
        load("OPAQUE", "[1,0,0,0]", 1);
        Require(pixel() == red && depthAtCenter() < 1, "OPAQUE must ignore alpha and write depth");
        load("BLEND", "[0,0,1,0.5]", 0);
        Require(pixel() == red, "Opaque foreground depth must occlude a blended surface behind it");

        renderer._depthPass->_usePass = true;
        for (const float cutoff : {0.3f, 0.2f})
        {
            renderer._modelManager.ClearModels();
            load("MASK", "[1,0,0,0.5]", 1, cutoff, true);
            const auto masked = pixel();
            const bool survives = cutoff == 0.2f;
            Require(survives ? DrawnPixels(CaptureTexture(renderer._viewportTexture.Get())) > 100 :
                masked == std::array<uint8_t, 4>{51, 51, 51, 255}, "MASK must apply the imported cutoff to texture alpha times factor alpha");
            Require((depthAtCenter() < 1) == survives && shadowWritten() == survives,
                "Cutout discard must match in scene depth and shadow depth");
            Require(masked[3] == 255, "Surviving cutouts must be opaque in the viewport");
        }
        renderer._modelManager.ClearModels();
        load("BLEND", "[1,0,0,0.5]", 1);
        pixel();
        Require(!shadowWritten(), "Blended surfaces must not cast solid opaque shadows");
        CheckGpuDiagnostics();
        std::cout << "PASS: transparency (global sorting, linear alpha, opaque depth, cutout color/depth/shadows)\n";
    }

    void TestRender(const std::filesystem::path& scene)
    {
        Renderer renderer;
        renderer.InitializeRenderer();
        SceneFrames frames;
        InitializeScene(renderer, scene);
        const auto mainPixels = TestShadowToggle(renderer, frames);
        WriteCapture(mainPixels, "pbr");
        Require(DrawnPixels(mainPixels) > 100, "GLB geometry/materials must produce visible PBR pixels");
        const auto depth = CaptureTexture(renderer._dLights.back()->_directionalShadowMapBuffer.Get());
        bool shadowWritten = false;
        for (size_t i = 0; i < depth.size(); i += sizeof(float))
        {
            float value;
            std::memcpy(&value, depth.data() + i, sizeof(value));
            Require(std::isfinite(value) && value >= 0 && value <= 1, "Shadow depth must stay in [0, 1]");
            shadowWritten |= value < 1;
        }
        Require(shadowWritten, "The shadow pass must write geometry depth");

        renderer._mainPass->_usePass = false;
        renderer._bbPass->_usePass = true;
        frames.Render(renderer, 0);
        const auto boxes = CaptureTexture(renderer._viewportTexture.Get());
        WriteCapture(boxes, "bounding_boxes");
        Require(DrawnPixels(boxes) > 10, "The bounding-box pass must draw visible geometry");

        renderer._bbPass->_usePass = false;
        frames.Render(renderer, 0);
        const auto empty = CaptureTexture(renderer._viewportTexture.Get());
        Require(DrawnPixels(empty) == 0, "Depth-only rendering must leave the color target at its clear color");

        renderer._mainPass->_usePass = true;
        frames.Render(renderer, 0);
        Require(CaptureTexture(renderer._viewportTexture.Get()) == mainPixels,
            "Restoring the main pass at the same pose must restore the same image");
        TestTransparency(renderer, frames);
        frames.Shutdown();
    }
}

    int RunGpuTest(std::string_view test, const std::string& argument, bool hardware)
    {
        try
        {
            ComApartment apartment;
            InitializeGpu(hardware, test == "upload" || test == "render");
            if (test == "constants") TestConstants();
            else if (test == "commands") TestCommandFailures();
            else if (test == "completion") TestCommandCompletion();
            else if (test == "frames") TestFrameLifecycle();
            else if (test == "frame-gui") TestFrameGui(argument);
            else if (test == "root-failures") TestRootSignatureFailures();
            else if (test == "root-layout") TestRootLayout(argument);
            else if (test == "bounds") TestBounds();
            else if (test == "rtv") TestRtvDescriptors();
            else if (test == "pipeline") TestPipeline(argument);
            else if (test == "upload") TestTextureUpload(argument);
            else if (test == "render") TestRender(argument);
            else if (test == "retirement") TestRetirement(argument);
            else if (test == "passes") TestPassStateAndTransforms(argument);
            else throw std::runtime_error("Unknown GPU test: " + std::string(test));
            // Only the isolated negative case expects a deliberately closed-list error.
            CheckGpuDiagnostics(test == "commands" ? std::optional{D3D12_MESSAGE_ID_COMMAND_LIST_CLOSED} : std::nullopt);
            return 0;
        }
        catch (const GpuUnavailable& error)
        {
            std::cout << "SKIP: " << error.what() << '\n';
            return SkipExitCode;
        }
    }
}
