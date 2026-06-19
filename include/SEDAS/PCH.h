#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <toml++/toml.hpp>

#include <spdlog/sinks/basic_file_sink.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace logger = SKSE::log;
using namespace std::literals;
