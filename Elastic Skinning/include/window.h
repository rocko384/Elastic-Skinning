#pragma once

#include "util.h"

#include <SDL.h>

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

class Window {

public:

	struct Dimensions {
		size_t width;
		size_t height;
	};

	using ResizedCallback = std::function<void(size_t w, size_t h)>;
	using MinimizedCallback = std::function<void()>;
	using MaximizedCallback = std::function<void()>;
	using RestoredCallback = std::function<void()>;

public:

	~Window();

	void init(const std::string& Title, bool Resizeable = false);
	void deinit();
	bool is_initialized() { return is_init; }
	bool should_close() { return saw_close_event; }
	bool is_minimized() { return is_minimized_; };

	Dimensions get_dimensions();
	float get_aspect_ratio();

	void poll_events();

	void add_resized_callback(ResizedCallback callback);
	void add_minimized_callback(MinimizedCallback callback);
	void add_maximized_callback(MaximizedCallback callback);
	void add_restored_callback(RestoredCallback callback);

	SDL_Window* window;

private:

	void handle_window_event(SDL_WindowEvent e);

	bool is_init{ false };
	bool saw_close_event{ false };
	bool is_minimized_{ false };

	std::vector<ResizedCallback> resized_callbacks;
	std::vector<MinimizedCallback> minimized_callbacks;
	std::vector<MaximizedCallback> maximized_callbacks;
	std::vector<RestoredCallback> restored_callbacks;
};