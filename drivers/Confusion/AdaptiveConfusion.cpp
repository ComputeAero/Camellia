#include "ConfusionManufacturedSolution.h"
#include "ConfusionBilinearForm.h"
#include "ConfusionProblem.h"
#include "MathInnerProduct.h"
#include "OptimalInnerProduct.h"
#include "Mesh.h"
#include "Solution.h"
#include "ZoltanMeshPartitionPolicy.h"
#include "Solver.h"
#include "SchwarzSolver.h"

// added by Jesse
#include "ConfusionInnerProduct.h"

// Trilinos includes
#include "Epetra_Time.h"
#include "Intrepid_FieldContainer.hpp"
#ifdef HAVE_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif

#ifdef HAVE_MPI
#include <Teuchos_GlobalMPISession.hpp>
#else
#endif

// Trilinos includes
#include "Intrepid_FieldContainer.hpp"

#ifdef HAVE_MPI
#include <Teuchos_GlobalMPISession.hpp>
#else
#endif

using namespace std;

int main(int argc, char *argv[])
{
#ifdef HAVE_MPI
  Teuchos::GlobalMPISession mpiSession(&argc, &argv,0);
  int rank=mpiSession.getRank();
  int numProcs=mpiSession.getNProc();
#else
  int rank = 0;
  int numProcs = 1;
#endif
  int polyOrder = 3;
  int pToAdd = 2; // for tests

  // define our manufactured solution or problem bilinear form:

  bool useSchwarz = false;
  double epsilon =  1e-2;
  double beta_x = 1.0, beta_y = 1.0;
  bool useTriangles = false;
  bool useEggerSchoeberl = false;
  bool usePatchBasis = false;
  bool enforceMBFluxContinuity = false;
  bool limitIrregularity = true;

  int numRefinements = 4;
  double thresholdFactor = 0.20;

  ConfusionManufacturedSolution exactSolution(epsilon,beta_x,beta_y);
//  Teuchos::RCP<ConfusionBilinearForm> bf = Teuchos::rcp(new ConfusionBilinearForm(epsilon,beta_x,beta_y));
  Teuchos::RCP<ConfusionBilinearForm> bf = Teuchos::rcp(new ConfusionBilinearForm(epsilon,beta_x,beta_y));


  FieldContainer<double> quadPoints(4,2);

  quadPoints(0,0) = 0.0; // x1
  quadPoints(0,1) = 0.0; // y1
  quadPoints(1,0) = 1.0;
  quadPoints(1,1) = 0.0;
  quadPoints(2,0) = 1.0;
  quadPoints(2,1) = 1.0;
  quadPoints(3,0) = 0.0;
  quadPoints(3,1) = 1.0;

  int H1Order = polyOrder + 1;
  int horizontalCells = 2, verticalCells = 2;
  // create a pointer to a new mesh:
  //  Teuchos::RCP<Mesh> mesh = Mesh::buildQuadMesh(quadPoints, horizontalCells, verticalCells, exactSolution.bilinearForm(), H1Order, H1Order+pToAdd, useTriangles);
  Teuchos::RCP<Mesh> mesh = Mesh::buildQuadMesh(quadPoints, horizontalCells, verticalCells,
                            bf, H1Order, H1Order+pToAdd, useTriangles);
  mesh->setUsePatchBasis(usePatchBasis);
  mesh->setEnforceMultiBasisFluxContinuity(enforceMBFluxContinuity);

  //  mesh->setPartitionPolicy(Teuchos::rcp(new ZoltanMeshPartitionPolicy("HSFC")));

  // define our inner product:
  Teuchos::RCP<ConfusionInnerProduct> ip = Teuchos::rcp( new ConfusionInnerProduct( bf, mesh ) );

  // create a solution object
  Teuchos::RCP<Solution> solution;
  if (useEggerSchoeberl)
    solution = Teuchos::rcp(new Solution(mesh, exactSolution.bc(), exactSolution.ExactSolution::rhs(), ip));
  else
  {
    Teuchos::RCP<ConfusionProblem> problem = Teuchos::rcp( new ConfusionProblem(bf) );
    solution = Teuchos::rcp(new Solution(mesh, problem, problem, ip));
  }

  Teuchos::RCP<Solver> solver;
  if ( useSchwarz )
  {
    int overlapLevel = 10;
    int maxIters = 4000;
    double tol = 5e-7;
    Teuchos::RCP<SchwarzSolver> schwarzSolver = Teuchos::rcp( new SchwarzSolver(overlapLevel,maxIters,tol) );
    schwarzSolver->setPrintToConsole(rank==0);
    solver = schwarzSolver;
  }
  else
  {
    // use KLU
    solver = Teuchos::rcp( new KluSolver() );
  }

  solution->solve(solver);

  if (rank==0)
  {
    //    solution->writeFieldsToFile(ConfusionBilinearForm::U, "Confusion_u_adaptive.dat");
    solution->writeFieldsToFile(ConfusionBilinearForm::U, "u_unif.m");
    solution->writeFluxesToFile(ConfusionBilinearForm::U_HAT, "u_hat_unif.dat");
  }

  int refIterCount = 0;
  vector<double> errorVector;
  vector<int> dofVector;
  for (int i=0; i<numRefinements; i++)
  {
    map<int, double> energyError = solution->energyError();
    vector< Teuchos::RCP< Element > > activeElements = mesh->activeElements();
    vector< Teuchos::RCP< Element > >::iterator activeElemIt;

    // greedy refinement algorithm - mark cells for refinement
    vector<int> triangleCellsToRefine;
    vector<int> quadCellsToRefine;
    double maxError = 0.0;
    double totalEnergyErrorSquared=0.0;
    for (activeElemIt = activeElements.begin(); activeElemIt != activeElements.end(); activeElemIt++)
    {
      Teuchos::RCP< Element > current_element = *(activeElemIt);
      int cellID = current_element->cellID();
      //      cout << "energy error for cellID " << cellID << " = " << energyError[cellID] << endl;
      maxError = max(energyError[cellID],maxError);
      totalEnergyErrorSquared += energyError[cellID]*energyError[cellID];
    }
    if (rank==0)
    {
      cout << "For refinement number " << refIterCount << ", energy error = " << totalEnergyErrorSquared<<endl;
    }
    errorVector.push_back(totalEnergyErrorSquared);
    dofVector.push_back(mesh->numGlobalDofs());

    // do refinements on cells with error above threshold
    for (activeElemIt = activeElements.begin(); activeElemIt != activeElements.end(); activeElemIt++)
    {
      Teuchos::RCP< Element > current_element = *(activeElemIt);
      int cellID = current_element->cellID();
      if (energyError[cellID]>=thresholdFactor*maxError)
      {
        if (current_element->numSides()==3)
        {
          triangleCellsToRefine.push_back(cellID);
        }
        else if (current_element->numSides()==4)
        {
          quadCellsToRefine.push_back(cellID);
        }
      }
    }
    mesh->hRefine(triangleCellsToRefine,RefinementPattern::regularRefinementPatternTriangle());
    triangleCellsToRefine.clear();
    mesh->hRefine(quadCellsToRefine,RefinementPattern::regularRefinementPatternQuad());
    quadCellsToRefine.clear();

    // enforce 1-irregularity if desired
    if (limitIrregularity)
    {
      mesh->enforceOneIrregularity();
    }

    refIterCount++;
    if (rank==0)
    {
      cout << "Solving on refinement iteration number " << refIterCount << "..." << endl;
    }
    solution->solve(solver);
    if (rank==0)
    {
      cout << "Solved..." << endl;
    }
  }

  if (useEggerSchoeberl)
  {
    // print out the L2 error of the solution:
    double l2error = exactSolution.L2NormOfError(*solution, ConfusionBilinearForm::U);
    cout << "L2 error: " << l2error << endl;
  }

  // save a data file for plotting in MATLAB
  if (rank==0)
  {
    //    solution->writeFieldsToFile(ConfusionBilinearForm::U, "Confusion_u_adaptive.dat");
    solution->writeFieldsToFile(ConfusionBilinearForm::U, "u.m");
    solution->writeFluxesToFile(ConfusionBilinearForm::U_HAT, "u_hat.dat");
    solution->writeFluxesToFile(ConfusionBilinearForm::BETA_N_U_MINUS_SIGMA_HAT, "beta_n_u_minus_sigma_hat.dat");

    ofstream fout1("errors.dat");
    fout1 << setprecision(15);
    for (int i = 0; i<errorVector.size(); i++)
    {
      fout1 << errorVector[i] << endl;
    }
    fout1.close();

    ofstream fout2("dofs.dat");
    for (int i = 0; i<dofVector.size(); i++)
    {
      fout2 << dofVector[i] << endl;
    }
    fout2.close();
    cout << "wrote files: u.m, u_hat.dat, errors.dat, dofs.dat, beta_n_u_minus_sigma_hat.dat\n";

//    cout << "Done writing soln to file." << endl;
  }

  return 0;
}
