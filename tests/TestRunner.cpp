#include "TestCases.h"
#include "TestSupport.h"

int Testing::RunTestCase(int argc, char** argv)
{
    try
    {
        Require(argc >= 2, "Usage: artisDX.exe -test-case camera|tangents|shader|shaders|shader-failures|root-failures|root-layout|constants|commands|completion|bounds|rtv|pipeline|upload|render|retirement|passes [arguments] [--hardware]");
        const std::string_view test(argv[1]);
        if (test == "camera" || test == "tangents")
        {
            Require(argc == 2, "CPU tests take no additional arguments");
            if (test == "camera") TestCamera();
            else TestTangents();
        }
        else if (test == "shader-failures")
        {
            Require(argc == 3, "shader-failures requires a fixture directory");
            TestShaderFailures(argv[2]);
        }
        else if (test == "shader")
        {
            Require(argc == 4, "shader requires a path and debug|optimized");
            const std::string_view mode(argv[3]);
            Require(mode == "debug" || mode == "optimized", "Unknown shader mode");
            TestShader(argv[2], mode == "optimized");
        }
        else if (test == "shaders")
        {
            Require(argc >= 4, "shaders requires debug|optimized followed by shader paths");
            const std::string_view mode(argv[2]);
            Require(mode == "debug" || mode == "optimized", "Unknown shader mode");
            std::vector<std::filesystem::path> paths;
            for (int i = 3; i < argc; ++i) paths.emplace_back(argv[i]);
            TestShaders(paths, mode == "optimized");
        }
        else
        {
            const bool needsArgument = test == "pipeline" || test == "upload" || test == "render" || test == "root-layout" || test == "retirement" || test == "passes";
            Require(needsArgument || test == "constants" || test == "bounds" || test == "rtv" || test == "commands" || test == "completion" || test == "root-failures", "Unknown test: " + std::string(test));
            const bool hardware = std::string_view(argv[argc - 1]) == "--hardware";
            Require(argc == 2 + static_cast<int>(needsArgument) + static_cast<int>(hardware),
                "GPU tests take a pass/scene argument where required, followed by optional --hardware");
            const int result = RunGpuTest(test, needsArgument ? argv[2] : "", hardware);
            if (result != 0)
                return result;
        }
        std::cout << "PASS: " << test << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
