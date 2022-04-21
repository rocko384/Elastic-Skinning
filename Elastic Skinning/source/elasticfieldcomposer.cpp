#include "elasticfieldcomposer.h"

#include <algorithm>

ElasticFieldComposer::ElasticFieldComposer(GfxContext* Context, Swapchain* Swapchain) {
	context = Context;
	swapchain = Swapchain;

	vk::PhysicalDeviceProperties deviceProperties = context->get_physical_device_properties();

	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	texture_sampler = context->primary_logical_device.createSampler(samplerInfo);

	field_tx_pipeline.shader_path = "shaders/elasticfieldtx.comp.bin";
	ComputePipelineImpl::Error txError = field_tx_pipeline.init(context);
	
	if (txError != ComputePipelineImpl::Error::OK) {
		LOG_ERROR("Failed to initialize field transform kernel");
		return;
	}

	field_blend_pipeline.shader_path = "shaders/elasticfieldblend.comp.bin";
	ComputePipelineImpl::Error blendError = field_blend_pipeline.init(context);

	if (blendError != ComputePipelineImpl::Error::OK) {
		LOG_ERROR("Failed to initialize field blending kernel");
		return;
	}
}

ElasticFieldComposer::~ElasticFieldComposer() {
	context->primary_logical_device.destroyDescriptorPool(descriptor_pool);

	context->primary_logical_device.destroy(texture_sampler);

	for (auto& f : frames) {
		for (auto& i : f.tx_intermediates) {
			context->destroy_image_view(i.isogradfield.view);

			context->destroy_texture(i.isogradfield.texture);
		}

		for (auto& i : f.blend_intermediates) {
			context->destroy_image_view(i.isogradfield.view);

			context->destroy_texture(i.isogradfield.texture);
		}
	}
}

void ElasticFieldComposer::init_render_data(size_t MaxBones, size_t TotalBones, size_t MaxJoints, size_t TotalJoints, vk::Extent3D MaxFieldDims) {
	field_dims = MaxFieldDims;
	
	// 1 fields per bone x (2 in bones + 1 out bone) = 3 total fields
	uint32_t numIntermediateFields = 3 * TotalJoints;

	// 1 in field + 1 out field
	numIntermediateFields += 2 * TotalBones;

	numIntermediateFields *= swapchain->size();

	std::vector<vk::DescriptorPoolSize> poolSizes = {
		{ vk::DescriptorType::eStorageImage, numIntermediateFields }
	};

	uint32_t totalSets = numIntermediateFields;

	vk::DescriptorPoolCreateInfo descriptorPoolInfo;
	descriptorPoolInfo.poolSizeCount = poolSizes.size();
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = totalSets;

	descriptor_pool = context->primary_logical_device.createDescriptorPool(descriptorPoolInfo);

	if (!descriptor_pool) {
		LOG_ERROR("Failed to create descriptor pool");
		return;
	}

	frames.resize(swapchain->size());

	for (auto& f : frames) {
		f.tx_intermediates.resize(MaxBones);

		for (auto& i : f.tx_intermediates) {
			i.isogradfield.texture = context->create_texture_3d(
				MaxFieldDims,
				vk::Format::eR32G32B32A32Sfloat
			);

			context->transition_image_layout(
				i.isogradfield.texture,
				i.isogradfield.texture.format,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eGeneral
			);

			i.isogradfield.view = context->create_image_view(i.isogradfield.texture, vk::ImageViewType::e3D);

			vk::ImageMemoryBarrier readBarrier;
			vk::ImageMemoryBarrier writeBarrier;

			readBarrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
			readBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			readBarrier.srcQueueFamilyIndex = context->primary_queue_family_index;
			readBarrier.dstQueueFamilyIndex = context->primary_queue_family_index;
			readBarrier.oldLayout = vk::ImageLayout::eGeneral;
			readBarrier.newLayout = vk::ImageLayout::eGeneral;
			readBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			readBarrier.subresourceRange.baseArrayLayer = 0;
			readBarrier.subresourceRange.baseMipLevel = 0;
			readBarrier.subresourceRange.layerCount = 1;
			readBarrier.subresourceRange.levelCount = 1;

			writeBarrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
			writeBarrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;
			writeBarrier.srcQueueFamilyIndex = context->primary_queue_family_index;
			writeBarrier.dstQueueFamilyIndex = context->primary_queue_family_index;
			writeBarrier.oldLayout = vk::ImageLayout::eGeneral;
			writeBarrier.newLayout = vk::ImageLayout::eGeneral;
			writeBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			writeBarrier.subresourceRange.baseArrayLayer = 0;
			writeBarrier.subresourceRange.baseMipLevel = 0;
			writeBarrier.subresourceRange.layerCount = 1;
			writeBarrier.subresourceRange.levelCount = 1;

			i.isogradfield_read_barrier = readBarrier;
			i.isogradfield_read_barrier.image = i.isogradfield.texture.image;
			i.isogradfield_write_barrier = writeBarrier;
			i.isogradfield_write_barrier.image = i.isogradfield.texture.image;
		}

		f.blend_intermediates.resize(MaxJoints);

		for (auto& i : f.blend_intermediates) {
			i.isogradfield.texture = context->create_texture_3d(
				MaxFieldDims,
				vk::Format::eR32G32B32A32Sfloat
			);

			context->transition_image_layout(
				i.isogradfield.texture,
				i.isogradfield.texture.format,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eGeneral
			);

			i.isogradfield.view = context->create_image_view(i.isogradfield.texture, vk::ImageViewType::e3D);

			vk::ImageMemoryBarrier readBarrier;
			vk::ImageMemoryBarrier writeBarrier;

			readBarrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
			readBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			readBarrier.srcQueueFamilyIndex = context->primary_queue_family_index;
			readBarrier.dstQueueFamilyIndex = context->primary_queue_family_index;
			readBarrier.oldLayout = vk::ImageLayout::eGeneral;
			readBarrier.newLayout = vk::ImageLayout::eGeneral;
			readBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			readBarrier.subresourceRange.baseArrayLayer = 0;
			readBarrier.subresourceRange.baseMipLevel = 0;
			readBarrier.subresourceRange.layerCount = 1;
			readBarrier.subresourceRange.levelCount = 1;

			writeBarrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
			writeBarrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;
			writeBarrier.srcQueueFamilyIndex = context->primary_queue_family_index;
			writeBarrier.dstQueueFamilyIndex = context->primary_queue_family_index;
			writeBarrier.oldLayout = vk::ImageLayout::eGeneral;
			writeBarrier.newLayout = vk::ImageLayout::eGeneral;
			writeBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			writeBarrier.subresourceRange.baseArrayLayer = 0;
			writeBarrier.subresourceRange.baseMipLevel = 0;
			writeBarrier.subresourceRange.layerCount = 1;
			writeBarrier.subresourceRange.levelCount = 1;

			i.isogradfield_read_barrier = readBarrier;
			i.isogradfield_read_barrier.image = i.isogradfield.texture.image;
			i.isogradfield_write_barrier = writeBarrier;
			i.isogradfield_write_barrier.image = i.isogradfield.texture.image;
		}
	}
}

void ElasticFieldComposer::record_descriptor_sets(MeshId MeshId, float FieldScale, std::vector<GPUTexture>& PartIsogradfields, std::vector<GPUTexture>& OutIsogradfields, std::vector<BufferAllocation>& BoneBuffers, Skeleton* Skeleton) {
	for (size_t frame = 0; frame < swapchain->size(); frame++) {
		
		// Transforms
		{
			frames[frame].kernel_contexts[MeshId].resize(PartIsogradfields.size());

			// Allocate field blend descriptor sets
			std::vector<vk::DescriptorSetLayout> descriptorLayouts(PartIsogradfields.size(), field_tx_pipeline.descriptor_set_layout);

			vk::DescriptorSetAllocateInfo descriptorSetInfo;
			descriptorSetInfo.descriptorPool = descriptor_pool;
			descriptorSetInfo.descriptorSetCount = static_cast<uint32_t>(descriptorLayouts.size());
			descriptorSetInfo.pSetLayouts = descriptorLayouts.data();

			frames[frame].tx_descriptor_sets[MeshId] = context->primary_logical_device.allocateDescriptorSets(descriptorSetInfo);

			// Populate field blend descriptor sets
			std::vector<vk::WriteDescriptorSet> descriptorWrites;
			std::list<vk::DescriptorBufferInfo> bufferInfos;
			std::list<vk::DescriptorImageInfo> imageInfos;

			for (size_t i = 0; i < PartIsogradfields.size(); i++) {

				frames[frame].kernel_contexts[MeshId][i] = {
					static_cast<uint32_t>(i),
					FieldScale
				};

				// Bones
				{
					vk::WriteDescriptorSet boneBufWrite;

					boneBufWrite.dstSet = frames[frame].tx_descriptor_sets[MeshId][i];
					boneBufWrite.dstBinding = BoneBuffer::layout_binding().binding;
					boneBufWrite.dstArrayElement = 0;
					boneBufWrite.descriptorType = BoneBuffer::layout_binding().descriptorType;
					boneBufWrite.descriptorCount = BoneBuffer::layout_binding().descriptorCount;

					vk::DescriptorBufferInfo boneBufferInfo;

					boneBufferInfo.buffer = BoneBuffers[frame].buffer;
					boneBufferInfo.offset = 0;
					boneBufferInfo.range = VK_WHOLE_SIZE;

					bufferInfos.push_back(boneBufferInfo);

					boneBufWrite.pBufferInfo = &bufferInfos.back();
					boneBufWrite.pImageInfo = nullptr;
					boneBufWrite.pTexelBufferView = nullptr;

					descriptorWrites.push_back(boneBufWrite);
				}

				// In isogradfield
				{
					vk::WriteDescriptorSet sourceBufWrite;

					sourceBufWrite.dstSet = frames[frame].tx_descriptor_sets[MeshId][i];
					sourceBufWrite.dstBinding = ElasticSkinning::IsogradfieldSourceBuffer::layout_binding().binding;
					sourceBufWrite.dstArrayElement = 0;
					sourceBufWrite.descriptorType = ElasticSkinning::IsogradfieldSourceBuffer::layout_binding().descriptorType;
					sourceBufWrite.descriptorCount = ElasticSkinning::IsogradfieldSourceBuffer::layout_binding().descriptorCount;

					vk::DescriptorImageInfo sourceImageInfo;

					sourceImageInfo.imageView = PartIsogradfields[i].view;
					sourceImageInfo.imageLayout = vk::ImageLayout::eGeneral;
					sourceImageInfo.sampler = texture_sampler;

					imageInfos.push_back(sourceImageInfo);

					sourceBufWrite.pBufferInfo = nullptr;
					sourceBufWrite.pImageInfo = &imageInfos.back();
					sourceBufWrite.pTexelBufferView = nullptr;

					descriptorWrites.push_back(sourceBufWrite);
				}

				// Out isogradfield
				{
					vk::WriteDescriptorSet sourceBufWrite;

					sourceBufWrite.dstSet = frames[frame].tx_descriptor_sets[MeshId][i];
					sourceBufWrite.dstBinding = ElasticSkinning::IsogradfieldOutBuffer::layout_binding().binding;
					sourceBufWrite.dstArrayElement = 0;
					sourceBufWrite.descriptorType = ElasticSkinning::IsogradfieldOutBuffer::layout_binding().descriptorType;
					sourceBufWrite.descriptorCount = ElasticSkinning::IsogradfieldOutBuffer::layout_binding().descriptorCount;

					vk::DescriptorImageInfo sourceImageInfo;

					sourceImageInfo.imageView = frames[frame].tx_intermediates[i].isogradfield.view;
					sourceImageInfo.imageLayout = vk::ImageLayout::eGeneral;

					imageInfos.push_back(sourceImageInfo);

					sourceBufWrite.pBufferInfo = nullptr;
					sourceBufWrite.pImageInfo = &imageInfos.back();
					sourceBufWrite.pTexelBufferView = nullptr;

					descriptorWrites.push_back(sourceBufWrite);
				}
			}

			context->primary_logical_device.updateDescriptorSets(descriptorWrites, nullptr);
		}


		// Blending
		{
			// Figure out join order

			std::vector<GPUTexture*> partFieldPtrs(PartIsogradfields.size());

			for (size_t i = 0; i < PartIsogradfields.size(); i++) {
				partFieldPtrs[i] = &frames[frame].tx_intermediates[i].isogradfield;
			}

			struct Operands {
				GPUTexture* in_a_isograd;
				GPUTexture* in_b_isograd;
				GPUTexture* out_isograd;
			};

			std::vector<Operands> joinOrder;

			size_t outIdx = 0;

			// Map of joint relationships to intermediate field, by index
			std::unordered_map<StringHash, size_t> jointOutFields;

			for (auto rel : Skeleton->bone_relationships) {
				jointOutFields[hash_combine(rel.parent, rel.child)] = outIdx;
				outIdx++;
			}

			auto [root, e] = Skeleton->get_root_bone();
			auto boneNames = Skeleton->bone_names;
			auto [leaves, e2] = Skeleton->get_leaf_bones();

			// Remove leaf bones
			std::erase_if(boneNames,
				[&leaves](StringHash b) {
					return std::find(leaves.begin(), leaves.end(), b) != leaves.end();
				}
			);

			// Sort such that furthest bones from root are first
			std::sort(boneNames.begin(), boneNames.end(),
				[Skeleton](StringHash a, StringHash b) {
					return Skeleton->distance_to_root(b).value < Skeleton->distance_to_root(a).value;
				}
			);

			for (auto b : boneNames) {
				auto [children, e3] = Skeleton->get_bone_children(b);

				for (auto c : children) {
					auto [parentIdx, e4] = Skeleton->get_bone_index(b);
					GPUTexture* parentIsograd = partFieldPtrs[parentIdx];

					auto [childIdx, e5] = Skeleton->get_bone_index(c);
					GPUTexture* childIsograd = partFieldPtrs[childIdx];

					size_t outIdx = jointOutFields[hash_combine(b, c)];
					GPUTexture* outIsograd = &frames[frame].blend_intermediates[outIdx].isogradfield;

					joinOrder.push_back(
						{
							parentIsograd,
							childIsograd,
							outIsograd
						}
					);

					partFieldPtrs[parentIdx] = outIsograd;
				}
			}

			joinOrder.back().out_isograd = &OutIsogradfields[frame];

			// Allocate field blend descriptor sets
			std::vector<vk::DescriptorSetLayout> descriptorLayouts(joinOrder.size(), field_blend_pipeline.descriptor_set_layout);

			vk::DescriptorSetAllocateInfo descriptorSetInfo;
			descriptorSetInfo.descriptorPool = descriptor_pool;
			descriptorSetInfo.descriptorSetCount = static_cast<uint32_t>(descriptorLayouts.size());
			descriptorSetInfo.pSetLayouts = descriptorLayouts.data();

			frames[frame].blend_descriptor_sets[MeshId] = context->primary_logical_device.allocateDescriptorSets(descriptorSetInfo);

			// Populate field blend descriptor sets
			std::vector<vk::WriteDescriptorSet> descriptorWrites;
			std::list<vk::DescriptorBufferInfo> bufferInfos;
			std::list<vk::DescriptorImageInfo> imageInfos;

			for (size_t i = 0; i < joinOrder.size(); i++) {

				// In isogradfield A
				{
					vk::WriteDescriptorSet sourceBufWrite;

					sourceBufWrite.dstSet = frames[frame].blend_descriptor_sets[MeshId][i];
					sourceBufWrite.dstBinding = ElasticSkinning::IsogradfieldABuffer::layout_binding().binding;
					sourceBufWrite.dstArrayElement = 0;
					sourceBufWrite.descriptorType = ElasticSkinning::IsogradfieldABuffer::layout_binding().descriptorType;
					sourceBufWrite.descriptorCount = ElasticSkinning::IsogradfieldABuffer::layout_binding().descriptorCount;

					vk::DescriptorImageInfo sourceImageInfo;

					sourceImageInfo.imageView = joinOrder[i].in_a_isograd->view;
					sourceImageInfo.imageLayout = vk::ImageLayout::eGeneral;

					imageInfos.push_back(sourceImageInfo);

					sourceBufWrite.pBufferInfo = nullptr;
					sourceBufWrite.pImageInfo = &imageInfos.back();
					sourceBufWrite.pTexelBufferView = nullptr;

					descriptorWrites.push_back(sourceBufWrite);
				}

				// In isogradfield B
				{
					vk::WriteDescriptorSet sourceBufWrite;

					sourceBufWrite.dstSet = frames[frame].blend_descriptor_sets[MeshId][i];
					sourceBufWrite.dstBinding = ElasticSkinning::IsogradfieldBBuffer::layout_binding().binding;
					sourceBufWrite.dstArrayElement = 0;
					sourceBufWrite.descriptorType = ElasticSkinning::IsogradfieldBBuffer::layout_binding().descriptorType;
					sourceBufWrite.descriptorCount = ElasticSkinning::IsogradfieldBBuffer::layout_binding().descriptorCount;

					vk::DescriptorImageInfo sourceImageInfo;

					sourceImageInfo.imageView = joinOrder[i].in_b_isograd->view;
					sourceImageInfo.imageLayout = vk::ImageLayout::eGeneral;

					imageInfos.push_back(sourceImageInfo);

					sourceBufWrite.pBufferInfo = nullptr;
					sourceBufWrite.pImageInfo = &imageInfos.back();
					sourceBufWrite.pTexelBufferView = nullptr;

					descriptorWrites.push_back(sourceBufWrite);
				}

				// Out isogradfield
				{
					vk::WriteDescriptorSet sourceBufWrite;

					sourceBufWrite.dstSet = frames[frame].blend_descriptor_sets[MeshId][i];
					sourceBufWrite.dstBinding = ElasticSkinning::IsogradfieldOutBuffer::layout_binding().binding;
					sourceBufWrite.dstArrayElement = 0;
					sourceBufWrite.descriptorType = ElasticSkinning::IsogradfieldOutBuffer::layout_binding().descriptorType;
					sourceBufWrite.descriptorCount = ElasticSkinning::IsogradfieldOutBuffer::layout_binding().descriptorCount;

					vk::DescriptorImageInfo sourceImageInfo;

					sourceImageInfo.imageView = joinOrder[i].out_isograd->view;
					sourceImageInfo.imageLayout = vk::ImageLayout::eGeneral;

					imageInfos.push_back(sourceImageInfo);

					sourceBufWrite.pBufferInfo = nullptr;
					sourceBufWrite.pImageInfo = &imageInfos.back();
					sourceBufWrite.pTexelBufferView = nullptr;

					descriptorWrites.push_back(sourceBufWrite);
				}
			}

			context->primary_logical_device.updateDescriptorSets(descriptorWrites, nullptr);
		}
	}
}

void ElasticFieldComposer::record_command_buffer(Swapchain::FrameId FrameId, vk::CommandBuffer CommandBuffer, MeshId MeshId) {
	FrameData& currentFrame = frames[FrameId];

	std::vector<ElasticSkinning::FieldTxContext>& currentTxContexts = currentFrame.kernel_contexts[MeshId];
	std::vector<vk::DescriptorSet>& currentTxDescriptors = currentFrame.tx_descriptor_sets[MeshId];

	std::vector<vk::DescriptorSet>& currentBlendDescriptors = currentFrame.blend_descriptor_sets[MeshId];

	// Transform
	for (auto& field : currentFrame.tx_intermediates) {
		vk::ClearColorValue clearColor;
		clearColor.setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f });

		vk::ImageSubresourceRange clearRange;
		clearRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		clearRange.layerCount = 1;
		clearRange.baseArrayLayer = 0;
		clearRange.levelCount = 1;
		clearRange.baseMipLevel = 0;

		CommandBuffer.clearColorImage(
			field.isogradfield.texture.image,
			vk::ImageLayout::eGeneral,
			clearColor,
			clearRange
		);
	}

	CommandBuffer.bindPipeline(
		vk::PipelineBindPoint::eCompute,
		field_tx_pipeline.pipeline
	);

	for (auto& field : currentFrame.tx_intermediates) {
		CommandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eComputeShader,
			(vk::DependencyFlagBits)(0),
			nullptr,
			nullptr,
			{
				field.isogradfield_read_barrier,
				field.isogradfield_write_barrier,
			}
		);
	}

	for (size_t i = 0; i < currentTxDescriptors.size(); i++) {
		CommandBuffer.pushConstants<ElasticSkinning::FieldTxContext>(
			field_tx_pipeline.pipeline_layout,
			field_tx_pipeline.context_push_constant.stageFlags,
			field_tx_pipeline.context_push_constant.offset,
			currentFrame.kernel_contexts[MeshId][i]
		);

		std::vector<vk::DescriptorSet> descriptorSets = {
			currentTxDescriptors[i]
		};

		CommandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eCompute,
			field_tx_pipeline.pipeline_layout,
			0,
			descriptorSets,
			nullptr
		);

		CommandBuffer.dispatch(field_dims.width / 8, field_dims.height / 8, field_dims.depth / 8);
	}

	// Blend
	for (auto& field : currentFrame.blend_intermediates) {
		vk::ClearColorValue clearColor;
		clearColor.setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f });

		vk::ImageSubresourceRange clearRange;
		clearRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		clearRange.layerCount = 1;
		clearRange.baseArrayLayer = 0;
		clearRange.levelCount = 1;
		clearRange.baseMipLevel = 0;

		CommandBuffer.clearColorImage(
			field.isogradfield.texture.image,
			vk::ImageLayout::eGeneral,
			clearColor,
			clearRange
		);
	}

	CommandBuffer.bindPipeline(
		vk::PipelineBindPoint::eCompute,
		field_blend_pipeline.pipeline
	);

	for (auto& field : currentFrame.blend_intermediates) {
		CommandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eComputeShader,
			(vk::DependencyFlagBits)(0),
			nullptr,
			nullptr,
			{
				field.isogradfield_read_barrier,
				field.isogradfield_write_barrier,
			}
		);
	}

	for (size_t i = 0; i < currentBlendDescriptors.size(); i++) {
		std::vector<vk::DescriptorSet> descriptorSets = {
			currentBlendDescriptors[i]
		};

		CommandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eCompute,
			field_blend_pipeline.pipeline_layout,
			0,
			descriptorSets,
			nullptr
		);

		CommandBuffer.dispatch(field_dims.width / 8, field_dims.height / 8, field_dims.depth / 8);
	}
}
