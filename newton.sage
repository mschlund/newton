# Test inputs :)
var('x,y,z,a,b,c,d,e,f,g,h,i')

f1 = a*x*y + b
f2 = c*y*z + d*y*x + e
f3 = g*x*h + i

F = matrix([f1,f2,f3]).transpose()
# J = jacobian(F,[x,y,z])

# n = newton_fixpoint_solve(F, [x,y,z], 10)
# n.subs( a=0.5, b=0.3 )
# TODO (plan, long run):
# substitute either functions (p_(i,j)(n) = transition probabilities) or constants for some parameters
# and substitute the function f(x) = 1/(1-x) for s() to evaluate :) --> we get a rational expression in the free parameters
# Ultimately: Use it to approximate e.g. the expected value of the "runtime" (or some other "cost") as well as its variance (in terms of the input size n)
# Also:
# - Derive the equations from Flowgraphs, Remopla/PDS (which are derived from e.g. Java Bytecode)
# - More semirings (other analyses), integrate semirings, polynomials over them, and Newton's method nicely into Sage


# symbolic functions for the Kleene star of elements
# TODO: perhaps add an evaluation function for "s" (so that we have s(0) = 1 etc.)?
s = function('s', nargs=1)


def compute_mat_star(M) :
#def compute_mat_star(M) :
    # TODO: implement recursive computation of the Kleene star for matrices 
    var('a_11 a_12 a_21 a_22 A_11 A_12 A_21 A_22')
    a_11 = M[:M.nrows()-1,:M.ncols()-1]
    a_12 = M[:M.nrows()-1,M.ncols()-1]
    a_21 = M[M.nrows()-1,:M.ncols()-1]
    a_22 = M[M.nrows()-1,M.ncols()-1]
    A_11 = (a_11 + a_12 * s(a_22) * a_21)
    A_22 = (a_22 + a_21 * a_11 * a_12)
    A_12 = a_11 * a_12 * A_22
    A_21 = a_22 * a_21 * A_11
#    if M.ncols() == 2 & M.nrows() == 2:
    return matrix(SR,[[A_11,A_12],[A_21,A_22]])

# TODO ... unfold the system of equations into a linear system in variables X_[d],X_(d)
# and compute delta^(i) symbolically(!) in terms of variables X_[d-1] and X_(d-1)
def compute_symbolic_delta :



# given a vector of polynomials F in variables poly_vars, its Jacobian,
# a starting value v, and the "update" delta, compute the next iterand
# via v_new = v + J^*|v * delta
def newton_step(F, poly_vars, J, v, delta) :
    assert(len(poly_vars) == len(v))

    sub_dict = dict(zip(poly_vars,v))

    J_s = compute_mat_star(J)

    J_s = J_s.subs(sub_dict)
    v_new = v + J_s*delta

    return v_new



# TODO: iterate until convergence, but for at most max_iter iterations
# 1) compute the concrete delta (from the precomputed symbolic expression)
# 2) execute a newton_step

def newton_fixpoint_solve(F, poly_vars, max_iter=100) :
    J = jacobian(F, poly_vars)

    delta = compute_symbolic_delta # TODO
        

    # v^0 = F(0)
    v = F.subst( dict( (v,0) for v in poly_vars )
    
    # TODO: iteration..




