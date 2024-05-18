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
uniform float u_a;
uniform float u_diskRotationAngle;

uniform int u_maxSteps;
uniform float u_stepSize;
uniform float u_maxDistance;
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

layout(location = 0) out vec4 fragColour;
layout(location = 1) out vec4 brightColour;

float distance(vec4 x)
{
    // Euclidean distance from the 3-D origin.
    vec3 p = x.yzw;
    float r2 = dot(p, p);
    return sqrt(r2);
}

mat4 diag(vec4 v)
{
    return mat4(v.x, 0, 0, 0,
        0, v.y, 0, 0,
        0, 0, v.z, 0,
        0, 0, 0, v.w);
}

mat4 metric(vec4 x)
{
    // Minkowski metric tensor with signature (-+++)
    return diag(vec4(-1.0, 1.0, 1.0, 1.0));
}

mat4 invmetric(vec4 x)
{
    // Minkowski inverse metric tensor with signature (-+++)
    return diag(vec4(-1.0, 1.0, 1.0, 1.0));
}

float hamiltonian(vec4 x, vec4 p)
{
    // Calculate the Super-Hamiltonian for position x and momentum p.
    return 0.5 * dot(invmetric(x) * p, p);
}

vec4 dHdx(vec4 x, vec4 p, float dx)
{
    // Naive dH/dx calculation.
    vec4 Hdx;
    Hdx[0] = hamiltonian(x + vec4(dx, 0.0, 0.0, 0.0), p);
    Hdx[1] = hamiltonian(x + vec4(0.0, dx, 0.0, 0.0), p);
    Hdx[2] = hamiltonian(x + vec4(0.0, 0.0, dx, 0.0), p);
    Hdx[3] = hamiltonian(x + vec4(0.0, 0.0, 0.0, dx), p);
    return (Hdx - hamiltonian(x, p)) / dx;
}

void integrationStep(inout vec4 x, inout vec4 p, float dl)
{
    // Let l (for lambda) be the affine parameter.  Then, according to Hamilton's equations for the Super-Hamiltonian,
    // dx/dl = dH/dp = invmetric(x)*p      and      dp/dl = -dH/dx = -(1/2) * dot((d/dx)invmetric(x) * p, p).
    // So we can calcluate dH/dx naively (and ignore the more complicated expression with (d/dx)invmetric(x) entirely)
    // to get dp/dl and then update p by p_new = p_old + dp = p_old - dH/dx * dl
    // Here, dl is just an arbitrarily chosen step-size.  Making it smaller increases the simulation accuracy.
    // Once we have p_new, we can calculate dx/dl = invmetric(x)*p_new
    // and so we update x by x_new = x_old + dx = x_old + invmetric(x)*p_new * dl

    // TODO: check if it's faster to use the (d/dx)invmetric(x) since it only needs to construct one matrix and
    // evaluate it at a single point.  Use the python metric script to generate code for
    // the precomputed (d/dx)invmetric(x).

    float h = 0.01; // governs accuracy of dHdx
    p -= dHdx(x, p, h) * dl;
    x += invmetric(x) * p * dl;
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
    // x and p are the 4-D position and momentum coordinates, respectively, in spacetime.
    // Set x_t = 0 and p_t = -1.
    vec4 x = vec4(0.0, cameraPos);
    vec4 p = vec4(-1.0, rayDir);

    vec3 col = vec3(0.0);

    float dist;
    float intersectionParameter;
    vec4 planeIntersectionPoint;
    bool hitDisk = false;
    float horizon = 2 * u_BHMass; // G = c = 1
    bool hitSphere = false;
    vec4 previousx;

    for (int i = 0; i < u_maxSteps; i++)
    {
        previousx = x;
        integrationStep(x, p, u_stepSize);
        dist = distance(x);

        // Check if the ray hit the disk
        // Check to see whether the Cartesian y-coordinate changed signs, i.e. if the ray
        // crossed the disk's plane.
        if (x.z * previousx.z < 0.0)
        {
            // Just draw a straight line between x and previousx.  This is a fine approximation when both
            // points are close to the xz-plane.
            intersectionParameter = -previousx.z / (x.z - previousx.z);
            planeIntersectionPoint = previousx + (x - previousx) * intersectionParameter;
            dist = distance(planeIntersectionPoint);
            if (dist < u_OuterRadius && dist > u_InnerRadius)
            {
                hitDisk = true;
                // Need to pass in the sign of the previousx's Cartesian y-value to colour the
                // top and bottom of the disk differently in debug mode.
                col = getDiskColour(vec3(planeIntersectionPoint.y, sign(previousx.z), planeIntersectionPoint.w), disk);
                break;
            }
        }
        // Check if the ray hit the sphere
        else if (dist < horizon)
        {
            hitSphere = true;
            col = getSphereColour(x.yzw, sphere);
            break;
        }
        // Background
        else if (dist > u_maxDistance)
        {
            col = texture(skybox, vec3(-p.y, p.z, p.w)).xyz;
            break;
        }

    }

    if (!hitDisk && !hitSphere)
    {
        // Just cast the ray to the skybox.
        col = texture(skybox, vec3(-p.y, p.z, p.w)).xyz;
    }


    return col;
}


void main()
{
    // Make UVs go from -1 to 1.
    //vec2 uv = ((gl_FragCoord.xy / u_ScreenSize.zw) - 0.5) * 2.0;
    vec2 uv = (TexCoords - 0.5) * 2.0;

    // The image "screen" we're casting through in view space.
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