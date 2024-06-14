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

uniform sampler2D diskTexture;
uniform sampler2D sphereTexture;
uniform samplerCube skybox;

uniform mat4 u_View;
uniform mat4 u_ViewInv;
uniform mat4 u_Proj;
uniform mat4 u_ProjInv;
uniform vec3 u_cameraPos;
uniform vec4 u_ScreenSize;

uniform float u_Time;
uniform float u_InnerRadius;
uniform float u_OuterRadius;
uniform float u_BHRadius;
uniform float u_BHMass;
uniform float u_diskRotationAngle;

uniform int u_useDebugSphereTexture;
uniform int u_useDebugDiskTexture;
uniform vec3 u_sphereDebugColour1;
uniform vec3 u_sphereDebugColour2;
uniform vec3 u_diskDebugColourTop1;
uniform vec3 u_diskDebugColourTop2;
uniform vec3 u_diskDebugColourBottom1;
uniform vec3 u_diskDebugColourBottom2;

out vec4 fragColour;

float rayMarchSphereDistance(vec3 p, vec4 sphere)
{
    return length(p) - sphere.w;
}

float rayMarchDiskDistance(vec3 p, vec2 disk)
{
    float xzPlaneLength = length(vec2(p.x, p.z));
    if (xzPlaneLength > disk.y)
    {
        return sqrt( (xzPlaneLength - disk.y)*(xzPlaneLength - disk.y) + p.y*p.y);
    }
    else if (xzPlaneLength < disk.x)
    {
        return sqrt( (disk.x - xzPlaneLength)*(disk.x - xzPlaneLength) + p.y*p.y);
    }
    else
    {
        // Because the two sides of the disk can be different colours in debug, we have to be careful not to overshoot.
        // If we subtract off abs(p.y) and get -0.0, it's possible to view the colour on the opposite side when we're
        // directly over/under the disk and looking straight at it.  So we scale by a factor close to one to not overshoot.
        return 0.99 * abs(p.y);
    }
}

vec3 getDiskColour(vec2 disk, vec3 p)
{
    float pi = 3.14159265359;
    // For drawing the debug textures.
    float disk_divisions = 5.0;
    vec3 col = vec3(0.0);
    float u = mod((atan(p.x, p.z) + pi) / (2.0 * pi) - u_diskRotationAngle, 1.0);
    float v = 1 - (length(vec2(p.x, p.z)) - disk.x) / (disk.y - disk.x);
    if (u_useDebugDiskTexture == 1)
    {
        float usign = sign(sin(disk_divisions * 2.0 * pi * u));
        float uscaled = (usign + 1.0) / 2.0;
        if (p.y > 0.0)
        {
            col = uscaled * u_diskDebugColourTop1 + (1 - uscaled) * u_diskDebugColourTop2;
        }
        else
        {
            col = uscaled * u_diskDebugColourBottom1 + (1 - uscaled) * u_diskDebugColourBottom2;
        }
    }
    else
    {
        vec2 uv = vec2(u, v);
        col = texture(diskTexture, uv).xyz * pow(v, 0.1);
    }
    return col;
}

vec3 getSphereColour(vec4 sphere, vec3 p)
{
    float pi = 3.14159265359;
    // For drawing the debug textures.
    float sphere_latitudes = 7.0;
    float sphere_longitudes = 7.0;
    
    vec3 col = vec3(0.0);
    vec3 normal = normalize(p);
    float u = (atan(normal.x, normal.z) + pi) / (2.0 * pi);
    float v = asin(normal.y) / pi + 1.0 / 2.0;
    vec2 uv = vec2(u, v);
    if (u_useDebugSphereTexture == 1)
    {
        float usign = sign(sin(sphere_latitudes * 2.0 * pi * u));
        float vsign = sign(sin(sphere_longitudes * 2.0 * pi * v));
        float uvsign = usign * vsign;
        float uvscaled = (uvsign + 1.0) / 2.0;
        col = uvscaled * u_sphereDebugColour1 + (1 - uvscaled) * u_sphereDebugColour2;
    }
    else
    {
        col = vec3(0.0);
        //col = texture(sphereTexture, uv);
    }

    return col;

}

vec3 rayMarch(vec4 sphere, vec2 disk, vec3 cameraPos, vec3 rayDir)
{
    int maxSteps = 1000;
    float drawDistance = 1000;
    float epsilon = 0.0001;
    float pi = 3.14159265359;

    vec3 col = vec3(0.0);
    vec3 p = cameraPos;
    float dist = 0.0;
    float sphereDist, diskDist;
    rayDir = normalize(rayDir);

    for (int i = 0; i < maxSteps; i++)
    {
        p = cameraPos + rayDir * dist;
        sphereDist = rayMarchSphereDistance(p, sphere);
        diskDist = rayMarchDiskDistance(p, disk);
        dist += min(sphereDist, diskDist);

        if (sphereDist < epsilon)
        {
            col = getSphereColour(sphere, p);
            break;
        }
        else if (diskDist < epsilon)
        {
            col = getDiskColour(disk, p);
            break;
        }
        else if (dist > drawDistance)
        {
            // Proper skybox usage is: texture(skybox, view_dir) where view_dir is a vec3.
            col = texture(skybox, vec3( -rayDir.x, rayDir.y, rayDir.z )).xyz;
            break;
        }
    }

    return col;
}


void main()
{
    int AA = 1;
    vec3 col = vec3(0.0);
    for (int m = 0; m < AA; m++)
    {
        for (int n = 0; n < AA; n++)
        {
            // Make UVs go from -1 to 1.
            vec2 uv = (((gl_FragCoord.xy + vec2(float(m), float(n))/float(AA)) / u_ScreenSize.zw) - 0.5) * 2.0;

            // The image "screen" we're casting through in view space.
            vec4 screen = u_ProjInv * vec4(uv, 1.0, 1.0);
            // The ray direction in world space.
            vec3 rayDir = normalize(vec3(u_ViewInv * vec4(normalize(screen.xyz / screen.w), 0.0)));

            // For simplicity, the sphere and disk are centered at (0,0,0).  The disk is in the xz-plane at y=0.
            // vec4 sphere is (vec3(center), radius)
            vec4 sphere = vec4(vec3(0.0), u_BHRadius);
            vec2 disk = vec2(u_InnerRadius, u_OuterRadius);

            col += rayMarch(sphere, disk, u_cameraPos, rayDir);
        }
    }
    col /= float(AA * AA);
    fragColour = vec4(col, 1.0);
}