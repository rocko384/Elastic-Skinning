#pragma once

#include "util.h"
#include "mesh.h"
#include "skeleton.h"
#include "computepipeline.h"

#include <unordered_map>
#include <vector>

using BoneBuffer = Compute::StorageBuffer<Bone, 0>;

namespace ElasticSkinning {

	using CurrentIsogradfieldSampler = Compute::ImageSampler<3>;

	struct SkinningContext {
		uint32_t vertex_count;
		uint32_t bone_count;
		float field_scale;
		alignas(8) glm::ivec3 field_dims;
	};

	using SkinningComputePipeline = ComputePipeline<SkinningContext, VertexBuffer, ElasticVertexBuffer, BoneBuffer, CurrentIsogradfieldSampler>;

	using IsogradfieldSourceBuffer = Compute::ImageSampler<1>;
	using IsogradfieldABuffer = Compute::StorageImage<1>;
	using IsogradfieldBBuffer = Compute::StorageImage<2>;
	using IsogradfieldOutBuffer = Compute::StorageImage<3>;

	struct FieldTxContext {
		uint32_t bone_idx;
		float scale;
	};

	struct FieldBlendContext {

	};

	using FieldTxComputePipeline = ComputePipeline<FieldTxContext, BoneBuffer, IsogradfieldSourceBuffer, IsogradfieldOutBuffer>;
	using FieldBlendComputePipeline = ComputePipeline<FieldBlendContext, IsogradfieldABuffer, IsogradfieldBBuffer, IsogradfieldOutBuffer>;

	template<typename T, size_t W = 32, size_t H = 32, size_t D = 32>
	struct ValueField3D {
		static const size_t Width = W;
		static const size_t Height = H;
		static const size_t Depth = D;

		ValueField3D() {
			values.resize(Width * Height * Depth);
		}

		T& valref(size_t x, size_t y, size_t z) {
			return values[(z * Width * Height) + (y * Width) + x];
		}

		T value(size_t x, size_t y, size_t z) const {
			return values[(z * Width * Height) + (y * Width) + x];
		}

		std::vector<T> values;
	};

	using ScalarField3D = ValueField3D<float>;
	using VectorField3D = ValueField3D<glm::vec3>;
	using ScalarVectorField3D = ValueField3D<glm::vec4>;

	inline ScalarVectorField3D combine_fields(const ScalarField3D& isofield, const VectorField3D& gradient_field) {
		ScalarVectorField3D ret;

		for (size_t x = 0; x < ret.Width; x++) {
			for (size_t y = 0; y < ret.Height; y++) {
				for (size_t z = 0; z < ret.Depth; z++) {
					float isovalue = isofield.value(x, y, z);
					glm::vec3 gradient = gradient_field.value(x, y, z);
					ret.valref(x, y, z) = glm::vec4(isovalue, gradient);
				}
			}
		}

		return ret;
	}

	struct HRBFData {
		static const size_t Width = ScalarField3D::Width;
		static const size_t Height = ScalarField3D::Height;
		static const size_t Depth = ScalarField3D::Depth;

		float Scale{ 1.0f };

		ScalarField3D isofield;
		VectorField3D gradients;

		float sample_isofield(float x, float y, float z) {
			float halfW = static_cast<float>(Width - 1) / 2.0f;
			float halfH = static_cast<float>(Height - 1) / 2.0f;
			float halfD = static_cast<float>(Depth - 1) / 2.0f;

			float graphX = (x * (halfW / Scale)) + halfW;
			float graphY = (y * (halfH / Scale)) + halfH;
			float graphZ = (z * (halfD / Scale)) + halfD;

			float minX = std::floor(graphX);
			float minY = std::floor(graphY);
			float minZ = std::floor(graphZ);

			float maxX = std::ceil(graphX);
			float maxY = std::ceil(graphY);
			float maxZ = std::ceil(graphZ);

			float interpX = (graphX - minX) / (maxX - minX);
			float interpY = (graphY - minY) / (maxY - minY);
			float interpZ = (graphZ - minZ) / (maxZ - minZ);

			float c000 = isofield.value(minX, minY, minZ);
			float c100 = isofield.value(maxX, minY, minZ);
			float c010 = isofield.value(minX, maxY, minZ);
			float c110 = isofield.value(maxX, maxY, minZ);
			float c001 = isofield.value(minX, minY, maxZ);
			float c101 = isofield.value(maxX, minY, maxZ);
			float c011 = isofield.value(minX, maxY, maxZ);
			float c111 = isofield.value(maxX, maxY, maxZ);

			float c00 = std::lerp(c000, c100, interpX);
			float c10 = std::lerp(c010, c110, interpX);
			float c01 = std::lerp(c001, c101, interpX);
			float c11 = std::lerp(c011, c111, interpX);

			float c0 = std::lerp(c00, c10, interpY);
			float c1 = std::lerp(c01, c11, interpY);

			return std::lerp(c0, c1, interpZ);
		}

		std::vector<glm::vec3> centers;
		std::vector<glm::vec4> constants;
	};

	struct MeshPart {
		SkeletalMesh mesh;

		struct {
			glm::vec3 head;
			glm::vec3 tail;

			StringHash parent;
			std::vector<StringHash> children;
		} bone;
	};

	void create_debug_csv(const HRBFData& hrbf, const std::string& filename);
	void create_debug_csv(const std::unordered_map<StringHash, HRBFData>& hrbfs, const std::string& meshname = "mesh");

	std::unordered_map<StringHash, MeshPart> partition_skeletal_mesh(const SkeletalMesh& mesh, Skeleton& skeleton);

	std::unordered_map<StringHash, HRBFData> create_hrbf_data(const std::unordered_map<StringHash, MeshPart>& mesh_partitions);

	HRBFData compose_hrbfs(const std::unordered_map<StringHash, HRBFData>& hrbfs, const std::unordered_map<StringHash, MeshPart>& mesh_partitions);

	struct MeshAndField {
		ElasticMesh mesh;
		HRBFData rest_field;
		std::unordered_map<StringHash, HRBFData> part_fields;
	};

	MeshAndField convert_skeletal_mesh(const SkeletalMesh& mesh, Skeleton& skeleton);
}
