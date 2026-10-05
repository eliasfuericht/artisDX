#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace Testing
{
    inline constexpr int SkipExitCode = 77;

    int RunTestCase(int argc, char** argv);

    void TestCamera();
    void TestTangents();
    void TestShader(const std::filesystem::path& path, bool optimized);
    int RunGpuTest(std::string_view test, const std::string& argument, bool hardware);
}
