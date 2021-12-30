#pragma once

#include "util.h"

#include <SDL.h>

#include <string>

class Window {

public:

	~Window();

	void init(const std::string& Title);
	void deinit();
	bool is_initialized() { return is_init; }
	bool should_close() { return saw_close_event; }

	void poll_events();

	SDL_Window* window;

private:

	bool is_init{ false };
	bool saw_close_event{ false };
};