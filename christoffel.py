import sympy

G, M, t, c, x, y, z, r, theta, phi = sympy.symbols(" G M t c x y z r theta phi ")
r_s = sympy.symbols("r_s") # r_s = 2*G*M/c**2

cartesian_coords = [t, x, y, z]
spherical_coords = [t, r, theta, phi]

newton_metric = sympy.matrices.Matrix( [
                                       [ -(1 - 2*G*M/(c*c*(x**2+y**2+z**2)**(1/2)))*c**2, 0, 0, 0 ],
                                       [ 0, 1, 0, 0],
                                       [ 0, 0, 1, 0],
                                       [ 0, 0, 0, 1] ] )

# When you take the classical limit of the Schwarzchild metric where 2*G*M/(c^2*r) << 1.  This leads
# to essentially Newtonian gravity with a doubling factor.
weak_field_limit_metric = sympy.matrices.Matrix( [
                                       [ -(1 - 2*G*M/(c*c*(x**2+y**2+z**2)**(1/2)))*c**2, 0, 0, 0 ],
                                       [ 0, (1 + 2*G*M/(c*c*(x**2+y**2+z**2)**(1/2))), 0, 0],
                                       [ 0, 0, (1 + 2*G*M/(c*c*(x**2+y**2+z**2)**(1/2))), 0],
                                       [ 0, 0, 0, (1 + 2*G*M/(c*c*(x**2+y**2+z**2)**(1/2)))] ] )
                                       
schwarzchild_metric = sympy.matrices.Matrix( [
                                             [-(1-r_s/r)*c**2, 0, 0, 0],
                                             [0, 1/(1-r_s/r), 0, 0],
                                             [0, 0, r**2, 0],
                                             [0, 0, 0, r**2 * (sympy.sin(theta))**2]])
                                       
                                       
def partialm(metric, coord, i, j, k):
    """
    Returns d(m(i,j))/dk, where i, j, and k are numbers that index the coordinates.
    """
    return sympy.diff(metric[i, j], coord[k])
                                       

def calculateChristoffel(metric, coords, i, k, l):
    """
    Calculates \Gamma^i_{kl}
    """
    inverse_metric = metric.inv()
    num_coords = len(coords)
    s = sympy.S.Zero

    for m in range(num_coords):
        s += (1/2)*inverse_metric[i, m]*(partialm(metric, coords, m, k, l) + partialm(metric, coords, m, l, k) - partialm(metric, coords, k, l, m))

    return sympy.simplify(s)
    


def test_newton_metric_christoffels():
    for i in range(4):
        for j in range(4):
            for k in range(4):
                x = calculateChristoffel(newton_metric, cartesian_coords, i, j, k)
                if x != 0:
                    print( "[", cartesian_coords[i], cartesian_coords[j], cartesian_coords[k], "]", x )
                
                
def test_schwarzchild_metric_christoffels():
    for i in range(4):
        for j in range(4):
            for k in range(4):
                x = calculateChristoffel(schwarzchild_metric, spherical_coords, i, j, k)
                if x != 0:
                    print( "[", spherical_coords[i], spherical_coords[j], spherical_coords[k], "]", x )


