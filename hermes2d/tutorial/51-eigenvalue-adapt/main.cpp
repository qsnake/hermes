#define H2D_REPORT_INFO
#include "hermes2d.h"
#include <stdio.h>

using namespace RefinementSelectors;

//  This example uses automatic adaptivity to solve the eigenproblem for the 
//  Laplace operator in a square with zero boundary conditions. Python and 
//  Pysparse must be installed. 
//
//  PDE: -Laplace u = lambda_k u,
//  where lambda_0, lambda_1, ... are the eigenvalues.
//
//  Domain: Square (0, pi)^2.
//
//  BC:  Homogeneous Dirichlet.
//
//  The following parameters can be changed:

const int NUMBER_OF_EIGENVALUES = 6;              // Desired number of eigenvalues.
int P_INIT = 2;                                   // Uniform polynomial degree of mesh elements.
const int INIT_REF_NUM = 2;                       // Number of initial mesh refinements.
double TARGET_VALUE = 2.0;                        // PySparse parameter: Eigenvalues in the vicinity of 
                                                  // this number will be computed. 
double TOL = 1e-10;                               // Pysparse parameter: Error tolerance.
int MAX_ITER = 1000;                              // PySparse parameter: Maximum number of iterations.
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
const CandList CAND_LIST = H2D_HP_ANISO_H;        // Predefined list of element refinement candidates. Possible values are
                                                  // H2D_P_ISO, H2D_P_ANISO, H2D_H_ISO, H2D_H_ANISO, H2D_HP_ISO,
                                                  // H2D_HP_ANISO_H, H2D_HP_ANISO_P, H2D_HP_ANISO.
                                                  // See User Documentation for details.
const int MESH_REGULARITY = -1;                   // Maximum allowed level of hanging nodes:
                                                  // MESH_REGULARITY = -1 ... arbitrary level hangning nodes (default),
                                                  // MESH_REGULARITY = 1 ... at most one-level hanging nodes,
                                                  // MESH_REGULARITY = 2 ... at most two-level hanging nodes, etc.
                                                  // Note that regular meshes are not supported, this is due to
                                                  // their notoriously bad performance.
const double CONV_EXP = 0.5;                      // Default value is 1.0. This parameter influences the selection of
                                                  // cancidates in hp-adaptivity. See get_optimal_refinement() for details.
const double ERR_STOP = 0.001;                    // Stopping criterion for adaptivity (rel. error tolerance between the
                                                  // reference mesh and coarse mesh solution in percent).
const int NDOF_STOP = 100000;                     // Adaptivity process stops when the number of degrees of freedom grows
                                                  // over this limit. This is to prevent h-adaptivity to go on forever.
MatrixSolverType matrix_solver = SOLVER_UMFPACK;  // Possibilities: SOLVER_AMESOS, SOLVER_MUMPS, 
                                                  // SOLVER_PARDISO, SOLVER_PETSC, SOLVER_UMFPACK

// Boundary condition types.
// Note: "essential" means that solution value is prescribed.
BCType bc_types(int marker)
{
  return BC_ESSENTIAL;
}

// Essential (Dirichlet) boundary condition values.
scalar essential_bc_values(int ess_bdy_marker, double x, double y)
{
  return 0;
}

// Weak forms.
#include "forms.cpp"

// Write the matrix in Matrix Market format.
void write_matrix_mm(const char* filename, Matrix* mat) 
{
  int ndof = mat->get_size();
  FILE *out = fopen(filename, "w" );
  int nz=0;
  for (int i=0; i < ndof; i++) {
    for (int j=0; j <=i; j++) { 
      double tmp = mat->get(i,j);
      if (fabs(tmp) > 1e-15) nz++;
    }
  } 

  fprintf(out,"%%%%MatrixMarket matrix coordinate real symmetric\n");
  fprintf(out,"%d %d %d\n", ndof, ndof, nz);
  for (int i=0; i < ndof; i++) {
    for (int j=0; j <=i; j++) { 
      double tmp = mat->get(i,j);
      if (fabs(tmp) > 1e-15) fprintf(out, "%d %d %24.15e\n", i+1, j+1, tmp);
    }
  } 
  fclose(out);
}

int main(int argc, char* argv[])
{
  info("Desired number of eigenvalues: %d.", NUMBER_OF_EIGENVALUES);

  // Load the mesh.
  Mesh mesh;
  H2DReader mloader;
  mloader.load("domain.mesh", &mesh);

  // Perform initial mesh refinements (optional).
  for (int i = 0; i < INIT_REF_NUM; i++) mesh.refine_all_elements();

  // Create an H1 space with default shapeset.
  H1Space space(&mesh, bc_types, essential_bc_values, P_INIT);

  // Initialize the weak formulation for the left hand side i.e. H 
  WeakForm wf_left, wf_right;
  wf_left.add_matrix_form(callback(bilinear_form_left));
  wf_right.add_matrix_form(callback(bilinear_form_right));

  // Initialize refinement selector.
  H1ProjBasedSelector selector(CAND_LIST, CONV_EXP, H2DRS_DEFAULT_ORDER);

  // Initialize views.
  ScalarView sview("Solution", new WinGeom(0, 0, 450, 350));
  sview.show_mesh(false);
  sview.fix_scale_width(60);
  OrderView  oview("Polynomial orders", new WinGeom(460, 0, 410, 350));

  // DOF and CPU convergence graphs.
  SimpleGraph graph_dof_est, graph_cpu_est;

  // Time measurement.
  TimePeriod cpu_time;
  cpu_time.tick();

  // Adaptivity loop:
  int as = 1;
  bool done = false;
  do
  {
    info("---- Adaptivity step %d:", as);
    info("Solving on reference mesh.");

    // Construct globally refined reference mesh and setup reference space.
    Space* ref_space = construct_refined_space(&space);
    int ref_ndof = Space::get_num_dofs(ref_space);
    info("ref_ndof: %d.", ref_ndof);

    // Initialize matrices and matrix solver on referenc emesh.
    SparseMatrix* matrix_left = create_matrix(matrix_solver);
    SparseMatrix* matrix_right = create_matrix(matrix_solver);
    Vector* eivec = create_vector(matrix_solver);
    Solver* solver = create_linear_solver(matrix_solver, matrix_left, eivec);

    // Assemble the matrices on reference mesh.
    bool is_linear = true;
    DiscreteProblem* dp_left = new DiscreteProblem(&wf_left, ref_space, is_linear);
    dp_left->assemble(matrix_left, eivec);
    DiscreteProblem* dp_right = new DiscreteProblem(&wf_right, ref_space, is_linear);
    dp_right->assemble(matrix_right, eivec);

    // Time measurement.
    cpu_time.tick();

    // Write matrix_left in MatrixMarket format.
    write_matrix_mm("mat_left.mtx", matrix_left);

    // Write matrix_left in MatrixMarket format.
    write_matrix_mm("mat_right.mtx", matrix_right);

    // Time measurement.
    cpu_time.tick(HERMES_SKIP);

    // Calling Python eigensolver. Solution will be written to "eivecs.dat".
    char call_cmd[255];
    sprintf(call_cmd, "python solveGenEigenFromMtx.py mat_left.mtx mat_right.mtx %g %d %g %d", 
	    TARGET_VALUE, NUMBER_OF_EIGENVALUES, TOL, MAX_ITER);
    system(call_cmd);

    // Initializing solution vector, solution and ScalarView.
    double* ref_coeff_vec = new double[ref_ndof];
    Solution sln[NUMBER_OF_EIGENVALUES], ref_sln[NUMBER_OF_EIGENVALUES];
    ScalarView view("Solution", new WinGeom(0, 0, 440, 350));

    // Reading solution vectors from file and visualizing.
    FILE *file = fopen("eivecs.dat", "r");
    char line [64];                  // Maximum line size.
    fgets(line, sizeof line, file);  // ref_ndof
    int n = atoi(line);            
    if (n != ref_ndof) error("Mismatched ndof in the eigensolver output file.");  
    fgets(line, sizeof line, file);  // Number of eigenvectors in the file.
    int neig = atoi(line);
    if (neig != NUMBER_OF_EIGENVALUES) error("Mismatched number of eigenvectors in the eigensolver output file.");  
    for (int ieig = 0; ieig < NUMBER_OF_EIGENVALUES; ieig++) {
      // Get next eigenvector from the file.
      for (int i = 0; i < ref_ndof; i++) {  
        fgets(line, sizeof line, file);
        ref_coeff_vec[i] = atof(line);
      }

      // Convert coefficient vector into a Solution.
      Solution::vector_to_solution(ref_coeff_vec, ref_space, &(ref_sln[ieig]));

      // Project the fine mesh solution onto the coarse mesh.
      info("Projecting reference solution %d on coarse mesh.", ieig);
      OGProjection::project_global(&space, &(ref_sln[ieig]), &(sln[ieig]), matrix_solver);
    }  
    fclose(file);
    delete [] ref_coeff_vec;

    // FIXME: Below, the adaptivity is done for the last eigenvector only,
    // this needs to be changed to take into account all eigenvectors.

    // View the coarse mesh solution and polynomial orders.
    sview.show(&(sln[NUMBER_OF_EIGENVALUES-1]));
    oview.show(&space);

    // Calculate element errors and total error estimate.
    info("Calculating error estimate.");
    Adapt* adaptivity = new Adapt(Tuple<Space *>(&space, &space, &space, &space, &space, &space), 
      Tuple<ProjNormType>(HERMES_H1_NORM, HERMES_H1_NORM, HERMES_H1_NORM, HERMES_H1_NORM, HERMES_H1_NORM, HERMES_H1_NORM));
    bool solutions_for_adapt = true;
    Tuple<Solution *> slns(&(sln[0]), &(sln[1]), &(sln[2]), &(sln[3]), &(sln[4]), &(sln[5]));
    Tuple<Solution *> ref_slns(&(ref_sln[0]), &(ref_sln[1]), &(ref_sln[2]), &(ref_sln[3]), &(ref_sln[4]), &(ref_sln[5]));
    double err_est_rel = adaptivity->calc_err_est(slns, ref_slns, solutions_for_adapt, 
                         HERMES_TOTAL_ERROR_REL | HERMES_ELEMENT_ERROR_REL) * 100;

    // Report results.
    info("ndof_coarse: %d, ndof_fine: %d, err_est_rel: %g%%", 
         Space::get_num_dofs(&space), Space::get_num_dofs(ref_space), err_est_rel);

    // Time measurement.
    cpu_time.tick();

    // Add entry to DOF and CPU convergence graphs.
    graph_dof_est.add_values(Space::get_num_dofs(&space), err_est_rel);
    graph_dof_est.save("conv_dof_est.dat");
    graph_cpu_est.add_values(cpu_time.accumulated(), err_est_rel);
    graph_cpu_est.save("conv_cpu_est.dat");

    // If err_est too large, adapt the mesh.
    if (err_est_rel < ERR_STOP) done = true;
    else
    {
      info("Adapting coarse mesh.");
      done = adaptivity->adapt(Tuple<RefinementSelectors::Selector *>(&selector, &selector, &selector, &selector, &selector, &selector), THRESHOLD, STRATEGY, MESH_REGULARITY);

      // Increase the counter of performed adaptivity steps.
      if (done == false)  as++;
    }
    if (Space::get_num_dofs(&space) >= NDOF_STOP) done = true;

    // Clean up.
    delete solver;
    delete matrix_left;
    delete matrix_right;
    delete eivec;
    delete adaptivity;
    if(done == false) delete ref_space->get_mesh();
    delete ref_space;
    delete dp_left;
    delete dp_right;
  }
  while (done == false);

  // Wait for all views to be closed.
  View::wait();
  return 0;
};
