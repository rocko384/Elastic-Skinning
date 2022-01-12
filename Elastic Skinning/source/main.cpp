// Elastic Skinning.cpp : Defines the entry point for the application.
//

#include "util.h"
#include "asset.h"
#include "window.h"
#include "gfxcontext.h"
#include "renderer.h"

#include <chrono>

const std::string APP_NAME{ "Elastic Skinning" };

int main(int argc, char** argv) {
	Window window;
	window.init(APP_NAME, true);

	GfxContext context;
	context.init(&window, APP_NAME);

	Renderer<ModelBuffer, CameraBuffer, ColorSampler> renderer;
	renderer.init(&context);

	GfxPipeline<Vertex, ModelBuffer, CameraBuffer> base_depth_pipeline;
	base_depth_pipeline
		.set_vertex_shader("shaders/base.vert.bin")
		.set_target(RenderTarget::DepthBuffer);

	GfxPipeline<Vertex, ModelBuffer, CameraBuffer, ColorSampler> base_pipeline;
	base_pipeline
		.set_vertex_shader("shaders/base.vert.bin")
		.set_fragment_shader("shaders/base.frag.bin");

	renderer.register_pipeline("base_depth", base_depth_pipeline);
	renderer.register_pipeline("base", base_pipeline);

	Retval<Image, AssetError> defaulttex = load_image("textures/default.png");
	Retval<Image, AssetError> colortest = load_image("textures/colortest.png");

	renderer.set_default_texture(defaulttex.value);
	renderer.register_texture("colortest", colortest.value);

	Mesh triangle;
	ModelTransform t1;
	t1.position.x = -0.5;
	triangle.pipeline_name = "base";
	triangle.vertices = {
		{{0.0f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
	};
	triangle.indices = {
		0, 1, 2
	};

	Mesh triangle2;
	ModelTransform t2;
	t2.position.x = 0.5;
	triangle2.pipeline_name = "base";
	triangle2.vertices = {
		{{0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
	};
	triangle2.indices = {
		0, 1, 2
	};

	Mesh square;
	ModelTransform s1;
	s1.position.z = -0.1f;
	square.pipeline_name = "base";
	square.texture_name = "colortest";
	square.vertices = {
		{{-0.25f, 0.25f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
		{{0.25f, 0.25f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
		{{0.25f, -0.25f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.25f, -0.25f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};
	square.indices = {
		0, 1, 2, 2, 3, 0
	};

	Mesh x_note;
	ModelTransform x;
	x_note.pipeline_name = "base";
	x_note.vertices = {
		{{-0.125f, 0.125f, 0.0f}, {0.0f, 0.0f, 1.0f}},
		{{0.125f, 0.125f, 0.0f}, {0.0f, 0.0f, 1.0f}},
		{{0.125f, -0.125f, 0.0f}, {0.0f, 0.0f, 1.0f}},
		{{-0.125f, -0.125f, 0.0f}, {0.0f, 0.0f, 1.0f}}
	};
	x_note.indices = {
		0, 1, 2, 2, 3, 0
	};
	x.position = { 1.0f, 0.0f, 0.0f };

	Mesh y_note;
	ModelTransform y;
	y_note.pipeline_name = "base";
	y_note.vertices = {
		{{-0.125f, 0.125f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{0.125f, 0.125f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{0.125f, -0.125f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{-0.125f, -0.125f, 0.0f}, {0.0f, 1.0f, 0.0f}}
	};
	y_note.indices = {
		0, 1, 2, 2, 3, 0
	};
	y.position = { 0.0f, 1.0f, 0.0f };

	Mesh z_note;
	ModelTransform z;
	z_note.pipeline_name = "base";
	z_note.vertices = {
		{{-0.125f, 0.125f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{0.125f, 0.125f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{0.125f, -0.125f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{-0.125f, -0.125f, 0.0f}, {1.0f, 0.0f, 0.0f}}
	};
	z_note.indices = {
		0, 1, 2, 2, 3, 0
	};
	z.position = { 0.0f, 0.0f, 1.0f };

	renderer.digest_mesh(triangle, &t1);
	renderer.digest_mesh(triangle2, &t2);
	renderer.digest_mesh(square, &s1);

	renderer.digest_mesh(x_note, &x);
	renderer.digest_mesh(y_note, &y);
	renderer.digest_mesh(z_note, &z);

	Camera c;
	c.look_at(
		{ 1.0f, 1.2f, -3.0f },
		{ 0.0f, 0.0f, 0.0f }
		);
	c.projection = glm::perspective(45.0f, window.get_aspect_ratio(), 0.1f, 10.0f);

	renderer.set_camera(&c);

	auto start_time = std::chrono::steady_clock::now();

	while (!window.should_close()) {
		window.poll_events();

		c.projection = glm::perspective(45.0f, window.get_aspect_ratio(), 0.1f, 10.f);

		auto current_time = std::chrono::steady_clock::now();

		std::chrono::milliseconds t = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);

		float sin_t = glm::sin(static_cast<float>(t.count()) / 200.f);
		float sin_15t_5 = glm::sin(1.5f * static_cast<float>(t.count() + 0.5f) / 200.f);
		float rocker_t = glm::pow(glm::abs(sin_t) + 1.0f, 2.0f) / 2.0f;

		s1.position.x = sin_t;
		s1.scale = { rocker_t, rocker_t, rocker_t };
		t1.position.y = sin_t;
		t2.position.y = -sin_15t_5;

		renderer.draw_frame();
	}


	renderer.deinit();
	context.deinit();
	window.deinit();

	return 0;
}
