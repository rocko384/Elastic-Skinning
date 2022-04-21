#pragma once

#include "util.h"
#include "gfxcontext.h"
#include "swapchain.h"
#include "renderingtypes.h"
#include "computepipeline.h"
#include "mesh.h"
#include "elasticskinning.h"

#include <vulkan/vulkan.hpp>

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstdint>

class ElasticFieldComposer {

public:

	ElasticFieldComposer(GfxContext* Context, Swapchain* Swapchain);
	~ElasticFieldComposer();

	void init_render_data(size_t MaxBones, size_t TotalBones, size_t MaxJoints, size_t TotalJoints, vk::Extent3D MaxFieldDims);

	void record_descriptor_sets(MeshId MeshId, float FieldScale, std::vector<GPUTexture>& PartIsogradfields, std::vector<GPUTexture>& OutIsogradfields, std::vector<BufferAllocation>& BoneBuffers, Skeleton* Skeleton);
	void record_command_buffer(Swapchain::FrameId FrameId, vk::CommandBuffer CommandBuffer, MeshId MeshId);

private:

	GfxContext* context{ nullptr };
	Swapchain* swapchain{ nullptr };

	vk::Sampler texture_sampler;

	ElasticSkinning::FieldTxComputePipeline field_tx_pipeline;
	ElasticSkinning::FieldBlendComputePipeline field_blend_pipeline;

	vk::DescriptorPool descriptor_pool;

	vk::Extent3D field_dims;

	struct IntermediateField {
		GPUTexture isogradfield;

		vk::ImageMemoryBarrier isogradfield_read_barrier;
		vk::ImageMemoryBarrier isogradfield_write_barrier;
	};

	struct FrameData {
		std::vector<IntermediateField> tx_intermediates;
		std::unordered_map<MeshId, std::vector<ElasticSkinning::FieldTxContext>> kernel_contexts;
		std::unordered_map<MeshId, std::vector<vk::DescriptorSet>> tx_descriptor_sets;

		std::vector<IntermediateField> blend_intermediates;
		std::unordered_map<MeshId, std::vector<vk::DescriptorSet>> blend_descriptor_sets;
	};

	std::vector<FrameData> frames;

};