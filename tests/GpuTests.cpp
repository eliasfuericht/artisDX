#include "TestCases.h"
#include "TestSupport.h"
#include "Renderer.h"

#include <array>
#include <cstring>
#include <thread>
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

    void CheckGpuDiagnostics(std::optional<D3D12_MESSAGE_ID> expectedError = std::nullopt)
    {
        ThrowIfFailed(D3D12Core::GraphicsDevice::device->GetDeviceRemovedReason(), "GPU device was removed");
        bool errors = false;
        uint32_t expectedCount = 0;
        for (uint64_t i = 0; i < infoQueue->GetNumStoredMessages(); ++i)
        {
            SIZE_T size = 0;
            ThrowIfFailed(infoQueue->GetMessage(i, nullptr, &size));
            std::vector<uint8_t> storage(size);
            auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
            ThrowIfFailed(infoQueue->GetMessage(i, message, &size));
            const bool expected = expectedError && message->ID == *expectedError && message->Severity == D3D12_MESSAGE_SEVERITY_ERROR;
            if (message->Severity <= D3D12_MESSAGE_SEVERITY_WARNING)
                std::cerr << (expected ? "Expected D3D12 [" : "D3D12 [") << message->ID << "]: " << message->pDescription << '\n';
            if (expected)
                ++expectedCount;
            else
                errors |= message->Severity <= D3D12_MESSAGE_SEVERITY_ERROR;
        }
        Require(!errors, "D3D12 debug layer reported errors; see the messages above");
        if (expectedError)
            Require(expectedCount == 1, "Negative case must produce exactly its one expected D3D12 error");
    }

    void TestCommandFailures()
    {
        CommandQueueManager::InitializeCommandQueueManager();
        for (const QUEUETYPE type : {QUEUE_GRAPHICS, QUEUE_UPLOAD})
        {
            CommandContext context;
            context.InitializeCommandContext(type);
            context.Finish(true);
            context.Reset();
            context.Finish(true);
            const auto& queue = CommandQueueManager::GetCommandQueue(type);
            Require(queue._fence->GetCompletedValue() >= queue._fenceValue, "Submitted work must finish before reuse");
        }
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
        MSWRL::ComPtr<ID3D12Fence> gate;
        ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gate)));
        // Always unblock even if an assertion throws, so negative tests cannot hang teardown.
        struct ReleaseGate { ID3D12Fence* fence; ~ReleaseGate() { fence->Signal(1); } } release{gate.Get()};
        ThrowIfFailed(graphics._commandQueue->Wait(gate.Get(), 1));
        const auto first = context.Finish(false);
        Require(first.GetFence() == graphics._fence.Get() && first.GetValue() > 0 && !graphics.IsComplete(first),
            "Nonblocking submission must signal its own identifiable, pending completion");
        ExpectFailure([&] { copy.WaitCPU(first); }, "does not belong to this queue");
        const auto signaled = graphics._fenceValue;
        std::thread unblock([&] { std::this_thread::sleep_for(std::chrono::milliseconds(25)); gate->Signal(1); });
        try { context.Reset(); }
        catch (...) { unblock.join(); throw; }
        unblock.join();
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
        producer.GetCommandList()->CopyBufferRegion(destination.Get(), 0, upload.Get(), 0, sizeof(uint32_t));
        const auto copied = producer.Finish(false);
        graphics.WaitGPU(copied);
        auto readback = ReadbackBuffer(sizeof(uint32_t));
        context.Reset();
        context.GetCommandList()->CopyBufferRegion(readback.Get(), 0, destination.Get(), 0, sizeof(uint32_t));
        const auto consumed = context.Finish(true);
        Require(consumed.GetFence() != copied.GetFence() && copy.IsComplete(copied), "Queue identity and copy-to-graphics dependency must be preserved");
        ThrowIfFailed(readback->Map(0, nullptr, reinterpret_cast<void**>(&mapped)));
        Require(*mapped == 0x12345678, "Consumer must read the producer's completed copy");
        readback->Unmap(0, &noRead);
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
        GUI::viewportWidth = GUI::viewportHeight = 0;
        renderer.CreateConstantBuffers();
        Require(std::isfinite(renderer._projectionMatrix._11) && renderer._projectionMatrix._11 > 0 &&
            renderer._projectionMatrix._11 == renderer._projectionMatrix._22, "Invalid startup extent must use a valid square projection");
        GUI::viewportWidth = GUI::viewportHeight = ViewportSize;
        renderer.UpdateBuffers(0);
        Window::keys[KEYCODE_W] = true;
        for (int frame = 1; frame <= 16; ++frame)
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
            const auto matrix = ReadConstant<XMFLOAT4X4>(renderer._VPBufferResource.Get());
            Require(std::memcmp(&matrix, &lastVP, sizeof(matrix)) == 0 &&
                std::memcmp(&renderer._projectionMatrix, &lastProjection, sizeof(lastProjection)) == 0,
                "Invalid panel extent must retain the last valid projection/VP");
        }
        GUI::SetViewportExtent(ImVec2(192.9f, 96.9f));
        Require(!GUI::viewportResized, "Unchanged integer extent must not repeatedly update projection");
        // Renderer must also defend against invalid dimensions supplied outside the GUI helper.
        for (const auto extent : {std::pair{0, 96}, std::pair{96, 0}, std::pair{-1, 96}, std::pair{96, -1}})
        {
            GUI::viewportWidth = extent.first;
            GUI::viewportHeight = extent.second;
            GUI::viewportResized = true;
            renderer.UpdateBuffers(0);
            const auto matrix = ReadConstant<XMFLOAT4X4>(renderer._VPBufferResource.Get());
            Require(std::memcmp(&matrix, &lastVP, sizeof(matrix)) == 0, "Invalid renderer extents must not change projection");
        }
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
            const auto expected = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), extent.x / extent.y, 0.1f, 100.0f);
            XMFLOAT4X4 expectedVP;
            XMStoreFloat4x4(&expectedVP, XMLoadFloat4x4(&renderer._viewMatrix) * expected);
            for (int row = 0; row < 4; ++row)
                for (int col = 0; col < 4; ++col)
                    Near(matrix.m[row][col], expectedVP.m[row][col], "Restored panel must upload the correct wide/tall VP");
        }
        renderer.Shutdown();
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

    std::vector<uint8_t> CaptureTexture(ID3D12Resource* texture)
    {
        const auto description = texture->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        uint64_t size = 0;
        D3D12Core::GraphicsDevice::device->GetCopyableFootprints(&description, 0, 1, 0, &footprint, nullptr, nullptr, &size);
        auto readback = ReadbackBuffer(size);
        CommandContext context;
        context.InitializeCommandContext(QUEUE_GRAPHICS);
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
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

    void WriteCapture(const std::vector<uint8_t>& pixels, const std::string& name)
    {
        std::filesystem::create_directories("Testing/artifacts");
        ScratchImage capture;
        ThrowIfFailed(capture.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, ViewportSize, ViewportSize, 1, 1));
        const auto* image = capture.GetImage(0, 0, 0);
        for (size_t row = 0; row < ViewportSize; ++row)
            std::memcpy(image->pixels + row * image->rowPitch, pixels.data() + row * ViewportSize * 4, ViewportSize * 4);
        const std::filesystem::path path("Testing/artifacts/" + name + ".png");
        ThrowIfFailed(SaveToWICFile(*image, WIC_FLAGS_NONE, GetWICCodec(WIC_CODEC_PNG), path.c_str()),
            "Could not save render capture");
    }

    void ClearShadowDepth(Renderer& renderer, float depth)
    {
        CommandContext context;
        context.InitializeCommandContext(QUEUE_GRAPHICS);
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderer._dLight->_directionalShadowMapBuffer.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        context.GetCommandList()->ResourceBarrier(1, &barrier);
        context.GetCommandList()->ClearDepthStencilView(renderer._dLight->_directionalShadowMapDSVCPUHandle,
            D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
        std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
        context.GetCommandList()->ResourceBarrier(1, &barrier);
        context.Finish(true);
    }

    std::vector<uint8_t> TestShadowToggle(Renderer& renderer)
    {
        renderer._depthPass->_usePass = false;
        renderer.Render(0);
        const auto disabled = CaptureTexture(renderer._viewportTexture.Get());
        WriteCapture(disabled, "shadows_off");
        Require(DrawnPixels(disabled) > 100, "Main must render before the first shadow frame, with DepthPass off");

        // Force stale depth to fully occluding and fully visible values.
        ClearShadowDepth(renderer, 0);
        renderer.Render(0);
        Require(CaptureTexture(renderer._viewportTexture.Get()) == disabled,
            "Shadows off must ignore fully occluding stale depth");

        renderer._dLight->_position = {-1, 0.7f, 1};
        renderer.Render(0);
        const auto movedLight = CaptureTexture(renderer._viewportTexture.Get());
        Require(DrawnPixels(movedLight) > 100 && movedLight != disabled,
            "Moving the light with shadows off must still update directional lighting");
        ClearShadowDepth(renderer, 1);
        renderer.Render(0);
        Require(CaptureTexture(renderer._viewportTexture.Get()) == movedLight,
            "Shadows off must ignore stale depth after moving the light");

        const auto lightMatrix = renderer._dLight->_lightViewProjMatrix;
        renderer._dLight->_orthoWidth = 10;
        renderer._dLight->_sceneCenter = {0.4f, 0.2f, 0};
        renderer.Render(0);
        Require(std::memcmp(&lightMatrix, &renderer._dLight->_lightViewProjMatrix, sizeof(lightMatrix)) != 0,
            "The test must change the shadow projection");
        Require(CaptureTexture(renderer._viewportTexture.Get()) == movedLight,
            "Shadow projection changes must not affect Main with shadows off");

        renderer._dLight->_position = {1, 1, 1};
        renderer._dLight->_orthoWidth = 7.5f;
        renderer._dLight->_sceneCenter = {0, 0, 0};
        renderer.Render(0);
        Require(CaptureTexture(renderer._viewportTexture.Get()) == disabled,
            "Restoring the light with shadows off must restore full visibility");

        renderer._depthPass->_usePass = true;
        renderer.Render(0);
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
        renderer.Render(0);
        Require(CaptureTexture(renderer._viewportTexture.Get()) == disabled,
            "Disabling an already rendered shadow map must restore full visibility");
        renderer._depthPass->_usePass = true;
        renderer.Render(0);
        Require(CaptureTexture(renderer._viewportTexture.Get()) == enabled,
            "Re-enabling DepthPass must regenerate the same shadows at the unchanged pose");
        std::cout << "PASS: shadow toggle checks (first frame, stale depth, moving light, off/on)\n";
        return enabled;
    }

    void TestTextureUpload(const std::filesystem::path& scene)
    {
        Renderer renderer;
        renderer.InitializeRenderer();
        renderer.InitializeResources(scene);
        CheckGpuDiagnostics();
        renderer.Render(0);
        const auto pixels = CaptureTexture(renderer._viewportTexture.Get());
        Require(DrawnPixels(pixels) > 100, "Copy-queue uploads must be usable by the first graphics draw");

        // The textured quad's center must retain the fixture's red albedo.
        XMFLOAT3 projectedCenter;
        XMStoreFloat3(&projectedCenter, XMVector3TransformCoord(XMVectorSet(-0.9f, 0, 0, 1),
            XMLoadFloat4x4(&renderer._viewProjectionMatrix)));
        const int x = static_cast<int>((projectedCenter.x * 0.5f + 0.5f) * ViewportSize);
        const int y = static_cast<int>((0.5f - projectedCenter.y * 0.5f) * ViewportSize);
        Require(x >= 0 && x < static_cast<int>(ViewportSize) && y >= 0 && y < static_cast<int>(ViewportSize),
            "The uploaded texture's test pixel must be inside the viewport");
        const auto offset = (static_cast<size_t>(y) * ViewportSize + x) * 4;
        Require(pixels[offset] > pixels[offset + 1] + 10 && pixels[offset] > pixels[offset + 2] + 10,
            "The first graphics draw must sample the uploaded red albedo texture");
        renderer.Shutdown();
    }

    void TestRetirement(const std::filesystem::path& scene)
    {
        Renderer renderer;
        renderer.InitializeRenderer();
        auto& graphics = CommandQueueManager::GetCommandQueue(QUEUE_GRAPHICS);
        auto& copy = CommandQueueManager::GetCommandQueue(QUEUE_UPLOAD);
        MSWRL::ComPtr<ID3D12Fence> gate;
        auto* device = D3D12Core::GraphicsDevice::device.Get();
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gate)));
        struct ReleaseGate { ID3D12Fence* fence; ~ReleaseGate() { fence->Signal(1); } } release{gate.Get()};
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
        auto context = std::make_unique<CommandContext>();
        context->InitializeCommandContext(QUEUE_GRAPHICS);
        context->KeepAlive(owner);
        context->KeepAlive(readback);
        context->GetCommandList()->CopyBufferRegion(readback.Get(), 0, upload.Get(), 0, sizeof(uint32_t));
        owner.reset();
        upload.Reset();
        Require(!weak.expired(), "Recorded-but-unsubmitted work must retain its source owner");
        ThrowIfFailed(graphics._commandQueue->Wait(gate.Get(), 1));
        const auto pending = context->Finish(false);
        context.reset(); // Queue also retains the list and allocator; destruction need not wait.
        graphics.CollectCompleted();
        Require(!graphics.IsComplete(pending) && !weak.expired() && graphics.GetPendingSubmissionCount() > 0,
            "Destroying the CPU context must preserve every pending object and owner");
        ThrowIfFailed(gate->Signal(1));
        graphics.WaitCPU(pending);
        Require(weak.expired() && graphics.GetPendingSubmissionCount() == 0, "Completed ownership must be collected");
        ThrowIfFailed(readback->Map(0, nullptr, reinterpret_cast<void**>(&mapped)));
        Require(*mapped == 0xABCDEF42, "GPU copy must survive deletion of its CPU source owner and context");
        const D3D12_RANGE noWrites{0, 0};
        readback->Unmap(0, &noWrites);

        MSWRL::ComPtr<ID3D12Fence> copyGate;
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&copyGate)));
        ReleaseGate releaseCopy{copyGate.Get()};
        ThrowIfFailed(copy._commandQueue->Wait(copyGate.Get(), 1));
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
        ThrowIfFailed(copyGate->Signal(1));
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

        renderer.InitializeResources(scene);
        renderer.Render(0);
        Require(DrawnPixels(CaptureTexture(renderer._viewportTexture.Get())) > 100, "Scene must draw before unload");
        renderer._modelManager.ClearModels();
        renderer.Render(0);
        Require(DrawnPixels(CaptureTexture(renderer._viewportTexture.Get())) == 0, "Clearing model ownership must produce an empty scene safely");
        renderer.Shutdown();
    }

    void TestResourceUses(const std::filesystem::path& scene)
    {
        Renderer renderer;
        renderer.InitializeRenderer();
        renderer.InitializeResources(scene);
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

        for (int cycle = 0; cycle < 2; ++cycle)
        {
            for (unsigned mask = 0; mask < 8; ++mask)
            {
                renderer._mainPass->_usePass = (mask & 1) != 0;
                renderer._depthPass->_usePass = (mask & 2) != 0;
                renderer._bbPass->_usePass = (mask & 4) != 0;
                renderer.Render(0);
                const auto pixels = CaptureTexture(viewport);
                Require((DrawnPixels(pixels) > 0) == ((mask & 5) != 0), "Every pass combination must preserve its color target contract");
                CaptureTexture(renderer._dLight->_directionalShadowMapBuffer.Get());
            }
        }
        renderer.Shutdown();
    }

    void TestTransforms(const std::filesystem::path& scene)
    {
        Renderer renderer;
        renderer.InitializeRenderer();
        renderer.InitializeResources(scene);
        renderer._mainPass->_usePass = renderer._depthPass->_usePass = renderer._bbPass->_usePass = false;
        renderer.Render(0);
        Require(DrawnPixels(CaptureTexture(renderer._viewportTexture.Get())) == 0,
            "All disabled passes must leave the color target clear");

        // No ordinary drawing pass has run: boxes must already have the authored hierarchy pose.
        renderer._bbPass->_usePass = true;
        renderer.Render(0);
        const auto boxesOnly = CaptureTexture(renderer._viewportTexture.Get());
        Require(DrawnPixels(boxesOnly) > 10, "Boxes must have valid transforms before Main or Depth ever draws");
        for (int cycle = 0; cycle < 3; ++cycle)
            for (unsigned mask = 0; mask < 8; ++mask)
            {
                renderer._mainPass->_usePass = (mask & 1) != 0;
                renderer._depthPass->_usePass = (mask & 2) != 0;
                renderer._bbPass->_usePass = (mask & 4) != 0;
                renderer.Render(0);
                const auto pixels = CaptureTexture(renderer._viewportTexture.Get());
                if (mask == 4)
                    Require(pixels == boxesOnly, "Ordinary passes and unchanged frames must preserve the boxes-only pose");
                if (mask == 1)
                    Require(DrawnPixels(pixels) > 100, "Re-enabled Main must draw the authored pose");
                if (mask == 0 || mask == 2)
                    Require(DrawnPixels(pixels) == 0, "All-off and depth-only rendering must leave the color target clear");
            }
        renderer.Shutdown();
    }

    void TestRender(const std::filesystem::path& scene)
    {
        Renderer renderer;
        renderer.InitializeRenderer();
        renderer.InitializeResources(scene);
        const auto mainPixels = TestShadowToggle(renderer);
        WriteCapture(mainPixels, "pbr");
        Require(DrawnPixels(mainPixels) > 100, "GLB geometry/materials must produce visible PBR pixels");
        const auto depth = CaptureTexture(renderer._dLight->_directionalShadowMapBuffer.Get());
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
        renderer.Render(0);
        const auto boxes = CaptureTexture(renderer._viewportTexture.Get());
        WriteCapture(boxes, "bounding_boxes");
        Require(DrawnPixels(boxes) > 10, "The bounding-box pass must draw visible geometry");

        renderer._bbPass->_usePass = false;
        renderer.Render(0);
        const auto empty = CaptureTexture(renderer._viewportTexture.Get());
        Require(DrawnPixels(empty) == 0, "Depth-only rendering must leave the color target at its clear color");

        renderer._mainPass->_usePass = true;
        renderer.Render(0);
        Require(CaptureTexture(renderer._viewportTexture.Get()) == mainPixels,
            "Restoring the main pass at the same pose must restore the same image");
        renderer.Shutdown();
    }
}

    int RunGpuTest(std::string_view test, const std::string& argument, bool hardware)
    {
        try
        {
            ComApartment apartment;
            InitializeGpu(hardware, test == "upload");
            if (test == "constants") TestConstants();
            else if (test == "commands") TestCommandFailures();
            else if (test == "completion") TestCommandCompletion();
            else if (test == "root-failures") TestRootSignatureFailures();
            else if (test == "root-layout") TestRootLayout(argument);
            else if (test == "bounds") TestBounds();
            else if (test == "rtv") TestRtvDescriptors();
            else if (test == "pipeline") TestPipeline(argument);
            else if (test == "upload") TestTextureUpload(argument);
            else if (test == "render") TestRender(argument);
            else if (test == "retirement") TestRetirement(argument);
            else if (test == "resource-uses") TestResourceUses(argument);
            else if (test == "transforms") TestTransforms(argument);
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
