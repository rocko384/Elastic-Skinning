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
	window.init(APP_NAME);

	GfxContext context;
	context.init(&window, APP_NAME);

	Renderer renderer;
	renderer.init(&context);


	while (!window.should_close()) {
		window.poll_events();

		renderer.draw_frame();
	}


	renderer.deinit();
	context.deinit();

	return 0;
}
