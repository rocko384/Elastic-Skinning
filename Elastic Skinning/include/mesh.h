#pragma once

#include "util.h"
#include "renderingtypes.h"

#include <variant>
#include <vector>
#include <span>
#include <cstdint>

struct Mesh {

	using VertexType = Vertex;
	using IndexType = uint32_t;

	std::string material_name;

	std::variant<std::span<VertexType>, std::vector<VertexType>> vertices;
	std::variant<std::span<IndexType>, std::vector<IndexType>> indices;

};