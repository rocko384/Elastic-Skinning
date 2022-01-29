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

template <typename T>
static const BinaryBlobAccessor<T> tinygltf_accessor_convert(const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
	BinaryBlobAccessor<T> ret;

	const auto& view = model.bufferViews[accessor.bufferView];
	const auto& buffer = model.buffers[view.buffer];

	ret.element_offset = accessor.byteOffset;
	ret.element_count = accessor.count;
	ret.offset = view.byteOffset;
	ret.size = view.byteLength;
	ret.stride = view.byteStride;
	ret.source = &buffer.data;

	return ret;
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
	for (auto& node : model.nodes) {
		if (node.mesh > -1) {
			Mesh outMesh;

			auto& mesh = model.meshes[node.mesh];
			auto& primitive = mesh.primitives[0];

			BinaryBlobAccessor<glm::vec3> position_accessor;
			BinaryBlobAccessor<glm::vec3> normal_accessor;
			BinaryBlobAccessor<glm::vec3> color_accessor;
			BinaryBlobAccessor<glm::vec2> texcoord_accessor;
			std::variant<
				BinaryBlobAccessor<glm::u8vec4>,
				BinaryBlobAccessor<glm::u16vec4>
			> joints_accessor;
			BinaryBlobAccessor<glm::vec4> weights_accessor;

			std::variant<
				BinaryBlobAccessor<uint16_t>,
				BinaryBlobAccessor<uint32_t>
			> index_accessor;

			if (primitive.attributes.contains("POSITION")) {
				int accessor_idx = primitive.attributes["POSITION"];

				position_accessor = tinygltf_accessor_convert<glm::vec3>(model, model.accessors[accessor_idx]);
			}

			if (primitive.attributes.contains("NORMAL")) {
				int accessor_idx = primitive.attributes["NORMAL"];

				normal_accessor = tinygltf_accessor_convert<glm::vec3>(model, model.accessors[accessor_idx]);
			}

			if (primitive.attributes.contains("COLOR_0")) {
				int accessor_idx = primitive.attributes["COLOR_0"];

				color_accessor = tinygltf_accessor_convert<glm::vec3>(model, model.accessors[accessor_idx]);
			}

			if (primitive.attributes.contains("TEXCOORD_0")) {
				int accessor_idx = primitive.attributes["TEXCOORD_0"];

				texcoord_accessor = tinygltf_accessor_convert<glm::vec2>(model, model.accessors[accessor_idx]);
			}

			if (primitive.attributes.contains("JOINTS_0")) {
				int accessor_idx = primitive.attributes["JOINTS_0"];
				auto& accessor = model.accessors[accessor_idx];
				
				switch (accessor.componentType) {
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					joints_accessor = tinygltf_accessor_convert<glm::u8vec4>(model, accessor);
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					joints_accessor = tinygltf_accessor_convert<glm::u16vec4>(model, accessor);
					break;
				}
			}

			if (primitive.attributes.contains("WEIGHTS_0")) {
				int accessor_idx = primitive.attributes["WEIGHTS_0"];

				weights_accessor = tinygltf_accessor_convert<glm::vec4>(model, model.accessors[accessor_idx]);
			}

			// Index data accessor prep.
			// Maybe should be conditionally verified, but I can't imagine
			// using a non-indexed mesh is likely
			{
				int accessor_idx = primitive.indices;
				auto& accessor = model.accessors[accessor_idx];

				switch (accessor.componentType) {
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					index_accessor = tinygltf_accessor_convert<uint16_t>(model, accessor);
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
					index_accessor = tinygltf_accessor_convert<uint32_t>(model, accessor);
					break;
				}
			}

			size_t index_count = std::visit([](auto&& accessor) { return accessor.element_count; }, index_accessor);
			outMesh.vertices.resize(position_accessor.element_count);
			outMesh.indices.resize(index_count);

			for (size_t vertex = 0; vertex < position_accessor.element_count; vertex++) {
				outMesh.vertices[vertex].position = position_accessor[vertex];

				outMesh.vertices[vertex].normal = !normal_accessor.empty() ?
					normal_accessor[vertex] : glm::vec3{ 0.0f, 0.0f, 0.0f };

				outMesh.vertices[vertex].color = !color_accessor.empty() ?
					color_accessor[vertex] : glm::vec3{ 1.0f, 1.0f, 1.0f };

				outMesh.vertices[vertex].texcoords = !texcoord_accessor.empty() ?
					texcoord_accessor[vertex] : glm::vec2{ 0.0f, 0.0f };

				bool is_joints_accessor_empty = std::visit([](auto&& accessor) {
					return accessor.empty();
					}, joints_accessor);

				if (!is_joints_accessor_empty) {
					std::visit([&outMesh, vertex](auto&& accessor) {
						outMesh.vertices[vertex].joints = accessor[vertex];
						}, joints_accessor);
				}
				else {
					outMesh.vertices[vertex].joints = glm::u16vec4{ 0, 0, 0, 0 };
				}

				outMesh.vertices[vertex].weights = !weights_accessor.empty() ?
					weights_accessor[vertex] : glm::vec4{ 0.0f, 0.0f, 0.0f, 0.0f };
			}

			std::visit([&outMesh](auto&& accessor) {
				for (size_t index = 0; index < accessor.element_count; index++) {
					outMesh.indices[index] = accessor[index];
				}
			}, index_accessor);

			outMesh.material_name = model.materials[primitive.material].name;

			ret.meshes.push_back(outMesh);

			// Skeleton data
			if (node.skin > -1) {
				auto& skin = model.skins[node.skin];
				auto matrix_accessor = tinygltf_accessor_convert<glm::mat4>(model, model.accessors[skin.inverseBindMatrices]);

				ret.skeleton.bones.resize(skin.joints.size());
				ret.skeleton.bone_names.resize(skin.joints.size());

				for (size_t i = 0; i < ret.skeleton.bones.size(); i++) {
					auto& joint = model.nodes[skin.joints[i]];

					ret.skeleton.bones[i].inverse_bind_matrix = matrix_accessor[i];

					bool has_rotation = joint.rotation.size() != 0;
					bool has_position = joint.translation.size() != 0;
					bool has_scale = joint.scale.size() != 0;

					ret.skeleton.bones[i].rotation = has_rotation ?
						glm::quat{
							static_cast<float>(joint.rotation[0]),
							static_cast<float>(joint.rotation[1]),
							static_cast<float>(joint.rotation[2]),
							static_cast<float>(joint.rotation[3])
								} : glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };

					ret.skeleton.bones[i].position = has_position ?
						glm::vec3{
							static_cast<float>(joint.translation[0]),
							static_cast<float>(joint.translation[1]),
							static_cast<float>(joint.translation[2])
								} : glm::vec3{ 0.0f, 0.0f, 0.0f };

					ret.skeleton.bones[i].scale = has_scale ?
						glm::vec3{
							static_cast<float>(joint.rotation[0]),
							static_cast<float>(joint.rotation[1]),
							static_cast<float>(joint.rotation[2])
						} : glm::vec3{ 1.0f, 1.0f, 1.0f };

					ret.skeleton.bone_names[i] = CRC::crc64(joint.name);
				}
			}
		}
	}

	// Animation data
	ret.skeleton.animations.resize(model.animations.size());
	ret.skeleton.animation_names.resize(model.animations.size());

	for (size_t i = 0; i < ret.skeleton.animations.size(); i++) {
		auto& animation = model.animations[i];

		std::vector<Channel> outChannels(ret.skeleton.bones.size());
		std::vector<StringHash> outChannelNames(ret.skeleton.bones.size());

		for (size_t channel = 0; channel < outChannelNames.size(); channel++) {
			outChannelNames[channel] = ret.skeleton.bone_names[channel];
		}

		for (size_t channel = 0; channel < outChannels.size(); channel++) {
			auto& targetNode = model.nodes[animation.channels[channel].target_node];
			auto& sampler = animation.samplers[animation.channels[channel].sampler];

			StringHash targetNodeName = CRC::crc64(targetNode.name);

			size_t outChannel = 0;

			for (size_t name = 0; name < outChannelNames.size(); name++) {
				if (outChannelNames[name] == targetNodeName) {
					outChannel = name;
					break;
				}
			}

			auto& inAccessor = model.accessors[sampler.input];
			auto& outAccessor = model.accessors[sampler.output];

			auto timeAccessor = tinygltf_accessor_convert<float>(model, inAccessor);
			auto translationAccessor = tinygltf_accessor_convert<glm::vec3>(model, outAccessor);
			auto rotationAccessor = tinygltf_accessor_convert<glm::quat>(model, outAccessor);
			auto scaleAccessor = tinygltf_accessor_convert<glm::vec3>(model, outAccessor);
			auto weightAccessor = tinygltf_accessor_convert<float>(model, outAccessor);

			if (outChannels[outChannel].time_points.empty()) {
				outChannels[outChannel].time_points.resize(timeAccessor.element_count);
				outChannels[outChannel].keyframes.resize(timeAccessor.element_count);
			}

			const auto& targetPath = animation.channels[channel].target_path;

			for (size_t keyframe = 0; keyframe < timeAccessor.element_count; keyframe++) {
				int64_t timePoint = static_cast<int64_t>(timeAccessor[keyframe] * 1000.0f);
				outChannels[outChannel].time_points[keyframe] = std::chrono::milliseconds(timePoint);

				if (targetPath == "translation") {
					glm::vec3 outPos = translationAccessor[keyframe];
					outChannels[outChannel].keyframes[keyframe].position = outPos;
				}
				else if (targetPath == "rotation") {
					glm::quat outRot = rotationAccessor[keyframe];
					outChannels[outChannel].keyframes[keyframe].rotation = outRot;
				}
				else if (targetPath == "scale") {
					glm::vec3 outScale = scaleAccessor[keyframe];
					outChannels[outChannel].keyframes[keyframe].scale = outScale;

				}
				else if (targetPath == "weights") {
					float outWeight = weightAccessor[keyframe];
					outChannels[outChannel].keyframes[keyframe].weight = outWeight;
				}
			}
		}

		for (size_t channel = 0; channel < outChannels.size(); channel++) {
			ret.skeleton.animations[i].add_channel(outChannels[channel], outChannelNames[channel]);
		}

		ret.skeleton.animation_names[i] = CRC::crc64(animation.name);
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
		ret.materials[i].pipeline_name = "base"; /// TODO: Figure out a decent way of deducing / specifying this
	}

	return {
		ret,
		AssetError::OK
	};

}