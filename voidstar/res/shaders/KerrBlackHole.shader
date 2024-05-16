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

uniform int u_useDebugSphereTexture;
uniform int u_useDebugDiskTexture;
uniform vec3 u_sphereDebugColour1;
uniform vec3 u_sphereDebugColour2;
uniform vec3 u_diskDebugColourTop1;
uniform vec3 u_diskDebugColourTop2;
uniform vec3 u_diskDebugColourBottom1;
uniform vec3 u_diskDebugColourBottom2;

layout(location = 0) out vec4 fragColour;
layout(location = 1) out vec4 brightColour;

float implicitr(vec4 x)
{
    // Calculate the implicit r value in the Kerr metric from the position.
    // This ends up being a polynomial of the form r**4 + b*r**2 + c = 0.
    vec3 p = x.yzw;
    float b = u_a * u_a - dot(p, p);
    float c = -u_a * u_a * p.y * p.y;
    float r2 = 0.5 * (-b + sqrt(b * b - 4.0 * c));
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
    // Calculate the Kerr metric in Kerr-Schild coordinates at the point x.  G = c = 1.
    vec3 p = x.yzw;
    float r = implicitr(x);
    float r2 = r * r;
    float a2 = u_a * u_a;
    float f = 2*u_BHMass*r2*r / (r2*r2 + u_a*u_a*p.y*p.y);
    vec4 k = vec4(1.0, (r * p.x - u_a * p.z) / (r2 + a2), p.y / r, (r * p.z + u_a * p.x) / (r2 + a2));
    return diag(vec4(-1.0, 1.0, 1.0, 1.0)) + f * mat4(k.x * k, k.y * k, k.z * k, k.w * k);
}

mat4 invmetric(vec4 x)
{
    // Calculate the inverse Kerr metric in Kerr-Schild coordinates at the point x.  G = c = 1.
    //return inverse(metric(x));
    vec3 p = x.yzw;
    float r = implicitr(x);
    float r2 = r * r;
    float a2 = u_a * u_a;
    float f = 2 * u_BHMass * r2 * r / (r2 * r2 + u_a * u_a * p.y * p.y);
    vec4 k = vec4(-1.0, (r * p.x - u_a * p.z) / (r2 + a2), p.y / r, (r * p.z + u_a * p.x) / (r2 + a2));
    return diag(vec4(-1.0, 1.0, 1.0, 1.0)) - f * mat4(k.x * k, k.y * k, k.z * k, k.w * k);
}

float hamiltonian(vec4 x, vec4 p)
{
    // Calculate the Super-Hamiltonian for position x and momentum p.
    return 0.5 * dot(invmetric(x) * p, p);
}

vec4 dHdx(vec4 x, vec4 p, float dx)
{
    // Naive dH/dx calculation.
    vec4 dHdx;
    dHdx[0] = hamiltonian(x + vec4(dx, 0.0, 0.0, 0.0), p);
    dHdx[1] = hamiltonian(x + vec4(0.0, dx, 0.0, 0.0), p);
    dHdx[2] = hamiltonian(x + vec4(0.0, 0.0, dx, 0.0), p);
    dHdx[3] = hamiltonian(x + vec4(0.0, 0.0, 0.0, dx), p);
    return (dHdx - hamiltonian(x, p)) / dx;
}

void integrationStep(inout vec4 x, inout vec4 p, float stepsize)
{
    // Let l (for lambda) be the affine parameter.  Then, according to Hamilton's equations from the Super-Hamiltonian,
    // dx/dl = invmetric(x)*p      and      dp/dl = -dH/dx.
    // So we can calcluate dH/dx naively to get dp/dl and then calculate p_new = p_old + dp = p_old - dH/dx * dl
    // Here, dl is just an arbitrarily chosen step-size.  Making it smaller increases the simulation accuracy.
    // Once we have p_new, we can calculate dx/dl = invmetric(x)*p_new
    // and so x_new = x_old + dx = x_old + invmetric(x)*p_new * dl
    float h = 0.01; // governs accuracy of dHdx
    vec4 dpdl = dHdx(x, p, h);
    float dl = stepsize;
    p -= dpdl * dl;
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
    // Makes the checkerboard pattern on the sphere.
    vec2 uvsignvec = sign(sin(2.0 * pi * vec2(sphere_latitudes * u, sphere_longitudes * v)));
    float uvsign = uvsignvec.x * uvsignvec.y;
    float uvscaled = (uvsign + 1.0) * 0.5;
    col = uvscaled * u_sphereDebugColour1 + (1.0 - uvscaled) * u_sphereDebugColour2;

    // Draw an actual texture instead.
    //vec2 uv = vec2(u, v);
    //col = texture(sphereTexture, uv);

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

    if (u_useDebugDiskTexture == 1)
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
    int maxSteps = u_maxSteps;
    float maxDistance = u_maxDistance;
    float maxDistanceSquared = maxDistance * maxDistance;
    float epsilon = u_epsilon;
    float stepsize = u_stepSize;

    vec3 col = vec3(0.0);
    vec4 x = vec4(0.0, cameraPos);
    vec4 p = vec4(-1.0, rayDir);

    float dist;
    float intersectionParameter;
    vec4 planeIntersectionPoint;
    bool hitDisk = false;
    bool hitSphere = false;
    vec4 previousx;

    for (int i = 0; i < maxSteps; i++)
    {
        previousx = x;
        integrationStep(x, p, stepsize);
        dist = implicitr(x);

        // Check if the ray hit anything
        // Disk
        if (x.z * previousx.z < 0.0) // Check to see whether the Cartesian y-coordinate changed signs
        {
            intersectionParameter = -previousx.z / (x.z - previousx.z);
            planeIntersectionPoint = previousx + (x - previousx)*intersectionParameter;
            dist = implicitr(planeIntersectionPoint);
            if (dist < u_OuterRadius && dist > u_InnerRadius)
            {
                hitDisk = true;
                col = getDiskColour(vec3(planeIntersectionPoint.y, sign(previousx.z), planeIntersectionPoint.w), disk);
                break;
            }
        }
        // Sphere
        else if (dist < u_BHMass + sqrt(u_BHMass * u_BHMass - u_a * u_a))
        {
            hitSphere = true;
            if (u_useDebugSphereTexture == 1)
            {
                col = getSphereColour(x.yzw, sphere);
            }
            break;
        }
        // Background
        else if (dist > maxDistance)
        {
            vec4 dir = invmetric(x) * p;
            //vec4 dir = p;
            // Proper skybox usage is: texture(skybox, view_dir) where view_dir is a vec3.
            col = texture(skybox, vec3(-dir.y, dir.z, dir.w)).xyz;
            break;
        }
        
    }

    if (!hitDisk && !hitSphere)
    {
        vec4 dir = invmetric(x) * p;
        col = texture(skybox, vec3(-dir.y, dir.z, dir.w)).xyz;
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