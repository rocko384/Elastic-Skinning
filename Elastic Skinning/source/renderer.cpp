#include "Renderer.h"

#include <array>
#include <list>
#include <memory>

RendererImpl::RendererImpl(GfxContext* Context) {
	constructor_impl(Context);
}

void RendererImpl::constructor_impl(GfxContext* Context) {
	if (Context == nullptr) {
		LOG_ERROR("Graphics context doesn't exist");
		return;
	}

	if (!Context->is_initialized()) {
		LOG_ERROR("Graphics context is uninitialized");
		return;
	}

	context = Context;

	context->window->add_resized_callback(
		[this](size_t w, size_t h) {
			this->window_resized_callback(w, h);
		}
	);

	context->window->add_minimized_callback(
		[this]() {
			this->window_minimized_callback();
		}
	);

	context->window->add_maximized_callback(
		[this]() {
			this->window_maximized_callback();
		}
	);

	context->window->add_restored_callback(
		[this]() {
			this->window_restored_callback();
		}
	);

	create_render_state();

	/*
	* Create command pool
	*/

	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.queueFamilyIndex = context->primary_queue_family_index;
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	command_pool = context->primary_logical_device.createCommandPool(poolInfo);

	if (!command_pool) {
		LOG_ERROR("Failed to create command pool");
		return;
	}

	/*
	* Allocate primary command buffers
	*/

	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.commandPool = command_pool;
	commandBufferInfo.level = vk::CommandBufferLevel::ePrimary;
	commandBufferInfo.commandBufferCount = static_cast<uint32_t>(render_swapchain.size());

	primary_render_command_buffers = context->primary_logical_device.allocateCommandBuffers(commandBufferInfo);

	if (primary_render_command_buffers.empty()) {
		LOG_ERROR("Failed to allocate primary command buffers");
		return;
	}

	/*
	* Texture sampler creation
	*/

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

	/*
	* Finish initialization
	*/

	is_init = true;
}

RendererImpl::~RendererImpl() {
	if (is_initialized()) {
		context->primary_logical_device.waitIdle();

		context->primary_logical_device.destroy(texture_sampler);

		context->primary_logical_device.destroy(descriptor_pool);

		for (auto& texture : textures) {
			context->primary_logical_device.destroy(texture.second.view);
			context->destroy_texture(texture.second.texture);
		}

		for (auto& mesh : meshes) {
			context->destroy_buffer(mesh.vertex_buffer);
			context->destroy_buffer(mesh.index_buffer);
		}

		context->primary_logical_device.destroy(command_pool);

		for (auto& pipeline : pipelines) {
			pipeline.second.deinit();
		}
		
		destroy_render_state();
	}

	is_init = false;
}

RendererImpl::Error RendererImpl::register_pipeline_impl(const std::string& Name, GfxPipelineImpl& Pipeline) {
	return register_pipeline_impl(CRC::crc64(Name), Pipeline);
}

RendererImpl::Error RendererImpl::register_pipeline_impl(StringHash Name, GfxPipelineImpl& Pipeline) {
	if (pipelines.contains(Name)) {
		return Error::PIPELINE_WITH_NAME_ALREADY_EXISTS;
	}

	pipelines[Name] = std::move(Pipeline);
	pipelines[Name].deinit();

	GfxPipelineImpl::Error pipelineError;

	switch(pipelines[Name].target) {
	case RenderTarget::Swapchain:
		pipelineError = pipelines[Name].init(context, &render_swapchain.extent, &geometry_render_pass, color_subpass);
		break;
	case RenderTarget::DepthBuffer:
		pipelineError = pipelines[Name].init(context, &render_swapchain.extent, &geometry_render_pass, depth_subpass);
	default:
		return Error::PIPELINE_HAS_UNSUPPORTED_RENDER_TARGET;
		break;
	}

	switch (pipelineError) {
	case GfxPipelineImpl::Error::INVALID_CONTEXT:
		LOG_ERROR("Pipeline was given invalid graphics context");
		return Error::PIPELINE_INIT_ERROR;
		break;
	case GfxPipelineImpl::Error::UNINITIALIZED_CONTEXT:
		LOG_ERROR("Pipeline was given uninitalized graphics context");
		return Error::PIPELINE_INIT_ERROR;
		break;
	case GfxPipelineImpl::Error::INVALID_RENDER_PASS:
		LOG_ERROR("Pipeline was given invalid render pass");
		return Error::PIPELINE_INIT_ERROR;
		break;
	case GfxPipelineImpl::Error::NO_SHADERS:
		LOG_ERROR("Pipeline was given no shaders");
		return Error::PIPELINE_INIT_ERROR;
		break;
	case GfxPipelineImpl::Error::FAIL_CREATE_DESCRIPTOR_SET_LAYOUT:
		LOG_ERROR("Failed to create descriptor set layout");
		return Error::PIPELINE_INIT_ERROR;
		break;
	case GfxPipelineImpl::Error::FAIL_CREATE_PIPELINE_LAYOUT:
		LOG_ERROR("Failed to create pipeline layout");
		return Error::PIPELINE_INIT_ERROR;
		break;
	case GfxPipelineImpl::Error::FAIL_CREATE_PIPELINE:
		LOG_ERROR("Failed to create graphics pipeline");
		return Error::PIPELINE_INIT_ERROR;
		break;
	default:
		break;
	}

	return Error::OK;
}

RendererImpl::Error RendererImpl::register_material(const Material& Material) {
	StringHash materialName = CRC::crc64(Material.name);

	return register_material(materialName, Material);
}

RendererImpl::Error RendererImpl::register_material(StringHash Name, const Material& Material) {
	if (materials.contains(Name)) {
		return Error::MATERIAL_WITH_NAME_ALREADY_EXISTS;
	}

	StringHash albedoName = Material.albedo.has_value() ?
		hash_combine(Name, ALBEDO_TEXTURE_NAME) : NULL_HASH;
	StringHash normalName = Material.normal.has_value() ?
		hash_combine(Name, NORMAL_TEXTURE_NAME) : NULL_HASH;
	StringHash metalName = Material.metallic_roughness.has_value() ?
		hash_combine(Name, METALROUGH_TEXTURE_NAME) : NULL_HASH;

	if (albedoName != NULL_HASH) {
		Error e = register_texture(albedoName, Material.albedo.value().image);

		if (e != Error::OK) {
			return e;
		}
	}

	if (normalName != NULL_HASH) {
		Error e = register_texture(normalName, Material.normal.value().image);

		if (e != Error::OK) {
			return e;
		}
	}

	if (metalName != NULL_HASH) {
		Error e = register_texture(metalName, Material.metallic_roughness.value().image);

		if (e != Error::OK) {
			return e;
		}
	}

	materials[Name] = {
		((albedoName != NULL_HASH) ? albedoName : DEFAULT_TEXTURE_NAME),
		normalName,
		metalName,
		CRC::crc64(Material.pipeline_name),
		Material.albedo_factor,
		Material.metallic_factor,
		Material.roughness_factor
	};

	return Error::OK;
}

RendererImpl::Error RendererImpl::set_default_material(const Material& Material) {
	if (materials.contains(DEFAULT_MATERIAL_NAME)) {
		materials.erase(DEFAULT_MATERIAL_NAME);
	}

	return register_material(DEFAULT_MATERIAL_NAME, Material);
}

RendererImpl::Error RendererImpl::register_texture(const std::string& Name, const Image& Image) {
	return register_texture(CRC::crc64(Name), Image);
}

RendererImpl::Error RendererImpl::register_texture(StringHash Name, const Image& Image) {
	if (textures.contains(Name)) {
		return Error::TEXTURE_WITH_NAME_ALREADY_EXISTS;
	}

	TextureAllocation texture = context->create_texture_2d({
		static_cast<uint32_t>(Image.width),
		static_cast<uint32_t>(Image.height)
	});
	context->upload_texture(texture, Image);

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = texture.image;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = texture.format;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	vk::ImageView imageView = context->primary_logical_device.createImageView(viewInfo);

	textures[Name] = { texture, imageView };

	return Error::OK;
}

RendererImpl::Error RendererImpl::set_default_texture(const Image& Image) {
	if (textures.contains(DEFAULT_TEXTURE_NAME)) {
		context->primary_logical_device.destroy(textures[DEFAULT_TEXTURE_NAME].view);
		context->destroy_texture(textures[DEFAULT_TEXTURE_NAME].texture);
		textures.erase(DEFAULT_TEXTURE_NAME);
	}

	return register_texture(DEFAULT_TEXTURE_NAME, Image);
}

Retval<RendererImpl::MeshId, RendererImpl::Error> RendererImpl::digest_mesh_impl(const std::string& MaterialName, size_t VertexCount, size_t VertexSizeBytes, size_t IndexCount, size_t IndexSizeBytes, void* VertexData, void* IndexData, ModelTransform* Transform) {
	StringHash MaterialHash = MaterialName.empty() ? DEFAULT_MATERIAL_NAME : CRC::crc64(MaterialName);

	if (!materials.contains(MaterialHash)) {
		return { {}, RendererImpl::Error::MATERIAL_NOT_FOUND };
	}

	InternalMaterial& material = materials[MaterialHash];

	InternalMesh digestedMesh;
	digestedMesh.pipeline_hash = material.pipeline_name;
	digestedMesh.depth_pipeline_hash = hash_combine(digestedMesh.pipeline_hash, RendererImpl::DEPTH_PIPELINE_NAME);
	digestedMesh.material_hash = MaterialHash;

	digestedMesh.vertex_count = VertexCount;
	digestedMesh.index_count = IndexCount;

	/*
	* Create and allocate GPU buffer for vertices
	*/
	size_t vertexMemorySize = VertexSizeBytes * VertexCount;

	digestedMesh.vertex_buffer = context->create_vertex_buffer(vertexMemorySize);

	/*
	* Create and allocate GPU buffer for indices
	*/
	size_t indexMemorySize = IndexSizeBytes * IndexCount;

	digestedMesh.index_buffer = context->create_index_buffer(indexMemorySize);

	/*
	* Upload data to GPU
	*/
	context->upload_to_gpu_buffer(digestedMesh.vertex_buffer, VertexData, vertexMemorySize);
	context->upload_to_gpu_buffer(digestedMesh.index_buffer, IndexData, indexMemorySize);

	/*
	* Allocate command buffers
	*/
	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.commandPool = command_pool;
	commandBufferInfo.level = vk::CommandBufferLevel::eSecondary;
	commandBufferInfo.commandBufferCount = 1;

	mesh_transforms.push_back(Transform);
	meshes.push_back(digestedMesh);

	return { static_cast<MeshId>(meshes.size() - 1), Error::OK };
}

Retval<RendererImpl::MeshId, RendererImpl::Error> RendererImpl::digest_mesh(Mesh Mesh, ModelTransform* Transform) {
	return digest_mesh_impl(Mesh.material_name, Mesh.vertices.size(), sizeof(Mesh::VertexType), Mesh.indices.size(), sizeof(Mesh::IndexType), Mesh.vertices.data(), Mesh.indices.data(), Transform);
}

Retval<RendererImpl::MeshId, RendererImpl::Error> RendererImpl::digest_mesh(SkeletalMesh Mesh, ModelTransform* Transform) {
	return digest_mesh_impl(Mesh.material_name, Mesh.vertices.size(), sizeof(SkeletalMesh::VertexType), Mesh.indices.size(), sizeof(SkeletalMesh::IndexType), Mesh.vertices.data(), Mesh.indices.data(), Transform);
}

Retval<RendererImpl::ModelId, RendererImpl::Error> RendererImpl::digest_model(Model Model, ModelTransform* Transform) {
	for (auto& material : Model.materials) {
		register_material(material);
	}

	for (auto& mesh : Model.meshes) {
		auto* meshptr = std::get_if<Mesh>(&mesh);
		auto* skelmeshptr = std::get_if<SkeletalMesh>(&mesh);

		if (meshptr != nullptr) {
			digest_mesh(*meshptr, Transform);
		}
		else if (skelmeshptr != nullptr) {
			digest_mesh(*skelmeshptr, Transform);
		}
	}

	return { 0, Error::OK };
}

void RendererImpl::set_camera(Camera* Camera) {
	current_camera = Camera;
}

void RendererImpl::draw_frame() {
	if (is_first_render) {
		finish_mesh_digestion();
		record_command_buffers();
		is_first_render = false;
	}

	if (!should_render()) {
		return;
	}

	auto frame = render_swapchain.prepare_frame();

	if (frame.status == Swapchain::Error::FAIL_ACQUIRE_IMAGE) {
		LOG_ERROR("Error acquiring swapchain image");
	}

	if (frame.status == Swapchain::Error::OUT_OF_DATE) {
		LOG_ERROR("Swapchain is out of date");
	}

	update_frame_data(frame.value.id);

	vk::Semaphore waitSemaphores[] = { frame.value.image_available_semaphore };
	vk::Semaphore signalSemaphores[] = { frame.value.render_finished_semaphore };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

	vk::SubmitInfo submitInfo;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &primary_render_command_buffers[frame.value.id];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	context->primary_queue.submit({ submitInfo }, frame.value.fence);

	if (render_swapchain.present_frame(frame.value) != Swapchain::Error::OK) {
		LOG_ERROR("Swapchain presentation failure");
	}
}

void RendererImpl::create_render_state() {
	/*
	* Create the swapchain
	*/

	auto swapchainError = render_swapchain.init(context);

	if (swapchainError != Swapchain::Error::OK) {
		switch (swapchainError) {
		case Swapchain::Error::INVALID_CONTEXT:
			LOG_ERROR("Swapchain was given invalid graphics context");
			return;
			break;
		case Swapchain::Error::UNINITIALIZED_CONTEXT:
			LOG_ERROR("Swapchain was given uninitalized graphics context");
			return;
			break;
		case Swapchain::Error::FAIL_CREATE_SWAPCHAIN:
			LOG_ERROR("Failed to create swapchain");
			return;
			break;
		case Swapchain::Error::FAIL_CREATE_IMAGE_VIEW:
			LOG_ERROR("Failed to create swapchain image view");
			return;
			break;
		case Swapchain::Error::FAIL_CREATE_SYNCH_OBJECTS:
			LOG_ERROR("Failed to create render synch primitives");
			return;
			break;
		default:
			break;
		}
	}

	/*
	* Create per swapchain frame data
	*/

	frames.resize(render_swapchain.size());

	/*
	* Create depth buffers
	*/

	for (auto& frame : frames) {
		auto& depthBuffer = frame.depthbuffer;

		depthBuffer.texture = context->create_depth_buffer(render_swapchain.extent);
		context->transition_image_layout(depthBuffer.texture, depthBuffer.texture.format, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

		vk::ImageViewCreateInfo viewInfo;
		viewInfo.image = depthBuffer.texture.image;
		viewInfo.viewType = vk::ImageViewType::e2D;
		viewInfo.format = depthBuffer.texture.format;
		viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		depthBuffer.view = context->primary_logical_device.createImageView(viewInfo);
	}

	/*
	* Create render pass
	*/

	vk::AttachmentDescription depthAttachment;
	depthAttachment.format = frames[0].depthbuffer.texture.format;
	depthAttachment.samples = vk::SampleCountFlagBits::e1;
	depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentDescription colorAttachment;
	colorAttachment.format = render_swapchain.format;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	std::vector<vk::AttachmentDescription> attachments{
		depthAttachment,
		colorAttachment
	};

	vk::AttachmentReference depthWriteAttachmentRef;
	depthWriteAttachmentRef.attachment = 0;
	depthWriteAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference depthReadAttachmentRef;
	depthReadAttachmentRef.attachment = 0;
	depthReadAttachmentRef.layout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;

	vk::AttachmentReference colorAttachmentRef;
	colorAttachmentRef.attachment = 1;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription depthSubpass;
	depthSubpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	depthSubpass.colorAttachmentCount = 0;
	depthSubpass.pColorAttachments = nullptr;
	depthSubpass.pDepthStencilAttachment = &depthWriteAttachmentRef;

	vk::SubpassDescription colorSubpass;
	colorSubpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	colorSubpass.colorAttachmentCount = 1;
	colorSubpass.pColorAttachments = &colorAttachmentRef;
	colorSubpass.pDepthStencilAttachment = &depthReadAttachmentRef;

	std::vector<vk::SubpassDescription> subpasses{
		depthSubpass,
		colorSubpass
	};

	vk::SubpassDependency depthSubpassDependency;
	depthSubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depthSubpassDependency.dstSubpass = 1;
	depthSubpassDependency.srcStageMask = vk::PipelineStageFlagBits::eLateFragmentTests;
	depthSubpassDependency.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	depthSubpassDependency.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	depthSubpassDependency.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead;

	vk::SubpassDependency colorSubpassDependency;
	colorSubpassDependency.srcSubpass = 0;
	colorSubpassDependency.dstSubpass = 1;
	colorSubpassDependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	colorSubpassDependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	colorSubpassDependency.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
	colorSubpassDependency.dstAccessMask = vk::AccessFlagBits::eInputAttachmentRead;

	std::vector<vk::SubpassDependency> subpassDependencies{
		depthSubpassDependency,
		colorSubpassDependency
	};

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.attachmentCount = attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = subpasses.size();
	renderPassInfo.pSubpasses = subpasses.data();
	renderPassInfo.dependencyCount = subpassDependencies.size();
	renderPassInfo.pDependencies = subpassDependencies.data();

	geometry_render_pass = context->primary_logical_device.createRenderPass(renderPassInfo);
	depth_subpass = 0;
	color_subpass = 1;

	if (!geometry_render_pass) {
		LOG_ERROR("Failed to create geometry render pass");
		return;
	}

	/*
	* Create framebuffers
	*/

	for (size_t i = 0; i < frames.size(); i++) {
		std::vector<vk::ImageView> attachments{
			frames[i].depthbuffer.view,
			render_swapchain.image_views[i]
		};

		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.renderPass = geometry_render_pass;
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = render_swapchain.extent.width;
		framebufferInfo.height = render_swapchain.extent.height;
		framebufferInfo.layers = 1;

		vk::Framebuffer framebuffer = context->primary_logical_device.createFramebuffer(framebufferInfo);

		if (!framebuffer) {
			LOG_ERROR("Failed to create swapchain framebuffer");
			return;
		}

		frames[i].framebuffer = framebuffer;
	}
}

void RendererImpl::destroy_render_state() {
	context->primary_logical_device.destroy(geometry_render_pass);

	for (auto& frame : frames) {
		context->primary_logical_device.destroy(frame.framebuffer);

		context->primary_logical_device.destroy(frame.depthbuffer.view);
		context->destroy_texture(frame.depthbuffer.texture);

		for (auto& buffer : frame.data_buffers) {
			context->destroy_buffer(buffer.second);
		}
	}

	render_swapchain.deinit();
}

bool RendererImpl::should_render() {
	return !context->window->is_minimized()
		&& render_swapchain.is_initialized()
		&& are_command_buffers_recorded;
}

void RendererImpl::update_frame_data(Swapchain::FrameId ImageIdx) {
	std::vector<VmaAllocation> updated_allocations;
	std::vector<VkDeviceSize> updated_allocation_offsets;
	std::vector<VkDeviceSize> updated_allocation_sizes;

	for (auto& name : buffer_type_names) {
		VmaAllocation activeAllocation = frames[ImageIdx].data_buffers[name].allocation;

		if (name == ModelBuffer::name()) {
			std::vector<glm::mat4> modelmats;
			modelmats.resize(mesh_transforms.size());

			for (size_t i = 0; i < modelmats.size(); i++) {
				modelmats[i] = glm::mat4(1.0f);

				if (mesh_transforms[i] != nullptr) {
					glm::mat4 scale = glm::scale(glm::mat4(1.0f), mesh_transforms[i]->scale);
					glm::mat4 rotation = glm::mat4_cast(mesh_transforms[i]->rotation);
					glm::mat4 position = glm::translate(glm::mat4(1.0f), mesh_transforms[i]->position);
					modelmats[i] = position * rotation * scale;
				}
			}

			size_t transferSize = modelmats.size() * sizeof(glm::mat4);

			void* data;
			vmaMapMemory(context->allocator, activeAllocation, &data);
			std::memcpy(data, modelmats.data(), transferSize);
			vmaUnmapMemory(context->allocator, activeAllocation);
		
			updated_allocations.push_back(activeAllocation);
			updated_allocation_offsets.push_back(0);
			updated_allocation_sizes.push_back(transferSize);
		}

		if (name == CameraBuffer::name()) {
			CameraBuffer camera{ glm::mat4(1.0f), glm::mat4(1.0f) };

			if (current_camera != nullptr) {
				camera.projection = current_camera->projection;
				camera.view = current_camera->view, glm::vec3{ 1.0f, 1.0f, 1.0f };
			}

			size_t transferSize = sizeof(CameraBuffer);

			void* data;
			vmaMapMemory(context->allocator, activeAllocation, &data);
			std::memcpy(data, &camera, transferSize);
			vmaUnmapMemory(context->allocator, activeAllocation);
		
			updated_allocations.push_back(activeAllocation);
			updated_allocation_offsets.push_back(0);
			updated_allocation_sizes.push_back(transferSize);
		}
	}

	vmaFlushAllocations(context->allocator, updated_allocations.size(), updated_allocations.data(), updated_allocation_offsets.data(), updated_allocation_sizes.data());
}

void RendererImpl::finish_mesh_digestion() {
	// Allocate buffer objects
	for (auto& name : buffer_type_names) {
		// Only allocate SSBOs that are per mesh
		if (buffer_type_is_per_mesh[name]) {
			for (auto& frame : frames) {
				frame.data_buffers[name] = context->create_storage_buffer(buffer_type_sizes[name] * meshes.size());
			}
		}
		// Allocate UBOs that aren't per mesh
		else {
			for (auto& frame : frames) {
				frame.data_buffers[name] = context->create_uniform_buffer(buffer_type_sizes[name]);
			}
		}
	}

	// Create descriptor pool

	uint32_t numPerMeshBuffers = 0;
	uint32_t numGlobalBuffers = 0;
	// Possible number of texture descriptor sets = # Sampler descriptors * # textures
	uint32_t numSamplers = sampler_type_names.size();
	numSamplers *= textures.size();

	for (auto& is_per_mesh : buffer_type_is_per_mesh) {
		if (is_per_mesh.second) {
			numPerMeshBuffers++;
		}
		else {
			numGlobalBuffers++;
		}
	}

	numPerMeshBuffers *= render_swapchain.size();
	numGlobalBuffers *= render_swapchain.size();

	numSamplers *= pipelines.size();
	numPerMeshBuffers *= pipelines.size();
	numGlobalBuffers *= pipelines.size();

	std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
		{ vk::DescriptorType::eStorageBuffer, numPerMeshBuffers },
		{ vk::DescriptorType::eUniformBuffer, numGlobalBuffers },
		{ vk::DescriptorType::eCombinedImageSampler, numSamplers }
	};

	uint32_t totalSets = numPerMeshBuffers + numGlobalBuffers + numSamplers;

	vk::DescriptorPoolCreateInfo descriptorPoolInfo;
	descriptorPoolInfo.poolSizeCount = descriptorPoolSizes.size();
	descriptorPoolInfo.pPoolSizes = descriptorPoolSizes.data();
	descriptorPoolInfo.maxSets = totalSets;

	descriptor_pool = context->primary_logical_device.createDescriptorPool(descriptorPoolInfo);

	if (!descriptor_pool) {
		LOG_ERROR("Failed to create descriptor pool");
		return;
	}

	// Allocate buffer descriptor sets
	for (auto& pipeline : pipelines) {
		std::vector<vk::DescriptorSetLayout> bufferLayouts(render_swapchain.size(), pipeline.second.buffer_descriptor_set_layout);

		vk::DescriptorSetAllocateInfo bufferDescriptorSetInfo;
		bufferDescriptorSetInfo.descriptorPool = descriptor_pool;
		bufferDescriptorSetInfo.descriptorSetCount = static_cast<uint32_t>(bufferLayouts.size());
		bufferDescriptorSetInfo.pSetLayouts = bufferLayouts.data();

		auto descriptorSets = context->primary_logical_device.allocateDescriptorSets(bufferDescriptorSetInfo);
	
		for (size_t i = 0; i < descriptorSets.size(); i++) {
			frames[i].buffer_descriptor_sets[pipeline.first] = descriptorSets[i];
		}
	}

	// Allocate texture descriptor sets
	for (auto& pipeline : pipelines) {
		for (auto& sampler : sampler_type_names) {
			for (auto& texture : textures) {
				vk::DescriptorSetLayout textureLayout = pipeline.second.texture_descriptor_set_layout;

				vk::DescriptorSetAllocateInfo textureDescriptorSetInfo;
				textureDescriptorSetInfo.descriptorPool = descriptor_pool;
				textureDescriptorSetInfo.descriptorSetCount = 1;
				textureDescriptorSetInfo.pSetLayouts = &textureLayout;

				StringHash compositeName = hash_combine(pipeline.first, sampler, texture.first);
				texture_descriptor_sets[compositeName] = context->primary_logical_device.allocateDescriptorSets(textureDescriptorSetInfo)[0];
			}
		}
	}

	// Populate descriptor sets
	// Per pipeline
	for (auto& pipeline : pipelines) {
		// Per descriptor type used by pipeline
		for (auto& descriptorName : pipeline.second.descriptor_type_names) {
			std::vector<vk::WriteDescriptorSet> descriptorWrites;
			std::list<vk::DescriptorBufferInfo> bufferInfos;
			std::list<vk::DescriptorImageInfo> imageInfos;

			if (pipeline.second.descriptor_is_buffer[descriptorName]) {
				// Per frame state
				for (size_t i = 0; i < render_swapchain.size(); i++) {
					vk::WriteDescriptorSet descriptorWrite;

					// buffer_descriptor_sets[Pipeline Name][Swapchain Image #]
					descriptorWrite.dstSet = frames[i].buffer_descriptor_sets[pipeline.first];
					descriptorWrite.dstBinding = pipeline.second.descriptor_layout_bindings[descriptorName].binding;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorType = pipeline.second.descriptor_layout_bindings[descriptorName].descriptorType;
					descriptorWrite.descriptorCount = pipeline.second.descriptor_layout_bindings[descriptorName].descriptorCount;

					vk::DescriptorBufferInfo bufferInfo;
					// frame_data_buffers[Buffer Type][Swapchain Image #]
					bufferInfo.buffer = frames[i].data_buffers[descriptorName].buffer;
					bufferInfo.offset = 0;
					bufferInfo.range = VK_WHOLE_SIZE;

					bufferInfos.push_back(bufferInfo);

					descriptorWrite.pBufferInfo = &bufferInfos.back();
					descriptorWrite.pImageInfo = nullptr;
					descriptorWrite.pTexelBufferView = nullptr;

					descriptorWrites.push_back(descriptorWrite);
				}
			}
			else {
				size_t i = 0;

				// Per texture
				for (auto& texture : textures) {
					vk::WriteDescriptorSet descriptorWrite;

					// texture_descriptor_sets[Pipeline Name][Sampler Name][Texture Name]
					StringHash compositeName = hash_combine(pipeline.first, descriptorName, texture.first);
					descriptorWrite.dstSet = texture_descriptor_sets[compositeName];
					descriptorWrite.dstBinding = pipeline.second.descriptor_layout_bindings[descriptorName].binding;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorType = pipeline.second.descriptor_layout_bindings[descriptorName].descriptorType;
					descriptorWrite.descriptorCount = pipeline.second.descriptor_layout_bindings[descriptorName].descriptorCount;

					vk::DescriptorImageInfo imageInfo;
					imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
					imageInfo.imageView = texture.second.view;
					imageInfo.sampler = texture_sampler;

					imageInfos.push_back(imageInfo);

					descriptorWrite.pBufferInfo = nullptr;
					descriptorWrite.pImageInfo = &imageInfos.back();
					descriptorWrite.pTexelBufferView = nullptr;

					descriptorWrites.push_back(descriptorWrite);

					i++;
				}
			}

			context->primary_logical_device.updateDescriptorSets(descriptorWrites, nullptr);
		}
	}
}

void RendererImpl::record_command_buffers() {
	context->primary_logical_device.waitIdle();

	for (size_t i = 0; i < primary_render_command_buffers.size(); i++) {
		vk::CommandBuffer currentCommandBuffer = primary_render_command_buffers[i];

		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = (vk::CommandBufferUsageFlagBits)(0);
		beginInfo.pInheritanceInfo = nullptr;

		currentCommandBuffer.begin(beginInfo);

		vk::RenderPassBeginInfo renderPassInfo;
		renderPassInfo.renderPass = geometry_render_pass;
		renderPassInfo.framebuffer = frames[i].framebuffer;
		renderPassInfo.renderArea.offset = vk::Offset2D{ 0,0 };
		renderPassInfo.renderArea.extent = render_swapchain.extent;

		vk::ClearValue depthClear;
		depthClear.color.float32 = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		depthClear.depthStencil.depth = 1.0f;
		depthClear.depthStencil.stencil = 0.0f;

		vk::ClearValue colorClear;
		colorClear.color.float32 = { { 0.0f, 0.0f, 0.0f, 1.0f } };

		std::vector<vk::ClearValue> clearValues{
			depthClear,
			colorClear
		};

		renderPassInfo.clearValueCount = clearValues.size();
		renderPassInfo.pClearValues = clearValues.data();

		currentCommandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		// Depth only subpass
		for (MeshId meshId = 0; meshId < meshes.size(); meshId++) {
			GfxPipelineImpl* pipeline = &pipelines[meshes[meshId].depth_pipeline_hash];

			currentCommandBuffer.bindPipeline(
				vk::PipelineBindPoint::eGraphics,
				pipeline->pipeline
			);

			std::vector<vk::DescriptorSet> descriptorSets = { frames[i].buffer_descriptor_sets[meshes[meshId].pipeline_hash] };
			
			currentCommandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				pipeline->pipeline_layout,
				0,
				descriptorSets,
				nullptr
			);

			currentCommandBuffer.pushConstants<MeshId>(
				pipeline->pipeline_layout,
				pipeline->mesh_id_push_constant.stageFlags,
				pipeline->mesh_id_push_constant.offset,
				meshId
				);

			std::array<vk::Buffer, 1> vertexBuffers = { meshes[meshId].vertex_buffer.buffer };
			std::array<vk::DeviceSize, 1> offsets = { 0 };
			currentCommandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
			currentCommandBuffer.bindIndexBuffer(
				meshes[meshId].index_buffer.buffer,
				0,
				vk::IndexType::eUint32
			);

			currentCommandBuffer.drawIndexed(
				static_cast<uint32_t>(meshes[meshId].index_count),
				1,
				0,
				0,
				0
			);
		}

		currentCommandBuffer.nextSubpass(vk::SubpassContents::eInline);

		// Color subpass
		for (MeshId meshId = 0; meshId < meshes.size(); meshId++) {
			GfxPipelineImpl* pipeline = &pipelines[meshes[meshId].pipeline_hash];

			currentCommandBuffer.bindPipeline(
				vk::PipelineBindPoint::eGraphics,
				pipeline->pipeline
			);

			std::vector<vk::DescriptorSet> descriptorSets = { frames[i].buffer_descriptor_sets[meshes[meshId].pipeline_hash] };

			if (std::find(sampler_type_names.begin(), sampler_type_names.end(), ColorSampler::name()) != sampler_type_names.end()) {
				InternalMaterial material = materials[meshes[meshId].material_hash];
				StringHash compositeName = hash_combine(meshes[meshId].pipeline_hash, ColorSampler::name(), material.albedo_texture_name);
				descriptorSets.push_back(texture_descriptor_sets[compositeName]);
			}

			currentCommandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				pipeline->pipeline_layout,
				0,
				descriptorSets,
				nullptr
			);

			currentCommandBuffer.pushConstants<MeshId>(
				pipeline->pipeline_layout,
				pipeline->mesh_id_push_constant.stageFlags,
				pipeline->mesh_id_push_constant.offset,
				meshId
				);

			std::array<vk::Buffer, 1> vertexBuffers = { meshes[meshId].vertex_buffer.buffer };
			std::array<vk::DeviceSize, 1> offsets = { 0 };
			currentCommandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
			currentCommandBuffer.bindIndexBuffer(
				meshes[meshId].index_buffer.buffer,
				0,
				vk::IndexType::eUint32
			);

			currentCommandBuffer.drawIndexed(
				static_cast<uint32_t>(meshes[meshId].index_count),
				1,
				0,
				0,
				0
			);
		}

		currentCommandBuffer.endRenderPass();

		currentCommandBuffer.end();
	}

	are_command_buffers_recorded = true;
}

void RendererImpl::reset_command_buffers() {
	context->primary_logical_device.waitIdle();
	are_command_buffers_recorded = false;

	for (auto& commandBuffer : primary_render_command_buffers) {
		commandBuffer.reset();
	}
}


void RendererImpl::window_resized_callback(size_t w, size_t h) {
	window_restored_callback();
}

void RendererImpl::window_minimized_callback() {
}

void RendererImpl::window_maximized_callback() {
	window_restored_callback();
}

void RendererImpl::window_restored_callback() {
	context->primary_logical_device.waitIdle();

	destroy_render_state();
	create_render_state();

	for (auto& pipeline : pipelines) {
		pipeline.second.reinit();
	}

	reset_command_buffers();
	record_command_buffers();
}