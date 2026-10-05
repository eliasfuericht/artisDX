#pragma once

#include <optional>

namespace Testing
{
    // A value is the exit code for a handled command; no value starts the editor.
    std::optional<int> HandleCommandLine(int argc, char** argv);
}
