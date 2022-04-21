#define EPSILON 1E-5
#define SIGMA 0.35

struct ElasticVertex {
	vec3 position;
	vec3 normal;
	vec3 color;
	vec2 texcoords;
	uint bone;
	float isovalue;
};

struct Bone {
	mat4 bind_matrix;
	mat4 inverse_bind_matrix;
	vec4 rotation; // Quaternion, x=w, y=x, z=y, w=x
	vec3 position;
	vec3 scale;
};

struct Vertex {
	vec3 position;
	vec3 normal;
	vec3 color;
	vec2 texcoords;
};

vec4 quat_mul(vec4 q1, vec4 q2) {
	return vec4(
		(q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z) - (q1.w * q1.w),
		(q1.x * q2.y) + (q1.y * q2.x) + (q1.z * q2.w) - (q1.w * q2.z),
		(q1.x * q2.z) - (q1.y * q2.w) + (q1.z * q2.x) + (q1.w * q2.y),
		(q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y) + (q1.w * q2.x)
	);
}

vec4 quat_conj(vec4 q) {
	return vec4(q.x, -q.y, -q.z, -q.w);
}

float quat_norm(vec4 q) {
	return length(q);
}

float quat_norm2(vec4 q) {
	return dot(q, q);
}

vec3 vec_quat_rotate(vec3 v, vec4 q) {
	vec4 qp = vec4(q.x, -q.y, -q.z, -q.w);
	vec4 v4 = vec4(0, v);
	
	return quat_mul(quat_mul(q, v4), qp).yzw;
}

vec3 transform_by_bone(vec3 p, Bone b) {
	vec3 boneRelPos = (b.inverse_bind_matrix * vec4(p, 1.0)).xyz;

	return vec_quat_rotate(boneRelPos, b.rotation) + b.position;
}

vec3 transform_by_bone_inv(vec3 p, Bone b) {
	vec3 p0 = vec_quat_rotate(p - b.position, quat_conj(b.rotation));

	return (b.bind_matrix * vec4(p0, 1.0)).xyz;
}

vec3 rotate_by_bone(vec3 v, Bone b) {
	vec3 boneRelPos = (b.inverse_bind_matrix * vec4(v, 0.0)).xyz;

	return vec_quat_rotate(boneRelPos, b.rotation);
}

vec3 grid_to_coords(ivec3 gridCoords, ivec3 gridDims, float scale) {
	vec3 halfDims = (vec3(gridDims) - vec3(1.0, 1.0, 1.0)) / 2.0;

	vec3 translatedP = vec3(gridCoords) - halfDims;

	vec3 scaledP = translatedP * (scale / halfDims);

	return scaledP;
}

vec3 coords_to_gridf(vec3 coords, ivec3 gridDims, float scale) {
	vec3 halfDims = (vec3(gridDims) - vec3(1.0, 1.0, 1.0)) / 2.0;

	vec3 scaledP = coords * (halfDims / scale);

	vec3 translatedP = scaledP + halfDims;

	return translatedP;
}

ivec3 coords_to_grid(vec3 coords, ivec3 gridDims, float scale) {
	return ivec3(coords_to_gridf(coords, gridDims, scale));
}

vec3 coords_to_sampler(vec3 coords, ivec3 gridDims, float scale) {
	vec3 grid = coords_to_gridf(coords, gridDims, scale);

	vec3 gridnorm = (grid + vec3(0.5, 0.5, 0.5)) / vec3(gridDims);

	return gridnorm;
}