#pragma once

#include "util.h"

#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <cstdint>

using BinaryBlob = std::vector<uint8_t>;

enum class AssetError {
	OK,
	NOT_FOUND,
	READ_ERROR,
	INVALID_DATA
};

Retval<BinaryBlob, AssetError> load_binary_asset(std::filesystem::path path);

Retval<std::string, AssetError> load_text_asset(std::filesystem::path path);

struct Image {

	BinaryBlob data;
	size_t width;
	size_t height;
	size_t channel_count;

};

Retval<Image, AssetError> load_image(std::filesystem::path path);