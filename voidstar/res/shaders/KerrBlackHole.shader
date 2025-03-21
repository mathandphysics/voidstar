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

in vec2 TexCoords;

uniform sampler2D diskTexture;
uniform sampler2D sphereTexture;
uniform sampler2D spectrumTexture;
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
uniform float u_risco;
uniform float u_fOfRISCO;
uniform float u_BHMass;
uniform float u_dMdt;
uniform float u_a;
uniform float u_Tmax;
uniform float u_diskRotationAngle;

uniform int u_msaa;

uniform int u_maxSteps;
uniform float u_drawDistance;
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
uniform int u_diskDebugDivisions;
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
uniform float u_brightnessFromDiskVel;
uniform float u_blueshiftPower;

layout(location = 0) out vec4 fragColour;
layout(location = 1) out vec4 brightColour;


/////////////////////////////////////////////////////
////////////////////   COMMON   /////////////////////
/////////////////////////////////////////////////////

mat4 diag(vec4 v)
{
    return mat4(v.x, 0.0, 0.0, 0.0,
                0.0, v.y, 0.0, 0.0,
                0.0, 0.0, v.z, 0.0,
                0.0, 0.0, 0.0, v.w);
}

float map(float val, float min1, float max1, float min2, float max2)
{
    float percent = (val - min1) / (max1 - min1);
    return percent * (max2 - min2) + min2;
}


/////////////////////////////////////////////////////
/////////////////////   NOISE   /////////////////////
/////////////////////////////////////////////////////

vec3 mod289(vec3 x)
// From Stefan Gustavson: https://stegu.github.io/webgl-noise/
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x)
// From Stefan Gustavson: https://stegu.github.io/webgl-noise/
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x)
// From Stefan Gustavson: https://stegu.github.io/webgl-noise/
{
    return mod289(((x * 34.0) + 10.0) * x);
}

vec4 taylorInvSqrt(vec4 r)
// From Stefan Gustavson: https://stegu.github.io/webgl-noise/
// A 1-degree Taylor expansion of 1/sqrt(r) at r=0.7.
{
    return 1.79284291400159 - 0.85373472095314 * r;
}

vec3 fade(vec3 t)
// From Ken Perlin or earlier
{
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

float cnoise3D(vec3 P)
// Classic Perlin noise created by Ken Perlin.
// Implementation by Stefan Gustavson: https://stegu.github.io/webgl-noise/
{
    vec3 Pi0 = floor(P); // Integer part for indexing
    vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
    Pi0 = mod289(Pi0);
    Pi1 = mod289(Pi1);
    vec3 Pf0 = fract(P); // Fractional part for interpolation
    vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
    vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
    vec4 iy = vec4(Pi0.yy, Pi1.yy);
    vec4 iz0 = Pi0.zzzz;
    vec4 iz1 = Pi1.zzzz;

    vec4 ixy = permute(permute(ix) + iy);
    vec4 ixy0 = permute(ixy + iz0);
    vec4 ixy1 = permute(ixy + iz1);

    vec4 gx0 = ixy0 * (1.0 / 7.0);
    vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
    gx0 = fract(gx0);
    vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
    vec4 sz0 = step(gz0, vec4(0.0));
    gx0 -= sz0 * (step(0.0, gx0) - 0.5);
    gy0 -= sz0 * (step(0.0, gy0) - 0.5);

    vec4 gx1 = ixy1 * (1.0 / 7.0);
    vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
    gx1 = fract(gx1);
    vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
    vec4 sz1 = step(gz1, vec4(0.0));
    gx1 -= sz1 * (step(0.0, gx1) - 0.5);
    gy1 -= sz1 * (step(0.0, gy1) - 0.5);

    vec3 g000 = vec3(gx0.x, gy0.x, gz0.x);
    vec3 g100 = vec3(gx0.y, gy0.y, gz0.y);
    vec3 g010 = vec3(gx0.z, gy0.z, gz0.z);
    vec3 g110 = vec3(gx0.w, gy0.w, gz0.w);
    vec3 g001 = vec3(gx1.x, gy1.x, gz1.x);
    vec3 g101 = vec3(gx1.y, gy1.y, gz1.y);
    vec3 g011 = vec3(gx1.z, gy1.z, gz1.z);
    vec3 g111 = vec3(gx1.w, gy1.w, gz1.w);

    vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
    g000 *= norm0.x;
    g010 *= norm0.y;
    g100 *= norm0.z;
    g110 *= norm0.w;
    vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
    g001 *= norm1.x;
    g011 *= norm1.y;
    g101 *= norm1.z;
    g111 *= norm1.w;

    float n000 = dot(g000, Pf0);
    float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
    float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
    float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
    float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
    float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
    float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
    float n111 = dot(g111, Pf1);

    vec3 fade_xyz = fade(Pf0);
    vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
    vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
    float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x);
    return 2.2 * n_xyz;
}

float snoise3D(vec3 v)
    // Simplex noise created by Ken Perlin.
    // Implementation by Stefan Gustavson and Ashima Arts: https://stegu.github.io/webgl-noise/
{
    const vec2  C = vec2(1.0 / 6.0, 1.0 / 3.0);
    const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

    // First corner
    vec3 i = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);

    // Other corners
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);

    //   x0 = x0 - 0.0 + 0.0 * C.xxx;
    //   x1 = x0 - i1  + 1.0 * C.xxx;
    //   x2 = x0 - i2  + 2.0 * C.xxx;
    //   x3 = x0 - 1.0 + 3.0 * C.xxx;
    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
    vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

    // Permutations
    i = mod289(i);
    vec4 p = permute(permute(permute(
        i.z + vec4(0.0, i1.z, i2.z, 1.0))
        + i.y + vec4(0.0, i1.y, i2.y, 1.0))
        + i.x + vec4(0.0, i1.x, i2.x, 1.0));

    // Gradients: 7x7 points over a square, mapped onto an octahedron.
    // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
    float n_ = 0.142857142857; // 1.0/7.0
    vec3  ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_);    // mod(j,N)

    vec4 x = x_ * ns.x + ns.yyyy;
    vec4 y = y_ * ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);

    //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
    //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);

    //Normalise gradients
    vec4 norm = taylorInvSqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    // Mix final noise value
    vec4 m = max(0.5 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
    m = m * m;
    return 105.0 * dot(m * m, vec4(dot(p0, x0), dot(p1, x1),
        dot(p2, x2), dot(p3, x3)));
}

float multifractal_noise3D(vec3 xyz, int octaves, float scale, float lacunarity, float dimension)
{
    // Multifractal noise with multiplied fractals rather than added.
    float val = 1.0;
    float amplitude = scale;
    float gain = pow(lacunarity, -dimension);
    float frequency = 1.0;

    for (int i = 0; i < octaves; i++)
    {
        val *= amplitude * cnoise3D(frequency * xyz) + 1.0;
        //val *= amplitude * snoise3D(frequency * xyz) + 1.0;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return val;

}


/////////////////////////////////////////////////////
//////////////   GENERAL RELATIVITY   ///////////////
/////////////////////////////////////////////////////

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
    // Calculate the Kerr metric in Kerr-Schild coordinates at the position x.  G = c = 1.  Signature (-+++).
    // This differs slightly from the definition in https://arxiv.org/abs/0706.0622
    // because that paper has Cartesian z as the up direction, while this program has Cartesian y
    // as the up direction.
    vec3 p = x.yzw;
    float r = implicitr(x);
    float r2 = r*r;
    float a2 = u_a*u_a;
    float f = 2.0 * u_BHMass * r2*r / (r2*r2 + a2*p.y*p.y);
#ifdef INSIDE_HORIZON
    // Outgoing Kerr-Schild coordinates.
    vec4 l = vec4(-1.0, (r * p.x - u_a * p.z) / (r2 + a2), p.y / r, (r * p.z + u_a * p.x) / (r2 + a2));
#else
    // Ingoing Kerr-Schild coordinates.
    vec4 l = vec4(1.0, (r * p.x - u_a * p.z) / (r2 + a2), p.y / r, (r * p.z + u_a * p.x) / (r2 + a2));
#endif
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

    // Calculate the inverse Kerr metric in Kerr-Schild coordinates at the position x.  G = c = 1. Signature (-+++).
    // As with the metric, this is also from https://arxiv.org/abs/0706.0622 with adjusted coordinates.
    // Due to the convenient decomposition of these coordinates, this has pretty much identical computational cost as
    // the metric.
    vec3 p = x.yzw;
    float r = implicitr(x);
    float r2 = r*r;
    float a2 = u_a*u_a;
    float f = 2.0 * u_BHMass * r2*r / (r2*r2 + a2*p.y*p.y);
#ifdef INSIDE_HORIZON
    // Outgoing Kerr-Schild coordinates.
    vec4 l = vec4(1.0, (r * p.x - u_a * p.z) / (r2 + a2), p.y / r, (r * p.z + u_a * p.x) / (r2 + a2));
#else
    // Ingoing Kerr-Schild coordinates.
    vec4 l = vec4(-1.0, (r * p.x - u_a * p.z) / (r2 + a2), p.y / r, (r * p.z + u_a * p.x) / (r2 + a2));
#endif
    return diag(vec4(-1.0, 1.0, 1.0, 1.0)) - f * outerProduct(l, l);
}
#endif

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

float metricDistance(vec4 x)
{
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
    // Naive dH/dx approximation.
    vec4 Hdx;
    float dx = 0.005; // Governs accuracy of dHdx
    Hdx[0] = H(x + vec4(dx, 0.0, 0.0, 0.0), p);
    Hdx[1] = H(x + vec4(0.0, dx, 0.0, 0.0), p);
    Hdx[2] = H(x + vec4(0.0, 0.0, dx, 0.0), p);
    Hdx[3] = H(x + vec4(0.0, 0.0, 0.0, dx), p);
    return (Hdx - H(x, p)) / dx;
}

#ifdef KERR
vec4 KerrdHdxExact(vec4 x, vec4 p)
{
    // Calculates dH/dx exactly for the Kerr metric in Kerr-Schild Cartesian coordinates.
    // Not only is this obviously more accurate than the naive approximation of dHdx, but it happens to be
    // much faster as well.
    // 
    // dH/dx^\alpha = (1/2) * dg^{\mu \nu}/dx^\alpha * p_\mu * p_\nu
    // Since g^ab = \eta^ab - f * outerProduct(l,l),
    // dg^ab / dx^\alpha = -df/dx^\alpha * outerProduct(l,l) - f * outerProduct(dl/dx^\alpha, l) - f * outerProduct(l, dl/dx^\alpha)
    // We note that since p^T outerProduct(dl/dx^\alpha, l) p = p^T  outerProduct(l, dl/dx^\alpha) p,
    // we only need to calculate -df/dx^\alpha * outerProduct(l,l) - 2 * f * outerProduct(dl/dx^\alpha, l).
    // So we need to calculate df/dx^\alpha and dl/dx^\alpha.  To calculate those, we'll also need dr/dx^\alpha.
    vec3 pos = x.yzw;
    float r = implicitr(x);
    float r2 = r * r;
    float r3 = r2 * r;
    float r4 = r3 * r;
    float a2 = u_a * u_a;
    float r2plusa2 = r2 + a2;
    float a4 = a2 * a2;
    float y2 = pos.y * pos.y;
    float f = 2.0 * u_BHMass * r3 / (r4 + a2 * y2);
    float xnumer = (r * pos.x - u_a * pos.z);
    float znumer = (r * pos.z + u_a * pos.x);
#ifdef INSIDE_HORIZON
    // Outgoing Kerr-Schild coordinates.
    vec4 l = vec4(1.0, xnumer / r2plusa2, pos.y / r, znumer / r2plusa2);
#else
    // Ingoing Kerr-Schild coordinates.
    vec4 l = vec4(-1.0, xnumer / r2plusa2, pos.y / r, znumer / r2plusa2);
#endif

    // Calculate dr/dx, dr/dy, dr/dz.
    float dr_common_factor = r3 * r2plusa2 * r2plusa2 / (y2 * r2plusa2 * r2plusa2 + r4 * (pos.x * pos.x + pos.z * pos.z));
    float drdx = (pos.x / r2plusa2) * dr_common_factor;
    float drdy = (pos.y / r2) * dr_common_factor;
    float drdz = (pos.z / r2plusa2) * dr_common_factor;

    // Calculate df/dt, df/dx, df/dy, df/dz using dr/dx, dr/dy, dr/dz.
    float df_common_numerator = (6.0 * u_BHMass * r2 * (r4 + a2 * y2) - 8.0 * u_BHMass * r4 * r2);
    float df_common_denominator = (r4 + a2 * y2) * (r4 + a2 * y2);
    float dfdt = 0.0;
    float dfdx = df_common_numerator * drdx / df_common_denominator;
    float dfdy = (df_common_numerator * drdy - 4.0 * u_BHMass * r3 * a2 * pos.y) / df_common_denominator;
    float dfdz = df_common_numerator * drdz / df_common_denominator;
    vec4 dfdxhat = vec4(dfdt, dfdx, dfdy, dfdz);

    // Calculate dl/dt, dl/dx, dl/dy, dl/dz.
    float denom = r2plusa2 * r2plusa2;
    vec4 dldt = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 dldx = vec4(0.0, (r2plusa2 * (drdx * pos.x + r) - xnumer * 2.0 * r * drdx) / denom,
        -pos.y * drdx / r2, (r2plusa2 * (drdx * pos.z + u_a) - znumer * 2.0 * r * drdx) / denom);
    vec4 dldy = vec4(0.0, (r2plusa2 * drdy * pos.x - xnumer * 2.0 * r * drdy) / denom, (r - pos.y * drdy) / r2,
        (r2plusa2 * drdy * pos.z - znumer * 2.0 * r * drdy) / denom);
    vec4 dldz = vec4(0.0, (r2plusa2 * (drdz * pos.x - u_a) - xnumer * 2.0 * r * drdz) / denom,
        -pos.y * drdz / r2, (r2plusa2 * (drdz * pos.z + r) - znumer * 2.0 * r * drdz) / denom);

    // Calculate dot products
    // Rather than calculating the outerProducts and then multiplying by p on both sides, we can skip this and just
    // calculate the dot products directly.
    float ldotp = dot(l, p);
    vec4 dldxhatdotp = vec4(dot(dldt, p), dot(dldx, p), dot(dldy, p), dot(dldz, p));
    vec4 firstterm = vec4(ldotp * ldotp);
    vec4 secondterm = vec4(2.0 * f * ldotp);

    // Equivalent to the 4-vector whose alpha-th component is:
    // (1/2) * (-df/dx^\alpha * p^T * outerProduct(l,l) * p - 2 * f * p^T * outerProduct(l, dl/dx^\alpha) * p)
    // which is precisely the vector dH/dx.
    return -0.5 * (dfdxhat * firstterm + secondterm * dldxhatdotp);
}
#endif

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

    mat2x4 dxp;
#ifdef KERR
    dxp[1] = -KerrdHdxExact(x, p) * dl;
#else
    // General approximation of dHdx.
    float Hatx = 0.5 * dot(ginv * p, p);
    float dx = 0.005; // Governs accuracy of dHdx
    vec4 Hdx = vec4( H(x + vec4(dx, 0.0, 0.0, 0.0), p), H(x + vec4(0.0, dx, 0.0, 0.0), p),
        H(x + vec4(0.0, 0.0, dx, 0.0), p), H(x + vec4(0.0, 0.0, 0.0, dx), p));
    vec4 dHdx = (Hdx - Hatx) / dx;
    dxp[1] = -dHdx * dl;
#endif
    dxp[0] = ginv * p * dl;

    return dxp;
}
#endif

#if (ODE_SOLVER == 0)
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

    mat2x4 dxp;
#ifdef KERR
    dxp[1] = -KerrdHdxExact(x, p) * dl;
#else
    // General formula.
    float Hatx = 0.5 * dot(ginv * p, p);
    float dx = 0.005; // Governs accuracy of dHdx
    vec4 Hdx = vec4(H(x + vec4(dx, 0.0, 0.0, 0.0), p), H(x + vec4(0.0, dx, 0.0, 0.0), p),
        H(x + vec4(0.0, 0.0, dx, 0.0), p), H(x + vec4(0.0, 0.0, 0.0, dx), p));
    vec4 dHdx = (Hdx - Hatx) / dx;
    dxp[1] = -dHdx * dl;
#endif
    // Correction for Euler-Cromer method as opposed to vanilla Euler method
    p += dxp[1];
    dxp[0] = ginv * p * dl;

    return dxp;
}
#endif


/////////////////////////////////////////////////////
/////////   DIFFERENTIAL EQUATION SOLVING   /////////
/////////////////////////////////////////////////////

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

#if (ODE_SOLVER == 1 || ODE_SOLVER == 3)
mat2x4 RK4integrationStep(mat2x4 xp, float dl)
{
    // Classic Runge-Kutta 4.
    // https://en.wikipedia.org/wiki/Runge%E2%80%93Kutta_methods#Examples
    // Note that the differential equations here don't explicitly depend on the 
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

#if (ODE_SOLVER == 2 || ODE_SOLVER == 3)
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

void RK23integrationStepFSAL(mat2x4 xp, inout mat2x4 nextxp1, inout mat2x4 nextxp2, float stepsize, inout mat2x4 FSAL)
{
    // Bogacki-Shampine Method.
    // https://en.wikipedia.org/wiki/Bogacki%E2%80%93Shampine_method
    // Note that the differential equations here don't explicitly depend on the 
    // affine parameter.  i.e. instead of dy/dt = f(t, y), we just have dy/dt = f(y).
    // So the vector field f is stationary in time.  This simplifies the xpupdate function.
    // Here y is the vector of (x, p) and f is the vector of (dH/dp, -dH/dx).

    // This version of the function uses the first-same-as-last (FSAL) optimization.
    mat2x4 k1;
    mat2x4 k2;
    mat2x4 k3;
    mat2x4 k4;

    k1 = FSAL * stepsize;
    k2 = fasterxpupdate(xp + 0.5 * k1, stepsize);
    k3 = fasterxpupdate(xp + 0.75 * k2, stepsize);
    nextxp1 = xp + (2.0 / 9.0) * k1 + (1.0 / 3.0) * k2 + (4.0 / 9.0) * k3;
    k4 = fasterxpupdate(nextxp1, stepsize);
    nextxp2 = xp + (7.0 / 24.0) * k1 + (1.0 / 4.0) * k2 + (1.0 / 3.0) * k3 + (1.0 / 8.0) * k4;
    FSAL = k4 / stepsize;
}
#endif

#if (ODE_SOLVER == 3)
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

void RK45integrationStepFSAL(mat2x4 xp, inout mat2x4 nextxp1, inout mat2x4 nextxp2, float stepsize, inout mat2x4 FSAL)
{
    // Dormand-Prince Method.
    // https://en.wikipedia.org/wiki/Dormand%E2%80%93Prince_method
    // Note that the differential equations here don't explicitly depend on the 
    // affine parameter.  i.e. instead of dy/dt = f(t, y), we just have dy/dt = f(y).
    // So the vector field f is stationary in time.  This simplifies the xpupdate function.
    // Here y is the vector of (x, p) and f is the vector of (dH/dp, -dH/dx).

    // This version of the function uses the first-same-as-last (FSAL) optimization.
    mat2x4 k1;
    mat2x4 k2;
    mat2x4 k3;
    mat2x4 k4;
    mat2x4 k5;
    mat2x4 k6;
    mat2x4 k7;

    k1 = FSAL * stepsize;
    k2 = fasterxpupdate(xp + 0.2 * k1, stepsize);
    k3 = fasterxpupdate(xp + (3.0 / 40.0) * k1 + (9.0 / 40.0) * k2, stepsize);
    k4 = fasterxpupdate(xp + (44.0 / 45.0) * k1 + (-56.0 / 15.0) * k2 + (32.0 / 9.0) * k3, stepsize);
    k5 = fasterxpupdate(xp + (19372.0 / 6561.0) * k1 + (-25360.0 / 2187.0) * k2 + (64448.0 / 6561.0) * k3 + (-212.0 / 729.0) * k4, stepsize);
    k6 = fasterxpupdate(xp + (9017.0 / 3168.0) * k1 + (-355.0 / 33.0) * k2 + (46732.0 / 5247.0) * k3 + (49.0 / 176.0) * k4 + (-5103.0 / 18656.0) * k5, stepsize);
    nextxp1 = xp + (35.0 / 384.0) * k1 + (0.0) * k2 + (500.0 / 1113.0) * k3 + (125.0 / 192.0) * k4 + (-2187.0 / 6784.0) * k5 + (11.0 / 84.0) * k6;
    k7 = fasterxpupdate(nextxp1, stepsize);
    nextxp2 = xp + (5179.0 / 57600.0) * k1 + (0.0) * k2 + (7571.0 / 16695.0) * k3 + (393.0 / 640.0) * k4 + (-92097.0 / 339200.0) * k5 + (187.0 / 2100.0) * k6 + (1.0 / 40.0) * k7;
    FSAL = k7 / stepsize;
}
#endif

#if (ODE_SOLVER == 2 || ODE_SOLVER == 3)
mat2x4 adaptiveRKDriver(mat2x4 xp, inout float stepsize, out float oldStepSize, inout mat2x4 FSAL)
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
    
    mat2x4 localFSAL = FSAL;

    // Rather than running a while loop, which can crash the program in rare edge cases, we attempt to find
    // a stepsize that produces an acceptable error size in at most max_attempts.
    int max_attempts = 10;
    for (int i = 0; i < max_attempts; i++)
    {
        localFSAL = FSAL;
        // HACK.  The adaptive driver steps too far for flat or close to flat spacetimes.  This causes it to miss
        // crossing the disk or the sphere.
#ifdef CLASSICAL
        stepsize = min(0.01 + (metricDistance(xp[0]) - 2.0 * u_BHMass) * 1.0 / 5.0, stepsize);
#endif
#ifdef MINKOWSKI
        stepsize = 0.01 + metricDistance(xp[0]) / 5.0;
#endif

#if (ODE_SOLVER == 2)
        RK23integrationStepFSAL(xp, nextxp1, nextxp2, stepsize, localFSAL);
        //RK23integrationStep(xp, nextxp1, nextxp2, stepsize);
#elif (ODE_SOLVER == 3)
        RK45integrationStepFSAL(xp, nextxp1, nextxp2, stepsize, localFSAL);
        //RK45integrationStep(xp, nextxp1, nextxp2, stepsize);
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

    FSAL = localFSAL;
    return nextxp1;
}
#endif


/////////////////////////////////////////////////////
//////////////   INTERSECTION POINTS   //////////////
/////////////////////////////////////////////////////

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
    mat2x4 FSAL = fasterxpupdate(previousxp, stepsize) / stepsize;
    mat2x4 localFSAL = FSAL;
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
#if (ODE_SOLVER == 2 || ODE_SOLVER == 3)
            localFSAL = FSAL;
#endif
            midpoint = (leftEndpoint + rightEndpoint) / 2.0;
#if (ODE_SOLVER == 0)
            xptest = integrationStep(previousxp, midpoint);
#elif (ODE_SOLVER == 1)
            xptest = RK4integrationStep(previousxp, midpoint);
#elif (ODE_SOLVER == 2)
            //RK23integrationStep(previousxp, xptest, xptest2, midpoint);
            RK23integrationStepFSAL(previousxp, xptest, xptest2, midpoint, localFSAL);
#elif (ODE_SOLVER == 3)
            //RK23integrationStepFSAL(previousxp, xptest, xptest2, midpoint, localFSAL);
            //RK45integrationStep(previousxp, xptest, xptest2, midpoint);
            //RK45integrationStepFSAL(previousxp, xptest, xptest2, midpoint, localFSAL);
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
    mat2x4 FSAL = fasterxpupdate(xp, stepsize) / stepsize;
    mat2x4 localFSAL = FSAL;
#endif
    for (int j = 0; j < BS_attempts; j++)
    {
#if (ODE_SOLVER == 2 || ODE_SOLVER == 3)
        localFSAL = FSAL;
#endif
        midpoint = (leftEndpoint + rightEndpoint) / 2.0;
#if (ODE_SOLVER == 0)
        xptest = integrationStep(xp, midpoint);
#elif (ODE_SOLVER == 1)
        xptest = RK4integrationStep(xp, midpoint);
#elif (ODE_SOLVER == 2)
        //RK23integrationStep(xp, xptest, xptest2, midpoint);
        RK23integrationStepFSAL(xp, xptest, xptest2, midpoint, localFSAL);
#elif (ODE_SOLVER == 3)
        RK45integrationStep(xp, xptest, xptest2, midpoint);
        //RK23integrationStep(xp, xptest, xptest2, midpoint);
        //RK23integrationStepFSAL(xp, xptest, xptest2, midpoint, localFSAL);
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


/////////////////////////////////////////////////////
////////////////   SPHERE SHADING   /////////////////
/////////////////////////////////////////////////////

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


/////////////////////////////////////////////////////
/////////////////   DISK SHADING   //////////////////
/////////////////////////////////////////////////////

vec3 drawDebugDiskTexture(const in vec3 p)
{
    float pi = 3.14159265359;
    vec3 col = vec3(0.0);

    float u = (atan(p.x, p.z) + pi) / (2.0 * pi) - u_diskRotationAngle;
    u = u - floor(u);
    float v = clamp(1.0 - (length(vec2(p.x, p.z)) - u_InnerRadius) / (u_OuterRadius - u_InnerRadius), 0.0, 1.0);

    // If drawing a texture to the disk:
    //col = texture(diskTexture, vec2(u, v)).xyz;

    // Makes alternating pattern between the two different colours.
    float usign = sign(sin(u_diskDebugDivisions * 2.0 * pi * u));
    float uscaled = (usign + 1.0) * 0.5;
    if (p.y >= 0)
    {
        col = uscaled * u_diskDebugColourTop1 + (1.0 - uscaled) * u_diskDebugColourTop2;
    }
    else
    {
        col = uscaled * u_diskDebugColourBottom1 + (1.0 - uscaled) * u_diskDebugColourBottom2;
    }
    return col;
}

float sampleNoiseTexture(const in vec3 p)
{
    // Inspired by https://www.youtube.com/watch?v=XWv1Ajc3tfU

    vec3 col = vec3(0.0);
    // To spherical coordinates
    float r = length(p.xz);
    float phi = atan(p.z, p.x);
    float theta = atan(r, p.y);

    // Smear theta
    float rotational_smear = -1.2;
    float power = pow(theta, rotational_smear);

    // Spiral phi and rotate it
    float rho = length(p);
    float rhohalf = sqrt(rho);
    float spiral_factor = 3.0;
    float spiral = map(rhohalf, 0.0, 5.0, 0.0, spiral_factor);
    phi += spiral + u_diskRotationAngle;

    // Back to Cartesian with the adjusted angles
    vec3 xyz = vec3(rho * sin(power) * cos(phi), rho * sin(power) * sin(phi), rho * cos(power));

    // Noise
    int octaves = 3;
    float lacunarity = 2.0;
    float dimension = 0.12;
    float amplitude = 0.6;
    float noise = multifractal_noise3D(xyz, octaves, amplitude, lacunarity, dimension);

    return noise;
}

vec3 TemperatureToRGB(in float temperature) {
    // TODO: replace this with the procedure described here: https://scipython.com/blog/converting-a-spectrum-to-a-colour/
    // For photons in free space, specific intensity / spectral radiance (brightness) I(\nu) is proportional to \nu**3
    // See: https://en.wikipedia.org/wiki/Planck%27s_law#The_law for B_\nu(\nu, T)
    // and https://en.wikipedia.org/wiki/Black-body_radiation#Doppler_effect

    // From https://www.shadertoy.com/view/4sc3D7
    // Converts a blackbody temperature to colour in RGB.
    // Valid from 1000 to 40000 K.
    // Values from: http://blenderartists.org/forum/showthread.php?270332-OSL-Goodness&p=2268693&viewfull=1#post2268693   
    temperature = clamp(temperature, 1000.0, 40000.0);
    mat3 m = (temperature <= 6500.0) ? mat3(vec3(0.0, -2902.1955373783176, -8257.7997278925690),
        vec3(0.0, 1669.5803561666639, 2575.2827530017594),
        vec3(1.0, 1.3302673723350029, 1.8993753891711275)) :
        mat3(vec3(1745.0425298314172, 1216.6168361476490, -8257.7997278925690),
            vec3(-2666.3474220535695, -2173.1012343082230, 2575.2827530017594),
            vec3(0.55995389139931482, 0.70381203140554553, 1.8993753891711275));
    vec3 colour = mix(clamp(vec3(m[0] / (vec3(temperature) + m[1]) + m[2]), vec3(0.0), vec3(1.0)), vec3(1.0), smoothstep(1000.0, 0.0, temperature));
    return colour;
}

vec3 TemperatureToRGBTexture(in float temperature)
{
    // Spectrum texture goes from 500K to 40000K.  Need to convert temperature to coordinates.
    temperature = clamp(temperature, 500.0, 40000.0);
    temperature = map(temperature, 500.0, 40000.0, 0.0, 1.0);
    return texture(spectrumTexture, vec2(temperature, 0.0)).xyz;
}

float f(const in float r)
{
    // Uses the black hole's mass and spin, along with a point on the equatorial accretion disk that is r away from the
    // center of the black hole, to calculate f, a function used in the calculation of the time-averaged flux of radiant energy.
    // From "Disk-accretion onto a black hole" by Page and Thorne (1973).
    if (r < u_risco)
    {
        return 0.0;
    }
    float pi = 3.14159265359;

    float astar = u_a / u_BHMass;
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

float observedTemperature(const float r, const float blueshift)
{
    // from "Detecting Accretion Disks in Active Galactic Nuclei" by Fanton, et al (1997).
    // (49/36)*R_isco is the approximate radial distance where the accretion disk attains its maximum brightness / temperature.
    float r_max = (49.0 / 36.0) * u_risco;

    // Realistic value for u_Tmax is roughly 10000K.
    return blueshift * u_Tmax * pow(f(r) * r_max / (r * f(r_max)), 1.0 / 4.0);
}

vec3 getDiskColour(const in mat2x4 diskIntersectionPoint, const in float previousy, const in float r, inout float T)
{
    vec3 rayCol;
    vec3 diskSample;
    vec3 emission;
    float brightnessFromRadius;
    float brightnessFromVel;
    float temperature;
    vec4 diskVel;

    vec4 planeIntersectionPoint = diskIntersectionPoint[0];
    if (u_useDebugDiskTexture)
    {
        // Need to pass in the sign of previousx's Cartesian y-value to colour the top and bottom of the disk
        // differently in debug mode.
        diskSample = drawDebugDiskTexture(vec3(planeIntersectionPoint.y, previousy, planeIntersectionPoint.w));
    }
    else
    {
        // Can do multiple noise texture samples here and combine them.
        diskSample = vec3(1.0) * sampleNoiseTexture(planeIntersectionPoint.yzw);
    }

    // This r-mapping is a hack to make the disc look nice when the disk's inner radius doesn't match the ISCO.
    // Of course, this has no basis in reality.
    float mappedr = map(r, u_InnerRadius, u_OuterRadius, u_risco, u_OuterRadius);

    if (!u_drawBasicDisk)
    {
        // Velocity (in Kerr-Schild cartesian coordinates) of a massive particle in circular orbit in the equatorial plane
        // at a distance r from a black hole with mass u_BHMass and spin a.
        // The minus sign in front of the time term is just to make the later dot product work out.  The ray of light
        // is moving toward the disk because we're tracing backwards, so we need to swap the sign on either
        // the light momentum or the disk velocity, so we just do it here for simplicity.  We flip observerVel's time term
        // for the same reason.
#ifdef INSIDE_HORIZON
        diskVel = vec4((r + u_a * sqrt(u_BHMass / r)), vec3(-planeIntersectionPoint.w, 0.0, planeIntersectionPoint.y) * sqrt(u_BHMass / r)) / sqrt(r * r - 3.0 * r * u_BHMass + 2.0 * u_a * sqrt(u_BHMass * r));
#else
        diskVel = vec4(-(r + u_a * sqrt(u_BHMass / r)), vec3(-planeIntersectionPoint.w, 0.0, planeIntersectionPoint.y) * sqrt(u_BHMass / r)) / sqrt(r * r - 3.0 * r * u_BHMass + 2.0 * u_a * sqrt(u_BHMass * r));
#endif
        // Ensure diskVel is normalized, otherwise the dot product produces incorrect results for g.
        diskVel /= sqrt(-dot(metric(vec4(diskIntersectionPoint[0])) * diskVel, diskVel));

        // g is the energy/frequency shift aka the Doppler effect.
        // Note that we actually want to compute the sum dx_i/dt * dy^i/dt, where x is the light ray's position and y is the
        // disk particle's position.  We are using momentum coordinates for the light, so we'd need to multiply by the
        // inverse metric to get dx^i/dt = g^ij * p_j.  But then in the metric dot product, we'd multiply by g_ij again:
        // metricdot(dx^i/dt, dy^i/dt) = regulardot(g_ij dx^j/dt, dy^i/dt) = regulardot(g_ik * g^kj * p_j, dy^i/dt) = regulardot(p_i, dy^i/dt)
        // See, e.g. Gravitation by Misner, Thorne, and Wheeler, page 64.
        // NOTE: because the camera is stationary, including in places where the gravitational field is very strong,
        // the camera would have to be moving faster than the speed of light to not fall into the black hole.
        // This would cause g to diverage at points if we included the numerator of g.
        float g = 1.0 / dot(diskIntersectionPoint[1], diskVel);

        // u_blueshiftPower = 1.0 is physically correct.
        float blueshift = pow(g, u_blueshiftPower);
        temperature = observedTemperature(mappedr, blueshift);

        // u_brightnessFromVel = 4.0 is physically correct.
        // Needs to be clamped because the blueshift near the horizon and inside the horizon are too extreme otherwise and
        // cause numerical issues.
        brightnessFromVel = clamp(pow(g, u_brightnessFromDiskVel), 0.0, 10.0);

        // In brightnessFromRadius, subtracting the outer radius term ensures that the emissivity is 0 at the outer edge
        // of the disk so that it perfectly smoothly fades away.
        // 
        // This is F(r) from "Disk-accretion onto a black hole" by Page and Thorne (1973)
        // They called it the time-averaged flux of radiant energy / (time * area)
        // \dot{M} = dM/dt is assumed to be constant in a steady disk.
        //brightnessFromRadius = u_dMdt * (f(r) / r - ((r - u_InnerRadius) / (u_OuterRadius - u_InnerRadius)) * (f(u_OuterRadius) / u_OuterRadius));
        // brightnessFromRadius uses mappedr instead of r in order to have sensible looking results when the inner radius
        // is not equal to r_ISCO.
        brightnessFromRadius = u_dMdt * (f(mappedr) / mappedr - ((mappedr - u_risco) / (u_OuterRadius - u_risco)) * (f(u_OuterRadius) / u_OuterRadius));
        // 
        // Instead, we can also use a simple power law as in https://arxiv.org/pdf/1601.02389.
        // They call this the emissivity: \epsilon(r) \propto r^-q, where q is called the emissivity index.
        // A common value for q is 2.5.
        //brightnessFromRadius = clamp(u_dMdt * (pow(r, -2.5) - pow(u_OuterRadius, -2.5)), 0.0, 1.0);

        emission = brightnessFromRadius * brightnessFromVel * u_bloomDiskMultiplier * diskSample * TemperatureToRGB(temperature);
        //emission = brightnessFromRadius * brightnessFromVel * u_bloomDiskMultiplier * diskSample * TemperatureToRGBTexture(temperature);
        rayCol = T * emission;
    }
    else
    {
        emission = diskSample * u_bloomDiskMultiplier;
        rayCol = T * emission;
    }
    if (u_transparentDisk && !u_useDebugDiskTexture && T > 0.05)
    {
        // Beer's law (https://en.wikipedia.org/wiki/Beer%E2%80%93Lambert_law)
        //brightnessFromRadius = clamp(10000.0 * ((f(r) / r) - (f(u_OuterRadius) / u_OuterRadius)), 0.0, 1.0);
        float absorptionDropOff = clamp(700.0 * (pow(mappedr, -2.5) - pow(u_OuterRadius, -2.5)), 0.0, 1.0);
        float absorptionNoise = sampleNoiseTexture(-planeIntersectionPoint.yzw);
        float absorption = u_diskAbsorption * absorptionNoise * absorptionDropOff;
        T *= exp(-absorption);
    }
    else if (u_useDebugDiskTexture)
    {
        // No transparency.
        T = 0.0;
    }
    return rayCol;
}


/////////////////////////////////////////////////////
/////////////////   RAY MARCHING   //////////////////
/////////////////////////////////////////////////////

void rayMarch(vec3 cameraPos, vec3 rayDir, inout vec3 rayCol, inout bool hitDisk)
{
    vec3 dir;
    float dist;
    float diskDist;
    float T = 1.0;  // Transmittance
    float horizon = u_BHMass + sqrt(u_BHMass * u_BHMass - u_a * u_a); // G = c = 1
    mat2x4 diskIntersectionPoint;
    mat2x4 sphereIntersectionPoint;
    bool hitSphere = false;
    bool hitInfinity = false;

    // x and p are the spacetime position and momentum coordinates.
    // p_i = g_ij * dx^j/dlambda
    // Set x_t = 0.  Set dx^0/dlambda = 1 for ingoing coordinates and -1 for outgoing coordinates.
    mat2x4 xp;
    mat2x4 previousxp;
    xp[0] = vec4(0.0, cameraPos);
    dist = metricDistance(xp[0]);
#ifdef INSIDE_HORIZON
    // Inside the event horizon, we change the metric to outgoing coordinates.  We adjust p accordingly.
    xp[1] = metric(xp[0]) * vec4(-1.0, rayDir);
#else
    xp[1] = metric(xp[0]) * vec4(1.0, rayDir);
#endif

    float stepSize;
    float oldStepSize;
    // Cheap, dumb initial stepsize heuristic
#ifdef INSIDE_HORIZON
    stepSize = dist / (100.0 * horizon);
#else
    stepSize = 0.01 + (dist - horizon) / 10.0;
#endif

#if (ODE_SOLVER == 2 || ODE_SOLVER == 3)
    // Prepare adaptive ODE solvers for first-same-as-last (FSAL).
    mat2x4 FSAL = fasterxpupdate(xp, stepSize) / stepSize;
#endif

    // MAIN RAYMARCH LOOP
    for (int i = 0; i < u_maxSteps; i++)
    {
        previousxp = xp;

        // The actual integration step, depending on which ODE solver is used.
#if (ODE_SOLVER == 0)
        xp = integrationStep(xp, stepSize);
        dist = metricDistance(xp[0]);
        oldStepSize = stepSize;
#ifdef INSIDE_HORIZON
        stepSize = dist / (10.0 * horizon);
#else
        stepSize = 0.01 + (dist - horizon) / 10.0;
#endif
#elif (ODE_SOLVER == 1)
        xp = RK4integrationStep(xp, stepSize);
        dist = metricDistance(xp[0]);
        oldStepSize = stepSize;
#ifdef INSIDE_HORIZON
        stepSize = dist / (10.0 * horizon);
#else
        stepSize = 0.01 + (dist - horizon) / 5.0;
#endif
#elif (ODE_SOLVER == 2 || ODE_SOLVER == 3)
        xp = adaptiveRKDriver(xp, stepSize, oldStepSize, FSAL);
        dist = metricDistance(xp[0]);
#endif

        // Check if the ray hit the disk
        // Check to see whether the Cartesian y-coordinate changed signs, i.e. if the ray crossed the disk's plane.
        // CAUTION: when the stepsize is too big (e.g. for RK45), then it's possible for a ray to cross the xz-plane
        // and then cross back over to the previous side all within a single step.  This can lead to odd holes in
        // the accretion disk.  Decreasing tolerance (i.e. forcing a smaller stepsize) fixes this.
        if (xp[0][2] * previousxp[0][2] < 0.0)
        {
            if (dist > horizon)
            {
                // Do a binary search on stepsize to find the point where the geodesic crosses the xz-plane.
                BSDiskIntersectionPoint(previousxp, xp, diskIntersectionPoint, oldStepSize);
                diskDist = metricDistance(diskIntersectionPoint[0]);
                if (diskDist <= u_OuterRadius && diskDist >= u_InnerRadius)
                {
                    hitDisk = true;
                    rayCol += getDiskColour(diskIntersectionPoint, sign(previousxp[0][2]), diskDist, T);
                }
                if (T < 0.05)
                {
                    break;
                }
            }
        }

        // If the camera is outside the horizon, check if the ray hit the sphere.
#ifndef INSIDE_HORIZON
        if (dist < horizon)
        {
            hitSphere = true;
            if (u_useDebugSphereTexture)
            {
                BSSphereIntersectionPoint(previousxp, sphereIntersectionPoint, horizon, oldStepSize);
                rayCol += T * getSphereColour(sphereIntersectionPoint[0].yzw);
            }
            break;
        }
#endif

        // Check if the ray escaped the black hole and hit the skybox.
        else if (dist > u_drawDistance)
        {
            hitInfinity = true;
            dir = pToDir(xp);
            rayCol += T * texture(skybox, vec3(-dir.x, dir.y, dir.z)).xyz * u_bloomBackgroundMultiplier;
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
            rayCol += T * texture(skybox, vec3(-dir.x, dir.y, dir.z)).xyz * u_bloomBackgroundMultiplier;
        }
    }
}


/////////////////////////////////////////////////////
/////////////////////   MAIN   //////////////////////
/////////////////////////////////////////////////////

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
            // the z=0.0 value here gives us a point in the view frustrum, but we could've chosen
            // any value <=1.0.  The w=1.0 value makes the point homogeneous without scaling the vector.
            // Then we undo the projection matrix to get to view space.
            vec4 screen = u_ProjInv * vec4(uv, 0.0, 1.0);
            // We're now in view space, where the camera is at the origin by definition.
            // Convert screen from a point to a direction in projective space.
            screen.xyz /= screen.w;
            screen.w = 0.0;
            // ViewInv * screen takes the direction to world space.  Then normalize for our final unit length ray direction.
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
    if (brightness > u_bloomThreshold)
        brightColour = vec4(fragColour.rgb, 1.0);
    else
        brightColour = vec4(0.0, 0.0, 0.0, 1.0);
}