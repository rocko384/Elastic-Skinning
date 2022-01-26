#include "asset.h"

#include <stb_image.h>
#include <tiny_gltf.h>

#include <fstream>
#include <variant>

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

	return load_image(fileData.value);
}

Retval<Image, AssetError> load_image(const BinaryBlob& data) {

	int width, height, channels;
	const stbi_uc* rawData = reinterpret_cast<const stbi_uc*>(data.data());
	int length = static_cast<int>(data.size());
	stbi_uc* pixels = stbi_load_from_memory(rawData, length, &width, &height, &channels, STBI_rgb_alpha);

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


static const Sampler tinygltf_sampler_convert(const tinygltf::Sampler& s) {
	Sampler::Filter magFilter;
	Sampler::Filter minFilter;
	Sampler::Wrap wrapS;
	Sampler::Wrap wrapT;

	switch (s.magFilter) {
	case TINYGLTF_TEXTURE_FILTER_NEAREST: magFilter = Sampler::Filter::Nearest;
		break;
	case TINYGLTF_TEXTURE_FILTER_LINEAR: magFilter = Sampler::Filter::Linear;
		break;
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST: magFilter = Sampler::Filter::NearestMipmapNearest;
		break;
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST: magFilter = Sampler::Filter::LinearMipmapNearest;
		break;
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR: magFilter = Sampler::Filter::NearestMipmapLinear;
		break;
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR: magFilter = Sampler::Filter::LinearMipmapLinear;
		break;
	}

	switch (s.minFilter) {
	case TINYGLTF_TEXTURE_FILTER_NEAREST: minFilter = Sampler::Filter::Nearest;
		break;
	case TINYGLTF_TEXTURE_FILTER_LINEAR: minFilter = Sampler::Filter::Linear;
		break;
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST: minFilter = Sampler::Filter::NearestMipmapNearest;
		break;
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST: minFilter = Sampler::Filter::LinearMipmapNearest;
		break;
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR: minFilter = Sampler::Filter::NearestMipmapLinear;
		break;
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR: minFilter = Sampler::Filter::LinearMipmapLinear;
		break;
	}

	switch (s.wrapS) {
	case TINYGLTF_TEXTURE_WRAP_REPEAT: wrapS = Sampler::Wrap::Repeat;
		break;
	case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: wrapS = Sampler::Wrap::ClampToEdge;
		break;
	case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: wrapS = Sampler::Wrap::Repeat;
		break;
	}

	switch (s.wrapT) {
	case TINYGLTF_TEXTURE_WRAP_REPEAT: wrapT = Sampler::Wrap::Repeat;
		break;
	case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: wrapT = Sampler::Wrap::ClampToEdge;
		break;
	case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: wrapT = Sampler::Wrap::Repeat;
		break;
	}

	return {
		magFilter,
		minFilter,
		wrapS,
		wrapT
	};
}

static const Image tinygltf_image_convert(const tinygltf::Image& i) {
	return {
		i.image,
		static_cast<size_t>(i.width),
		static_cast<size_t>(i.height),
		static_cast<size_t>(i.component)
	};
}

Retval<Model, AssetError> load_model(std::filesystem::path path) {

	Model ret;

	tinygltf::TinyGLTF loader;

	tinygltf::Model model;
	std::string error;
	std::string warning;

	if (path.extension() == ".glb") {
		loader.LoadBinaryFromFile(&model, &error, &warning, path.string());
	}
	else if (path.extension() == ".gltf") {
		loader.LoadASCIIFromFile(&model, &error, &warning, path.string());
	}
	else {
		return { {}, AssetError::INCORRECT_FILE_FORMAT };
	}

	if (!error.empty() || !warning.empty()) {
		return { {}, AssetError::READ_ERROR };
	}

	// Mesh data
	size_t total_vertices = 0;
	size_t total_indices = 0;

	for (auto& mesh : model.meshes) {
		int pos_accessor_index = mesh.primitives[0].attributes["POSITION"];
		int index_accessor_index = model.meshes[0].primitives[0].indices;

		total_vertices += model.accessors[pos_accessor_index].count;
		total_indices += model.accessors[index_accessor_index].count;
	}

	ret.meshes.resize(model.meshes.size());
	ret.vertices.resize(total_vertices);
	ret.indices.resize(total_indices);

	size_t vertex_offset = 0;
	size_t index_offset = 0;

	for (size_t i = 0; i < ret.meshes.size(); i++) {
		auto& primitive = model.meshes[i].primitives[0];

		BinaryBlobAccessor<glm::vec3> position_accessor;
		BinaryBlobAccessor<glm::vec3> normal_accessor;
		BinaryBlobAccessor<glm::vec3> color_accessor;
		BinaryBlobAccessor<glm::vec2> texcoord_accessor;

		std::variant<
			BinaryBlobAccessor<uint16_t>,
			BinaryBlobAccessor<uint32_t>
		> index_accessor;

		if (primitive.attributes.contains("POSITION")) {
			int accessor_idx = primitive.attributes["POSITION"];
			int view_idx = model.accessors[accessor_idx].bufferView;
			int buffer_idx = model.bufferViews[view_idx].buffer;

			position_accessor.element_offset = model.accessors[accessor_idx].byteOffset;
			position_accessor.element_count = model.accessors[accessor_idx].count;
			position_accessor.offset = model.bufferViews[view_idx].byteOffset;
			position_accessor.size = model.bufferViews[view_idx].byteLength;
			position_accessor.stride = model.bufferViews[view_idx].byteStride;
			position_accessor.source = &model.buffers[buffer_idx].data;
		}

		if (primitive.attributes.contains("NORMAL")) {
			int accessor_idx = primitive.attributes["NORMAL"];
			int view_idx = model.accessors[accessor_idx].bufferView;
			int buffer_idx = model.bufferViews[view_idx].buffer;

			normal_accessor.element_offset = model.accessors[accessor_idx].byteOffset;
			normal_accessor.element_count = model.accessors[accessor_idx].count;
			normal_accessor.offset = model.bufferViews[view_idx].byteOffset;
			normal_accessor.size = model.bufferViews[view_idx].byteLength;
			normal_accessor.stride = model.bufferViews[view_idx].byteStride;
			normal_accessor.source = &model.buffers[buffer_idx].data;
		}

		if (primitive.attributes.contains("COLOR_0")) {
			int accessor_idx = primitive.attributes["COLOR_0"];
			int view_idx = model.accessors[accessor_idx].bufferView;
			int buffer_idx = model.bufferViews[view_idx].buffer;

			color_accessor.element_offset = model.accessors[accessor_idx].byteOffset;
			color_accessor.element_count = model.accessors[accessor_idx].count;
			color_accessor.offset = model.bufferViews[view_idx].byteOffset;
			color_accessor.size = model.bufferViews[view_idx].byteLength;
			color_accessor.stride = model.bufferViews[view_idx].byteStride;
			color_accessor.source = &model.buffers[buffer_idx].data;
		}

		if (primitive.attributes.contains("TEXCOORD_0")) {
			int accessor_idx = primitive.attributes["TEXCOORD_0"];
			int view_idx = model.accessors[accessor_idx].bufferView;
			int buffer_idx = model.bufferViews[view_idx].buffer;

			texcoord_accessor.element_offset = model.accessors[accessor_idx].byteOffset;
			texcoord_accessor.element_count = model.accessors[accessor_idx].count;
			texcoord_accessor.offset = model.bufferViews[view_idx].byteOffset;
			texcoord_accessor.size = model.bufferViews[view_idx].byteLength;
			texcoord_accessor.stride = model.bufferViews[view_idx].byteStride;
			texcoord_accessor.source = &model.buffers[buffer_idx].data;
		}

		// Index data accessor prep.
		// Maybe should be conditionally verified, but I can't imagine
		// using a non-indexed mesh is likely
		{
			int accessor_idx = primitive.indices;
			int view_idx = model.accessors[accessor_idx].bufferView;
			int buffer_idx = model.bufferViews[view_idx].buffer;

			switch (model.accessors[accessor_idx].componentType) {
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: index_accessor = BinaryBlobAccessor<uint16_t>();
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: index_accessor = BinaryBlobAccessor<uint32_t>();
				break;
			}

			std::visit([&model, &accessor_idx, &view_idx, &buffer_idx](auto&& accessor) {
				accessor.element_offset = model.accessors[accessor_idx].byteOffset;
				accessor.element_count = model.accessors[accessor_idx].count;
				accessor.offset = model.bufferViews[view_idx].byteOffset;
				accessor.size = model.bufferViews[view_idx].byteLength;
				accessor.stride = model.bufferViews[view_idx].byteStride;
				accessor.source = &model.buffers[buffer_idx].data;
			}, index_accessor);
		}

		for (size_t vertex = 0; vertex < position_accessor.element_count; vertex++) {
			ret.vertices[vertex + vertex_offset].position = position_accessor[vertex];

			ret.vertices[vertex + vertex_offset].normal = !normal_accessor.empty() ?
				normal_accessor[vertex] : glm::vec3{ 0.0f, 0.0f, 0.0f };

			ret.vertices[vertex + vertex_offset].color = !color_accessor.empty() ?
				color_accessor[vertex] : glm::vec3{ 1.0f, 1.0f, 1.0f };

			ret.vertices[vertex + vertex_offset].texcoords = !texcoord_accessor.empty() ?
				texcoord_accessor[vertex] : glm::vec2{ 0.0f, 0.0f };
		}

		std::visit([&ret, &index_offset, &vertex_offset](auto&& accessor) {
			for (size_t index = 0; index < accessor.element_count; index++) {
				ret.indices[index + index_offset] = accessor[index] + vertex_offset;
			}
		}, index_accessor);

		size_t index_count = std::visit([](auto&& accessor) { return accessor.element_count; }, index_accessor);

		ret.meshes[i].vertices = std::span{
			std::next(ret.vertices.begin(), vertex_offset),
			std::next(ret.vertices.begin(), vertex_offset + position_accessor.element_count)
		};

		ret.meshes[i].indices = std::span{
			std::next(ret.indices.begin(), index_offset),
			std::next(ret.indices.begin(), index_offset + index_count)
		};

		ret.meshes[i].material_name = model.materials[primitive.material].name;

		vertex_offset += position_accessor.element_count;
		index_offset += index_count;
	}

	// Texture/Material data
	ret.materials.resize(model.materials.size());

	for (size_t i = 0; i < ret.materials.size(); i++) {
		auto& material = model.materials[i];
		int albedo_index = material.pbrMetallicRoughness.baseColorTexture.index;
		int normal_index = material.normalTexture.index;
		int metallic_roughness_index = material.pbrMetallicRoughness.metallicRoughnessTexture.index;

		ret.materials[i].albedo = (albedo_index > -1) ?
			std::optional<Texture>({
					tinygltf_image_convert(model.images[model.textures[albedo_index].source]),
					tinygltf_sampler_convert(model.samplers[model.textures[albedo_index].sampler])
				}) : std::nullopt;
		ret.materials[i].normal = (normal_index > -1) ?
			std::optional<Texture>({
				tinygltf_image_convert(model.images[model.textures[normal_index].source]),
					tinygltf_sampler_convert(model.samplers[model.textures[normal_index].sampler])
				}) : std::nullopt;
		ret.materials[i].metallic_roughness = (metallic_roughness_index > -1) ?
			std::optional<Texture>({
				tinygltf_image_convert(model.images[model.textures[metallic_roughness_index].source]),
					tinygltf_sampler_convert(model.samplers[model.textures[metallic_roughness_index].sampler])
				}) : std::nullopt;

		ret.materials[i].albedo_factor = {
			static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[0]),
			static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[1]),
			static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[2]),
			static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[3])
		};

		ret.materials[i].metallic_factor = static_cast<float>(material.pbrMetallicRoughness.metallicFactor);
		ret.materials[i].roughness_factor = static_cast<float>(material.pbrMetallicRoughness.roughnessFactor);

		ret.materials[i].name = material.name;
	}

	return {
		ret,
		AssetError::OK
	};

}