#pragma once

#include "util.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <chrono>

struct Keyframe {
	glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
	glm::vec3 position{ 0.0f, 0.0f, 0.0f };
	glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
	float weight{ 0.0f };
};

struct Channel {
	enum class InterpolationMethod {
		Linear,
		Step,
		CubicSpline
	};

	Keyframe sample(std::chrono::milliseconds time);

	std::vector<std::chrono::milliseconds> time_points;
	std::vector<Keyframe> keyframes;
	InterpolationMethod interpolation{ InterpolationMethod::Linear };
};

struct Animation {
	enum class Error {
		OK,
		CHANNEL_NOT_FOUND
	};

	void add_channel(const Channel& channel, const std::string& name);
	void add_channel(const Channel& channel, StringHash name);
	Retval<Channel*, Error> get_channel(const std::string& name);
	Retval<Channel*, Error> get_channel(StringHash name);

	std::vector<Channel> channels;
	std::vector<StringHash> channel_names;
};