#pragma once

#include "util.h"
#include "renderingtypes.h"
#include "model.h"

#include <string>
#include <filesystem>
#include <cstdint>


enum class AssetError {
	OK,
	NOT_FOUND,
	INCORRECT_FILE_FORMAT,
	READ_ERROR,
	INVALID_DATA
};

Retval<BinaryBlob, AssetError> load_binary_asset(std::filesystem::path path);

Retval<std::string, AssetError> load_text_asset(std::filesystem::path path);

Retval<Image, AssetError> load_image(std::filesystem::path path);
Retval<Image, AssetError> load_image(const BinaryBlob& data);

Retval<Model, AssetError> load_model(std::filesystem::path path);