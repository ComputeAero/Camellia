#include "RefinementStrategy.h"
#include "PreviousSolutionFunction.h"
#include "MeshFactory.h"
#include "SolutionExporter.h"
#include <Teuchos_GlobalMPISession.hpp>
#include "GnuPlotUtil.h"

#include "Teuchos_CommandLineProcessor.hpp"
#include "Teuchos_ParameterList.hpp"

#include "Solver.h"

#include "ErrorPercentageRefinementStrategy.h"

#include "CamelliaConfig.h"

#include "MeshTools.h"

#ifdef ENABLE_INTEL_FLOATING_POINT_EXCEPTIONS
#include <xmmintrin.h>
#endif

#include "EpetraExt_ConfigDefs.h"
#ifdef HAVE_EPETRAEXT_HDF5
#include "HDF5Exporter.h"
#endif


class Cone_U0 : public SimpleFunction {
  double _r; // cone radius
  double _h; // height
  double _x0, _y0; // center
  bool _usePeriodicData; // if true, for x > 0.5 we set x = x-1; similarly for y
public:
  Cone_U0(double x0 = 0, double y0 = 0.25, double r = 0.1, double h = 1.0, bool usePeriodicData = true) {
    _x0 = x0;
    _y0 = y0;
    _r = r;
    _h = h;
    _usePeriodicData = usePeriodicData;
  }
  double value(double x, double y, double z) {
    if (_usePeriodicData) {
      if (x > 0.5) {
        x = x - 1;
      }
      if (y > 0.5) y = y - 1;
    }
    double d = sqrt( (x-_x0) * (x-_x0) + (y-_y0) * (y-_y0) );
    double u = max(0.0, _h * (1 - d/_r));
    
    return u;
  }
};

class InflowFilterForClockwisePlanarRotation : public SpatialFilter {
  double _xLeft, _yBottom, _xRight, _yTop;
  double _xMiddle, _yMiddle;
public:
  InflowFilterForClockwisePlanarRotation(double leftBoundary_x, double rightBoundary_x,
                                         double bottomBoundary_y, double topBoundary_y,
                                         double rotationCenter_x, double rotationCenter_y) {
    _xLeft = leftBoundary_x;
    _yBottom = bottomBoundary_y;
    _xRight = rightBoundary_x;
    _yTop = topBoundary_y;
    _xMiddle = rotationCenter_x;
    _yMiddle = rotationCenter_y;
  }
  bool matchesPoint(double x, double y, double z) {
    double tol = 1e-14;
    bool inflow;
    if (abs(x-_xLeft)<tol) {
      inflow = (y > _yMiddle);
    } else if (abs(x-_xRight)<tol) {
      inflow = (y < _yMiddle);
    } else if (abs(y-_yBottom)<tol) {
      inflow = (x < _xMiddle);
    } else if (abs(y-_yTop)<tol) {
      inflow = (x > _xMiddle);
    } else {
      inflow = false; // not a boundary point at all...
    }
    return inflow;
  }
};

int main(int argc, char *argv[]) {
#ifdef ENABLE_INTEL_FLOATING_POINT_EXCEPTIONS
  cout << "NOTE: enabling floating point exceptions for divide by zero.\n";
  _MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() & ~_MM_MASK_INVALID);
#endif
  
  Teuchos::GlobalMPISession mpiSession(&argc, &argv);
  int rank = Teuchos::GlobalMPISession::getRank();
  
  Teuchos::CommandLineProcessor cmdp(false,true); // false: don't throw exceptions; true: do return errors for unrecognized options

  const static double PI  = 3.141592653589793238462;
  
  bool useCondensedSolve = false; // condensed solve not yet compatible with minimum rule meshes
  
  int k = 2; // poly order for u
  int numCells = 32; // in x, y
  int numTimeCells = 1;
  int numTimeSlabs = -1;
  int numFrames = 201;
  int delta_k = 3;   // test space enrichment: should be 3 for 3D
  int maxRefinements = 0; // maximum # of refinements on each time slab
  bool useMumpsIfAvailable  = true;
  bool useConstantConvection = false;
  double refinementTolerance = 0.1;
  
  cmdp.setOption("polyOrder",&k,"polynomial order for field variable u");
  cmdp.setOption("delta_k", &delta_k, "test space polynomial order enrichment");

  cmdp.setOption("numCells",&numCells,"number of cells in x and y directions");
  cmdp.setOption("numTimeCells",&numTimeCells,"number of time axis cells");
  cmdp.setOption("numFrames",&numFrames,"number of frames for export");
  
  cmdp.setOption("useConstantConvection", "useVariableConvection", &useConstantConvection);
  
  cmdp.setOption("useCondensedSolve", "useUncondensedSolve", &useCondensedSolve, "use static condensation to reduce the size of the global solve");
  cmdp.setOption("useMumps", "useKLU", &useMumpsIfAvailable, "use MUMPS (if available)");
  
  cmdp.setOption("refinementTolerance", &refinementTolerance, "relative error beyond which to stop refining");
  cmdp.setOption("maxRefinements", &maxRefinements, "maximum # of refinements on each time slab");
  
  if (cmdp.parse(argc,argv) != Teuchos::CommandLineProcessor::PARSE_SUCCESSFUL) {
#ifdef HAVE_MPI
    MPI_Finalize();
#endif
    return -1;
  }
  
  int H1Order = k + 1;
  
  VarFactory varFactory;
  // traces:
  VarPtr qHat = varFactory.fluxVar("\\widehat{q}");
 
  // fields:
  VarPtr u = varFactory.fieldVar("u", L2);

  // test functions:
  VarPtr v = varFactory.testVar("v", HGRAD);
  
  FunctionPtr x = Function::xn(1);
  FunctionPtr y = Function::yn(1);
  
  FunctionPtr c;
  if (useConstantConvection) {
    c = Function::vectorize(Function::constant(0.5), Function::constant(0.5), Function::constant(1.0));
  } else {
    c = Function::vectorize(y-0.5, 0.5-x, Function::constant(1.0));
  }
  FunctionPtr n = Function::normal();
  
  BFPtr bf = Teuchos::rcp( new BF(varFactory) );
  
  bf->addTerm( u, c * v->grad());
  bf->addTerm(qHat, v);

  double width = 2.0, height = 2.0;
  int horizontalCells = numCells, verticalCells = numCells;
  int depthCells = numTimeCells;
  double x0 = -0.5; double y0 = -0.5;
  double t0 = 0;
  
  double totalTime = 2.0 * PI;
  
  vector<double> frameTimes;
  for (int i=0; i<numFrames; i++) {
    frameTimes.push_back((totalTime*i) / (numFrames-1));
  }
  
  if (numTimeSlabs==-1) {
    double h = width / horizontalCells; // want t approx equal to h
    numTimeSlabs = (int) totalTime / h / numTimeCells;
  }
  double timeLengthPerSlab = totalTime / numTimeSlabs;
  
  if (rank==0) {
    cout << "solving on " << numCells << " x " << numCells << " x " << numTimeCells << " mesh " << "of order " << k << ".\n";
    cout << "numTimeSlabs: " << numTimeSlabs << endl;
  }
  
  BCPtr bc = BC::bc();
  
  SpatialFilterPtr inflowFilter  = Teuchos::rcp( new InflowFilterForClockwisePlanarRotation (x0,x0+width,y0,y0+height,0.5,0.5));
  
  vector<double> dimensions;
  dimensions.push_back(width);
  dimensions.push_back(height);
  dimensions.push_back(timeLengthPerSlab);
  
  vector<int> elementCounts(3);
  elementCounts[0] = horizontalCells;
  elementCounts[1] = verticalCells;
  elementCounts[2] = depthCells;
  
  vector<double> origin(3);
  origin[0] = x0;
  origin[1] = y0;
  origin[2] = t0;
  
  MeshPtr mesh = MeshFactory::rectilinearMesh(bf, dimensions, elementCounts, H1Order, delta_k, origin);
  
  FunctionPtr u0 = Teuchos::rcp( new Cone_U0(0.0, 0.25, 0.1, 1.0, false) );
  
  bc->addDirichlet(qHat, inflowFilter, Function::zero()); // zero BCs enforced at the inflow boundary.
  bc->addDirichlet(qHat, SpatialFilter::matchingZ(t0), u0);

  IPPtr ip;
  ip = bf->graphNorm();
  
  // create two Solution objects; we'll switch between these for time steps
  SolutionPtr soln = Solution::solution(mesh, bc, RHS::rhs(), ip);
  
  Teuchos::RCP<Solver> solver = Teuchos::rcp( new KluSolver );
  
#ifdef USE_MUMPS
  if (useMumpsIfAvailable) solver = Teuchos::rcp( new MumpsSolver );
#endif
  
#ifdef USE_VTK
  NewVTKExporter vtkExporter(mesh->getTopology());
#endif
  
//  double errorPercentage = 0.5; // for mesh refinements: ask to refine elements that account for 80% of the error in each step
//  Teuchos::RCP<RefinementStrategy> refinementStrategy;
//  refinementStrategy = Teuchos::rcp( new ErrorPercentageRefinementStrategy( soln, errorPercentage ));

  double energyThreshold = 0.2; // for mesh refinements: ask to refine elements that account for 80% of the error in each step
  Teuchos::RCP<RefinementStrategy> refinementStrategy;
  refinementStrategy = Teuchos::rcp( new RefinementStrategy( soln, energyThreshold ));
  
  if (rank==0) cout << "Initial mesh has " << mesh->getTopology()->activeCellCount() << " active (leaf) cells " << "and " << mesh->globalDofCount() << " degrees of freedom.\n";
  
  if (maxRefinements != 0) {
    cout << "Warning: maxRefinements is not 0, but the slice exporter implicitly assumes there won't be any refinements.\n";
  }
  
#ifdef HAVE_EPETRAEXT_HDF5
  ostringstream dir_name;
  dir_name << "spacetime_slice_convectingCone_k" << k;
  map<GlobalIndexType,GlobalIndexType> cellMap;
  MeshPtr meshSlice = MeshTools::timeSliceMesh(mesh, 0, cellMap, H1Order);
  HDF5Exporter sliceExporter(meshSlice,dir_name.str());
#endif
  
  FunctionPtr sideParity = Function::sideParity();
  FunctionPtr u_spacetime = Function::solution(u, soln);
  
  int lastFrameOutputted = -1;
  
  for(int timeSlab = 0; timeSlab<numTimeSlabs; timeSlab++) {
    double relativeEnergyError;
    int refNumber = 0;
   
//    {
//      // DEBUGGING: just to try running the time slicing:
//      double t_slab_final = (timeStep+1) * timeLengthPerSlab;
//      int frameOrdinal = lastFrameOutputted + 1;
//      while (frameTimes[frameOrdinal] < t_slab_final) {
//        FunctionPtr u_spacetime = Function::solution(u, soln);
//        ostringstream dir_name;
//        dir_name << "spacetime_slice_convectingCone_k" << k;
//        MeshTools::timeSliceExport(dir_name.str(), mesh, u_spacetime, frameTimes[frameOrdinal], "u_slice");
//        
//        cout << "Exported frame " << frameOrdinal << ", t=" << frameTimes[frameOrdinal] << endl;
//        frameOrdinal++;
//      }
//    }
    
    do {
      soln->solve(solver);
      soln->reportTimings();
      
  #ifdef HAVE_EPETRAEXT_HDF5
      ostringstream dir_name;
      dir_name << "spacetime_convectingCone_k" << k << "_t" << timeSlab;
      HDF5Exporter exporter(soln->mesh(),dir_name.str());
      exporter.exportSolution(soln, varFactory);
//      string u_name = "u_spacetime";
//      exporter.exportFunction(u_spacetime, u_name);
      
      ostringstream file_name;
      file_name << dir_name.str();
      
      dir_name << ".soln";
      soln->writeToFile(dir_name.str());
      if (rank==0) cout << endl << "wrote " << dir_name.str() << endl;
      
      file_name << ".mesh";
      soln->mesh()->saveToHDF5(file_name.str());
  #endif
      FunctionPtr u_soln = Function::solution(u, soln);
  #ifdef USE_VTK
      ostringstream fileName;
      fileName << "u_soln_ref_" << refNumber;
      if (rank==0) vtkExporter.exportFunction(u_soln, "u_soln", fileName.str());
  #endif
      

  //    LinearTermPtr solnFunctional = bf->testFunctional(soln);
  //    RieszRep solnRieszRep(mesh, ip, solnFunctional);
  //    
  //    solnRieszRep.computeRieszRep();
  //    double solnEnergyNorm = solnRieszRep.getNorm();
      
      double solnNorm = u_soln->l2norm(mesh);
      
      double energyError = soln->energyErrorTotal();
      relativeEnergyError = energyError / solnNorm;
      
      if (rank==0) {
        cout << "Relative energy error for refinement " << refNumber++ << ": " << relativeEnergyError << endl;
      }
      
      if ((relativeEnergyError > refinementTolerance) && (refNumber < maxRefinements)) {
        refinementStrategy->refine();
        if (rank==0) {
          cout << "After refinement, mesh has " << mesh->getTopology()->activeCellCount() << " active (leaf) cells " << "and " << mesh->globalDofCount() << " degrees of freedom.\n";
        }
      }
      
    } while ((relativeEnergyError > refinementTolerance) && (refNumber < maxRefinements));
  
    double t_slab_final = (timeSlab+1) * timeLengthPerSlab;
    int frameOrdinal = lastFrameOutputted + 1;
    vector<double> timesForSlab;
    while (frameTimes[frameOrdinal] < t_slab_final) {
      double t = frameTimes[frameOrdinal];
      if (rank==0) cout << "exporting t=" << t << " on slab " << timeSlab << endl;
      FunctionPtr sliceFunction = MeshTools::timeSliceFunction(mesh, cellMap, u_spacetime, t);
      sliceExporter.exportFunction(sliceFunction, "u_slice", t);
      lastFrameOutputted = frameOrdinal++;
    }
    
    // set up next mesh/solution:
    FunctionPtr q_prev = Function::solution(qHat, soln);
    cout << "Error in setup of q_prev: simple solution doesn't know about the map from the previous time slab to the current one. (TODO: fix this.)\n";
    
    double tn = (timeSlab+1) * timeLengthPerSlab;
    origin[2] = tn;
    mesh = MeshFactory::rectilinearMesh(bf, dimensions, elementCounts, H1Order, delta_k, origin);
    bc = BC::bc();
    bc->addDirichlet(qHat, inflowFilter, Function::zero()); // zero BCs enforced at the inflow boundary.
    bc->addDirichlet(qHat, SpatialFilter::matchingZ(tn), q_prev * sideParity);

    soln = Solution::solution(mesh, bc, RHS::rhs(), ip);
  }
  
  return 0;
}