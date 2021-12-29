#pragma once

#include "util.h"

#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <cstdint>

enum class AssetError {
	OK,
	NOT_FOUND,
	READ_ERROR
};

Retval<std::vector<uint8_t>, AssetError> load_binary_asset(std::filesystem::path path);

Retval<std::string, AssetError> load_text_asset(std::filesystem::path path);