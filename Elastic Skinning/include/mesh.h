#pragma once

#include "util.h"
#include "renderingtypes.h"

#include <vector>
#include <span>
#include <cstdint>

struct Mesh {

	using VertexType = Vertex;
	using IndexType = uint32_t;

	std::string material_name;

	std::vector<VertexType> vertices;
	std::vector<IndexType> indices;

};