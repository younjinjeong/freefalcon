#version 450

// Vertex shader for simple colored triangle test

// Input vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

// Output to fragment shader
layout(location = 0) out vec4 fragColor;

// Uniform buffer (matrices)
layout(binding = 0) uniform UniformBufferObject {
    mat4 world;
    mat4 view;
    mat4 projection;
} ubo;

void main() {
    // Transform vertex position
    gl_Position = ubo.projection * ubo.view * ubo.world * vec4(inPosition, 1.0);

    // Pass color to fragment shader
    fragColor = inColor;
}
