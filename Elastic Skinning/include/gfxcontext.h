#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <string>

class GfxContext {

public:

	~GfxContext();

	void init(const std::string& AppName);
	void deinit();

private:
	bool is_initialized{ false };

	SDL_Window* window;
	vk::Instance vulkan_instance;

};