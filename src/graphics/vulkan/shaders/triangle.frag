#version 450

// Fragment shader for simple colored triangle test

// Input from vertex shader
layout(location = 0) in vec4 fragColor;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // Output the interpolated vertex color
    outColor = fragColor;
}
