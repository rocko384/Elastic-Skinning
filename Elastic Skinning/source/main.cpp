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

	Renderer<ModelBuffer, CameraBuffer> renderer;
	renderer.init(&context);

	GfxPipeline<Vertex, ModelBuffer, CameraBuffer> base_pipeline;
	base_pipeline
		.set_vertex_shader("shaders/base.vert.bin")
		.set_fragment_shader("shaders/base.frag.bin");

	renderer.register_pipeline("base", std::move(base_pipeline));

	Mesh triangle;
	ModelTransform t1;
	triangle.pipeline_name = "base";
	triangle.vertices = {
		{{-0.25f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},
		{{0.25f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{-0.75f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}
	};
	triangle.indices = {
		0, 1, 2
	};

	Mesh triangle2;
	ModelTransform t2;
	triangle2.pipeline_name = "base";
	triangle2.vertices = {
		{{0.25f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{0.75f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{-0.25f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}
	};
	triangle2.indices = {
		0, 1, 2
	};

	Mesh square;
	ModelTransform s1;
	square.pipeline_name = "base";
	square.vertices = {
		{{-0.25f, 0.25f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{0.25f, 0.25f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{0.25f, -0.25f, 0.0f}, {0.0f, 0.0f, 1.0f}},
		{{-0.25f, -0.25f, 0.0f}, {1.0f, 1.0f, 0.0f}}
	};
	square.indices = {
		0, 1, 2, 2, 3, 0
	};

	renderer.digest_mesh(triangle, &t1);
	renderer.digest_mesh(triangle2, &t2);
	renderer.digest_mesh(square, &s1);

	Camera c;
	c.look_at(
		{ 0.0f, 0.2f, -3.0f },
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

		s1.position.x = sin_t;
		t1.position.y = sin_t;
		t2.position.y = -sin_t;

		renderer.draw_frame();
	}


	renderer.deinit();
	context.deinit();
	window.deinit();

	return 0;
}
