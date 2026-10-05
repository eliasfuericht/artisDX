#pragma once

#include "pch.h"
#include <cmath>
#include <stdexcept>
#include <string_view>

namespace Testing
{
    inline void Require(bool condition, std::string_view message)
    {
        if (!condition)
            throw std::runtime_error(std::string(message));
    }

    template<typename Action>
    void ExpectFailure(Action action, std::string_view diagnostic)
    {
        try { action(); }
        catch (const std::runtime_error& error)
        {
            Require(std::string_view(error.what()).find(diagnostic) != std::string_view::npos,
                "Failure must identify " + std::string(diagnostic) + ": " + error.what());
            return;
        }
        Require(false, "Expected failure identifying " + std::string(diagnostic));
    }

    inline void Near(float actual, float expected, const std::string& message)
    {
        Require(std::isfinite(actual) && std::abs(actual - expected) < 0.0001f,
            message + " (expected " + std::to_string(expected) + ", got " + std::to_string(actual) + ")");
    }

    inline void Near(const XMFLOAT3& actual, const XMFLOAT3& expected, const std::string& message)
    {
        Near(actual.x, expected.x, message + ".x");
        Near(actual.y, expected.y, message + ".y");
        Near(actual.z, expected.z, message + ".z");
    }

    inline std::vector<Vertex> Triangle(bool mirrored)
    {
        std::vector<Vertex> vertices(3);
        vertices[0].position = {0, 0, 0};
        vertices[1].position = {1, 0, 0};
        vertices[2].position = {0, 1, 0};
        vertices[0].uv = {0, 0};
        vertices[1].uv = {1, 0};
        vertices[2].uv = {0, mirrored ? -1.0f : 1.0f};
        for (auto& vertex : vertices)
            vertex.normal = {0, 0, 1};
        return vertices;
    }
}
