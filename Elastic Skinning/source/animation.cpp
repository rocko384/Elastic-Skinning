#include "animation.h"

#include <glm/gtx/compatibility.hpp>

Keyframe Channel::sample(std::chrono::milliseconds time) {
	if (time > time_points.back()) {
		return keyframes.back();
	}

	std::chrono::milliseconds timepointA;
	std::chrono::milliseconds timepointB;
	
	Keyframe keyframeA;
	Keyframe keyframeB;

	for (size_t keyframe = 1; keyframe < time_points.size(); keyframe++) {
		if (time > time_points[keyframe - 1] && time <= time_points[keyframe]) {
			timepointA = time_points[keyframe - 1];
			timepointB = time_points[keyframe];
			
			keyframeA = keyframes[keyframe - 1];
			keyframeB = keyframes[keyframe];

			break;
		}
	}

	float numerator = static_cast<float>(time.count() - timepointA.count());
	float denominator = static_cast<float>(timepointB.count() - timepointA.count());
	float interp = numerator / denominator;

	return {
		glm::lerp(keyframeA.rotation, keyframeB.rotation, interp),
		glm::lerp(keyframeA.position, keyframeB.position, interp),
		glm::lerp(keyframeA.scale, keyframeB.scale, interp),
		glm::lerp(keyframeA.weight, keyframeB.weight, interp)
	};
}

void Animation::add_channel(const Channel& channel, const std::string& name) {
	add_channel(channel, CRC::crc64(name));
}

void Animation::add_channel(const Channel& channel, StringHash name) {
	channels.push_back(channel);
	channel_names.push_back(name);
}

Retval<Channel*, Animation::Error> Animation::get_channel(const std::string& name) {
	return get_channel(CRC::crc64(name));
}

Retval<Channel*, Animation::Error> Animation::get_channel(StringHash name) {
	for (size_t i = 0; i < channel_names.size(); i++) {
		if (channel_names[i] == name) {
			return { &channels[i], Error::OK };
		}
	}

	return { nullptr, Error::CHANNEL_NOT_FOUND };
}