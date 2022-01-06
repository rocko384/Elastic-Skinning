#pragma once

#include "util.h"
#include "renderingtypes.h"

#include <vector>
#include <cstdint>

struct Mesh {

	using VertexType = Vertex;
	using IndexType = uint32_t;

	std::string pipeline_name;
	std::vector<VertexType> vertices;
	std::vector<IndexType> indices;

};