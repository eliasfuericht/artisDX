#include "TestLauncher.h"
#include "pch.h"
#if defined(ARTISDX_TESTING_ENABLED)
#include "TestCases.h"
#endif

#include <string_view>

namespace Testing
{
namespace
{
    int RunTests()
    {
#if defined(ARTISDX_TESTING_ENABLED)
        // CTest retains process isolation, timeouts, and the existing test summary.
        std::wstring commandLine = L"\"" ARTISDX_CTEST_PATH L"\" --test-dir \"" ARTISDX_TEST_BUILD_DIR
            L"\" -C \"" ARTISDX_TEST_CONFIG L"\" --output-on-failure --no-tests=error --interactive-debug-mode 0";
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        PROCESS_INFORMATION process{};
        std::cout << "Running artisDX regression tests (" << std::filesystem::path(ARTISDX_TEST_CONFIG).string()
            << ")...\n" << std::flush;
        if (!CreateProcessW(ARTISDX_CTEST_PATH, commandLine.data(), nullptr, nullptr, true,
            CREATE_NO_WINDOW, nullptr, ARTISDX_TEST_BUILD_DIR, &startup, &process))
        {
            std::cerr << "Could not start the test runner (Windows error " << GetLastError() << ").\n";
            return 1;
        }
        CloseHandle(process.hThread);
        const auto waitResult = WaitForSingleObject(process.hProcess, INFINITE);
        DWORD exitCode = 1;
        if (waitResult != WAIT_OBJECT_0 || !GetExitCodeProcess(process.hProcess, &exitCode))
        {
            std::cerr << "Could not retrieve the test result (Windows error " << GetLastError() << ").\n";
            exitCode = 1;
        }
        CloseHandle(process.hProcess);
        return static_cast<int>(exitCode);
#else
        std::cerr << "Tests are disabled in this build. Configure with BUILD_TESTING=ON and rebuild artisDX.\n";
        return 1;
#endif
    }
}

    std::optional<int> HandleCommandLine(int argc, char** argv)
    {
        if (argc == 1)
            return std::nullopt;
        if (argc == 2 && std::string_view(argv[1]) == "-test")
            return RunTests();
#if defined(ARTISDX_TESTING_ENABLED)
        // CTest launches a fresh copy of this executable for each case.
        if (argc >= 3 && std::string_view(argv[1]) == "-test-case")
            return RunTestCase(argc - 1, argv + 1);
#endif
        std::cerr << "Usage: artisDX.exe [-test]\n";
        return 2;
    }
}
