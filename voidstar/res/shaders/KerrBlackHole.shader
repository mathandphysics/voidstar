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
uniform float u_drawDistance;
uniform float u_epsilon;
uniform float u_tolerance;

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
uniform float u_brightnessFromRadius;
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
    return mat4(v.x, 0.0, 0.0, 0.0,
                0.0, v.y, 0.0, 0.0,
                0.0, 0.0, v.z, 0.0,
                0.0, 0.0, 0.0, v.w);
}

mat4 metric(vec4 x)
{
    // Calculate the Kerr metric in Kerr-Schild coordinates at the position x.  G = c = 1.
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
    // Calculate the inverse Kerr metric in Kerr-Schild coordinates at the position x.  G = c = 1.
    // As with the metric, this is also from https://arxiv.org/abs/0706.0622 with adjusted coordinates.
    vec3 p = x.yzw;
    float r = implicitr(x);
    float r2 = r*r;
    float a2 = u_a*u_a;
    float f = 2.0 * u_BHMass * r2*r / (r2*r2 + a2*p.y*p.y);
    vec4 k = vec4(-1.0, (r*p.x - u_a*p.z) / (r2 + a2), p.y / r, (r*p.z + u_a*p.x) / (r2 + a2));
    return diag(vec4(-1.0, 1.0, 1.0, 1.0)) - f * mat4(k.x * k, k.y * k, k.z * k, k.w * k);
}

float gravRedshift(vec4 emitterx, vec4 receiverx)
{
    // Calculates the ratio between frequency_emitted and frequency_received for an emitter located at
    // emitterx and a receiver located at receiverx.
    // Then frequency_received = redshift(emitterx, receiverx) * frequency_emitted.
    // Here we assume that both the emitter and receiver are stationary and that the metric does not change with time.
    // Under these assumptions, dlambda = sqrt(-g_00(x))dt, and we are just comparing the ratio between the dlambdas of
    // the emitter and receiver.
    // See: "Einstein Gravity in a Nutshell" by Zee.
    return sqrt(metric(emitterx)[0][0] / metric(receiverx)[0][0]);
}

vec3 pToDir(vec4 x, vec4 p)
{
    // dx^i/dl = g^ij * p_j
    // We discard the time "velocity", dx^0/dl.
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
    float dx = 0.005; // Governs accuracy of dHdx
    Hdx[0] = H(x + vec4(dx, 0.0, 0.0, 0.0), p);
    Hdx[1] = H(x + vec4(0.0, dx, 0.0, 0.0), p);
    Hdx[2] = H(x + vec4(0.0, 0.0, dx, 0.0), p);
    Hdx[3] = H(x + vec4(0.0, 0.0, 0.0, dx), p);
    return (Hdx - H(x, p)) / dx;
}

mat2x4 xpupdate(in mat2x4 xp, float dl)
{
    vec4 x = xp[0];
    vec4 p = xp[1];

    mat2x4 dxp;
    dxp[1] = -dHdx(x, p) * dl;
    dxp[0] = invmetric(x) * p * dl;
    
    return dxp;
}

mat2x4 fasterxpupdate(in mat2x4 xp, float dl)
{
    // Saves one invmetric(x) calculation over calling the dHdx function...
    vec4 x = xp[0];
    vec4 p = xp[1];
    mat4 ginv = invmetric(x);

    float dx = 0.005; // Governs accuracy of dHdx
    vec4 Hdx = vec4(H(x + vec4(dx, 0.0, 0.0, 0.0), p), H(x + vec4(0.0, dx, 0.0, 0.0), p),
                    H(x + vec4(0.0, 0.0, dx, 0.0), p), H(x + vec4(0.0, 0.0, 0.0, dx), p));
    vec4 dHdx = (Hdx - 0.5 * dot(ginv * p, p)) / dx;

    mat2x4 dxp;
    dxp[1] = -dHdx * dl;
    dxp[0] = ginv * p * dl;

    return dxp;
}

mat2x4 xpupdateImplicitEuler(in mat2x4 xp, float dl)
{
    vec4 x = xp[0];
    vec4 p = xp[1];

    mat2x4 dxp;
    dxp[1] = -dHdx(x, p) * dl;
    p += dxp[1];
    dxp[0] = invmetric(x) * p * dl;

    return dxp;
}


mat2x4 fasterxpupdateImplicitEuler(in mat2x4 xp, float dl)
{
    // Correction for Euler-Cromer method as opposed to vanilla Euler method
    // https://en.wikipedia.org/wiki/Semi-implicit_Euler_method
    // Saves one invmetric(x) calculation over calling the dHdx function...
    vec4 x = xp[0];
    vec4 p = xp[1];
    mat4 ginv = invmetric(x);

    float dx = 0.005; // Governs accuracy of dHdx
    vec4 Hdx = vec4(H(x + vec4(dx, 0.0, 0.0, 0.0), p), H(x + vec4(0.0, dx, 0.0, 0.0), p),
        H(x + vec4(0.0, 0.0, dx, 0.0), p), H(x + vec4(0.0, 0.0, 0.0, dx), p));
    vec4 dHdx = (Hdx - 0.5 * dot(ginv * p, p)) / dx;

    mat2x4 dxp;
    dxp[1] = -dHdx * dl;
    p += dxp[1];
    dxp[0] = ginv * p * dl;

    return dxp;
}

void RK23integrationStep(mat2x4 xp, inout mat2x4 nextxp1, inout mat2x4 nextxp2, float stepsize)
{
    // Bogacki-Shampine Method.
    // https://en.wikipedia.org/wiki/Bogacki%E2%80%93Shampine_method
    // Note that the differential equations here don't explicitly depend on the 
    // affine parameter.  i.e. instead of dy/dt = f(t, y), we just have dy/dt = f(y).
    // So the vector field f is stationary in time.  This simplifies the xpupdate function.
    // Here y is the vector of (x, p) and f is the vector of (dH/dp, -dH/dx).
    mat2x4 k1;
    mat2x4 k2;
    mat2x4 k3;
    mat2x4 k4;

    k1 = fasterxpupdate(xp, stepsize);
    k2 = fasterxpupdate(xp + 0.5 * k1, stepsize);
    k3 = fasterxpupdate(xp + 0.75 * k2, stepsize);
    nextxp1 = xp + (2.0 / 9.0) * k1 + (1.0 / 3.0) * k2 + (4.0 / 9.0) * k3;
    k4 = fasterxpupdate(nextxp1, stepsize);
    nextxp2 = xp + (7.0 / 24.0) * k1 + (1.0 / 4.0) * k2 + (1.0 / 3.0) * k3 + (1.0 / 8.0) * k4;
}

void RK45integrationStep(mat2x4 xp, inout mat2x4 nextxp1, inout mat2x4 nextxp2, float stepsize)
{
    // Dormand-Prince Method.
    // https://en.wikipedia.org/wiki/Dormand%E2%80%93Prince_method
    // Note that the differential equations here don't explicitly depend on the 
    // affine parameter.  i.e. instead of dy/dt = f(t, y), we just have dy/dt = f(y).
    // So the vector field f is stationary in time.  This simplifies the xpupdate function.
    // Here y is the vector of (x, p) and f is the vector of (dH/dp, -dH/dx).
    mat2x4 k1;
    mat2x4 k2;
    mat2x4 k3;
    mat2x4 k4;
    mat2x4 k5;
    mat2x4 k6;
    mat2x4 k7;

    k1 = fasterxpupdate(xp, stepsize);
    k2 = fasterxpupdate(xp + 0.2 * k1, stepsize);
    k3 = fasterxpupdate(xp + (3.0/40.0)*k1 + (9.0/40.0)*k2, stepsize);
    k4 = fasterxpupdate(xp + (44.0/45.0)*k1 + (-56.0/15.0)*k2 + (32.0/9.0)*k3, stepsize);
    k5 = fasterxpupdate(xp + (19372.0/6561.0)*k1 + (-25360.0/2187.0)*k2 + (64448.0/6561.0)*k3 + (-212.0/729.0)*k4, stepsize);
    k6 = fasterxpupdate(xp + (9017.0/3168.0)*k1 + (-355.0/33.0)*k2 + (46732.0/5247.0)*k3 + (49.0/176.0)*k4 + (-5103.0/18656.0)*k5, stepsize);
    nextxp1 = xp + (35.0/384.0)*k1 + (0.0)*k2 + (500.0/1113.0)*k3 + (125.0/192.0)*k4 + (-2187.0/6784.0)*k5 + (11.0/84.0)*k6;
    k7 = fasterxpupdate(nextxp1, stepsize);
    nextxp2 = xp + (5179.0/57600.0)*k1 + (0.0)*k2 + (7571.0/16695.0)*k3 + (393.0/640.0)*k4 + (-92097.0/339200.0)*k5 + (187.0/2100.0)*k6 + (1.0/40.0)*k7;
}


void adaptiveRKDriver(inout vec4 x, inout vec4 p, inout float stepsize)
{
    mat2x4 xp = mat2x4(x, p);

    mat2x4 nextxp1;
    mat2x4 nextxp2;
    mat2x4 errormat;
    float error;
    float stepsizeRatio;
    float ratio;

    // Rather than running a while loop, which can crash the program in rare edge cases, we attempt to find
    // a stepsize that produces an acceptable error size in at most max_attempts.
    int max_attempts = 100;
    for (int i = 0; i < max_attempts; i++)
    {
        // TODO: Reduce this to one fewer xpupdate calls using the First Same As Last (FSAL) property.
        RK23integrationStep(xp, nextxp1, nextxp2, stepsize);
        //RK45integrationStep(xp, nextxp1, nextxp2, stepsize);

        // Now adapt stepsize:
        // https://en.wikipedia.org/wiki/Adaptive_step_size
        // https://jonshiach.github.io/ODEs-book/pages/2.5_Adaptive_step_size_control.html
        // Calculate the error
        errormat = nextxp1 - nextxp2;
        // L2-norm error
        error = sqrt(dot(errormat[0], errormat[0]) + dot(errormat[1], errormat[1]));

        ratio = u_tolerance / error;
        // Sometimes error can be exactly 0.0 just due to luck, in which case ratio will diverge.
        // Other times, the tolerance might be 0.0 (e.g. if we're resetting it in the GUI).
        if (isinf(ratio) || isnan(ratio))
        {
            ratio = 20.0;
        }

        // The min and max in the clamp are guarding against stepsize changes that are too large and cause issues.
        // At most, 1/5x or 5x the stepsize between steps.  If the geodesics were less smooth, we might have to
        // choose numbers that are closer to 1.0.
        // 0.9 is also a safety term here to prevent needing to recalculate steps too often since we should be
        // spending most of our time with error very near tolerance.
        stepsize *= clamp(0.9 * pow(ratio, 1.0 / 3.0), 0.2, 5.0);

        if (error <= u_tolerance)
        {
            // Successful step.
            break;
        }
    }

    //x = nextxp1[0];
    //p = nextxp1[1];
    x = nextxp2[0];
    p = nextxp2[1];
}

void RK4integrationStep(inout vec4 x, inout vec4 p, float dl)
{
    // Classic Runge-Kutta 4.  Note that the differential equations here don't explicitly depend on the 
    // affine parameter.  i.e. instead of dy/dt = f(t, y), we just have dy/dt = f(y).
    // So the vector field f is stationary in time.  This simplifies the xpupdate function.
    // Here y is the vector of (x, p) and f is the vector of (dH/dp, -dH/dx).
    mat2x4 xp = mat2x4(x, p);

    mat2x4 k1 = fasterxpupdate(xp, dl);
    mat2x4 k2 = fasterxpupdate(xp + 0.5 * k1, dl);
    mat2x4 k3 = fasterxpupdate(xp + 0.5 * k2, dl);
    mat2x4 k4 = fasterxpupdate(xp + k3, dl);
    xp += (k1 + 2.0 * k2 + 2.0 * k3 + k4) / 6.0;

    x = xp[0];
    p = xp[1];
}


void integrationStep(inout vec4 x, inout vec4 p, float dl)
{
    // Forward-Euler-Cromer integration of Hamilton's equations.  Unlike the other methods, this method is symplectic.

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

    //p -= dHdx(x, p) * dl;
    //x += invmetric(x) * p * dl;

    mat2x4 xp = mat2x4(x, p);
    //mat2x4 dxp = xpupdateImplicitEuler(xp, dl);
    mat2x4 dxp = fasterxpupdateImplicitEuler(xp, dl);
    xp += dxp;

    x = xp[0];
    p = xp[1];
}

vec3 getSphereColour(vec3 p)
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

vec3 getDiskColour(vec3 p)
{
    float pi = 3.14159265359;
    vec3 col = vec3(0.0);
    // For drawing the debug textures.
    float disk_divisions = 5.0;

    float u = (atan(p.x, p.z) + pi) / (2.0 * pi) - u_diskRotationAngle;
    u = u - floor(u); // equivalent to fract(u) since we want u to be between 0 and 1.
    float v = clamp(1.0 - (length(vec2(p.x, p.z)) - u_InnerRadius) / (u_OuterRadius - u_InnerRadius), 0.0, 1.0);

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


void rayMarch(vec3 cameraPos, vec3 rayDir, inout vec3 rayCol, inout bool hitDisk)
{
    // x and p are the 4-D position and momentum coordinates, respectively, in spacetime.
    // p_i = g_ij * dx^j/dlambda
    // Set x_t = 0 and dx^0/dlambda = 1.
    vec4 x = vec4(0.0, cameraPos);
    vec4 p = metric(x) * vec4(1.0, normalize(rayDir));

    vec3 diskSample = vec3(0.0);

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
    float T = 1.0;  // Transmittance
    float absorption;
    float intersectionParameter;
    vec4 planeIntersectionPoint;
    float horizon = u_BHMass + sqrt(u_BHMass*u_BHMass - u_a*u_a);
    float stepSize = u_stepSize;
    bool hitSphere = false;
    bool hitInfinity = false;
    vec4 previousx;

    dist = implicitr(x);
    // Cheap, dumb stepsize heuristic
    stepSize = 0.01 + (dist - horizon) * 1.0 / 10.0;
    for (int i = 0; i < u_maxSteps; i++)
    {
        previousx = x;

        adaptiveRKDriver(x, p, stepSize);
        dist = implicitr(x);

        //RK4integrationStep(x, p, stepSize);
        //integrationStep(x, p, stepSize);
        //dist = implicitr(x);
        //stepSize = 0.01 + (dist - horizon) * 1.0 / 10.0;

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
                diskSample = getDiskColour(vec3(planeIntersectionPoint.y, sign(previousx.z), planeIntersectionPoint.w));

                // Brightness of the disk is adapted from https://www.shadertoy.com/view/MctGWj
                // TODO: do a more realistic colouring of the accretion disk
                // (see (vii) on page 33 of https://arxiv.org/pdf/1502.03808)
                // For photons in free space, specific intensity / spectral radiance (brightness) I(\nu) is proportional to \nu**3
                // See: https://en.wikipedia.org/wiki/Planck%27s_law#The_law for B_\nu(\nu, T)
                // and https://en.wikipedia.org/wiki/Black-body_radiation#Doppler_effect
                if (!u_drawBasicDisk)
                {
                    vec4 discVel = vec4(-(dist + u_a / sqrt(dist)), vec3(-planeIntersectionPoint.w, 0.0, planeIntersectionPoint.y) * sign(u_a) / sqrt(dist)) / sqrt(dist * dist - 3.0 * dist + 2.0 * u_a * sqrt(dist));
                    float colourLookup = pow(dot(p, discVel), -u_colourshiftPower) * u_colourshiftMultiplier + u_colourshiftOffset;
                    float brightnessFromVel = pow(dot(p, discVel), u_brightnessFromDiskVel);
                    float brightnessFromRadius = pow(dist / u_InnerRadius, u_brightnessFromRadius);
                    //brightnessFromRadius = pow(pow(u_OuterRadius - dist, 1.0)/(u_OuterRadius - u_InnerRadius), -u_brightnessFromRadius);
                    rayCol += T * diskSample * colorTemperatureToRGB(colourLookup) / (brightnessFromVel * brightnessFromRadius) * u_bloomDiskMultiplier;
                }
                else
                {
                    rayCol += T * diskSample * bloomDiskMultiplier;
                }
                if (!u_transparentDisk || u_useDebugDiskTexture || T < 0.05)
                {
                    break;
                }
                // Beer's law (https://en.wikipedia.org/wiki/Beer%E2%80%93Lambert_law)
                vec3 brightnessMultiplier = vec3(0.2126, 0.7152, 0.0722);
                absorption = dot(brightnessMultiplier, diskSample) * u_diskAbsorption * (u_OuterRadius - dist);
                T *= exp(-absorption);
            }
        }
        // Check if the ray hit the sphere.
        else if (dist < horizon)
        {
            hitSphere = true;
            rayCol += T * getSphereColour(x.yzw);
            break;
        }
        // Check if the ray escaped the black hole and hit the skybox.
        else if (dist > u_drawDistance)
        {
            dir = pToDir(x, p);
            rayCol += T * texture(skybox, vec3(-dir.x, dir.y, dir.z)).xyz * bloomBackgroundMultiplier;
            hitInfinity = true;
            break;
        }
        
    }

    // If the ray went max steps without hitting anything, just cast the ray to the skybox.
    if (!hitDisk && !hitSphere && !hitInfinity || !hitSphere && !hitInfinity && u_transparentDisk && !u_useDebugDiskTexture)
    {
        dir = pToDir(x, p);
        rayCol += T * texture(skybox, vec3(-dir.x, dir.y, dir.z)).xyz * bloomBackgroundMultiplier;
    }
}


void main()
{
    vec3 pixelCol = vec3(0.0);
    bool hitDisk = false;

    for (int i = 0; i < u_msaa; i++)
    {
        for (int j = 0; j < u_msaa; j++)
        {
            bool rayHitDisk = false;
            vec3 rayCol = vec3(0.0);
            //vec2 uv = ((gl_FragCoord.xy / u_ScreenSize.zw) - 0.5) * 2.0;

            // TexCoords go from 0 to 1 for both x and y.  uv will go from -1 to 1.
            // TexCoordOffset is how to offset the original uv when we're using MSAA.
            vec2 TexCoordOffset = vec2(float(i) / float(u_msaa + 1), float(j) / float(u_msaa + 1)) / u_ScreenSize.zw;
            vec2 uv = (TexCoords + TexCoordOffset - 0.5) * 2.0;

            // The image "screen" we're casting through in view space.
            vec4 screen = u_ProjInv * vec4(uv, 1.0, 1.0);
            // The ray direction in world space at the current pixel.
            vec3 rayDir = vec3(u_ViewInv * vec4(normalize(screen.xyz / screen.w), 0.0));

            // For simplicity, the BH and disk are centered at (0,0,0).  The disk is in the xz-plane at y=0.
            rayMarch(u_cameraPos, rayDir, rayCol, rayHitDisk);
            pixelCol += rayCol;
            if (rayHitDisk)
            {
                // At least one of the rays in the pixel the disk.
                hitDisk = true;
            }
        }
    }

    pixelCol /= float(u_msaa*u_msaa);

    fragColour = vec4(pixelCol, 1.0);
    float brightness = dot(fragColour.rgb, vec3(0.2126, 0.7152, 0.0722));
    // Only apply bloom to rays that hit the disk, never the background.
    if (brightness > u_bloomThreshold && hitDisk)
        brightColour = vec4(fragColour.rgb, 1.0);
    else
        brightColour = vec4(0.0, 0.0, 0.0, 1.0);
}