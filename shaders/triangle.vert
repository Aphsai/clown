#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 color;
layout (location = 3) in vec2 tex_coord;

layout (location = 0) out vec3 out_color;
layout (location = 1) out vec2 out_tex_coord;

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
} camera_buffer;

struct GPUObjectData {
    mat4 model;
};

layout (std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
    GPUObjectData objects[];
} object_buffer;

layout (push_constant) uniform constants {
    vec4 data;
    mat4 render_matrix;
} push_constants;

void main() {
    mat4 model_matrix = object_buffer.objects[gl_BaseInstance].model;
    mat4 transform_matrix = (camera_buffer.viewproj) * push_constants.render_matrix;
    gl_Position = transform_matrix * vec4(position, 1.0f);
    out_color = color;
    out_tex_coord = tex_coord;
}
