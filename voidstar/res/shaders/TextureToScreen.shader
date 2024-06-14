#shader vertex
#version 460 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tcs;

out vec2 TexCoords;

void main()
{
    TexCoords = tcs;
    gl_Position = vec4(position, 1.0);
}


#shader fragment
#version 460 core
out vec4 colour;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec4 u_ScreenSize;

void main()
{
    colour = texture(screenTexture, TexCoords);
}