#include "asset.h"

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