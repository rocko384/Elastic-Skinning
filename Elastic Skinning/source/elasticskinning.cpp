#include "elasticskinning.h"

#include <glm/glm.hpp>
#include <Eigen/Dense>

#include <stack>
#include <algorithm>
#include <numeric>
#include <random>
#include <fstream>
#include <concepts>

float phi(float a) {
	return a * a * a;
}

float d_phi(float a) {
	return 3 * a * a;
}

float dx_phi(const glm::vec3& v, const glm::vec3& P) {
	float dist = glm::distance(v, P);

	return 3 * dist * (v.x - P.x);
}

float dy_phi(const glm::vec3& v, const glm::vec3& P) {
	float dist = glm::distance(v, P);

	return 3 * dist * (v.y - P.y);
}

float dz_phi(const glm::vec3& v, const glm::vec3& P) {
	float dist = glm::distance(v, P);

	return 3 * dist * (v.z - P.z);
}

glm::vec3 gradient_phi(const glm::vec3& v, const glm::vec3& P) {
	float dist = glm::distance(v, P);

	return d_phi(dist) * ((v - P) / dist);
}

float dx2_phi(const glm::vec3& v, const glm::vec3& P) {
	glm::vec3 diff = v - P;
	float mag = glm::length(diff);
	
	return (3 * ((diff.x * diff.x) + (mag * mag))) / mag;
}

float dxdy_phi(const glm::vec3& v, const glm::vec3& P) {
	glm::vec3 diff = v - P;
	float mag = glm::length(diff);

	return (3 * diff.x * diff.y) / mag;
}

float dxdz_phi(const glm::vec3& v, const glm::vec3& P) {
	glm::vec3 diff = v - P;
	float mag = glm::length(diff);

	return (3 * diff.x * diff.z) / mag;
}

float dy2_phi(const glm::vec3& v, const glm::vec3& P) {
	glm::vec3 diff = v - P;
	float mag = glm::length(diff);

	return (3 * ((diff.y * diff.y) + (mag * mag))) / mag;
}

float dydx_phi(const glm::vec3& v, const glm::vec3& P) {
	glm::vec3 diff = v - P;
	float mag = glm::length(diff);

	return (3 * diff.y * diff.x) / mag;
}

float dydz_phi(const glm::vec3& v, const glm::vec3& P) {
	glm::vec3 diff = v - P;
	float mag = glm::length(diff);

	return (3 * diff.y * diff.z) / mag;
}

float dz2_phi(const glm::vec3& v, const glm::vec3& P) {
	glm::vec3 diff = v - P;
	float mag = glm::length(diff);

	return (3 * ((diff.z * diff.z) + (mag * mag))) / mag;
}

float dzdx_phi(const glm::vec3& v, const glm::vec3& P) {
	glm::vec3 diff = v - P;
	float mag = glm::length(diff);

	return (3 * diff.z * diff.x) / mag;
}

float dzdy_phi(const glm::vec3& v, const glm::vec3& P) {
	glm::vec3 diff = v - P;
	float mag = glm::length(diff);

	return (3 * diff.z * diff.y) / mag;
}

glm::mat3 hessian_phi(const glm::vec3& x, const glm::vec3& P) {
	glm::mat3 out{
		dx2_phi(x, P), dxdy_phi(x, P), dxdz_phi(x, P),
		dydx_phi(x, P), dy2_phi(x, P), dydz_phi(x, P),
		dzdx_phi(x, P), dzdy_phi(x, P), dz2_phi(x, P)
	};

	return out;
};

std::vector<float> coefficients_f(const glm::vec3& x, const std::vector<SkeletalVertex>& centers) {
	std::vector<float> out;

	for (auto& p : centers) {
		float alpha{ 0.0f };
		float beta_x{ 0.0f };
		float beta_y{ 0.0f };
		float beta_z{ 0.0f };

		if (glm::distance(p.position, x) > FLT_EPSILON) {
			alpha = phi(glm::distance(x, p.position));
			beta_x = dx_phi(x, p.position);
			beta_y = dy_phi(x, p.position);
			beta_z = dz_phi(x, p.position);

			if (std::isnan(alpha) || std::isnan(beta_x) || std::isnan(beta_y) || std::isnan(beta_z)) {
				LOG("NaN coefficient produced\n");
			}
		}

		out.push_back(alpha);
		out.push_back(beta_x);
		out.push_back(beta_y);
		out.push_back(beta_z);
	}

	return out;
}

std::vector<glm::vec3> coefficients_grad_f(const glm::vec3& x, const std::vector<SkeletalVertex>& centers) {
	std::vector<glm::vec3> out;

	for (auto& p : centers) {
		glm::vec3 alpha{ 0.0f };
		glm::mat3 hessian{ 0.0f };

		if (glm::distance(p.position, x) > FLT_EPSILON) {
			alpha = { dx_phi(x, p.position), dy_phi(x, p.position), dz_phi(x, p.position) };

			hessian = hessian_phi(x, p.position);

			bool isnan = glm::isnan(alpha).x
				|| glm::isnan(alpha).y
				|| glm::isnan(alpha).z
				|| glm::isnan(hessian[0].x)
				|| glm::isnan(hessian[0].y)
				|| glm::isnan(hessian[0].z)
				|| glm::isnan(hessian[1].x)
				|| glm::isnan(hessian[1].y)
				|| glm::isnan(hessian[1].z)
				|| glm::isnan(hessian[2].x)
				|| glm::isnan(hessian[2].y)
				|| glm::isnan(hessian[2].z);

			if (isnan) {
				LOG("NaN coefficient produced\n");
			}
		}

		out.push_back(alpha);
		out.push_back(hessian[0]);
		out.push_back(hessian[1]);
		out.push_back(hessian[2]);
	}

	return out;
}

std::vector<SkeletalVertex> sample_points(const ElasticSkinning::MeshPart& part, size_t count) {
	auto unique_verts = part.mesh.vertices;

	// Exhaustive search/removal for all duplicate points
	auto last_it = [](std::vector<SkeletalVertex>::iterator begin, std::vector<SkeletalVertex>::iterator end) {
		auto new_end = end;

		for (auto itr = begin; itr < new_end; itr = std::next(itr)) {
			for (auto dup = begin; dup < new_end; dup = std::next(dup)) {
				if (itr != dup) {
					if (glm::distance(itr->position, dup->position) < FLT_EPSILON) {
						std::swap(*dup, *std::prev(new_end));
						new_end = std::prev(new_end);
						dup = std::prev(dup);
					}
				}
			}
		}

		return new_end;
	}(unique_verts.begin(), unique_verts.end());

	unique_verts.erase(last_it, unique_verts.end());

	// Remove points too close to the bone
	last_it = std::remove_if(unique_verts.begin(), unique_verts.end(), [&bone = part.bone](const SkeletalVertex& v) {
			const float h = 0.05f;
			glm::vec3 bonevec = bone.tail - bone.head;
			float val = glm::dot((v.position - bone.head), (bonevec)) / glm::dot(bonevec, bonevec);

			return val < h || val > (1.0f - h);
		}
	);

	unique_verts.erase(last_it, unique_verts.end());
	
	std::vector<float> minDists;

	for (auto& v1 : unique_verts) {
		float min = 1000000.0f;

		for (auto& v2 : unique_verts) {
			float dist = glm::distance(v1.position, v2.position);

			if (dist > FLT_EPSILON && dist < min) {
				min = dist;
			}
		}

		minDists.push_back(min);
	}

	auto [minItr, maxItr] = std::minmax_element(minDists.begin(), minDists.end());

	float minDist = *minItr;
	float maxDist = *maxItr;
	float avgDist = std::accumulate(minDists.begin(), minDists.end(), 0.0f) / static_cast<float>(minDists.size());

	size_t k = 60;

	auto take_samples = [&unique_verts, k](float r) -> std::vector<size_t> {
		std::vector<size_t> samples;
		std::vector<size_t> activesList;

		auto tooNearSamples = [&unique_verts, &samples, r](size_t p) -> bool {
			glm::vec3 test = unique_verts[p].position;

			for (auto& s : samples) {
				if (glm::distance(unique_verts[s].position, test) < r) {
					return true;
				}
			}

			return false;
		};

		auto sampleDistance = [&unique_verts](size_t a, size_t b) -> float {
			glm::vec3 pa = unique_verts[a].position;
			glm::vec3 pb = unique_verts[b].position;

			return glm::distance(pa, pb);
		};

		auto points_r_2r = [&unique_verts, &samples, r](size_t p) -> std::vector<size_t> {
			std::vector<size_t> out;

			for (size_t i = 0; i < unique_verts.size(); i++) {
				float dist = glm::distance(unique_verts[p].position, unique_verts[i].position);

				if (dist >= r && dist <= (2.0f * r)) {
					if (std::find(samples.begin(), samples.end(), i) == samples.end()) {
						out.push_back(i);
					}
				}
			}

			return out;
		};

		std::random_device seeder;
		std::mt19937 random(seeder());
		std::uniform_int_distribution<size_t> verts_dist(0, unique_verts.size() - 1);

		size_t initial_sample = verts_dist(random);
		samples.push_back(initial_sample);
		activesList.push_back(initial_sample);

		while (!activesList.empty()) {
			std::uniform_int_distribution<size_t> actives_dist(0, activesList.size() - 1);

			size_t sample_i = activesList[actives_dist(random)];

			std::vector<size_t> nearPoints = points_r_2r(sample_i);

			size_t bound = nearPoints.size() > k ? k : nearPoints.size();
			std::uniform_int_distribution<size_t> nears_dist(0, bound - 1);

			bool failed = bound == 0 ? true : false;

			for (size_t attempt = 0; attempt < k && attempt < nearPoints.size(); attempt++) {
				size_t test_p = nearPoints[nears_dist(random)];

				if (!tooNearSamples(test_p)) {
					samples.push_back(test_p);
					activesList.push_back(test_p);
				}

				if (attempt == (k - 1) || attempt == (nearPoints.size() - 1)) {
					failed = true;
				}
			}

			if (failed) {
				auto last_active = std::remove(activesList.begin(), activesList.end(), sample_i);
				activesList.erase(last_active, activesList.end());
			}
		}

		return samples;
	};

	std::vector<size_t> samples;

	float mul = 8.0f;
	while (samples.size() != count) {
		if (mul < 0.0f) {
			mul = 8.0f;
		}

		for (size_t i = 0; i < 20; i++) {
			samples = take_samples(minDist * mul);
			
			if (samples.size() == count) {
				break;
			}
		}

		mul += -0.1;
	}

	std::vector<SkeletalVertex> out;

	for (auto& idx : samples) {
		out.push_back(unique_verts[idx]);
	}

	SkeletalVertex boneHead;
	boneHead.position = part.bone.head;
	boneHead.normal = glm::normalize(part.bone.head - part.bone.tail);

	SkeletalVertex boneTail;
	boneTail.position = part.bone.tail;
	boneTail.normal = glm::normalize(part.bone.tail - part.bone.head);

	out.push_back(boneHead);
	out.push_back(boneTail);

	return out;
}

std::vector<glm::vec4> solve_constants(const std::vector<SkeletalVertex>& vertices) {
	std::vector<glm::vec4> out(vertices.size());

	std::vector<std::vector<float>> coefficients;
	std::vector<float> b;

	// first n rows of matrix
	std::for_each(vertices.rbegin(), vertices.rend(),
		[&vertices, &coefficients, &b](auto& p) {
			coefficients.push_back(coefficients_f(p.position, vertices));
			b.push_back(0.0f);
		}
	);

	// last 3n rows of matrix
	std::for_each(vertices.rbegin(), vertices.rend(),
		[&vertices, &coefficients, &b](auto& p) {
			auto gradient_coeffs = coefficients_grad_f(p.position, vertices);

			std::vector<float> fx_coeffs;
			std::vector<float> fy_coeffs;
			std::vector<float> fz_coeffs;

			for (auto& termSet : gradient_coeffs) {
				fx_coeffs.push_back(termSet.x);
			}

			for (auto& termSet : gradient_coeffs) {
				fy_coeffs.push_back(termSet.y);
			}

			for (auto& termSet : gradient_coeffs) {
				fz_coeffs.push_back(termSet.z);
			}

			coefficients.push_back(fx_coeffs);
			coefficients.push_back(fy_coeffs);
			coefficients.push_back(fz_coeffs);

			glm::vec3 n = glm::normalize(p.normal);

			b.push_back(n.x);
			b.push_back(n.y);
			b.push_back(n.z);
		}
	);

	size_t num_nans = 0;
	for (auto& row : coefficients) {
		for (auto& value : row) {
			if (std::isnan(value)) {
				num_nans++;
			}
		}
	}

	if (num_nans > 0) {
		size_t total_values = coefficients.size() * coefficients.size();
		double percent = 100.0 * (static_cast<double>(num_nans) / static_cast<double>(total_values));

		LOG("Coefficients matrix has %llu NANs\n", num_nans);
		LOG("Percent of total: %f \%\n", percent);
	}

	size_t dim = vertices.size() * 4;

	Eigen::MatrixXf coefficients_matrix(dim, dim);
	Eigen::VectorXf x_vector(dim);
	Eigen::VectorXf b_vector(dim);

	for (size_t i = 0; i < dim; i++) {
		for (size_t j = 0; j < dim; j++) {
			coefficients_matrix(i, j) = coefficients[i][j];
		}

		b_vector[i] = b[i];
	}

	x_vector = coefficients_matrix.fullPivLu().solve(b_vector);


	for (size_t i = 0; i < vertices.size(); i++) {
		size_t residx = i * 4;

		float alpha = x_vector[residx + 0];
		float beta_x = x_vector[residx + 1];
		float beta_y = x_vector[residx + 2];
		float beta_z = x_vector[residx + 3];

		out[i] = glm::vec4{ alpha, beta_x, beta_y, beta_z };	
	}

	return out;
}

float hrbf(const glm::vec3& x, const std::vector<glm::vec3>& centers, const std::vector<glm::vec4>& constants) {
	float sum = 0.0f;

	for (size_t i = 0; i < centers.size(); i++) {
		const glm::vec3& P = centers[i];
		float alpha{ constants[i].x };
		glm::vec3 beta{ constants[i].y, constants[i].z, constants[i].w };

		float dist = glm::distance(x, P);

		float term1 = alpha * phi(dist);
		float term2 = glm::dot(beta, gradient_phi(x, P));

		sum += term1 + term2;
	}

	return sum;
}

glm::vec3 hrbf_gradient(const glm::vec3& x, const std::vector<glm::vec3>& centers, const std::vector<glm::vec4>& constants) {
	glm::vec3 sum{ 0.0f, 0.0f, 0.0f };

	for (size_t i = 0; i < centers.size(); i++) {
		const glm::vec3& P = centers[i];
		float alpha{ constants[i].x };
		glm::vec3 beta{ constants[i].y, constants[i].z, constants[i].w };

		float dist = glm::distance(x, P);

		glm::vec3 term1 = ((x - P) / dist) * alpha * d_phi(dist);
		glm::vec3 term2 = hessian_phi(x, P) * beta;

		sum += term1 + term2;
	}

	return sum;
}

float hrbf_compact_map(float x, float r) {
	if (x < -r) {
		return 1.0f;
	}

	if (x > r) {
		return 0.0f;
	}

	float x_r = x / r;
	float x_r_3 = x_r * x_r * x_r;
	float x_r_5 = x_r_3 * x_r * x_r;

	return ((-3.0f / 16.0f) * x_r_5) + ((5.0f / 8.0f) * x_r_3) + ((-15.0f / 16.0f) * x_r) + 0.5f;
}

float hrbf_gradient_compact_map(float x, float r) {
	// Paper says -r < x < r but this doesn't match the original function's domains
	if ((std::abs(x) - r) > FLT_EPSILON) {
		return 0.0f;
	}

	float x_r = x / r;
	float x_r_2 = x_r * x_r;
	float x_r_4 = x_r_2 * x_r_2;

	return ((-15.0f / (16.0f * r)) * x_r_4) + ((15.0f / (8.0f * r)) * x_r_2) + (-15.0f / (16.0f * r));
}

ElasticSkinning::HRBFData union_hrbfs(const ElasticSkinning::HRBFData& a, const ElasticSkinning::HRBFData& b) {
	ElasticSkinning::HRBFData out;

	for (size_t z = 0; z < a.Depth; z++) {
		for (size_t y = 0; y < a.Height; y++) {
			for (size_t x = 0; x < a.Width; x++) {
				out.isofield.valref(x, y, z) = std::max(a.isofield.value(x, y, z), b.isofield.value(x, y, z));
			}
		}
	}

	return out;
}

float dc_theta(glm::vec3 a, glm::vec3 b) {
	float k = glm::dot(a, b);

	if (k >= 0.0f) {
		return 0.0f;
	}

	float k_2 = k * k;
	float k_3 = k_2 * k;
	float k_4 = k_3 * k;
	float k_8 = k_4 * k_4;

	// The secret behind this formula is I played around in desmos until
	// I got a curve that matched close enough to the dc graph
	// in the paper for theta > pi/2
	// -- Charles Ricchio
	return (k_3 / 8.0f) * ((-40.0f) + (-55.0f * k) + (-21.0f * k_2) + (-k_3) + (-7.0f * k_4) + (4.0f * k_8));
};

float db_theta(glm::vec3 a, glm::vec3 b) {
	float k = glm::dot(a, b);
	float k_3 = k * k * k;

	// The secret behind this formula is I played around in desmos again
	// until I got a curve that matched close enough to the db graph
	// in the paper for 0 < theta < pi
	// -- Charles Ricchio
	return (1.0f / 4.0f) * ((-3.0f * k) + k_3 + 2);
};

template <float(InterpFn)(glm::vec3 a, glm::vec3 b)>
ElasticSkinning::HRBFData gradient_blend_hrbfs(const ElasticSkinning::HRBFData& a, const ElasticSkinning::HRBFData& b) {
	ElasticSkinning::HRBFData out;

	for (size_t z = 0; z < a.Depth; z++) {
		for (size_t y = 0; y < a.Height; y++) {
			for (size_t x = 0; x < a.Width; x++) {
				glm::vec3 gradA = a.gradients.value(x, y, z);
				glm::vec3 gradB = b.gradients.value(x, y, z);

				glm::vec3 normA = glm::length(gradA) > FLT_EPSILON ? glm::normalize(gradA) : glm::vec3{ 0, 0, 0 };
				glm::vec3 normB = glm::length(gradB) > FLT_EPSILON ? glm::normalize(gradB) : glm::vec3{ 0, 0, 0 };

				float valA = a.isofield.value(x, y, z);
				float valB = b.isofield.value(x, y, z);

				float unionres = std::max(valA, valB);
				float blendres = valA + valB;

				float interp = InterpFn(normA, normB);

				out.isofield.valref(x, y, z) = std::lerp(unionres, blendres, interp);
				out.gradients.valref(x, y, z) = gradA + gradB;
			}
		}
	}

	return out;
}

ElasticSkinning::HRBFData contact_blend_hrbfs(const ElasticSkinning::HRBFData& a, const ElasticSkinning::HRBFData& b) {
	return gradient_blend_hrbfs<dc_theta>(a, b);
}

ElasticSkinning::HRBFData bulge_in_contact_blend_hrbfs(const ElasticSkinning::HRBFData& a, const ElasticSkinning::HRBFData& b) {
	return gradient_blend_hrbfs<db_theta>(a, b);
}

namespace ElasticSkinning {

	void create_debug_csv(const HRBFData& hrbf, const std::string& filename) {
		std::ofstream isofieldfile(filename + "_isofield.csv");
		std::ofstream gradientfile(filename + "_gradients.csv");
		std::ofstream centersfile(filename + "_centers.csv");

		isofieldfile << "x, y, z, value\n";
		gradientfile << "x, y, z, normx, normy, normz\n";

		for (size_t z = 0; z < hrbf.Depth; z++) {
			for (size_t y = 0; y < hrbf.Height; y++) {
				for (size_t x = 0; x < hrbf.Width; x++) {
					float isoval = hrbf.isofield.value(x, y, z);
					glm::vec3 gradval = hrbf.gradients.value(x, y, z);
					isofieldfile << x << ", " << y << ", " << z << ", " << isoval << "\n";
					gradientfile << x << ", " << y << ", " << z << ", " << gradval.x << ", " << gradval.y << ", " << gradval.z << "\n";
				}
			}
		}

		centersfile << "x, y, z\n";

		for (auto& p : hrbf.centers) {
			centersfile << p.x << ", " << p.y << ", " << p.z << ", " << "\n";
		}

		isofieldfile.close();
		gradientfile.close();
		centersfile.close();
	}

	void create_debug_csv(const std::unordered_map<StringHash, HRBFData>& hrbfs, const std::string& meshname) {
		for (auto& [name, part] : hrbfs) {
			std::string filename = meshname + "_HRBF_Part_" + std::to_string(name);

			create_debug_csv(part, filename);
		}
	}

	std::unordered_map<StringHash, MeshPart> partition_skeletal_mesh(const SkeletalMesh& mesh, Skeleton& skeleton) {
		std::unordered_map<StringHash, MeshPart> out;

		for (auto& name : skeleton.bone_names) {
			out[name] = {};
		}

		for (auto& vertex : mesh.vertices) {
			StringHash boneName = skeleton.bone_names[vertex.joints.x];

			out[boneName].mesh.vertices.push_back(vertex);
		}

		for (auto& [boneName, part] : out) {
			auto [parent, pe] = skeleton.get_bone_parent(boneName);
			part.bone.parent = parent;

			std::stack<StringHash> ancestorBones;

			ancestorBones.push(boneName);

			Retval<StringHash, Skeleton::Error> parentResult;
			while ((parentResult = skeleton.get_bone_parent(ancestorBones.top())), (parentResult.value != NULL_HASH)) {
				ancestorBones.push(parentResult.value);
			}

			glm::vec3 basePos{ 0, 0, 0 };
			glm::quat baseRot{ 1, 0, 0, 0 };

			while (!ancestorBones.empty()) {
				auto [parent, pe] = skeleton.get_bone(ancestorBones.top());

				basePos += baseRot * parent->position;
				baseRot = parent->rotation * baseRot;

				ancestorBones.pop();
			}

			part.bone.head = basePos;

			auto [children, be2] = skeleton.get_bone_children(boneName);

			if (children.size() > 0) {
				part.bone.children = children;

				auto [firstChild, be3] = skeleton.get_bone(children[0]);

				part.bone.tail = part.bone.head + (baseRot * firstChild->position);
			}
			else {
				float furthestVert = 0.0f;

				for (auto& fv : part.mesh.vertices) {
					if (glm::distance(fv.position, part.bone.head) > furthestVert) {
						furthestVert = glm::distance(fv.position, part.bone.head);
					}
				}

				glm::vec3 dirVec = baseRot * (glm::vec3(0, 1, 0) * furthestVert);
				part.bone.tail = part.bone.head + dirVec;
			}
		}

		return out;
	}

	std::unordered_map<StringHash, HRBFData> create_hrbf_data(const std::unordered_map<StringHash, MeshPart>& mesh_partitions) {
		std::unordered_map<StringHash, HRBFData> out;

		float maxAxis = 0.0f;

		for (auto& [name, meshPart] : mesh_partitions) {
			for (auto& p : meshPart.mesh.vertices) {
				if (std::abs(p.position.x) > maxAxis) {
					maxAxis = std::abs(p.position.x);
				}

				if (std::abs(p.position.y) > maxAxis) {
					maxAxis = std::abs(p.position.y);
				}

				if (std::abs(p.position.z) > maxAxis) {
					maxAxis = std::abs(p.position.z);
				}
			}
		}

		for (auto& [name, meshPart] : mesh_partitions) {
			std::vector<SkeletalVertex> samples = sample_points(meshPart, 50);

			std::vector<glm::vec4> constants = solve_constants(samples);

			out[name].constants = constants;

			out[name].centers.resize(samples.size());

			for (size_t i = 0; i < samples.size(); i++) {
				out[name].centers[i] = samples[i].position;
			}

			float maxDist = 0.0f;

			glm::vec3 boneVec = glm::normalize(meshPart.bone.tail - meshPart.bone.head);

			for (auto& p : samples) {
				glm::vec3 pAtO = p.position - meshPart.bone.head;

				glm::vec3 proj = glm::dot(pAtO, boneVec) * boneVec;

				if (glm::distance(pAtO, proj) > maxDist) {
					maxDist = glm::distance(pAtO, proj);
				}
			}

			out[name].Scale = maxAxis * 1.5f;

			for (size_t z = 0; z < out[name].Depth; z++) {
				for (size_t y = 0; y < out[name].Height; y++) {
					for (size_t x = 0; x < out[name].Width; x++) {
						float halfW = static_cast<float>(out[name].Width - 1) / 2.0f;
						float halfH = static_cast<float>(out[name].Height - 1) / 2.0f;
						float halfD = static_cast<float>(out[name].Depth - 1) / 2.0f;

						glm::vec3 point{
							static_cast<float>(x) - halfW,
							static_cast<float>(y) - halfH,
							static_cast<float>(z) - halfD
						};

						point *= glm::vec3{
							out[name].Scale / halfW,
							out[name].Scale / halfH,
							out[name].Scale / halfD
						};
						
						float f_x = hrbf(point, out[name].centers, constants);
						glm::vec3 grad_f_x = hrbf_gradient(point, out[name].centers, constants);
						float tr_f_x = hrbf_compact_map(f_x, maxDist);
						float dtr_f_x = hrbf_gradient_compact_map(f_x, maxDist);

						if (tr_f_x > 0.0f) {
							LOG("");
						}

						out[name].isofield.valref(x, y, z) = tr_f_x;
						out[name].gradients.valref(x, y, z) = dtr_f_x * grad_f_x;
					}
				}
			}
		}

		return out;
	}

	HRBFData compose_hrbfs(const std::unordered_map<StringHash, HRBFData>& hrbfs, const std::unordered_map<StringHash, MeshPart>& mesh_partitions) {
		HRBFData out;

		std::unordered_map<StringHash, HRBFData> intermediates = hrbfs;
		std::vector<StringHash> leafBones;

		for (auto& [boneName, part] : mesh_partitions) {
			if (part.bone.children.empty()) {
				leafBones.push_back(boneName);
			}
		}

		bool foundRoot = false;
		StringHash root = NULL_HASH;

		while (!foundRoot) {
			std::vector<StringHash> newLeaves;

			for (StringHash boneA : leafBones) {
				StringHash parent = mesh_partitions.at(boneA).bone.parent;

				if (parent == NULL_HASH) {
					foundRoot = true;

					root = boneA;

					break;
				}

				intermediates[parent] = contact_blend_hrbfs(intermediates[boneA], intermediates[parent]);

				intermediates.erase(boneA);
				leafBones.erase(std::remove(leafBones.begin(), leafBones.end(), boneA), leafBones.end());
				
				newLeaves.push_back(parent);
			}

			leafBones = newLeaves;
		}

		out = intermediates[root];

		out.Scale = hrbfs.begin()->second.Scale;

		return out;
	}

	MeshAndField convert_skeletal_mesh(const SkeletalMesh& mesh, Skeleton& skeleton) {
		auto partitions = partition_skeletal_mesh(mesh, skeleton);
		auto partFields = create_hrbf_data(partitions);
		auto outField = compose_hrbfs(partFields, partitions);

		ElasticMesh outMesh;
		outMesh.material_name = mesh.material_name;
		outMesh.indices = mesh.indices;

		outMesh.vertices.resize(mesh.vertices.size());

		for (size_t i = 0; i < mesh.vertices.size(); i++) {
			outMesh.vertices[i].position = mesh.vertices[i].position;
			outMesh.vertices[i].normal = mesh.vertices[i].normal;
			outMesh.vertices[i].color = mesh.vertices[i].color;
			outMesh.vertices[i].texcoords = mesh.vertices[i].texcoords;
			outMesh.vertices[i].isovalue = outField.sample_isofield(outMesh.vertices[i].position.x, outMesh.vertices[i].position.y, outMesh.vertices[i].position.z);
		}

		for (auto& [boneName, part] : partitions) {
			for (auto& v : part.mesh.vertices) {
				for (size_t i = 0; i < mesh.vertices.size(); i++) {
					if (glm::distance(mesh.vertices[i].position, v.position) < FLT_EPSILON) {
						outMesh.vertices[i].bone = v.joints.x;
					}
				}
			}
		}

		return { outMesh, outField, partFields };
	}

}
