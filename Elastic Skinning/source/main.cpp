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
	Window window(APP_NAME, true);
	GfxContext context(&window, APP_NAME);
	Renderer<ModelBuffer, CameraBuffer, ColorSampler> renderer(&context);

	GfxPipeline<Vertex, ModelBuffer, CameraBuffer, ColorSampler> base_pipeline;
	base_pipeline
		.set_vertex_shader("shaders/base.vert.bin")
		.set_fragment_shader("shaders/base.frag.bin");

	renderer.register_pipeline("base", base_pipeline);

	Retval<Image, AssetError> defaulttex = load_image("textures/default.png");
	Retval<Image, AssetError> colortest = load_image("textures/colortest.png");

	Material defaultmaterial{
		std::nullopt,
		std::nullopt,
		std::nullopt,
		"DefaultMaterial",
		"base",
		{1.0f, 1.0f, 1.0f, 1.0f},
		0.0f,
		0.0f
	};

	Material colortestmaterial{
		std::optional<Texture>({colortest.value, Sampler{}}),
		std::nullopt,
		std::nullopt,
		"ColorTestMaterial",
		"base",
		{1.0f, 1.0f, 1.0f, 1.0f},
		0.0f,
		0.0f
	};

	renderer.set_default_texture(defaulttex.value);

	renderer.register_material(colortestmaterial);
	renderer.set_default_material(defaultmaterial);

	Mesh triangle;
	ModelTransform t1;
	t1.position.x = -0.5;
	triangle.vertices = std::vector<Vertex>{
		{{0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
	};
	triangle.indices = std::vector<uint32_t>{
		0, 1, 2
	};

	Mesh triangle2;
	ModelTransform t2;
	t2.position.x = 0.5;
	triangle2.vertices = std::vector<Vertex>{
		{{0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
	};
	triangle2.indices = std::vector<uint32_t>{
		0, 1, 2
	};

	Mesh square;
	ModelTransform s1;
	s1.position.z = -0.1f;
	square.material_name = "ColorTestMaterial";
	square.vertices = std::vector<Vertex>{
		{{-0.25f, 0.25f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
		{{0.25f, 0.25f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
		{{0.25f, -0.25f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.25f, -0.25f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};
	square.indices = std::vector<uint32_t>{
		0, 1, 2, 2, 3, 0
	};

	Mesh x_note;
	ModelTransform x;
	x_note.vertices = std::vector<Vertex>{
		{{-0.125f, 0.125f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
		{{0.125f, 0.125f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
		{{0.125f, -0.125f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
		{{-0.125f, -0.125f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
	};
	x_note.indices = std::vector<uint32_t>{
		0, 1, 2, 2, 3, 0
	};
	x.position = { 1.0f, 0.0f, 0.0f };

	Mesh y_note;
	ModelTransform y;
	y_note.vertices = std::vector<Vertex>{
		{{-0.125f, 0.125f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.125f, 0.125f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.125f, -0.125f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{-0.125f, -0.125f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}
	};
	y_note.indices = std::vector<uint32_t>{
		0, 1, 2, 2, 3, 0
	};
	y.position = { 0.0f, 1.0f, 0.0f };

	Mesh z_note;
	ModelTransform z;
	z_note.vertices = std::vector<Vertex>{
		{{-0.125f, 0.125f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.125f, 0.125f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.125f, -0.125f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{-0.125f, -0.125f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}
	};
	z_note.indices = std::vector<uint32_t>{
		0, 1, 2, 2, 3, 0
	};
	z.position = { 0.0f, 0.0f, 1.0f };

	Retval<Model, AssetError> model = load_model("models/plaidtube.glb");
	ModelTransform model_transform;
	model_transform.scale = { 0.25f, 0.25f, 0.25f };

	renderer.digest_mesh(triangle, &t1);
	renderer.digest_mesh(triangle2, &t2);
	renderer.digest_mesh(square, &s1);

	renderer.digest_mesh(x_note, &x);
	renderer.digest_mesh(y_note, &y);
	renderer.digest_mesh(z_note, &z);

	renderer.digest_model(model.value, &model_transform);

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
		float t_f = static_cast<float>(t.count());

		float sin_t = glm::sin(t_f / 200.f);
		float sin_15t_5 = glm::sin(1.5f * (t_f + 0.5f) / 200.f);
		float rocker_t = glm::pow(glm::abs(sin_t) + 1.0f, 2.0f) / 2.0f;

		s1.position.x = sin_t;
		s1.scale = { rocker_t, rocker_t, rocker_t };
		t1.position.y = sin_t;
		t2.position.y = -sin_15t_5;

		auto r1 = glm::rotate(glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f }, 0.0007f * t_f, glm::vec3{ 0.0f, 1.0f, 0.0f });
		auto r2 = glm::rotate(glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f }, (0.0022f * t_f) + glm::pow(sin_t + 1.0f, 2.0f), glm::vec3{1.0f, 0.0f, 1.0f});
			
		model_transform.rotation = r1 * r2;

		renderer.draw_frame();
	}

	return 0;
}
