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