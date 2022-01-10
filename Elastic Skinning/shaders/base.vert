#version 450

layout(std140, set = 0, binding = 0) readonly buffer ModelUBO {
	mat4 transforms[];
} models;

layout(set = 0, binding = 1) uniform CameraUBO {
	mat4 view;
	mat4 proj;
} camera;

layout(push_constant) uniform PushConstants {
	uint ModelId;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoords;

void main() {
	gl_Position = camera.proj * camera.view * models.transforms[push.ModelId] * vec4(inPosition, 1.0);
	fragColor = inColor;
	fragTexCoords = inTexCoords;
}