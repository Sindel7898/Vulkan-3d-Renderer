#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(texture(texSampler, fragTexCoord).rgb, 1.0);
}