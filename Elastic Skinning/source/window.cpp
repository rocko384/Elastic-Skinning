#include "window.h"

Window::~Window() {

}

void Window::init(const std::string& Title) {

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
		LOG_ERROR("SDL failed to init");
		return;
	}

	/*
	* Window Creation
	*/

	window = SDL_CreateWindow(
		Title.c_str(),
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		1280,
		720,
		SDL_WINDOW_VULKAN
	);

	if (window == nullptr) {
		LOG_ERROR("Failed to create window");
		return;
	}

	is_init = true;
}

void Window::deinit() {
	if (is_initialized()) {
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	is_init = false;
}

void Window::poll_events() {
	SDL_Event e;

	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			LOG("Window quit event received\n");
			saw_close_event = true;
			break;
		default:
			break;
		}
	}
}