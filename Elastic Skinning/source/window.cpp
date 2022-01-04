#include "window.h"

#include <algorithm>

Window::~Window() {

}

void Window::init(const std::string& Title, bool Resizeable) {

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
		LOG_ERROR("SDL failed to init");
		return;
	}

	/*
	* Window Creation
	*/

	Uint32 WindowFlags = SDL_WINDOW_VULKAN;

	if (Resizeable) {
		WindowFlags |= SDL_WINDOW_RESIZABLE;
	}

	window = SDL_CreateWindow(
		Title.c_str(),
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		1280,
		720,
		WindowFlags
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
		case SDL_WINDOWEVENT:
			handle_window_event(e.window);
			break;
		default:
			break;
		}
	}
}

void Window::add_resized_callback(Window::ResizedCallback callback) {
	resized_callbacks.push_back(callback);
}

void Window::add_minimized_callback(Window::MinimizedCallback callback) {
	minimized_callbacks.push_back(callback);
}

void Window::add_maximized_callback(Window::MaximizedCallback callback) {
	maximized_callbacks.push_back(callback);
}

void Window::add_restored_callback(Window::RestoredCallback callback) {
	restored_callbacks.push_back(callback);
}

void Window::handle_window_event(SDL_WindowEvent e) {
	switch (e.event) {
	case SDL_WINDOWEVENT_SHOWN:
		LOG("Window shown\n");
		break;
	case SDL_WINDOWEVENT_HIDDEN:
		LOG("Window hidden\n");
		break;
	case SDL_WINDOWEVENT_EXPOSED:
		LOG("Window exposed\n");
		break;
	case SDL_WINDOWEVENT_RESIZED:
		LOG("Window resized: %u x %u\n", e.data1, e.data2);
		std::for_each(resized_callbacks.begin(), resized_callbacks.end(),
			[w = e.data1, h = e.data2](ResizedCallback& callback) {
				callback(static_cast<size_t>(w), static_cast<size_t>(h));
			});
		break;
	case SDL_WINDOWEVENT_MINIMIZED:
		LOG("Window minimized\n");
		std::for_each(minimized_callbacks.begin(), minimized_callbacks.end(),
			[](MinimizedCallback& callback) {
				callback();
			});
		is_minimized_ = true;
		break;
	case SDL_WINDOWEVENT_MAXIMIZED:
		LOG("Window maximized\n");
		std::for_each(maximized_callbacks.begin(), maximized_callbacks.end(),
			[](MaximizedCallback& callback) {
				callback();
			});
		break;
	case SDL_WINDOWEVENT_RESTORED:
		LOG("Window restored\n");
		std::for_each(restored_callbacks.begin(), restored_callbacks.end(),
			[](RestoredCallback& callback) {
				callback();
			});
		is_minimized_ = false;
		break;
	default:
		break;
	}
}