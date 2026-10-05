#include "TestCases.h"
#include "TestSupport.h"
#include "Renderer.h"

#include <array>
#include <cstring>

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

    void InitializeGpu(bool hardware)
    {
        MSWRL::ComPtr<ID3D12Debug> debug;
        if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
            throw GpuUnavailable("D3D12 debug layer unavailable. Install Windows Graphics Tools and rerun the GPU tests.");
        debug->EnableDebugLayer();
        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&D3D12Core::GraphicsDevice::factory)));
        if (hardware)
        {
            D3D12Core::GraphicsDevice::InitializeDevice();
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

    void CheckGpuDiagnostics()
    {
        ThrowIfFailed(D3D12Core::GraphicsDevice::device->GetDeviceRemovedReason(), "GPU device was removed");
        bool errors = false;
        for (uint64_t i = 0; i < infoQueue->GetNumStoredMessages(); ++i)
        {
            SIZE_T size = 0;
            ThrowIfFailed(infoQueue->GetMessage(i, nullptr, &size));
            std::vector<uint8_t> storage(size);
            auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
            ThrowIfFailed(infoQueue->GetMessage(i, message, &size));
            if (message->Severity <= D3D12_MESSAGE_SEVERITY_WARNING)
                std::cerr << "D3D12 [" << message->ID << "]: " << message->pDescription << '\n';
            errors |= message->Severity <= D3D12_MESSAGE_SEVERITY_ERROR;
        }
        Require(!errors, "D3D12 debug layer reported errors; see the messages above");
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
        renderer.CreateConstantBuffers();
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
            InitializeGpu(hardware);
            if (test == "constants") TestConstants();
            else if (test == "bounds") TestBounds();
            else if (test == "rtv") TestRtvDescriptors();
            else if (test == "pipeline") TestPipeline(argument);
            else if (test == "render") TestRender(argument);
            else throw std::runtime_error("Unknown GPU test: " + std::string(test));
            CheckGpuDiagnostics();
            return 0;
        }
        catch (const GpuUnavailable& error)
        {
            std::cout << "SKIP: " << error.what() << '\n';
            return SkipExitCode;
        }
    }
}
