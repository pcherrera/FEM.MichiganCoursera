/*This is a template file for use with 1D finite elements.
  The portions of the code you need to fill in are marked with the comment "//EDIT".

  Do not change the name of any existing functions, but feel free
  to create additional functions, variables, and constants.
  It uses the deal.II FEM library.*/

//Include files
//Data structures and solvers
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/base/function.h>
#include <deal.II/base/logstream.h>
#include <deal.II/base/tensor_function.h>
#include <deal.II/lac/vector.h>
#include <deal.II/lac/full_matrix.h>
#include <deal.II/lac/sparse_matrix.h>
#include <deal.II/lac/sparse_direct.h>
#include <deal.II/numerics/vector_tools.h>
#include <deal.II/numerics/matrix_tools.h>
#include <deal.II/numerics/data_out.h>
//Mesh related classes
#include <deal.II/grid/tria.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>
//#include <deal.II/grid/tria_boundary_lib.h>
#include <deal.II/grid/grid_tools.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/dofs/dof_tools.h>
//Finite element implementation classes
#include <deal.II/fe/fe_system.h>
#include <deal.II/fe/fe_values.h>
#include <deal.II/fe/fe_q.h>
//Standard C++ libraries
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <math.h>

// The EDO is 
// (EAu_x)_x + fA = 0 
// If E,A are constant the EDO results
// u_xx+(f/E) = 0 

using namespace dealii;

template <int dim>
class FEM
{
 public:
  //Class functions
  FEM (unsigned int order,unsigned int problem); // Class constructor 
  ~FEM(); //Class destructor

  //Function to find the value of xi at the given node (using deal.II node numbering)
  double xi_at_node(unsigned int dealNode);

  //Define your 1D basis functions and derivatives
  double basis_function(unsigned int node, double xi);
  double basis_gradient(unsigned int node, double xi);

  //Solution steps
  void generate_mesh(unsigned int numberOfElements);
  void define_boundary_conds();
  void setup_system();
  void assemble_system();
  void solve();
  void output_results();

  //Function to calculate the l2 norm of the error in the finite element sol'n vs. the exact solution
  double l2norm_of_error();

  //Class objects
  Triangulation<dim>   triangulation; //mesh
  FESystem<dim>        fe; 	      //FE element
  DoFHandler<dim>      dof_handler;   //Connectivity matrices

  //Gaussian quadrature - These will be defined in setup_system()
  unsigned int	        quadRule;    //quadrature rule, i.e. number of quadrature points
  std::vector<double>	quad_points; //vector of Gauss quadrature points
  std::vector<double>	quad_weight; //vector of the quadrature point weights
    
  //Data structures
  SparsityPattern       sparsity_pattern; //Sparse matrix pattern
  SparseMatrix<double>  K;		 //Global stiffness (sparse) matrix
  Vector<double>        D, F; 		 //Global vectors - Solution vector (D) and Global force vector (F)
  std::vector<double>   nodeLocation;	 //Vector of the x-coordinate of nodes by global dof number
  std::map<unsigned int,double> boundary_values;	//Map of dirichlet boundary conditions
  double                basisFunctionOrder, prob, L, g1, g2, E, f_bar, h,A;    

  //solution name array
  std::vector<std::string> nodal_solution_names;
  std::vector<DataComponentInterpretation::DataComponentInterpretation> nodal_data_component_interpretation;
};

// Class constructor for a vector field
template <int dim>
FEM<dim>::FEM(unsigned int order,unsigned int problem)
:
fe (FE_Q<dim>(QIterated<1>(QTrapez<1>(),order)), dim), 
  dof_handler (triangulation)
{
  basisFunctionOrder = order;
  if(problem == 1 || problem == 2){
    prob = problem;
  }
  else{
    std::cout << "Error: problem number should be 1 or 2.\n";
    exit(0);
  }

  //Nodal Solution names - this is for writing the output file
  for (unsigned int i=0; i<dim; ++i){
    nodal_solution_names.push_back("u");
    nodal_data_component_interpretation.push_back(DataComponentInterpretation::component_is_part_of_vector);
  }
}

//Class destructor
template <int dim>
FEM<dim>::~FEM(){
  dof_handler.clear();
}

//Find the value of xi at the given node (using deal.II node numbering)
template <int dim>
double FEM<dim>::xi_at_node(unsigned int dealNode){
  double xi;

  if(dealNode == 0){
    xi = -1.;
  }
  else if(dealNode == 1){
    xi = 1.;
  }
  else if(dealNode <= basisFunctionOrder){
    xi = -1. + 2.*(dealNode-1.)/basisFunctionOrder;
  }
  else{
    std::cout << "Error: you input node number "
	      << dealNode << " but there are only " 
	      << basisFunctionOrder + 1 << " nodes in an element.\n";
    exit(0);
  }

  return xi;
}

//Define basis functions
template <int dim>
double FEM<dim>::basis_function(unsigned int node, double xi){
  /*"basisFunctionOrder" defines the polynomial order of the basis function,
    "node" specifies which node the basis function corresponds to, 
    "xi" is the point (in the bi-unit domain) where the function is being evaluated.
    You need to calculate the value of the specified basis function and order at the given quadrature pt.*/

  double value = 1.; //Store the value of the basis function in this variable

  /*You can use the function "xi_at_node" (defined above) to get the value of xi (in the bi-unit domain)
    at any node in the element - using deal.II's element node numbering pattern.*/

  //EDIT
    for ( unsigned int i = 0; i <= basisFunctionOrder ; i ++)
    {
        if(i!=node) value = value * (xi - xi_at_node(i)) / (xi_at_node(node) - xi_at_node(i));
    }
  

  return value;
}

//Define basis function gradient
template <int dim>
double FEM<dim>::basis_gradient(unsigned int node, double xi){
  /*"basisFunctionOrder" defines the polynomial order of the basis function,
    "node" specifies which node the basis function corresponds to, 
    "xi" is the point (in the bi-unit domain) where the function is being evaluated.
    You need to calculate the value of the derivative of the specified basis function and order at the given quadrature pt.
    Note that this is the derivative with respect to xi (not x)*/

  double value = 0.; //Store the value of the gradient of the basis function in this variable

  /*You can use the function "xi_at_node" (defined above) to get the value of xi (in the bi-unit domain)
    at any node in the element - using deal.II's element node numbering pattern.*/

  //EDIT
  
    if (basisFunctionOrder == 1)
  {
      if (node==0) value = -1./2.;
      else
      {
          assert(node==1);
          value = 1. / 2.;
      }
  }

  double xi0, xi1, xi2;
  if (basisFunctionOrder == 2)
  {
      xi0 = xi_at_node(node);
      if (node==0)
      {
          xi1=xi_at_node(1);
          xi2=xi_at_node(2);
      }
      if (node==1)
      {
          xi1=xi_at_node(0);
          xi2=xi_at_node(2);
      }

      if (node==2)
      {
          xi1=xi_at_node(0);
          xi2=xi_at_node(1);
      }

      double denum= (xi0-xi1)*(xi0-xi2);
      value = (xi-xi2)/denum + (xi-xi1)/denum;
  }

  double xi3;
  if (basisFunctionOrder == 3)
  {
      xi0 = xi_at_node(node);
      if (node==0)
      {
          xi1=xi_at_node(1);
          xi2=xi_at_node(2);
          xi3=xi_at_node(3);
      }
      if (node==1)
      {
          xi1=xi_at_node(0);
          xi2=xi_at_node(2);
          xi3=xi_at_node(3);
      }

      if (node==2)
      {
          xi1=xi_at_node(0);
          xi2=xi_at_node(1);
          xi3=xi_at_node(3);
      }

      if (node==3)
      {
          xi1=xi_at_node(0);
          xi2=xi_at_node(1);
          xi3=xi_at_node(2);
      }

      double denum= (xi0-xi1)*(xi0-xi2) * (xi0-xi3);
      value = (xi-xi2)*(xi-xi3)/denum + (xi-xi1)*(xi-xi3)/denum+(xi-xi1)*(xi-xi2)/denum;
  }
  
  return value;
}

//Define the problem domain and generate the mesh
template <int dim>
void FEM<dim>::generate_mesh(unsigned int numberOfElements){

  //Define the limits of your domain
  L = 0.1; //EDIT
  double x_min = 0.;
  double x_max = L;

  Point<dim,double> min(x_min),
    max(x_max);
  std::vector<unsigned int> meshDimensions (dim,numberOfElements);
  GridGenerator::subdivided_hyper_rectangle (triangulation, meshDimensions, min, max);
}

//Specify the Dirichlet boundary conditions
template <int dim>
void FEM<dim>::define_boundary_conds(){
  const unsigned int totalNodes = dof_handler.n_dofs(); //Total number of nodes

  //Identify dirichlet boundary nodes and specify their values.
  //This function is called from within "setup_system"

  /*The vector "nodeLocation" gives the x-coordinate in the real domain of each node,
    organized by the global node number.*/

  /*This loops through all nodes in the system and checks to see if they are
    at one of the boundaries. If at a Dirichlet boundary, it stores the node number
    and the applied displacement value in the std::map "boundary_values". Deal.II
    will use this information later to apply Dirichlet boundary conditions.
    Neumann boundary conditions are applied when constructing Flocal in "assembly"*/
  for(unsigned int globalNode=0; globalNode<totalNodes; globalNode++){
    if(nodeLocation[globalNode] == 0){
      boundary_values[globalNode] = g1;
    }
    if(nodeLocation[globalNode] == L){
      if(prob == 1){
	boundary_values[globalNode] = g2;
      }
    }
  }
			
}

//Setup data structures (sparse matrix, vectors)
template <int dim>
void FEM<dim>::setup_system(){

  //Define constants for problem (Dirichlet boundary values)
  g1 = 0; g2 =1.e-3 ; f_bar = 1.e11; E =1.e11, A=1.e-4; h=1.e6; //EDIT

  //Let deal.II organize degrees of freedom
  dof_handler.distribute_dofs (fe);

  //Enter the global node x-coordinates into the vector "nodeLocation"
  MappingQ1<dim,dim> mapping;
  std::vector< Point<dim,double> > dof_coords(dof_handler.n_dofs());
  nodeLocation.resize(dof_handler.n_dofs());
  DoFTools::map_dofs_to_support_points<dim,dim>(mapping,dof_handler,dof_coords);
  for(unsigned int i=0; i<dof_coords.size(); i++){
    nodeLocation[i] = dof_coords[i][0];
  }

  //Specify boundary condtions (call the function)
  define_boundary_conds();

  //Define the size of the global matrices and vectors
  sparsity_pattern.reinit (dof_handler.n_dofs(), dof_handler.n_dofs(),
			   dof_handler.max_couplings_between_dofs());
  DoFTools::make_sparsity_pattern (dof_handler, sparsity_pattern);
  sparsity_pattern.compress();
  K.reinit (sparsity_pattern);
  F.reinit (dof_handler.n_dofs());
  D.reinit (dof_handler.n_dofs());

  //Define quadrature rule
  /*A quad rule of 2 is included here as an example. You will need to decide
    what quadrature rule is needed for the given problems*/
  //quadRule = 2; //EDIT - Number of quadrature points along one dimension
  //quad_points.resize(quadRule); quad_weight.resize(quadRule);

  //quad_points[0] = -sqrt(1./3.); //EDIT
  //quad_points[1] = sqrt(1./3.); //EDIT

  //quad_weight[0] = 1.; //EDIT
  //quad_weight[1] = 1.; //EDIT


  //quadRule = 3; //- Number of quadrature points along one dimension
  //quad_points.resize(quadRule); quad_weight.resize(quadRule);

  //quad_points[0] = 0.; 
  //quad_points[1] = -sqrt(3./5.); 
  //quad_points[2] = sqrt(3./5.); 

  //quad_weight[0] = 8./9.; 
  //quad_weight[1] = 5./9.; 
  //quad_weight[2] = 5./9.; 
  
  //quadRule = 4;
  //quad_points.resize(quadRule);
  //quad_weight.resize(quadRule);
  //quad_points[0]= 0.339981;
  //quad_points[1]= -0.339981;
  //quad_points[2]= 0.861136;
  //quad_points[3]= -0.861136;

  //quad_weight[0]=0.652145;
  //quad_weight[1]=0.652145;
  //quad_weight[2]=0.347855;
  //quad_weight[3]=0.347855;
  
  quadRule=5;
  quad_points.resize(quadRule);
  quad_weight.resize(quadRule);

  quad_weight[0]= 128./225.;
  quad_weight[1]= (322.+13.*sqrt(70.))/900.;
  quad_weight[2]= (322.+13.*sqrt(70.))/900.;
  quad_weight[3]= (322.-13.*sqrt(70.))/900.;
  quad_weight[4]= (322.-13.*sqrt(70.))/900.;

  quad_points[0]=0.;
  quad_points[1]= +1./3. * sqrt(5.-2*sqrt(10./7.));
  quad_points[2]= -1./3. * sqrt(5.-2*sqrt(10./7.));
  quad_points[3]= +1./3. * sqrt(5.+2*sqrt(10./7.));
  quad_points[4]= -1./3. * sqrt(5.+2*sqrt(10./7.));
  
  //Just some notes...
  std::cout << "   Number of active elems:       " << triangulation.n_active_cells() << std::endl;
  std::cout << "   Number of degrees of freedom: " << dof_handler.n_dofs() << std::endl;   
}

//Form elmental vectors and matrices and assemble to the global vector (F) and matrix (K)
template <int dim>
void FEM<dim>::assemble_system(){

  K=0; F=0;

  const unsigned int   			dofs_per_elem = fe.dofs_per_cell; //This gives you number of degrees of freedom per element
  FullMatrix<double> 				Klocal (dofs_per_elem, dofs_per_elem);
  Vector<double>      			Flocal (dofs_per_elem);
  std::vector<unsigned int> local_dof_indices (dofs_per_elem);
  double										h_e, x, f;

  //loop over elements  
  typename DoFHandler<dim>::active_cell_iterator elem = dof_handler.begin_active(), 
    endc = dof_handler.end();
  for (;elem!=endc; ++elem){

    /*Retrieve the effective "connectivity matrix" for this element
      "local_dof_indices" relates local dofs to global dofs,
      i.e. local_dof_indices[i] gives the global dof number for local dof i.*/
    elem->get_dof_indices (local_dof_indices);

    /*We find the element length by subtracting the x-coordinates of the two end nodes
      of the element. Remember that the vector "nodeLocation" holds the x-coordinates, indexed
      by the global node number. "local_dof_indices" gives us the global node number indexed by
      the element node number.*/
    h_e = nodeLocation[local_dof_indices[1]] - nodeLocation[local_dof_indices[0]];

    //Loop over local DOFs and quadrature points to populate Flocal and Klocal.
    Flocal = 0.;
    for(unsigned int A=0; A<dofs_per_elem; A++){
      for(unsigned int q=0; q<quadRule; q++){
	x = 0;
	//Interpolate the x-coordinates at the nodes to find the x-coordinate at the quad pt.
	for(unsigned int B=0; B<dofs_per_elem; B++){
	  x += nodeLocation[local_dof_indices[B]]*basis_function(B,quad_points[q]);
	}
	//EDIT - Define Flocal
	f=(f_bar/E)*x;
	Flocal[A] += h_e/2.* basis_function(A,quad_points[q]) * quad_weight[q]  *f;
      }
    }
    //Add nonzero Neumann condition, if applicable
    if(prob == 2){ 
      if(nodeLocation[local_dof_indices[1]] == L){
	//EDIT - Modify Flocal to include the traction on the right boundary.
	Flocal[1]+=h/(E*A);
      }
    }

    //Loop over local DOFs and quadrature points to populate Klocal
    Klocal = 0;
    for(unsigned int A=0; A<dofs_per_elem; A++){
      for(unsigned int B=0; B<dofs_per_elem; B++){
	for(unsigned int q=0; q<quadRule; q++){
	  //EDIT - Define Klocal.
	  Klocal[A][B] += 2. / h_e *  basis_gradient(A,quad_points[q]) * basis_gradient(B,quad_points[q]) * quad_weight[q];
	}
      }
    }

    //Assemble local K and F into global K and F
    //You will need to used local_dof_indices[A]
    for(unsigned int A=0; A<dofs_per_elem; A++){
      //EDIT - add component A of Flocal to the correct location in F
      /*Remember, local_dof_indices[A] is the global degree-of-freedom number
	corresponding to element node number A*/
	F[local_dof_indices[A]] += Flocal[A];
      for(unsigned int B=0; B<dofs_per_elem; B++){
	//EDIT - add component A,B of Klocal to the correct location in K (using local_dof_indices)
	/*Note: K is a sparse matrix, so you need to use the function "add".
	  For example, to add the variable C to K[i][j], you would use:
	  K.add(i,j,C);*/
	 K.add(local_dof_indices[A],local_dof_indices[B],Klocal[A][B]);	  
      }
    }

  }

  //Apply Dirichlet boundary conditions
  /*deal.II applies Dirichlet boundary conditions (using the boundary_values map we
    defined in the function "define_boundary_conds") without resizing K or F*/
  MatrixTools::apply_boundary_values (boundary_values, K, D, F, false);
}

//Solve for D in KD=F
template <int dim>
void FEM<dim>::solve(){

  //Solve for D
  SparseDirectUMFPACK  A;
  A.initialize(K);
  A.vmult (D, F); //D=K^{-1}*F

}

//Output results
template <int dim>
void FEM<dim>::output_results (){

  //Write results to VTK file
  std::ofstream output1("solution.vtk");
  DataOut<dim> data_out;
  data_out.attach_dof_handler(dof_handler);

  //Add nodal DOF data
  data_out.add_data_vector(D, nodal_solution_names, DataOut<dim>::type_dof_data,
			   nodal_data_component_interpretation);
  data_out.build_patches();
  data_out.write_vtk(output1);
  output1.close();
}

template <int dim>
double FEM<dim>::l2norm_of_error(){
	
  double l2norm = 0.;

  //Find the l2 norm of the error between the finite element sol'n and the exact sol'n
  const unsigned int   			dofs_per_elem = fe.dofs_per_cell; //This gives you dofs per element
  std::vector<unsigned int> local_dof_indices (dofs_per_elem);
  double u_exact, u_h, x, h_e;

  //loop over elements  
  typename DoFHandler<dim>::active_cell_iterator elem = dof_handler.begin_active (), 
    endc = dof_handler.end();
  for (;elem!=endc; ++elem){

    //Retrieve the effective "connectivity matrix" for this element
    elem->get_dof_indices (local_dof_indices);

    //Find the element length
    h_e = nodeLocation[local_dof_indices[1]] - nodeLocation[local_dof_indices[0]];

    for(unsigned int q=0; q<quadRule; q++){
      x = 0.; u_h = 0.;
      //Find the values of x and u_h (the finite element solution) at the quadrature points
      for(unsigned int B=0; B<dofs_per_elem; B++){
	x += nodeLocation[local_dof_indices[B]]*basis_function(B,quad_points[q]);
	u_h += D[local_dof_indices[B]]*basis_function(B,quad_points[q]);
      }
      //EDIT - Find the l2-norm of the error through numerical integration.
      /*This includes evaluating the exact solution at the quadrature points*/
	if (prob==1){
		u_exact = -pow(x,3)/6. + (g2-g1+pow(L,3)/6.)/L *x + g1;
		}
	if (prob==2){
		u_exact = -pow(x,3)/6. + ((h/(E*A))+pow(L,2)/2)*x + g1;
		}	
	l2norm += (u_h-u_exact)*(u_h-u_exact) * h_e / 2. * quad_weight[q];					
    }
  }

  return sqrt(l2norm);
}
