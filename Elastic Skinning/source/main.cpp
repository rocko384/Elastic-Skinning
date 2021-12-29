// Elastic Skinning.cpp : Defines the entry point for the application.
//

#include "util.h"
#include "asset.h"
#include "gfxcontext.h"

using namespace std;

int main(int argc, char** argv) {
	GfxContext context;
	context.init("Elastic Skinning");

	SDL_Delay(1000);

	context.deinit();

	return 0;
}
