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

uniform int u_msaa;

uniform int u_maxSteps;
uniform float u_stepSize;
uniform float u_maxDistance;
uniform float u_epsilon;

uniform bool u_useSphereTexture;
uniform bool u_useDebugSphereTexture;
uniform bool u_drawBasicDisk;
uniform bool u_transparentDisk;
uniform bool u_useDebugDiskTexture;
uniform vec3 u_sphereDebugColour1;
uniform vec3 u_sphereDebugColour2;
uniform vec3 u_diskDebugColourTop1;
uniform vec3 u_diskDebugColourTop2;
uniform vec3 u_diskDebugColourBottom1;
uniform vec3 u_diskDebugColourBottom2;

uniform bool u_bloom;
uniform float u_bloomThreshold;
uniform float u_diskAbsorption;
uniform float u_bloomBackgroundMultiplier;
uniform float u_bloomDiskMultiplier;
uniform float u_exposure;
uniform float u_gamma;
uniform float u_brightnessFromDistance;
uniform float u_brightnessFromDiskVel;
uniform float u_colourshiftPower;
uniform float u_colourshiftMultiplier;
uniform float u_colourshiftOffset;

layout(location = 0) out vec4 fragColour;
layout(location = 1) out vec4 brightColour;


float implicitr(vec4 x)
{
    // Calculate the implicitly defined r value in the Kerr-Schild coordinates from the position.
    // This reduces to solving a polynomial of the form r**4 + b*r**2 + c = 0.
    vec3 p = x.yzw;
    float b = u_a*u_a - dot(p, p);
    float c = -u_a*u_a * p.y*p.y;
    float r2 = 0.5 * (-b + sqrt(b*b - 4.0*c));
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
    // This differs slightly from the definition in https://arxiv.org/abs/0706.0622
    // because that paper has Cartesian z as the up direction, while this program has Cartesian y
    // as the up direction.
    vec3 p = x.yzw;
    float r = implicitr(x);
    float r2 = r*r;
    float a2 = u_a*u_a;
    float f = 2.0 * u_BHMass * r2*r / (r2*r2 + a2*p.y*p.y);
    vec4 k = vec4(1.0, (r*p.x - u_a*p.z) / (r2 + a2), p.y / r, (r*p.z + u_a*p.x) / (r2 + a2));
    return diag(vec4(-1.0, 1.0, 1.0, 1.0)) + f * mat4(k.x * k, k.y * k, k.z * k, k.w * k);
}

mat4 invmetric(vec4 x)
{
    // Calculate the inverse Kerr metric in Kerr-Schild coordinates at the point x.  G = c = 1.
    // As with the metric, this is also from https://arxiv.org/abs/0706.0622 with adjusted coordinates.
    vec3 p = x.yzw;
    float r = implicitr(x);
    float r2 = r*r;
    float a2 = u_a*u_a;
    float f = 2.0 * u_BHMass * r2*r / (r2*r2 + a2*p.y*p.y);
    vec4 k = vec4(-1.0, (r*p.x - u_a*p.z) / (r2 + a2), p.y / r, (r*p.z + u_a*p.x) / (r2 + a2));
    return diag(vec4(-1.0, 1.0, 1.0, 1.0)) - f * mat4(k.x * k, k.y * k, k.z * k, k.w * k);
}

vec3 pToDir(vec4 x, vec4 p)
{
    // dx^i/dt = g^ij * p_j
    // We discard the time "direction", dx^0/dt.
    vec4 dxdt = invmetric(x) * p;
    return normalize(dxdt.yzw);
}

float H(vec4 x, vec4 p)
{
    // Calculate the Super-Hamiltonian for position x and momentum p.
    return 0.5 * dot(invmetric(x)*p, p);
}

vec4 dHdx(vec4 x, vec4 p)
{
    // Naive dH/dx calculation.
    vec4 Hdx;
    float dx = 0.01; // Governs accuracy of dHdx
    Hdx[0] = H(x + vec4(dx, 0.0, 0.0, 0.0), p);
    Hdx[1] = H(x + vec4(0.0, dx, 0.0, 0.0), p);
    Hdx[2] = H(x + vec4(0.0, 0.0, dx, 0.0), p);
    Hdx[3] = H(x + vec4(0.0, 0.0, 0.0, dx), p);
    return (Hdx - H(x, p)) / dx;
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
    // For more details, see https://web.mit.edu/edbert/GR/gr3.pdf.  The above is from equations (9) and (10)
    // in that paper.

    // TODO: check if it's faster to use the (d/dx)invmetric(x) expression since it only needs to construct one
    // matrix and evaluate it at a single point.  Use the python metric script to generate code for the
    // precomputed (d/dx)invmetric(x).

    p -= dHdx(x, p) * dl;
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
    float v = clamp(1.0 - (length(vec2(p.x, p.z)) - disk.x) / (disk.y - disk.x), 0.0, 1.0);

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
        float radialDimming = clamp((1.0 + -exp(-10.0 * v)), 0.0, 1.0);
        col = texture(diskTexture, uv).xyz * radialDimming;
    }
    return col;
}


vec3 colorTemperatureToRGB(const in float temperature) {
    // Valid from 1000 to 40000 K (and additionally 0 for pure full white)
    // Values from: http://blenderartists.org/forum/showthread.php?270332-OSL-Goodness&p=2268693&viewfull=1#post2268693   
    mat3 m = (temperature <= 6500.0) ? mat3(vec3(0.0, -2902.1955373783176, -8257.7997278925690),
        vec3(0.0, 1669.5803561666639, 2575.2827530017594),
        vec3(1.0, 1.3302673723350029, 1.8993753891711275)) :
        mat3(vec3(1745.0425298314172, 1216.6168361476490, -8257.7997278925690),
            vec3(-2666.3474220535695, -2173.1012343082230, 2575.2827530017594),
            vec3(0.55995389139931482, 0.70381203140554553, 1.8993753891711275));
    return mix(clamp(vec3(m[0] / (vec3(clamp(temperature, 1000.0, 40000.0)) + m[1]) + m[2]), vec3(0.0), vec3(1.0)), vec3(1.0), smoothstep(1000.0, 0.0, temperature));
}


vec3 rayMarch(vec4 sphere, vec2 disk, vec3 cameraPos, vec3 rayDir)
{
    // x and p are the 4-D position and momentum coordinates, respectively, in spacetime.
    // p_i = g_ij * dx^j/dt
    // Set x_t = 0 and dx^0/dt = 1.
    vec4 x = vec4(0.0, cameraPos);
    vec4 p = metric(x) * vec4(1.0, normalize(rayDir));

    vec3 diskSample = vec3(0.0);
    vec3 col = vec3(0.0);

    float bloomDiskMultiplier;
    float bloomBackgroundMultiplier;
    if (u_bloom)
    {
        bloomDiskMultiplier = u_bloomDiskMultiplier;
        bloomBackgroundMultiplier = u_bloomBackgroundMultiplier;
    }
    else
    {
        bloomDiskMultiplier = 1.0;
        bloomBackgroundMultiplier = 1.0;
    }

    vec3 dir;
    float dist;
    // Transmittance
    float T = 1.0;
    float absorption;
    float intersectionParameter;
    vec4 planeIntersectionPoint;
    bool hitDisk = false;
    float horizon = u_BHMass + sqrt(u_BHMass*u_BHMass - u_a*u_a);
    bool hitSphere = false;
    vec4 previousx;

    for (int i = 0; i < u_maxSteps; i++)
    {
        previousx = x;
        integrationStep(x, p, u_stepSize);
        dist = implicitr(x);

        // Check if the ray hit the disk
        // Check to see whether the Cartesian y-coordinate changed signs, i.e. if the ray
        // crossed the disk's plane.
        if (x.z * previousx.z < 0.0)
        {
            // Just draw a straight line between x and previousx.  This is a fine approximation when both
            // points are close to the xz-plane.
            intersectionParameter = -previousx.z / (x.z - previousx.z);
            planeIntersectionPoint = previousx + (x - previousx)*intersectionParameter;
            dist = implicitr(planeIntersectionPoint);
            if (dist <= u_OuterRadius && dist >= u_InnerRadius)
            {
                hitDisk = true;
                // Need to pass in the sign of the previousx's Cartesian y-value to colour the
                // top and bottom of the disk differently in debug mode.
                diskSample = getDiskColour(vec3(planeIntersectionPoint.y, sign(previousx.z), planeIntersectionPoint.w), disk);

                // Brightness of the disk is adapted from https://www.shadertoy.com/view/MctGWj
                // TODO: do a more realistic colouring of the accretion disk
                // (see (vii) on page 33 of https://arxiv.org/pdf/1502.03808)
                // For photons in free space, specific intensity (brightness) I(v) is proportional to 1/v**3
                if (!u_drawBasicDisk)
                {
                    vec4 discVel = vec4(-(dist + u_a / sqrt(dist)), vec3(-planeIntersectionPoint.w, 0.0, planeIntersectionPoint.y) * sign(u_a) / sqrt(dist)) / sqrt(dist * dist - 3.0 * dist + 2.0 * u_a * sqrt(dist));
                    float colourLookup = pow(dot(p, discVel), -u_colourshiftPower) * u_colourshiftMultiplier + u_colourshiftOffset;
                    float brightnessFromVel = pow(dot(p, discVel), u_brightnessFromDiskVel);
                    float brightnessFromDistance = pow(dist / u_InnerRadius, u_brightnessFromDistance);
                    //brightnessFromDistance = pow(pow(u_OuterRadius - dist, 1.0)/(u_OuterRadius - u_InnerRadius), -u_brightnessFromDistance);
                    col += T * diskSample * colorTemperatureToRGB(colourLookup) / (brightnessFromVel * brightnessFromDistance) * u_bloomDiskMultiplier;
                }
                else
                {
                    col += T * diskSample * bloomDiskMultiplier;
                }
                if (!u_transparentDisk || u_useDebugDiskTexture || T < 0.05)
                {
                    break;
                }
                // Beer's law (https://en.wikipedia.org/wiki/Beer%E2%80%93Lambert_law)
                absorption = (diskSample.x + diskSample.y + diskSample.z) * u_diskAbsorption * (u_OuterRadius - dist);
                T *= exp(-absorption);
            }
        }
        // Check if the ray hit the sphere
        else if (dist < horizon)
        {
            hitSphere = true;
            col += T * getSphereColour(x.yzw, sphere);
            break;
        }
        // Background
        else if (dist > u_maxDistance)
        {
            dir = pToDir(x, p);
            col += T * texture(skybox, vec3(-dir.x, dir.y, dir.z)).xyz * bloomBackgroundMultiplier;
            break;
        }
        
    }

    // If the ray went max steps without hitting anything, just cast the ray to the skybox.
    if (!hitDisk && !hitSphere || !hitSphere && u_transparentDisk && !u_useDebugDiskTexture)
    {
        dir = pToDir(x, p);
        col += T * texture(skybox, vec3(-dir.x, dir.y, dir.z)).xyz * bloomBackgroundMultiplier;
    }


    return col;
}


void main()
{
    vec3 col = vec3(0.0);

    for (int i = 0; i < u_msaa; i++)
    {
        for (int j = 0; j < u_msaa; j++)
        {
            //vec2 uv = ((gl_FragCoord.xy / u_ScreenSize.zw) - 0.5) * 2.0;

            // TexCoords go from 0 to 1 for both x and y.  uv will go from -1 to 1.
            // TexCoordOffset is how to offset the original uv when we're using MSAA.
            vec2 TexCoordOffset = vec2(float(i) / float(u_msaa + 1), float(j) / float(u_msaa + 1)) / u_ScreenSize.zw;
            vec2 uv = (TexCoords + TexCoordOffset - 0.5) * 2.0;

            // The image "screen" we're casting through in view space.
            vec4 screen = u_ProjInv * vec4(uv, 1.0, 1.0);
            // The ray direction in world space at the current pixel.
            vec3 rayDir = vec3(u_ViewInv * vec4(normalize(screen.xyz / screen.w), 0.0));

            // For simplicity, the sphere and disk are centered at (0,0,0).  The disk is in the xz-plane at y=0.
            vec4 sphere = vec4(vec3(0.0), u_BHRadius);
            vec2 disk = vec2(u_InnerRadius, u_OuterRadius);

            col += rayMarch(sphere, disk, u_cameraPos, rayDir);
        }
    }

    col /= float(u_msaa*u_msaa);

    fragColour = vec4(col, 1.0);

    float brightness = dot(fragColour.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > u_bloomThreshold)
        brightColour = vec4(fragColour.rgb, 1.0);
    else
        brightColour = vec4(0.0, 0.0, 0.0, 1.0);
}