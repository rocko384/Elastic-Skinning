#include "skeleton.h"

void Skeleton::add_bone(const Bone& bone, const std::string& name) {
	add_bone(bone, CRC::crc64(name));
}

void Skeleton::add_bone(const Bone& bone, StringHash name) {
	bones.push_back(bone);
	bone_names.push_back(name);
}

Retval<Bone*, Skeleton::Error> Skeleton::get_bone(const std::string& name) {
	return get_bone(CRC::crc64(name));
}

Retval<Bone*, Skeleton::Error> Skeleton::get_bone(StringHash name) {
	for (size_t i = 0; i < bone_names.size(); i++) {
		if (bone_names[i] == name) {
			return { &bones[i], Error::OK };
		}
	}

	return { nullptr, Error::BONE_NOT_FOUND };
}


void Skeleton::add_animation(const Animation& animation, const std::string& name) {
	add_animation(animation, CRC::crc64(name));
}

void Skeleton::add_animation(const Animation& animation, StringHash name) {
	animations.push_back(animation);
	animation_names.push_back(name);
}

Retval<Animation*, Skeleton::Error> Skeleton::get_animation(const std::string& name) {
	return get_animation(CRC::crc64(name));
}

Retval<Animation*, Skeleton::Error> Skeleton::get_animation(StringHash name) {
	for (size_t i = 0; i < animation_names.size(); i++) {
		if (animation_names[i] == name) {
			return { &animations[i], Error::OK };
		}
	}

	return { nullptr, Error::ANIMATION_NOT_FOUND };
}

Skeleton::Error Skeleton::play_animation(const std::string& name) {
	return play_animation(CRC::crc64(name));
}

Skeleton::Error Skeleton::play_animation(StringHash name) {
	auto [anim, e] = get_animation(name);

	if (e != Error::OK) {
		return e;
	}

	active_animation = anim;

	return e;
}

void Skeleton::clear_animation() {
	active_animation = nullptr;
}

std::vector<Bone> Skeleton::sample_animation_frame() {
	if (active_animation == nullptr) {
		return bones;
	}

	std::vector<Bone> outBones{ bones.begin(), bones.end() };

	auto now = std::chrono::steady_clock::now();
	auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - animation_start_time);

	for (StringHash name : active_animation->channel_names) {
		Bone* outBone = nullptr;
		size_t boneIdx = 0;

		for (size_t i = 0; i < bone_names.size(); i++) {
			if (bone_names[i] == name) {
				outBone = &outBones[i];
				boneIdx = i;
				break;
			}
		}

		auto [channel, error] = active_animation->get_channel(name);

		Keyframe k = channel->sample(timeDiff);

		if (outBone != nullptr) {
			outBone->rotation = k.rotation;
			outBone->position = k.position;
			outBone->rotation = k.rotation;
		}
	}

	return outBones;
}