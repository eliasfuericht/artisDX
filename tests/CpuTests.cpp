#include "TestCases.h"
#include "TestSupport.h"
#include "Camera.h"
#include "GLTFLoader.h"

#include <array>

namespace Testing
{
namespace
{
    Camera MakeCamera()
    {
        Camera camera(XMVectorSet(0, 0, 5, 0), XMVectorSet(0, 1, 0, 0), 90, 0, 2.5f, 0.1f);
        camera.Update();
        return camera;
    }
}

    void TestCamera()
    {
        auto camera = MakeCamera();
        std::array<bool, 1024> keys{};
        camera.ConsumeKey(keys.data(), 1.0f);
        XMFLOAT3 position;
        XMStoreFloat3(&position, camera._position);
        Near(position, {0, 0, 5}, "No input must leave the camera stationary");

        keys[KEYCODE_W] = true;
        camera.ConsumeKey(keys.data(), 0.4f);
        XMStoreFloat3(&position, camera._position);
        Near(position, {0, 0, 4}, "Forward movement must use speed times delta time");
        keys[KEYCODE_W] = false;
        keys[KEYCODE_S] = true;
        camera.ConsumeKey(keys.data(), 0.4f);
        XMStoreFloat3(&position, camera._position);
        Near(position, {0, 0, 5}, "Backward movement must undo forward movement");

        keys.fill(false);
        keys[KEYCODE_A] = true;
        camera.ConsumeKey(keys.data(), 0.4f);
        XMStoreFloat3(&position, camera._position);
        Near(position, {1, 0, 5}, "Left movement must follow the initial camera's screen-left direction");
        keys[KEYCODE_A] = false;
        keys[KEYCODE_D] = true;
        camera.ConsumeKey(keys.data(), 0.4f);
        XMStoreFloat3(&position, camera._position);
        Near(position, {0, 0, 5}, "Right movement must undo left movement");

        auto boosted = MakeCamera();
        keys.fill(false);
        keys[KEYCODE_W] = keys[KEYCODE_SHIFT] = true;
        boosted.ConsumeKey(keys.data(), 0.4f);
        XMStoreFloat3(&position, boosted._position);
        Near(position, {0, 0, -10}, "Shift movement must apply the configured 15x multiplier");

        auto split = MakeCamera();
        keys[KEYCODE_SHIFT] = false;
        split.ConsumeKey(keys.data(), 0.2f);
        split.ConsumeKey(keys.data(), 0.2f);
        XMStoreFloat3(&position, split._position);
        Near(position, {0, 0, 4}, "Movement distance must be independent of frame subdivision");

        keys.fill(false);
        keys[KEYCODE_SPACE] = true;
        camera.ConsumeKey(keys.data(), 0.4f);
        XMStoreFloat3(&position, camera._position);
        Near(position, {0, 1, 5}, "Space must move upward");
        keys[KEYCODE_SPACE] = false;
        keys[KEYCODE_LCTRL] = true;
        camera.ConsumeKey(keys.data(), 0.4f);
        XMStoreFloat3(&position, camera._position);
        Near(position, {0, 0, 5}, "Control must move downward");

        const auto view = camera.GetViewMatrix();
        XMFLOAT3 eyeInView;
        XMStoreFloat3(&eyeInView, XMVector3TransformCoord(camera._position, XMLoadFloat4x4(&view)));
        Near(eyeInView, {0, 0, 0}, "The view matrix must put the camera at the origin");
        camera.ConsumeMouse(100, 10000);
        Near(camera._yaw, 100, "Mouse movement must update yaw using turn speed");
        Near(camera._pitch, 89, "Pitch must clamp before the upper pole");
        camera.ConsumeMouse(0, -20000);
        Near(camera._pitch, -89, "Pitch must clamp before the lower pole");
        camera.Update();
        const auto clampedView = camera.GetViewMatrix();
        for (const auto& row : clampedView.m)
            for (float value : row)
                Require(std::isfinite(value), "The view matrix must remain finite at the pitch limit");
    }

    void TestTangents()
    {
        for (bool mirrored : {false, true})
        {
            auto vertices = Triangle(mirrored);
            GLTFLoader::GenerateTangents(vertices, {0, 1, 2});
            GLTFLoader::GenerateBiTangents(vertices);
            for (const auto& vertex : vertices)
            {
                Near(XMFLOAT3(vertex.tangent.x, vertex.tangent.y, vertex.tangent.z), {1, 0, 0},
                    "A planar UV triangle must have a unit tangent along X");
                Near(vertex.tangent.w, mirrored ? -1.0f : 1.0f, "Mirrored UVs must preserve tangent handedness");
                Near(vertex.bitangent, {0, mirrored ? -1.0f : 1.0f, 0},
                    "Normal mapping must use the matching bitangent");
            }
        }
    }
}
