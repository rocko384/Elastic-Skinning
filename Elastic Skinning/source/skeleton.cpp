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

Retval<size_t, Skeleton::Error> Skeleton::get_bone_index(const std::string& name) {
	return get_bone_index(CRC::crc64(name));
}

Retval<size_t, Skeleton::Error> Skeleton::get_bone_index(StringHash name) {
	for (size_t i = 0; i < bone_names.size(); i++) {
		if (bone_names[i] == name) {
			return { i, Error::OK };
		}
	}

	return { UINT64_MAX, Error::BONE_NOT_FOUND };
}


void Skeleton::add_bone_relationship(const std::string& parent, const std::string& child) {
	add_bone_relationship(CRC::crc64(parent), CRC::crc64(child));
}

void Skeleton::add_bone_relationship(StringHash parent, StringHash child) {
	bone_relationships.push_back({ parent, child });
}

Retval<std::vector<StringHash>, Skeleton::Error> Skeleton::get_bone_children(const std::string& parent) {
	return get_bone_children(CRC::crc64(parent));
}

Retval<std::vector<StringHash>, Skeleton::Error> Skeleton::get_bone_children(StringHash parent) {
	auto [_, e] = get_bone(parent);

	if (e != Error::OK) {
		return { {}, e };
	}

	std::vector<StringHash> children;

	for (auto& relationship : bone_relationships) {
		if (relationship.parent == parent) {
			children.push_back(relationship.child);
		}
	}

	return { children, Error::OK };
}

Retval<StringHash, Skeleton::Error> Skeleton::get_bone_parent(const std::string& child) {
	return get_bone_parent(CRC::crc64(child));
}

Retval<StringHash, Skeleton::Error> Skeleton::get_bone_parent(StringHash child) {
	auto [_, e] = get_bone(child);

	if (e != Error::OK) {
		return { {}, e };
	}

	for (auto& relationship : bone_relationships) {
		if (relationship.child == child) {
			return { relationship.parent, Error::OK };
		}
	}

	return { NULL_HASH, Error::OK };
}

Retval<StringHash, Skeleton::Error> Skeleton::get_root_bone() {
	if (bones.empty()) {
		return { NULL_HASH, Error::BONE_NOT_FOUND };
	}

	if (bone_relationships.empty()) {
		return { bone_names[0], Error::OK };
	}

	for (auto b : bone_names) {
		auto [p, e] = get_bone_parent(b);

		if (p == NULL_HASH) {
			return { b, Error::OK };
		}
	}

	return { NULL_HASH, Error::BONE_NOT_FOUND };
}

Retval<std::vector<StringHash>, Skeleton::Error> Skeleton::get_leaf_bones() {
	std::vector<StringHash> ret;

	if (bones.empty()) {
		return { { NULL_HASH }, Error::BONE_NOT_FOUND };
	}

	if (bone_relationships.empty()) {
		return { bone_names, Error::OK };
	}

	for (auto b : bone_names) {
		auto [c, e] = get_bone_children(b);

		if (c.empty()) {
			ret.push_back(b);
		}
	}

	return { ret, Error::OK };
}

Retval<size_t, Skeleton::Error> Skeleton::distance_to_root(const std::string& name) {
	return distance_to_root(CRC::crc64(name));
}

Retval<size_t, Skeleton::Error> Skeleton::distance_to_root(StringHash name) {
	auto [bone, e] = get_bone(name);

	if (e != Error::OK) {
		return { UINT64_MAX, e };
	}

	auto [root, e2] = get_root_bone();

	size_t distance = 0;
	StringHash current = name;

	while (current != root) {
		distance++;
		auto [next, e3] = get_bone_parent(current);
		current = next;
	}

	return { distance, Error::OK };
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

Skeleton::Error Skeleton::play_animation(const std::string& name, bool loop) {
	return play_animation(CRC::crc64(name), loop);
}

Skeleton::Error Skeleton::play_animation(StringHash name, bool loop) {
	auto [anim, e] = get_animation(name);

	if (e != Error::OK) {
		return e;
	}

	active_animation = anim;
	is_looped = loop;

	return e;
}

void Skeleton::clear_animation() {
	active_animation = nullptr;
}

std::vector<Bone> Skeleton::sample_animation_frame() {
	std::vector<Bone> outBones{ bones.begin(), bones.end() };

	// Sample active bones
	if (active_animation != nullptr) {
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

			auto [frame, status] = channel->sample(timeDiff);

			if (status == Channel::Status::PAST_END) {
				animation_start_time = now;
			}

			if (outBone != nullptr) {
				outBone->rotation = frame.rotation;
				outBone->position = frame.position;
				outBone->rotation = frame.rotation;
			}
		}
	}

	auto getBoneIdx = [this](StringHash name) -> size_t {
		for (size_t i = 0; i < bone_names.size(); i++) {
			if (bone_names[i] == name) {
				return i;
			}
		}

		return 0;
	};

	// Propagate transformations
	auto [root, e] = get_root_bone();

	std::vector<Retval<std::vector<StringHash>, Error>> currGen;
	currGen.push_back(get_bone_children(root));

	do {
		std::vector<Retval<std::vector<StringHash>, Error>> newGen;

		for (auto& children : currGen) {
			for (auto child : children.value) {
				auto [parentName, pe] = get_bone_parent(child);
				size_t parentIdx = getBoneIdx(parentName);
				size_t childIdx = getBoneIdx(child);

				Bone* parentBone = &outBones[parentIdx];
				Bone* childBone = &outBones[childIdx];

				glm::vec3 rotatedOffset = parentBone->rotation * childBone->position;
				childBone->position = parentBone->position + rotatedOffset;

				childBone->rotation = parentBone->rotation * childBone->rotation;
				
				newGen.push_back(get_bone_children(child));
			}
		}

		currGen = newGen;
	} while (!currGen.empty());

	return outBones;
}