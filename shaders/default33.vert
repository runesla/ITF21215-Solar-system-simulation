#version 330

// Incoming vertex position, Model Space.
layout (location = 0) in vec3 position;

// Incoming vertex color.
layout (location = 1) in vec3 normal;

// Incoming normal
layout (location = 2) in vec2 uv;

// Projection, view and model matrices.
uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

// Output variables
out vec2 UV;
out vec3 N;
out vec3 worldVertex;

void main() {

    // Normally gl_Position is in Clip Space and we calculate it by multiplying together all the matrices
    gl_Position = proj * (view * (model * vec4(position, 1)));

    // Set the world vertex for calculating the light direction in the fragment shader
    worldVertex = vec3(model * vec4(position, 1));

    // Set the transformed normal
    N = mat3(model) * normal;

    // We assign the uv to the outgoing variable.
    UV = uv;

}
