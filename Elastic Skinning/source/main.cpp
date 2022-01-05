// Elastic Skinning.cpp : Defines the entry point for the application.
//

#include "util.h"
#include "asset.h"
#include "window.h"
#include "gfxcontext.h"
#include "renderer.h"

const std::string APP_NAME{ "Elastic Skinning" };

int main(int argc, char** argv) {
	Window window;
	window.init(APP_NAME, true);

	GfxContext context;
	context.init(&window, APP_NAME);

	Renderer renderer;
	renderer.init(&context);

	GfxPipeline<Vertex> base_pipeline;
	base_pipeline
		.set_vertex_shader("shaders/base.vert.bin")
		.set_fragment_shader("shaders/base.frag.bin");

	renderer.register_pipeline("base", std::move(base_pipeline));

	Mesh triangle;
	triangle.pipeline_name = "base";
	triangle.vertices = {
		{{-0.25f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},
		{{0.25f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{-0.75f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}
	};
	triangle.indices = {
		0, 1, 2
	};

	Mesh triangle2;
	triangle2.pipeline_name = "base";
	triangle2.vertices = {
		{{0.25f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{0.75f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{-0.25f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}
	};
	triangle2.indices = {
		0, 1, 2
	};

	Mesh square;
	square.pipeline_name = "base";
	square.vertices = {
		{{-0.25f, -0.25f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{0.25f, -0.25f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{0.25f, 0.25f, 0.0f}, {0.0f, 0.0f, 1.0f}},
		{{-0.25f, 0.25f, 0.0f}, {1.0f, 1.0f, 0.0f}}
	};
	square.indices = {
		0, 1, 2, 2, 3, 0
	};

	renderer.digest_mesh(triangle);
	renderer.digest_mesh(triangle2);
	renderer.digest_mesh(square);

	while (!window.should_close()) {
		window.poll_events();

		renderer.draw_frame();
	}


	renderer.deinit();
	context.deinit();
	window.deinit();

	return 0;
}
