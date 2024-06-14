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
uniform float u_risco;
uniform float u_fOfRISCO;
uniform float u_BHMass;
uniform float u_a;
uniform float u_Tmax;
uniform float u_diskRotationAngle;

uniform int u_msaa;

uniform int u_maxSteps;
uniform float u_stepSize;
uniform float u_drawDistance;
uniform float u_epsilon;
uniform int u_ODESolver;
uniform float u_tolerance;
uniform float u_diskIntersectionThreshold;
uniform float u_sphereIntersectionThreshold;

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


mat4 diag(vec4 v)
{
    return mat4(v.x, 0.0, 0.0, 0.0,
                0.0, v.y, 0.0, 0.0,
                0.0, 0.0, v.z, 0.0,
                0.0, 0.0, 0.0, v.w);
}

#ifdef KERR
float implicitr(vec4 x)
{
    // Calculate the implicitly defined r value in the Kerr-Schild coordinates from the position.
    // This reduces to solving a polynomial of the form r**4 + b*r**2 + c = 0.
    vec3 p = x.yzw;
    float a2 = u_a * u_a;
    float b = a2 - dot(p, p);
    float c = -a2 * p.y * p.y;
    float r2 = 0.5 * (-b + sqrt(b * b - 4.0 * c));
    return sqrt(r2);
}

mat4 metric(vec4 x)
{
    // Calculate the Kerr metric in ingoing Kerr-Schild coordinates at the position x.  G = c = 1.  Signature (-+++).
    // This differs slightly from the definition in https://arxiv.org/abs/0706.0622
    // because that paper has Cartesian z as the up direction, while this program has Cartesian y
    // as the up direction.
    vec3 p = x.yzw;
    float r = implicitr(x);
    float r2 = r*r;
    float a2 = u_a*u_a;
    float f = 2.0 * u_BHMass * r2*r / (r2*r2 + a2*p.y*p.y);
    vec4 l = vec4(1.0, (r * p.x - u_a * p.z) / (r2 + a2), p.y / r, (r * p.z + u_a * p.x) / (r2 + a2));
    return diag(vec4(-1.0, 1.0, 1.0, 1.0)) + f * outerProduct(l, l);
}

mat4 invmetric(vec4 x)
{
    // Generic inverse metric:
    //return inverse(metric(x));

    // For general Kerr-Schild form decompositions, we can use
    // https://en.wikipedia.org/wiki/Sherman%E2%80%93Morrison_formula.
    // In this specific case of the Kerr metric though, we have a formula for the inverse that has identical
    // cost to the metric.

    // Calculate the inverse Kerr metric in ingoing Kerr-Schild coordinates at the position x.  G = c = 1. Signature (-+++).
    // As with the metric, this is also from https://arxiv.org/abs/0706.0622 with adjusted coordinates.
    // Due to the convenient nature of these coordinates, this has pretty much identical computational cost as
    // the metric.
    vec3 p = x.yzw;
    float r = implicitr(x);
    float r2 = r*r;
    float a2 = u_a*u_a;
    float f = 2.0 * u_BHMass * r2*r / (r2*r2 + a2*p.y*p.y);
    vec4 l = vec4(-1.0, (r * p.x - u_a * p.z) / (r2 + a2), p.y / r, (r * p.z + u_a * p.x) / (r2 + a2));
    return diag(vec4(-1.0, 1.0, 1.0, 1.0)) - f * outerProduct(l, l);
}
#endif

float metricDistance(vec4 x)
{
    // Not really a distance in spacetime.
#ifdef KERR
    return implicitr(x);
#endif

#ifdef MINKOWSKI
    return sqrt(x.y * x.y + x.z * x.z + x.w * x.w);
#endif

#ifdef CLASSICAL
    return sqrt(x.y * x.y + x.z * x.z + x.w * x.w);
#endif
}

#ifdef MINKOWSKI
mat4 metric(vec4 x)
{
    return diag(vec4(-1.0, 1.0, 1.0, 1.0));
}

mat4 invmetric(vec4 x)
{
    return diag(vec4(-1.0, 1.0, 1.0, 1.0));
}
#endif

#ifdef CLASSICAL
mat4 metric(vec4 x)
{
    return diag(vec4(1.0, 1.0, 1.0, 1.0));
}

mat4 invmetric(vec4 x)
{
    return diag(vec4(1.0, 1.0, 1.0, 1.0));
}
#endif

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

vec3 pToDir(mat2x4 xp)
{
    // dx^i/dl = g^ij * p_j
    vec4 dxdl = invmetric(xp[0]) * xp[1];
    // Discard the time "velocity", dx^0/dl.
    return normalize(dxdl.yzw);
}

float H(vec4 x, vec4 p)
{
    // Calculate the Super-Hamiltonian for position x and momentum p.
    return 0.5 * dot(invmetric(x) * p, p);
}

float L(vec4 x, vec4 dxdl)
{
    // Calculate the Lagrangian for position x and momentum p.
    // Recall dx^i/dl = g^ij * p_j.  Therefore, L(x, dxdl) = H(x, p) at a given point in spacetime.
    // Thus, if we want to calculate -dH/dx(x,p) for the update function, we can instead calculate
    // -dL/dx(x, dxdl) at the same point in spacetime.
    // This allows us to calculate the update with only a single metric inverse, which is especially useful
    // if a particular metric has an expensive inversion.
    return 0.5 * dot(metric(x)*dxdl, dxdl);
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

#ifdef CLASSICAL
mat2x4 xpupdate(in mat2x4 xp, float dl)
{
    // Update function for classical black holes.
    // This is not a Hamiltonian method and the differential equations here are less well-behaved.
    // Technically, this takes account not only the curvature of time (as in Newtonian gravity), but also
    // the curvature of space.  In plain Newtonian, we'd have a 1.0 factor for the bending acceleration, but
    // because we also know space curves and Einstein found that the curving of space doubles the bending,
    // we simply account for this with a factor of 2.0.  For strictly Newtonian gravity, use 1.0, but then the
    // photon rings will no longer be visible, e.g.  Technically speaking, the 2.0 is the result of the static weak-field
    // limit of the Schwarzchild metric.  It is the classical Newtonian limit where we take 2*G*M/(c^2*r) << 1.
    mat2x4 dxp;
    float dist = metricDistance(xp[0]);
    vec3 bendingAcceleration = -2.0 * (u_BHMass / (dist*dist)) * (xp[0].yzw / dist);
    dxp[1] = vec4(1.0, normalize(xp[1].yzw + bendingAcceleration * dl)) - xp[1];
    dxp[0] = xp[1] * dl;
    return dxp;
}
mat2x4 fasterxpupdate(in mat2x4 xp, float dl)
{
    // Update function for classical black holes.
    // This is not a Hamiltonian method and the differential equations here are less well-behaved.
    // Technically, this takes account not only the curvature of time (as in Newtonian gravity), but also
    // the curvature of space.  In plain Newtonian, we'd have a 1.0 factor for the bending acceleration, but
    // because we also know space curves and Einstein found that the curving of space doubles the bending,
    // we simply account for this with a factor of 2.0.  For strictly Newtonian gravity, use 1.0, but then the
    // photon rings will no longer be visible, e.g.  Technically speaking, the 2.0 is the result of the static weak-field
    // limit of the Schwarzchild metric.  It is the classical Newtonian limit where we take 2*G*M/(c^2*r) << 1.
    mat2x4 dxp;
    vec3 p = xp[0].yzw;
    float dist2 = dot(p, p);
    float dist = sqrt(dist2);
    vec3 bendingAcceleration = -2.0 * (u_BHMass / dist2) * (p / dist);
    dxp[1] = vec4(1.0, normalize(xp[1].yzw + bendingAcceleration * dl)) - xp[1];
    dxp[0] = xp[1] * dl;
    return dxp;
}
#else
mat2x4 xpupdate(in mat2x4 xp, float dl)
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
    mat2x4 dxp;
    dxp[1] = -dHdx(xp[0], xp[1]) * dl;
    dxp[0] = invmetric(xp[0]) * xp[1] * dl;
    
    return dxp;
}

mat2x4 fasterxpupdate(in mat2x4 xp, float dl)
{
    // Same as the xpupdate function but slightly optimized.
    // Saves one invmetric(x) over calling the dHdx function.
    vec4 x = xp[0];
    vec4 p = xp[1];
    // Calculate the Hamiltonian at x.
    mat4 ginv = invmetric(x);
    float Hatx = 0.5 * dot(ginv * p, p);

    float dx = 0.005; // Governs accuracy of dHdx

#ifdef KERR
    // Note that for the Kerr metric in Kerr-Schild cartesian coordinates specifically,
    // H(x) = H(x + (dx, 0.0, 0.0, 0.0)), so we can additionally save a calculation of H.
    vec4 Hdx = vec4(Hatx, H(x + vec4(0.0, dx, 0.0, 0.0), p),
                    H(x + vec4(0.0, 0.0, dx, 0.0), p), H(x + vec4(0.0, 0.0, 0.0, dx), p));
#else
    // General formula.
    vec4 Hdx = vec4( H(x + vec4(dx, 0.0, 0.0, 0.0), p), H(x + vec4(0.0, dx, 0.0, 0.0), p),
        H(x + vec4(0.0, 0.0, dx, 0.0), p), H(x + vec4(0.0, 0.0, 0.0, dx), p));
#endif
    vec4 dHdx = (Hdx - Hatx) / dx;

    mat2x4 dxp;
    dxp[1] = -dHdx * dl;
    dxp[0] = ginv * p * dl;

    return dxp;
}
#endif

mat2x4 xpupdateImplicitEuler(in mat2x4 xp, float dl)
{
    // https://en.wikipedia.org/wiki/Semi-implicit_Euler_method
    mat2x4 dxp;
    dxp[1] = -dHdx(xp[0], xp[1]) * dl;
    // Correction for Euler-Cromer method as opposed to vanilla Euler method
    xp[1] += dxp[1];
    dxp[0] = invmetric(xp[0]) * xp[1] * dl;

    return dxp;
}


mat2x4 fasterxpupdateImplicitEuler(in mat2x4 xp, float dl)
{
    // https://en.wikipedia.org/wiki/Semi-implicit_Euler_method
    // Saves one invmetric(x) calculation over calling the dHdx function...
    vec4 x = xp[0];
    vec4 p = xp[1];
    // Calculate the Hamiltonian at x.
    mat4 ginv = invmetric(x);
    float Hatx = 0.5 * dot(ginv * p, p);

    float dx = 0.005; // Governs accuracy of dHdx

#ifdef KERR
    // Note that for the Kerr metric in Kerr-Schild cartesian coordinates specifically,
    // H(x) = H(x + (dx, 0.0, 0.0, 0.0)), so we can additionally save a calculation of H.
    vec4 Hdx = vec4(Hatx, H(x + vec4(0.0, dx, 0.0, 0.0), p),
        H(x + vec4(0.0, 0.0, dx, 0.0), p), H(x + vec4(0.0, 0.0, 0.0, dx), p));
#else
    // General formula.
    vec4 Hdx = vec4(H(x + vec4(dx, 0.0, 0.0, 0.0), p), H(x + vec4(0.0, dx, 0.0, 0.0), p),
        H(x + vec4(0.0, 0.0, dx, 0.0), p), H(x + vec4(0.0, 0.0, 0.0, dx), p));
#endif
    vec4 dHdx = (Hdx - Hatx) / dx;

    mat2x4 dxp;
    dxp[1] = -dHdx * dl;
    // Correction for Euler-Cromer method as opposed to vanilla Euler method
    p += dxp[1];
    dxp[0] = ginv * p * dl;

    return dxp;
}

#if (ODE_SOLVER == 0)
mat2x4 integrationStep(mat2x4 xp, float dl)
{
    // Forward-Euler-Cromer integration of Hamilton's equations.  Unlike the other methods, this method is symplectic.

    //p -= dHdx(x, p) * dl;
    //x += invmetric(x) * p * dl;

    //mat2x4 dxp = xpupdateImplicitEuler(xp, dl);
#ifdef CLASSICAL
    mat2x4 dxp = fasterxpupdate(xp, dl);
#else
    mat2x4 dxp = fasterxpupdateImplicitEuler(xp, dl);
#endif
    xp += dxp;
    return xp;
}
#endif

#if (ODE_SOLVER == 1 || ODE_SOLVER == 2 || ODE_SOLVER == 3)
mat2x4 RK4integrationStep(mat2x4 xp, float dl)
{
    // Classic Runge-Kutta 4.  Note that the differential equations here don't explicitly depend on the 
    // affine parameter.  i.e. instead of dy/dt = f(t, y), we just have dy/dt = f(y).
    // So the vector field f is stationary in time.  This simplifies the xpupdate function.
    // Here y is the vector of (x, p) and f is the vector of (dH/dp, -dH/dx).

    mat2x4 k1 = fasterxpupdate(xp, dl);
    mat2x4 k2 = fasterxpupdate(xp + 0.5 * k1, dl);
    mat2x4 k3 = fasterxpupdate(xp + 0.5 * k2, dl);
    mat2x4 k4 = fasterxpupdate(xp + k3, dl);
    xp += (k1 + 2.0 * k2 + 2.0 * k3 + k4) / 6.0;

    return xp;

}
#endif

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

void BSDiskIntersectionPoint(mat2x4 previousxp, mat2x4 xp, out mat2x4 diskIntersectionPoint, float stepsize)
{
    // Binary search on stepsize to get within some threshold of the xz-plane.
    // glsl doesn't support recursion, so we write a loop.
    int BS_attempts = 20;
    float leftEndpoint = 0.0;
    float rightEndpoint = stepsize;
    float midpoint;
    mat2x4 xptest;
#if (ODE_SOLVER == 2 || ODE_SOLVER == 3)
    mat2x4 xptest2;
#endif
    if (abs(previousxp[0][2]) < u_diskIntersectionThreshold)
    {
        diskIntersectionPoint = previousxp;
    }
    else if (abs(xp[0][2]) < u_diskIntersectionThreshold)
    {
        diskIntersectionPoint = xp;
    }
    else
    {
        for (int j = 0; j < BS_attempts; j++)
        {
            midpoint = (leftEndpoint + rightEndpoint) / 2.0;
#if (ODE_SOLVER == 0)
            xptest = integrationStep(previousxp, midpoint);
#elif (ODE_SOLVER == 1)
            xptest = RK4integrationStep(previousxp, midpoint);
#elif (ODE_SOLVER == 2)
            RK23integrationStep(previousxp, xptest, xptest2, midpoint);
#elif (ODE_SOLVER == 3)
            //RK45integrationStep(previousxp, xptest, xptest2, midpoint);
            xptest = RK4integrationStep(previousxp, midpoint);
#endif
            if (abs(xptest[0][2]) < u_diskIntersectionThreshold)
            {
                // Then we're done.
                break;
            }
            else
            {
                if (xptest[0][2] * previousxp[0][2] > 0.0)
                {
                    // Then xptest[0][2] is too close to xp[0][2].
                    leftEndpoint = midpoint;
                }
                else
                {
                    // Then the point is too far from xp[0][2].
                    rightEndpoint = midpoint;
                }
            }
        }
        diskIntersectionPoint = xptest;
    }
}

void BSSphereIntersectionPoint(mat2x4 xp, out mat2x4 sphereIntersectionPoint, float horizon, float stepsize)
{
    // Binary search on stepsize to get within some threshold of the sphere horizon.
    // glsl doesn't support recursion, so we write a loop.
    int BS_attempts = 20;
    float leftEndpoint = 0.0;
    float rightEndpoint = stepsize;
    float midpoint;
    float dist;
    mat2x4 xptest;
#if (ODE_SOLVER == 2 || ODE_SOLVER == 3)
    mat2x4 xptest2;
#endif
    for (int j = 0; j < BS_attempts; j++)
    {
        midpoint = (leftEndpoint + rightEndpoint) / 2.0;
#if (ODE_SOLVER == 0)
        xptest = integrationStep(xp, midpoint);
#elif (ODE_SOLVER == 1)
        xptest = RK4integrationStep(xp, midpoint);
#elif (ODE_SOLVER == 2)
        RK23integrationStep(xp, xptest, xptest2, midpoint);
#elif (ODE_SOLVER == 3)
        //RK45integrationStep(xp, xptest, xptest2, midpoint);
        RK23integrationStep(xp, xptest, xptest2, midpoint);
        //xptest = RK4integrationStep(xp, midpoint);
#endif
        dist = metricDistance(xptest[0]);
        if (abs(dist - horizon) < u_sphereIntersectionThreshold)
        {
            // Then we're done.
            break;
        }
        else
        {
            if (dist > horizon)
            {
                // Then xptest is too close to xp.
                leftEndpoint = midpoint;
            }
            else
            {
                // Then the point is too far from xp.
                rightEndpoint = midpoint;
            }
        }
    }
    sphereIntersectionPoint = xptest;
}

#if (ODE_SOLVER == 2 || ODE_SOLVER == 3)
mat2x4 adaptiveRKDriver(mat2x4 xp, inout float stepsize, out float oldStepSize)
{
    // oldStepSize is the size of the step that is actually used in the integration step.  stepsize is then updated
    // for the next step based on the error.
    mat2x4 nextxp1;
    mat2x4 nextxp2;
    mat2x4 errormat;
    float error;
    float stepSizeRatio;
    float ratio;

    // The min and max in the clamp are guarding against stepsize changes that are too large and cause issues.
    // At most, 1/5x or 2x the stepsize between steps.  If the geodesics were less smooth, we might have to
    // choose numbers that are closer to 1.0 (1/2x and 2x are popular numbers).
    // 0.9 is also a safety term here to prevent needing to recalculate steps too often since we should be
    // spending most of our time with error very near tolerance.
    float safety = 0.9;
    float minstep = 0.2;
    float maxstep = 2.0;

    float power;
#if (ODE_SOLVER == 2)
    power = 1.0 / 3.0;
#elif (ODE_SOLVER == 3)
    power = 1.0 / 5.0;
#endif

    // Rather than running a while loop, which can crash the program in rare edge cases, we attempt to find
    // a stepsize that produces an acceptable error size in at most max_attempts.
    int max_attempts = 10;
    for (int i = 0; i < max_attempts; i++)
    {

        // HACK.  The adaptive driver steps too far for flat or close to flat spacetimes.  This causes it to miss
        // crossing the disk or the sphere.
#ifdef CLASSICAL
        stepsize = min(0.01 + (metricDistance(xp[0]) - 2.0 * u_BHMass) * 1.0 / 5.0, stepsize);
#endif
#ifdef MINKOWSKI
        stepsize = min(0.01 + (metricDistance(xp[0]) - 2.0 * u_BHMass) * 1.0 / 5.0, stepsize);
#endif

        // TODO: Reduce this to one fewer xpupdate calls using the First Same As Last (FSAL) property.
#if (ODE_SOLVER == 2)
        RK23integrationStep(xp, nextxp1, nextxp2, stepsize);
#elif (ODE_SOLVER == 3)
        RK45integrationStep(xp, nextxp1, nextxp2, stepsize);
#endif

        // Now adapt stepsize:
        // https://en.wikipedia.org/wiki/Adaptive_step_size
        // https://jonshiach.github.io/ODEs-book/pages/2.5_Adaptive_step_size_control.html
        // Calculate the error
        errormat = nextxp1 - nextxp2;
        // L2-norm error
        error = sqrt(dot(errormat[0], errormat[0]) + dot(errormat[1], errormat[1]));
        // L1-norm error
        //error = abs(errormat[0][0]) + abs(errormat[0][1]) + abs(errormat[0][2]) + abs(errormat[0][3])
        //    + abs(errormat[1][0]) + abs(errormat[1][1]) + abs(errormat[1][2]) + abs(errormat[1][3]);

        ratio = u_tolerance / error;
        stepSizeRatio = clamp(safety * pow(ratio, power), minstep, maxstep);

        if (error <= u_tolerance)
        {
            // Successful step.
            if (stepSizeRatio > 0.5)
            {
                oldStepSize = stepsize;
                stepsize *= stepSizeRatio;
                break;
            }
        }

        stepsize *= stepSizeRatio;
    }

#if (ODE_SOLVER == 2)
    return nextxp1;
#elif (ODE_SOLVER == 3)
    return nextxp2;
#endif
}
#endif

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
    if (u_useSphereTexture)
    {
        // Draw an actual texture instead.
        vec2 uv = vec2(u, v);
        col = texture(sphereTexture, uv).xyz;
    }
    else
    {
        // Makes the checkerboard pattern on the sphere.
        vec2 uvsignvec = sign(sin(2.0 * pi * vec2(sphere_latitudes * u, sphere_longitudes * v)));
        float uvsign = uvsignvec.x * uvsignvec.y;
        float uvscaled = (uvsign + 1.0) * 0.5;
        col = uvscaled * u_sphereDebugColour1 + (1.0 - uvscaled) * u_sphereDebugColour2;
    }
    return col;
}

vec3 sampleDiskTexture(vec3 p)
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

vec3 TemperatureToRGB(const in float temperature) {
    // https://www.shadertoy.com/view/4sc3D7
    // Converts a blackbody temperature to colour in RGB.
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

float f(float r, float a)
{
    // Uses the black hole's mass and spin, along with a point on the equatorial accretion disk that is r away from the
    // center of the black hole, to calculate f, a function used in the time-averaged flux of radiant energy.
    // From "Disk-accretion onto a black hole" by Page and Thorne (1973).
    if (r < u_risco)
    {
        return 0.0;
    }
    float pi = 3.141592653589793;

    float astar = a / u_BHMass;
    float x = sqrt(r / u_BHMass);
    float x0 = sqrt(u_risco / u_BHMass);
    // x1, x2, x3 are the roots of x^3 - 3*x + 2*astar = 0.
    float x1 = 2.0 * cos((1.0 / 3.0) * acos(astar) - pi / 3.0);
    float x2 = 2.0 * cos((1.0 / 3.0) * acos(astar) + pi / 3.0);
    float x3 = -2.0 * cos((1.0 / 3.0) * acos(astar));

    float x1numer = 3.0 * (x1 - astar) * (x1 - astar);
    float x1denom = x1 * (x1 - x2) * (x1 - x3);
    float x2numer = 3.0 * (x2 - astar) * (x2 - astar);
    float x2denom = x2 * (x2 - x1) * (x2 - x3);
    float x3numer = 3.0 * (x3 - astar) * (x3 - astar);
    float x3denom = x3 * (x3 - x1) * (x3 - x2);
    float xdenom = x * x * (x * x * x - 3.0 * x + 2 * astar);

    return (3.0 / (2.0 * u_BHMass)) * (1.0 / xdenom) * (x - x0 - (3.0 * astar / 2.0) * log(x/x0)
        - (x1numer / x1denom) * log((x - x1) / (x0 - x1)) - (x2numer / x2denom) * log((x - x2) / (x0 - x2))
        - (x3numer / x3denom) * log((x - x3) / (x0 - x3)));
}

float observedTemperature(float r, float a, float blueshift)
{
    // from "Detecting Accretion Disks in Active Galactic Nuclei" by Fanton, et al (1997).
    // (49/36)*R_isco is the radial distance where the accretion disk attains its maximum brightness / temperature.
    float r_max = (49.0 / 36.0) * u_risco;
    return blueshift * u_Tmax * pow(f(r, a) * r_max / (r * f(r_max, a)), 1.0 / 4.0);
}

void getDiskColour(mat2x4 diskIntersectionPoint, float previousy, float r, float a, inout vec3 rayCol, inout float T, float bloomDiskMultiplier)
{
    float absorption;

    // Need to pass in the sign of the previousx's Cartesian y-value to colour the
    // top and bottom of the disk differently in debug mode.
    vec4 planeIntersectionPoint = diskIntersectionPoint[0];
    vec3 diskSample = sampleDiskTexture(vec3(planeIntersectionPoint.y, previousy, planeIntersectionPoint.w));

    // TODO: do a more realistic colouring of the accretion disk
    // For photons in free space, specific intensity / spectral radiance (brightness) I(\nu) is proportional to \nu**3
    // See: https://en.wikipedia.org/wiki/Planck%27s_law#The_law for B_\nu(\nu, T)
    // and https://en.wikipedia.org/wiki/Black-body_radiation#Doppler_effect
    float brightnessFromRadius;
    if (!u_drawBasicDisk)
    {
        // Velocity (in Kerr-Schild cartesian coordinates) of a massive particle in circular orbit in the equatorial plane
        // at a distance r from a black hole with mass u_BHMass and spin a.
        // The minus sign in front of the time term is just to make the later dot product work out.  The ray of light
        // is moving toward the disk because we're tracing backwards, so we need to swap the sign on time on either
        // the light momentum or the disk velocity, so we just do it here for simplicity.
        vec4 diskVel = vec4(-(r + a * sqrt(u_BHMass / r)), vec3(-planeIntersectionPoint.w, 0.0, planeIntersectionPoint.y) * sqrt(u_BHMass / r)) / sqrt(r * r - 3.0 * r * u_BHMass + 2.0 * a * sqrt(u_BHMass * r));
#ifdef CLASSICAL
        // HACK
        float brightnessFromVel = 1.0;
        float temperature = observedTemperature(r, a, brightnessFromVel);
#else
        // Brightness of the disk is adapted from https://www.shadertoy.com/view/MctGWj
        float blueshift = pow(1.0 / dot(diskIntersectionPoint[1], diskVel), 3.0);
        float temperature = observedTemperature(r, a, blueshift);
        //float brightnessFromVel = pow(dot(diskIntersectionPoint[1], diskVel), -u_brightnessFromDiskVel);
        float brightnessFromVel = blueshift;
#endif
        // 5000.0 is a magic number proportional to the accretion rate, \dot{M}, which is assumed to be constant
        // in a steady disk.
         brightnessFromRadius = 5000.0 * f(r, a) / r;
        //rayCol += T * diskSample * TemperatureToRGB(temperature) * brightnessFromVel * brightnessFromRadius * bloomDiskMultiplier;
        rayCol += T * diskSample * TemperatureToRGB(temperature) * brightnessFromVel * bloomDiskMultiplier;
    }
    else
    {
        rayCol += T * diskSample * bloomDiskMultiplier;
    }
    if (u_transparentDisk && !u_useDebugDiskTexture && T > 0.05)
    {
        // Beer's law (https://en.wikipedia.org/wiki/Beer%E2%80%93Lambert_law)
        vec3 brightnessMultiplier = vec3(0.2126, 0.7152, 0.0722);
        // The brighter the disk is at the intersection point, the higher the absorption.
        //absorption = dot(brightnessMultiplier, diskSample) * u_diskAbsorption * (u_OuterRadius - diskDist);
        //absorption = brightnessFromRadius * u_diskAbsorption * (u_OuterRadius - diskDist);
        absorption = u_diskAbsorption * (u_OuterRadius - r);
        T *= exp(-absorption);
    }
}

void rayMarch(vec3 cameraPos, vec3 rayDir, inout vec3 rayCol, inout bool hitDisk)
{
    vec3 dir;
    float cameraDist;
    float dist;
    float diskDist;
    float T = 1.0;  // Transmittance
    float a = 0.0;
#ifdef KERR
    a = u_a;
#else
    a = 0.0;
#endif
    float horizon = u_BHMass + sqrt(u_BHMass * u_BHMass - a * a); // G = c = 1
    mat2x4 diskIntersectionPoint;
    mat2x4 sphereIntersectionPoint;
    bool hitSphere = false;
    bool hitInfinity = false;


    // x and p are the spacetime position and momentum coordinates.
    // p_i = g_ij * dx^j/dlambda
    // Set x_t = 0 and dx^0/dlambda = 1.
    mat2x4 xp;
    xp[0] = vec4(0.0, cameraPos);
    mat2x4 previousxp;
    cameraDist = metricDistance(xp[0]);
    if (cameraDist > horizon)
    {
        xp[1] = metric(xp[0]) * vec4(1.0, rayDir);
    }
    else
    {
        // Inside the event horizon, we need to change metric to outgoing coordinates.  We adjust p accordingly.
        //xp[1] = metric(xp[0]) * vec4(-1.0, -rayDir);
        xp[1] = metric(xp[0]) * vec4(-1.0, rayDir);
    }

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
        bloomDiskMultiplier = min(1.0, u_bloomDiskMultiplier);
        bloomBackgroundMultiplier = min(1.0, u_bloomBackgroundMultiplier);
    }

    dist = metricDistance(xp[0]);
    // Cheap, dumb stepsize heuristic
    float stepSize = 0.01 + (dist - horizon) * 1.0 / 10.0;
    float oldStepSize;
    for (int i = 0; i < u_maxSteps; i++)
    {
        previousxp = xp;

        // The actual integration step, depending on which ODE solver is used.
#if (ODE_SOLVER == 0)
        xp = integrationStep(xp, stepSize);
        dist = metricDistance(xp[0]);
        oldStepSize = stepSize;
        stepSize = 0.01 + (dist - horizon) * 1.0 / 10.0;
#elif (ODE_SOLVER == 1)
        xp = RK4integrationStep(xp, stepSize);
        dist = metricDistance(xp[0]);
        oldStepSize = stepSize;
        stepSize = 0.01 + (dist - horizon) * 1.0 / 5.0;
#elif (ODE_SOLVER == 2 || ODE_SOLVER == 3)
        xp = adaptiveRKDriver(xp, stepSize, oldStepSize);
        dist = metricDistance(xp[0]);
#endif

        // Check if the ray hit the disk
        // Check to see whether the Cartesian y-coordinate changed signs, i.e. if the ray crossed the disk's plane.
        // CAUTION: if the stepsize is too big (e.g. for RK45), then it's possible for a ray to cross the xz-plane
        // and then cross back over to the previous side all within a single step.  This can lead to a jagged edge
        // at the inner edge of the accretion disk.  Decreasing tolerance (i.e. forcing a smaller stepsize) fixes this.
        if (xp[0][2] * previousxp[0][2] < 0.0)
        {
            // Do a binary search on stepsize to find the point where the geodesic crosses the xz-plane.
            BSDiskIntersectionPoint(previousxp, xp, diskIntersectionPoint, oldStepSize);
            diskDist = metricDistance(diskIntersectionPoint[0]);
            if (diskDist <= u_OuterRadius && diskDist >= u_InnerRadius)
            {
                hitDisk = true;
                getDiskColour(diskIntersectionPoint, sign(previousxp[0][2]), diskDist, a, rayCol, T, bloomDiskMultiplier);
                if (u_useDebugDiskTexture)
                {
                    // No transparency.
                    break;
                }
            }
        }

        // Check if the ray hit the sphere.
        if (dist < horizon && cameraDist > horizon)
        {
            hitSphere = true;
            if (u_useDebugSphereTexture)
            {
                BSSphereIntersectionPoint(previousxp, sphereIntersectionPoint, horizon, oldStepSize);
                rayCol += T * getSphereColour(sphereIntersectionPoint[0].yzw);
            }
            break;
        }

        // Check if the ray escaped the black hole and hit the skybox.
        else if (dist > u_drawDistance)
        {
            hitInfinity = true;
            dir = pToDir(xp);
            rayCol += T * texture(skybox, vec3(-dir.x, dir.y, dir.z)).xyz * bloomBackgroundMultiplier;
            break;
        }
        
    }

    // If the ray went max steps without hitting anything, just cast the ray to the skybox.
    if (!hitDisk && !hitSphere && !hitInfinity || !hitSphere && !hitInfinity && u_transparentDisk && !u_useDebugDiskTexture)
    {
        dist = metricDistance(xp[0]);
        if (dist > horizon)
        {
            dir = pToDir(xp);
            rayCol += T * texture(skybox, vec3(-dir.x, dir.y, dir.z)).xyz * bloomBackgroundMultiplier;
        }
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

            // https://sibaku.github.io/computer-graphics/2017/01/10/Camera-Ray-Generation.html
            // The image "screen" we're casting through in view space.
            // the z=1.0 value here gives us a point on the far-plane of the view frustrum, but we could've chosen
            // any value.  The w=1.0 value makes the point homogeneous without scaling the vector.
            // Then we undo the projection matrix to get to view space.
            vec4 screen = u_ProjInv * vec4(uv, 1.0, 1.0);
            // We're now in view space, where the camera is at the origin by definition.
            // Convert screen from a point to a direction.
            screen.xyz /= screen.w;
            screen.w = 0.0;
            // ViewInv * screen takes the direction to world space.  Then normalize.
            vec3 rayDir = normalize((u_ViewInv * screen).xyz);

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