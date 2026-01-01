#pragma once

#include <string>

namespace Platform
{
    // Returns the user data directory for this application, creating it if it doesn't exist.
    // macOS: ~/Library/Application Support/Cambrai/
    // Windows: %APPDATA%/Cambrai/
    // Linux: ~/.local/share/Cambrai/
    std::string getUserDataDirectory();
}
