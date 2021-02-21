#version 450

layout (location = 0) in vec3 in_color;
layout (location = 1) in vec2 tex_coord;

layout (location = 0) out vec4 out_frag_color;

layout(set = 0, binding = 1) uniform SceneData {
    vec4 fog_color;
    vec4 fog_distances;
    vec4 ambient_color;
    vec4 sunlight_direction;
    vec4 sunlight_color;
} scene_data;

layout(set = 2, binding = 0) uniform sampler2D tex;

void main() {
    vec3 color = texture(tex, tex_coord).xyz;
    out_frag_color = vec4(color, 1.0f);
}
