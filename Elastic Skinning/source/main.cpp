// Elastic Skinning.cpp : Defines the entry point for the application.
//

#include "util.h"
#include "asset.h"
#include "gfxcontext.h"
#include "renderer.h"

int main(int argc, char** argv) {
	GfxContext context;
	context.init("Elastic Skinning");
	Renderer renderer;
	renderer.init(&context);

	SDL_Delay(1000);

	renderer.deinit();
	context.deinit();

	return 0;
}
