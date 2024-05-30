#shader vertex
#version 430 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tcs;

out vec2 TexCoords;

void main()
{
    TexCoords = tcs;
    gl_Position = vec4(position, 1.0);
}

#shader fragment
#version 430 core
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

uniform int u_maxSteps;
uniform float u_drawDistance;
uniform float u_epsilon;

uniform bool u_useSphereTexture;
uniform bool u_useDebugSphereTexture;
uniform bool u_useDebugDiskTexture;
uniform vec3 u_sphereDebugColour1;
uniform vec3 u_sphereDebugColour2;
uniform vec3 u_diskDebugColourTop1;
uniform vec3 u_diskDebugColourTop2;
uniform vec3 u_diskDebugColourBottom1;
uniform vec3 u_diskDebugColourBottom2;

layout (location=0) out vec4 fragColour;
layout (location=1) out vec4 brightColour;

float rayMarchSphereDistance(vec3 p, vec4 sphere)
{
    return length(p) - sphere.w;
}

float rayMarchDiskDistance(vec3 p, vec2 disk)
{
    float xzPlaneLength = length(vec2(p.x, p.z));
    // Signed distance from the outer edge of the disk.  It's positive when (p.x, p.z) is outside of the disk.
    float outerDist = xzPlaneLength - disk.y;
    // Signed distance from the inner edge of the disk.  It's positive when (p.x, p.z) is inside of the disk.
    float innerDist = disk.x - xzPlaneLength;
    // If (p.x, p.z) is outside of the disk, then xzDist = outerDist,
    // if (p.x, p.z) is inside of the disk, then xzDist = innerDist,
    // and if (p.x, p.z) is on the disk, then xzDist = 0.0.
    float xzDist = max(max(outerDist, innerDist), 0.0);
    // Exact ray march distance.
    return sqrt(xzDist * xzDist + p.y * p.y);
}

vec3 getSphereColour(vec3 p, vec4 sphere)
{
    vec3 col = vec3(0.0);
    float pi = 3.14159265359;
    // Number of divisions of the sphere for the checkboard pattern.
    float sphere_latitudes = 7.0;
    float sphere_longitudes = 7.0;

    vec3 normal = normalize(p);
    float u = (atan(normal.x, normal.z) + pi) / (2.0 * pi);
    float v = asin(normal.y) / pi + 0.5;
    if (u_useDebugSphereTexture)
    {
        // Makes the checkerboard pattern on the sphere.
        vec2 uvsignvec = sign(sin(2.0 * pi * vec2(sphere_latitudes * u, sphere_longitudes * v)));
        float uvsign = uvsignvec.x * uvsignvec.y;
        float uvscaled = (uvsign + 1.0) * 0.5;
        col = uvscaled * u_sphereDebugColour1 + (1.0 - uvscaled) * u_sphereDebugColour2;
    }

    else if (u_useSphereTexture)
    {
        // Draw an actual texture instead.
        vec2 uv = vec2(u, v);
        col = texture(sphereTexture, uv).xyz;
    }

    return col;
}

vec3 getDiskColour(vec3 p, vec2 disk)
{
    float pi = 3.14159265359;
    vec3 col = vec3(0.0);
    // For drawing the debug textures.
    float disk_divisions = 5.0;

    float u = (atan(p.x, p.z) + pi) / (2.0 * pi) - u_diskRotationAngle;
    u = u - floor(u); // equivalent to fract(u) since we want u to be between 0 and 1.
    float v = 1.0 - (length(vec2(p.x, p.z)) - disk.x) / (disk.y - disk.x);

    if (u_useDebugDiskTexture)
    {
        // Makes alternating pattern between the two different colours.
        float usign = sign(sin(disk_divisions * 2.0 * pi * u));
        float uscaled = (usign + 1.0) * 0.5;
        if (p.y >= 0)
        {
            col = uscaled * u_diskDebugColourTop1 + (1.0 - uscaled) * u_diskDebugColourTop2;
        }
        else
        {
            col = uscaled * u_diskDebugColourBottom1 + (1.0 - uscaled) * u_diskDebugColourBottom2;
        }
    }
    else
    {
        vec2 uv = vec2(u, v);
        col = texture(diskTexture, uv).xyz * pow(v, 0.1);
    }
    return col;
}

vec3 rayMarch(vec4 sphere, vec2 disk, vec3 cameraPos, vec3 rayDir)
{
    //int maxSteps = 2000;
    int maxSteps = u_maxSteps;
    //float drawDistance = 1000.0;
    float drawDistance = u_drawDistance;
    float drawDistanceSquared = drawDistance * drawDistance;
    float epsilon = 0.0001;

    vec3 col = vec3(0.0);
    vec3 p = cameraPos;

    float sphereDist, diskDist;
    float rayMarchStepsize;
    float curvedSpaceStepsize;
    float stepsize;

    float invMass = 1.0 / u_BHMass;
    float BHDistSquared;
    float BHDist;
    float bendingAcceleration;

    for (int i = 0; i < maxSteps; i++)
    {
        sphereDist = rayMarchSphereDistance(p, sphere);
        diskDist = rayMarchDiskDistance(p, disk);
        BHDistSquared = dot(p, p);

        // Check if the ray hit anything
        // Sphere
        if (sphereDist < epsilon)
        {
            col = getSphereColour(p, sphere);
            break;
        }
        // Disk
        else if (diskDist < epsilon)
        {
            col = getDiskColour(p, disk);
            break;
        }
        // Background
        else if (BHDistSquared > drawDistanceSquared)
        {
            // Proper skybox usage is: texture(skybox, view_dir) where view_dir is a vec3.
            col = texture(skybox, vec3(-rayDir.x, rayDir.y, rayDir.z)).xyz;
            break;
        }

        BHDist = sqrt(BHDistSquared);
        rayMarchStepsize = min(sphereDist, diskDist);
        // Note: u_BHMass / distBHSquared is a measure of how curved space is (G is taken to be 1).  The
        // more space is curved, the smaller the step we want to take.  Likewise, if space is flatter, we want
        // to take a larger step.  Therefore, our desired step size for the curving of light is inversely proportional
        // to u_BHMass / distBHSquared.
        curvedSpaceStepsize = BHDistSquared * invMass;
        // The constant out front is just to help with accuracy and numerical stability, but it comes at
        // the cost of more work for the GPU.  Smaller values give better accuracy and stability.
        stepsize = 0.5 * min(rayMarchStepsize, curvedSpaceStepsize);

        // Technically, this takes account not only the curvature of time (as in Newtonian gravity), but also
        // the curvature of space.  In plain Newtonian, we'd have a 1.0 factor for the bending acceleration, but
        // because we also know space curves and Einstein found that the curving of space doubles the bending,
        // we simply account for this with a factor of 2.0.  For strictly Newtonian gravity, use 1.0, but then the
        // photon rings will no longer be visible, e.g.  Technically speaking, the 2.0 is the result of the static weak-field
        // limit of the Schwarzchild metric.  It is the classical Newtonian limit where we take 2*G*M/(c^2*r) << 1.
        bendingAcceleration = 2.0 * u_BHMass / (BHDistSquared * BHDist);
        rayDir = normalize(rayDir - stepsize * bendingAcceleration * p);
        p += rayDir * stepsize;
    }

    return col;
}


void main()
{
    // Make UVs go from -1 to 1.
    //vec2 uv = ((gl_FragCoord.xy / u_ScreenSize.zw) - 0.5) * 2.0;
    vec2 uv = (TexCoords - 0.5) * 2.0;

    // The image "screen" we're casting through in view space.
    //vec4 screen = u_ProjInv * vec4(uv, 1.0, 1.0);
    vec4 screen = u_ProjInv * vec4(uv, 1.0, 1.0);
    // The ray direction in world space at the current pixel.
    vec3 rayDir = vec3(u_ViewInv * vec4(normalize(screen.xyz / screen.w), 0.0));

    // For simplicity, the sphere and disk are centered at (0,0,0).  The disk is in the xz-plane at y=0.
    vec4 sphere = vec4(vec3(0.0), u_BHRadius);
    vec2 disk = vec2(u_InnerRadius, u_OuterRadius);

    fragColour = vec4(rayMarch(sphere, disk, u_cameraPos, rayDir), 1.0);

    float brightness = dot(fragColour.rgb, vec3(0.2126, 0.7152, 0.0722));
    // TODO: adjust the rayMarch brightness for hdr
    if (brightness > 0.95)
        brightColour = vec4(fragColour.rgb, 1.0);
    else
        brightColour = vec4(0.0, 0.0, 0.0, 1.0);
}