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

layout (binding=0) uniform sampler2D sceneTexture;
layout (binding=1) uniform sampler2D blurTexture;
uniform vec4 u_ScreenSize;
uniform bool u_bloom;
uniform float u_exposure;
uniform float u_gamma;

void main()
{
    vec3 result;
    vec3 hdrColour = texture(sceneTexture, TexCoords).rgb;
    vec3 bloomColour = texture(blurTexture, TexCoords).rgb;
    if (u_bloom)
    {
        hdrColour += bloomColour;
        // Tone Mapping
        result = vec3(1.0) - exp(-hdrColour * u_exposure);
        // Gamma Correction
        result = pow(result, vec3(1.0 / u_gamma));
    }
    else
    {
        result = hdrColour;
    }
    colour = vec4(result, 1.0);
}