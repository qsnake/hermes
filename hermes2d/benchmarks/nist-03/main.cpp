#define HERMES_REPORT_ALL
#define HERMES_REPORT_FILE "application.log"
#include "hermes2d.h"

using namespace RefinementSelectors;


//  This is the third in the series of NIST benchmarks with known exact solutions.
//
//  Reference: W. Mitchell, A Collection of 2D Elliptic Problems for Testing Adaptive Algorithms, 
//                          NIST Report 7668, February 2010.
//
//  PDE: Linear elasticity coupled system of two equations given below
//
//  -E \frac{1-nu^2}{1-2*nu} \frac{\partial^{2} u}{\partial x^{2}} - E\frac{1-nu^2}{2-2*nu} \frac{\partial^{2} u}{\partial y^{2}} 
//  -E \frac{1-nu^2}{(1-2*nu)(2-2*nu)} \frac{\partial^{2} v}{\partial x \partial y} = F_{x}
//
//  -E \frac{1-nu^2}{2-2*nu} \frac{\partial^{2} v}{\partial x^{2}} - E\frac{1-nu^2}{1-2*nu} \frac{\partial^{2} v}{\partial y^{2}} 
//  -E \frac{1-nu^2}{(1-2*nu)(2-2*nu)} \frac{\partial^{2} u}{\partial x \partial y} = F_{y}
//
//  where F_{x} = F_{y} = 0.
//
//  Known exact solution for mode 1: 
//  u(x, y) = \frac{1}{2G} r^{\lambda}[(k - Q(\lambda + 1))cos(\lambda \theta) - \lambda cos((\lambda - 2) \theta)]
//  v(x, y) = \frac{1}{2G} r^{\lambda}[(k + Q(\lambda + 1))sin(\lambda \theta) + \lambda sin((\lambda - 2) \theta)]
//  here \lambda = 0.5444837367825, Q = 0.5430755788367.
//
//  Known exact solution for mode 2: 
//  u(x, y) =  \frac{1}{2G} r^{\lambda}[(k - Q(\lambda + 1))sin(\lambda \theta) - \lambda sin((\lambda - 2) \theta)]
//  v(x, y) = -\frac{1}{2G} r^{\lambda}[(k + Q(\lambda + 1))cos(\lambda \theta) + \lambda cos((\lambda - 2) \theta)]
//  here \lambda = 0.9085291898461, Q = -0.2189232362488.
//
//  See functions fn() and fndd() in "exact_solution.cpp".
//
//  Domain: square (-1, 1)^2, with a slit from (0, 0) to (1, 0).
//
//  BC:  Dirichlet, given by exact solution.
//
//  The following parameters can be changed:

const int P_INIT_U = 2;                           // Initial polynomial degree for u.
const int P_INIT_V = 2;                           // Initial polynomial degree for v.
const int INIT_REF_NUM = 1;                       // Number of initial uniform mesh refinements.
const double THRESHOLD = 0.3;                     // This is a quantitative parameter of the adapt(...) function and
                                                  // it has different meanings for various adaptive strategies (see below).
const int STRATEGY = 0;                           // Adaptive strategy:
                                                  // STRATEGY = 0 ... refine elements until sqrt(THRESHOLD) times total
                                                  //   error is processed. If more elements have similar errors, refine
                                                  //   all to keep the mesh symmetric.
                                                  // STRATEGY = 1 ... refine all elements whose error is larger
                                                  //   than THRESHOLD times maximum element error.
                                                  // STRATEGY = 2 ... refine all elements whose error is larger
                                                  //   than THRESHOLD.
                                                  // More adaptive strategies can be created in adapt_ortho_h1.cpp.
const CandList CAND_LIST = H2D_HP_ANISO;          // Predefined list of element refinement candidates. Possible values are
                                                  // H2D_P_ISO, H2D_P_ANISO, H2D_H_ISO, H2D_H_ANISO, H2D_HP_ISO,
                                                  // H2D_HP_ANISO_H, H2D_HP_ANISO_P, H2D_HP_ANISO.
                                                  // See the User Documentation for details.
const int MESH_REGULARITY = -1;                   // Maximum allowed level of hanging nodes:
                                                  // MESH_REGULARITY = -1 ... arbitrary level hangning nodes (default),
                                                  // MESH_REGULARITY = 1 ... at most one-level hanging nodes,
                                                  // MESH_REGULARITY = 2 ... at most two-level hanging nodes, etc.
                                                  // Note that regular meshes are not supported, this is due to
                                                  // their notoriously bad performance.
const double CONV_EXP = 1;                        // Default value is 1.0. This parameter influences the selection of
                                                  // cancidates in hp-adaptivity. See get_optimal_refinement() for details.
const double ERR_STOP = 0.1;                      // Stopping criterion for adaptivity (rel. error tolerance between the
                                                  // fine mesh and coarse mesh solution in percent).
const int NDOF_STOP = 60000;                      // Adaptivity process stops when the number of degrees of freedom grows over
                                                  // this limit. This is mainly to prevent h-adaptivity to go on forever.
MatrixSolverType matrix_solver = SOLVER_UMFPACK;  // Possibilities: SOLVER_AMESOS, SOLVER_AZTECOO, SOLVER_MUMPS,
                                                  // SOLVER_PETSC, SOLVER_SUPERLU, SOLVER_UMFPACK.

// Problem parameters.
const double E = 1.0;                             // Young modulus.
const double nu = 0.3;                            // Poisson ratio.
const double k = 3.0 - 4.0 * nu;
const double G = E / (2.0 * (1.0 + nu));
const double lambda = 0.5444837367825;            // Mode 1.
const double Q = 0.5430755788367;

// TEMPORARY: if forms.cpp.bracket is used:
//const double lambda = (E * nu) / ((1 + nu) * (1 - 2*nu));
//const double mu = E / (2*(1 + nu));

// Boundary markers.
const int BDY_DIRICHLET = 1;

// Exact solution.
#include "exact_solution.cpp"

// Weak forms.
#include "forms.cpp"

// Essential (Dirichlet) boundary condition values.
scalar essential_bc_values_u(double x, double y) { return u_fn(x, y);}
scalar essential_bc_values_v(double x, double y) { return v_fn(x, y);}

int main(int argc, char* argv[])
{
  // Time measurement.
  TimePeriod cpu_time;
  cpu_time.tick();

  // Load the mesh.
  Mesh u_mesh, v_mesh;
  H2DReader mloader;
  mloader.load("elasticity.mesh", &u_mesh);

  // Create initial mesh (master mesh).
  v_mesh.copy(&u_mesh);

  // Perform initial mesh refinements.
  for (int i = 0; i < INIT_REF_NUM; i++) u_mesh.refine_all_elements();
  for (int i = 0; i < INIT_REF_NUM; i++) v_mesh.refine_all_elements();

  // Enter boundary markers.
  BCTypes bc_types;
  bc_types.add_bc_dirichlet(BDY_DIRICHLET);

  // Enter Dirichlet boundary values.
  BCValues bc_values_u, bc_values_v;
  bc_values_u.add_function(BDY_DIRICHLET, essential_bc_values_u);
  bc_values_v.add_function(BDY_DIRICHLET, essential_bc_values_v);

  // Create H1 spaces with default shapeset for both displacement components.
  H1Space u_space(&u_mesh, &bc_types, &bc_values_u, P_INIT_U);
  H1Space v_space(&v_mesh, &bc_types, &bc_values_v, P_INIT_V);

  // Initialize the weak formulation.
  WeakForm wf(2);
  wf.add_matrix_form(0, 0, callback(bilinear_form_0_0), HERMES_SYM);
  wf.add_matrix_form(0, 1, callback(bilinear_form_0_1), HERMES_SYM);
  wf.add_matrix_form(1, 1, callback(bilinear_form_1_1), HERMES_SYM);
  //wf.add_vector_form(0, linear_form_u, linear_form_0_ord, HERMES_SYM);
  //wf.add_vector_form(1, linear_form_v, linear_form_1_ord, HERMES_SYM);

  Solution u_sln, v_sln, u_ref_sln, v_ref_sln;

  // Initialize exact solutions.
  ExactSolution u_exact(&u_mesh, u_fndd);
  ExactSolution v_exact(&v_mesh, v_fndd);

  // Initialize refinement selector.
  H1ProjBasedSelector selector(CAND_LIST, CONV_EXP, H2DRS_DEFAULT_ORDER);

  // Initialize views.
  ScalarView s_view_u("Solution for u", new WinGeom(0, 0, 440, 350));
  s_view_u.show_mesh(false);
  OrderView  o_view_u("Mesh for u", new WinGeom(450, 0, 420, 350));

  ScalarView s_view_v("Solution for v", new WinGeom(880, 0, 440, 350));
  s_view_v.show_mesh(false);
  OrderView  o_view_v("Mesh for v", new WinGeom(1330, 0, 420, 350));

  /*  
  ScalarView sview_u_exact("", new WinGeom(550, 0, 500, 400));
  sview_u_exact.fix_scale_width(50);
  ScalarView sview_v_exact("", new WinGeom(550, 500, 500, 400));
  sview_v_exact.fix_scale_width(50);
  char title[100];
  */

  // DOF and CPU convergence graphs.
  SimpleGraph graph_dof_est, graph_cpu_est, graph_dof_exact, graph_cpu_exact;

  // Adaptivity loop:
  int as = 1; 
  bool done = false;
  do
  {
    info("---- Adaptivity step %d:", as);

    // Construct globally refined reference mesh and setup reference space.
    Hermes::vector<Space *>* ref_spaces = construct_refined_spaces(Hermes::vector<Space *>(&u_space, &v_space));

    // Initialize matrix solver.
    SparseMatrix* matrix = create_matrix(matrix_solver);
    Vector* rhs = create_vector(matrix_solver);
    Solver* solver = create_linear_solver(matrix_solver, matrix, rhs);
    solver->set_factorization_scheme(HERMES_REUSE_MATRIX_REORDERING);

    // Assemble the reference problem.
    info("Solving on reference mesh.");
    bool is_linear = true;
    DiscreteProblem* dp = new DiscreteProblem(&wf, *ref_spaces, is_linear);
    dp->assemble(matrix, rhs);

    // Time measurement.
    cpu_time.tick();

    // Solve the linear system of the reference problem. If successful, obtain the solution.
    if(solver->solve()) Solution::vector_to_solutions(solver->get_solution(), *ref_spaces,
                                                      Hermes::vector<Solution *>(&u_ref_sln, &v_ref_sln));
    else error ("Matrix solver failed.\n");

    // Time measurement.
    cpu_time.tick();

    // Project the fine mesh solution onto the coarse mesh.
    info("Projecting reference solution on coarse mesh.");
    OGProjection::project_global(Hermes::vector<Space *>(&u_space, &v_space), 
                                 Hermes::vector<Solution *>(&u_ref_sln, &v_ref_sln), 
                                 Hermes::vector<Solution *>(&u_sln, &v_sln), matrix_solver); 
   
    // View the coarse mesh solution and polynomial orders.
    s_view_u.show(&u_sln); 
    o_view_u.show(&u_space);
    s_view_v.show(&v_sln); 
    o_view_v.show(&v_space);

    /*
    // Exact solution for comparison with computational results.
    u_exact.update(&u_mesh, u_fndd);
    v_exact.update(&v_mesh, v_fndd);

    // Show exact solution.
    sview_u_exact.show(&u_exact);
    sprintf(title, "Exact solution for u.");
    sview_u_exact.set_title(title);
    
    sview_v_exact.show(&v_exact);
    sprintf(title, "Exact solution for v.");
    sview_v_exact.set_title(title);
    */

    // Calculate element errors.
    info("Calculating error estimate and exact error."); 
    Adapt* adaptivity = new Adapt(Hermes::vector<Space *>(&u_space, &v_space));
    
    // Calculate error estimate for each solution component and the total error estimate.
    Hermes::vector<double> err_est_rel;
    double err_est_rel_total = adaptivity->calc_err_est(Hermes::vector<Solution *>(&u_sln, &v_sln), 
                               Hermes::vector<Solution *>(&u_ref_sln, &v_ref_sln), &err_est_rel) * 100;

    // Calculate exact error for each solution component and the total exact error.
    Hermes::vector<double> err_exact_rel;
    bool solutions_for_adapt = false;
    double err_exact_rel_total = adaptivity->calc_err_exact(Hermes::vector<Solution *>(&u_sln, &v_sln), 
                                 Hermes::vector<Solution *>(&u_exact, &v_exact), 
                                 &err_exact_rel, solutions_for_adapt) * 100;

    // Time measurement.
    cpu_time.tick();

    // Report results.
    info("ndof_coarse[u]: %d, ndof_fine[u]: %d",
         u_space.Space::get_num_dofs(), Space::get_num_dofs((*ref_spaces)[0]));

    info("err_est_rel[u]: %g%%, err_exact_rel[u]: %g%%", err_est_rel[0]*100, err_exact_rel[0]*100);

    info("ndof_coarse[v]: %d, ndof_fine[v]: %d",
         v_space.Space::get_num_dofs(), Space::get_num_dofs((*ref_spaces)[1]));

    info("err_est_rel[v]: %g%%, err_exact_rel[v]: %g%%", err_est_rel[1]*100, err_exact_rel[1]*100);

    info("ndof_coarse_total: %d, ndof_fine_total: %d",
         Space::get_num_dofs(Hermes::vector<Space *>(&u_space, &v_space)), Space::get_num_dofs(*ref_spaces));

    info("err_est_rel_total: %g%%, err_est_exact_total: %g%%", err_est_rel_total, err_exact_rel_total);

    // Add entry to DOF and CPU convergence graphs.
    graph_dof_est.add_values(Space::get_num_dofs(Hermes::vector<Space *>(&u_space, &v_space)), err_est_rel_total);
    graph_dof_est.save("conv_dof_est.dat");
    graph_cpu_est.add_values(cpu_time.accumulated(), err_est_rel_total);
    graph_cpu_est.save("conv_cpu_est.dat");
    graph_dof_exact.add_values(Space::get_num_dofs(Hermes::vector<Space *>(&u_space, &v_space)), err_exact_rel_total);
    graph_dof_exact.save("conv_dof_exact.dat");
    graph_cpu_exact.add_values(cpu_time.accumulated(), err_exact_rel_total);
    graph_cpu_exact.save("conv_cpu_exact.dat");

    // If err_est too large, adapt the mesh.
    if (err_est_rel_total < ERR_STOP) 
      done = true;
    else 
    {
      info("Adapting coarse mesh.");
      done = adaptivity->adapt(Hermes::vector<RefinementSelectors::Selector *>(&selector, &selector), 
                               THRESHOLD, STRATEGY, MESH_REGULARITY);
    }
    if (Space::get_num_dofs(Hermes::vector<Space *>(&u_space, &v_space)) >= NDOF_STOP) done = true;

    // Clean up.
    delete solver;
    delete matrix;
    delete rhs;
    delete adaptivity;
    if(done == false)
      for(unsigned int i = 0; i < ref_spaces->size(); i++)
        delete (*ref_spaces)[i]->get_mesh();
    delete ref_spaces;
    
    // Increase counter.
    as++;
  }
  while (done == false);

  verbose("Total running time: %g s", cpu_time.accumulated());

  // Wait for all views to be closed.
  View::wait();
  return 0;
}
