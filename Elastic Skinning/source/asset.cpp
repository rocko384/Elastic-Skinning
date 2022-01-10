#include "asset.h"

#include <stb_image.h>

#include <fstream>

Retval<BinaryBlob, AssetError> load_binary_asset(std::filesystem::path path) {
	BinaryBlob ret{};

	if (!std::filesystem::exists(path)) {
		return { ret, AssetError::NOT_FOUND };
	}

	std::ifstream file(path, std::ios::binary);

	if (!file.is_open()) {
		return { ret, AssetError::READ_ERROR };
	}

	size_t fileSize = std::filesystem::file_size(path);

	ret.resize(fileSize);

	try {
		file.read(reinterpret_cast<char*>(ret.data()), fileSize);
	}
	catch (std::exception e) {
		ret.clear();
		LOG_ERROR("File read error:\n\tPath %s\n\t%s", path.string().c_str(), e.what());
		return { ret, AssetError::READ_ERROR };
	}

	return { ret, AssetError::OK };
}

Retval<std::string, AssetError> load_text_asset(std::filesystem::path path) {
	std::string ret{};

	if (!std::filesystem::exists(path)) {
		return { ret, AssetError::NOT_FOUND };
	}

	std::ifstream file(path);

	if (!file.is_open()) {
		return { ret, AssetError::READ_ERROR };
	}

	size_t fileSize = std::filesystem::file_size(path);

	ret.resize(fileSize);

	try {
		file.read(ret.data(), fileSize);
	}
	catch (std::exception e) {
		ret.clear();
		LOG_ERROR("File read error:\n\tPath %s\n\t%s", path.string().c_str(), e.what());
		return { ret, AssetError::READ_ERROR };
	}

	return { ret, AssetError::OK };
}

Retval<Image, AssetError> load_image(std::filesystem::path path) {

	auto fileData = load_binary_asset(path);

	if (fileData.status != AssetError::OK) {
		return { {}, fileData.status };
	}

	int width, height, channels;
	const stbi_uc* data = reinterpret_cast<const stbi_uc*>(fileData.value.data());
	int length = static_cast<int>(fileData.value.size());
	stbi_uc* pixels = stbi_load_from_memory(data, length, &width, &height, &channels, STBI_rgb_alpha);

	if (!pixels) {
		return { {}, AssetError::INVALID_DATA };
	}

	const uint8_t* outData = reinterpret_cast<const uint8_t*>(pixels);
	auto outBlob = BinaryBlob(outData, outData + (width * height * 4));

	stbi_image_free(pixels);

	return {
		{
			outBlob,
			static_cast<size_t>(width),
			static_cast<size_t>(height),
			static_cast<size_t>(channels)
		},
		AssetError::OK
	};
}