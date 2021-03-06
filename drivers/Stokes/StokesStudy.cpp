/*
 *  StokesStudy.cpp
 *
 *  Created by Nathan Roberts on 7/21/11.
 *  Original version © 2011 Sandia Corporation.
 *  Later revisions © 2011-2012 Nathan V. Roberts.
 *
 */

#include "StokesBilinearForm.h"

#include "HConvergenceStudy.h"
#include "MathInnerProduct.h"
#include "OptimalInnerProduct.h"
#include "StokesVVPBilinearForm.h"
#include "StokesMathBilinearForm.h"

#include "InnerProductScratchPad.h"

#include "../MultiOrderStudy/MultiOrderStudy.h"

#include "PreviousSolutionFunction.h"

#include "LagrangeConstraints.h"

#include "BasisFactory.h"

#include "MeshUtilities.h"

#include "Solver.h"
#include "CGSolver.h"

#ifdef HAVE_MPI
#include <Teuchos_GlobalMPISession.hpp>
#else
#endif

#include "StokesFormulation.h"

using namespace std;

class SquareBoundary : public SpatialFilter
{
public:
  bool matchesPoint(double x, double y)
  {
    double tol = 1e-14;
    bool xMatch = (abs(x+1.0) < tol) || (abs(x-1.0) < tol);
    bool yMatch = (abs(y+1.0) < tol) || (abs(y-1.0) < tol);
    return xMatch || yMatch;
  }
};

class ReentrantCornerBoundary : public SpatialFilter
{
  // implementing the one I believe is right (contrary to what's said in the HDG paper)
public:
  bool equals(double a, double b, double tol)
  {
    return abs(a-b)<tol;
  }
  bool matchesPoint(double x, double y)
  {
    double tol = 1e-14;
    bool xIsZero = equals(x,0,tol);
    bool yIsZero = equals(y,0,tol);
    bool xIsOne = equals(x,1,tol);
    bool yIsOne = equals(y,1,tol);
    bool xIsMinusOne = equals(x,-1,tol);
    bool yIsMinusOne = equals(y,-1,tol);
    if (xIsOne) return true;
    if (yIsOne) return true;
    if (xIsMinusOne && (y > 0)) return true;
    if (yIsZero && (x < 0)) return true;
    if (xIsZero && (y < 0)) return true;
    if (yIsMinusOne && (x > 0)) return true;
    return false;
  }
  // here's the one from the HDG paper:
//  bool matchesPoint(double x, double y) {
//    double tol = 1e-14;
//    bool xIsZero = equals(x,0,tol);
//    bool yIsZero = equals(y,0,tol);
//    bool xIsOne = equals(x,1,tol);
//    bool yIsOne = equals(y,1,tol);
//    bool xIsMinusOne = equals(x,-1,tol);
//    bool yIsMinusOne = equals(y,-1,tol);
//    if (xIsMinusOne) return true;
//    if (yIsOne) return true;
//    if (xIsOne && (y > 0)) return true;
//    if (yIsZero && (x > 0)) return true;
//    if (xIsZero && (y < 0)) return true;
//    if (yIsMinusOne && (x < 0)) return true;
//    return false;
//  }
};

class Xp : public SimpleFunction   // x^p, for x >= 0
{
  double _p;
public:
  Xp(double p)
  {
    _p = p;
  }
  double value(double x, double y)
  {
    if (x < 0)
    {
      cout << "calling pow(x," << _p << ") for x < 0: x=" << x << endl;
    }
    return pow(x,_p);
  }
  FunctionPtr dx()
  {
    return _p * (FunctionPtr) Teuchos::rcp( new Xp(_p-1) );
  }
  FunctionPtr dy()
  {
    return Function::zero();
  }
  string displayString()
  {
    ostringstream ss;
    ss << "x^{" << _p << "}";
    return ss.str();
  }
};

bool checkDivergenceFree(FunctionPtr u1_exact, FunctionPtr u2_exact)
{
  static const int NUM_POINTS_1D = 10;
  double x[NUM_POINTS_1D] = {-1.0,-0.8,-0.6,-.4,-.2,0,0.2,0.4,0.6,0.8};
  double y[NUM_POINTS_1D] = {-0.8,-0.6,-.4,-.2,0,0.2,0.4,0.6,0.8,1.0};

  FieldContainer<double> testPoints(1,NUM_POINTS_1D*NUM_POINTS_1D,2);
  for (int i=0; i<NUM_POINTS_1D; i++)
  {
    for (int j=0; j<NUM_POINTS_1D; j++)
    {
      testPoints(0,i*NUM_POINTS_1D + j, 0) = x[i];
      testPoints(0,i*NUM_POINTS_1D + j, 1) = y[i];
    }
  }
  BasisCachePtr basisCache = Teuchos::rcp( new PhysicalPointCache(testPoints) );

  FunctionPtr zeroExpected = u1_exact->dx() + u2_exact->dy();

  FieldContainer<double> values(1,NUM_POINTS_1D*NUM_POINTS_1D);

  zeroExpected->values(values,basisCache);

  bool success = true;
  double tol = 1e-14;
  for (int i=0; i<values.size(); i++)
  {
    if (abs(values[i]) > tol)
    {
      success = false;
      cout << "value not within tolerance: " << values[i] << endl;
    }
  }
  return success;
}

enum NormChoice
{
  GraphNorm, NaiveNorm, L2Norm, H1ExperimentalNorm, UnitCompliantGraphNorm, UnitCompliantGraphNormQScaled
};

enum ExactSolutionChoice
{
  HDGSingular, HDGSmooth, KanschatSmooth, TestPolynomial
};

void parseArgs(int argc, char *argv[], int &polyOrder, int &minLogElements, int &maxLogElements,
               StokesFormulationChoice &formulationType,
               bool &useTriangles, bool &useMultiOrder, NormChoice &normChoice, string &formulationTypeStr)
{
  polyOrder = 2;
  minLogElements = 0;
  maxLogElements = 4;

  // set up defaults:
  useTriangles = false;
  normChoice = GraphNorm;
  formulationType = VSP;
  formulationTypeStr = "original conforming";
  string multiOrderStudyType = "multiOrderQuad";

  useMultiOrder = false;

  string normChoiceStr = "opt";

  /* Usage:
   Multi-Order, naive norm:
     StokesStudy "multiOrder{Tri|Quad}" formulationTypeStr
   Single Order, original conforming, optimal norm, quad:
     StokesStudy polyOrder minLogElements maxLogElements
   Single Order, original conforming, quad:
   StokesStudy normChoice polyOrder minLogElements maxLogElements
   Single Order, quad:
     StokesStudy normChoice formulationTypeStr polyOrder minLogElements maxLogElements
   Single Order, quad:
     StokesStudy normChoice formulationTypeStr polyOrder minLogElements maxLogElements {"quad"|"tri"}

   where:
   formulationTypeStr = {"vgp"|"vgpf"|"vvp"|"vsp"|"dds"|"ddsp"|"ce"}
   normChoice = {"opt"|"naive"|"l2"|"h1"|"compliant"|"compliantQScaled"}

   */

  if (argc == 3)
  {
    string multiOrderStudyType = argv[1];
    formulationTypeStr = argv[2];
    if (multiOrderStudyType == "multiOrderTri")
    {
      useTriangles = true;
    }
    else
    {
      useTriangles = false;
    }
    useMultiOrder = true;
    normChoice = NaiveNorm; // using naive norm for paper
    polyOrder = 1;
  }
  else if (argc == 4)
  {
    polyOrder = atoi(argv[1]);
    minLogElements = atoi(argv[2]);
    maxLogElements = atoi(argv[3]);
  }
  else if (argc == 5)
  {
    normChoiceStr = argv[1];
    polyOrder = atoi(argv[2]);
    minLogElements = atoi(argv[3]);
    maxLogElements = atoi(argv[4]);
  }
  else if (argc == 6)
  {
    normChoiceStr = argv[1];

    formulationTypeStr = argv[2];
    polyOrder = atoi(argv[3]);
    minLogElements = atoi(argv[4]);
    maxLogElements = atoi(argv[5]);
  }
  else if (argc == 7)
  {
    normChoiceStr = argv[1];
    formulationTypeStr = argv[2];
    polyOrder = atoi(argv[3]);
    minLogElements = atoi(argv[4]);
    maxLogElements = atoi(argv[5]);
    string elementTypeStr = argv[6];
    if (elementTypeStr == "tri")
    {
      useTriangles = true;
    }
    else if (elementTypeStr == "quad")
    {
      useTriangles = false;
    } // otherwise, just use whatever was defined above
  }
  if (normChoiceStr == "naive")
  {
    normChoice = NaiveNorm;
  }
  else if (normChoiceStr == "l2")
  {
    normChoice = L2Norm;
  }
  else if (normChoiceStr == "h1")
  {
    normChoice = H1ExperimentalNorm;
  }
  else if (normChoiceStr == "compliant")
  {
    normChoice = UnitCompliantGraphNorm;
  }
  else if (normChoiceStr == "compliantQScaled")
  {
    normChoice = UnitCompliantGraphNormQScaled;
  }
  if (formulationTypeStr == "vgp")
  {
    formulationType = VGP;
  }
  else if (formulationTypeStr == "vgpf")
  {
    formulationType = VGPF;
  }
  else if (formulationTypeStr == "vvp")
  {
    formulationType = VVP;
  }
  else if (formulationTypeStr == "dds")
  {
    formulationType = DDS;
  }
  else if (formulationTypeStr == "ddsp")
  {
    formulationType = DDSP;
  }
  else if (formulationTypeStr == "ce")
  {
    formulationType = CE;
  }
  else
  {
    formulationType = VSP;
    formulationTypeStr = "vsp";
  }
}

void LShapedDomain(vector<FieldContainer<double> > &vertices, vector< vector<int> > &elementVertices, bool useTriangles)
{
  FieldContainer<double> p1(2);
  FieldContainer<double> p2(2);
  FieldContainer<double> p3(2);
  FieldContainer<double> p4(2);
  FieldContainer<double> p5(2);
  FieldContainer<double> p6(2);
  FieldContainer<double> p7(2);
  FieldContainer<double> p8(2);

// this is what's claimed to be the domain in the Cockburn/Nguyen paper.
// below is the one that I think they actually used...
// builds a domain for (-1,1)^2 \ (0,1) x (-1,0)
  p1(0) = -1.0;
  p1(1) = -1.0;
  p2(0) = -1.0;
  p2(1) =  0.0;
  p3(0) = -1.0;
  p3(1) =  1.0;
  p4(0) =  0.0;
  p4(1) =  1.0;
  p5(0) =  1.0;
  p5(1) =  1.0;
  p6(0) =  1.0;
  p6(1) =  0.0;
  p7(0) =  0.0;
  p7(1) =  0.0;
  p8(0) =  0.0;
  p8(1) = -1.0;

// build a domain for (-1,1)^2 \ (-1,0) x (-1,0)
// (this is a rotation about the origin of the points above)
//  p1(0) = -1.0; p1(1) =  1.0;
//  p2(0) =  0.0; p2(1) =  1.0;
//  p3(0) =  1.0; p3(1) =  1.0;
//  p4(0) =  1.0; p4(1) =  0.0;
//  p5(0) =  1.0; p5(1) = -1.0;
//  p6(0) =  0.0; p6(1) = -1.0;
//  p7(0) =  0.0; p7(1) =  0.0;
//  p8(0) = -1.0; p8(1) =  0.0;

  vertices.push_back(p1);
  vertices.push_back(p2);
  vertices.push_back(p3);
  vertices.push_back(p4);
  vertices.push_back(p5);
  vertices.push_back(p6);
  vertices.push_back(p7);
  vertices.push_back(p8);

  if (useTriangles)
  {
    vector<int> element;
    element.push_back(0);
    element.push_back(7);
    element.push_back(6);
    elementVertices.push_back(element);
    element.clear();

    element.push_back(0);
    element.push_back(6);
    element.push_back(1);
    elementVertices.push_back(element);
    element.clear();

    element.push_back(1);
    element.push_back(6);
    element.push_back(3);
    elementVertices.push_back(element);
    element.clear();

    element.push_back(1);
    element.push_back(3);
    element.push_back(2);
    elementVertices.push_back(element);
    element.clear();

    element.push_back(3);
    element.push_back(6);
    element.push_back(4);
    elementVertices.push_back(element);
    element.clear();

    element.push_back(4);
    element.push_back(6);
    element.push_back(5);
    elementVertices.push_back(element);
  }
  else
  {
    vector<int> element;
    element.push_back(0);
    element.push_back(7);
    element.push_back(6);
    element.push_back(1);
    elementVertices.push_back(element);
    element.clear();

    element.push_back(1);
    element.push_back(6);
    element.push_back(3);
    element.push_back(2);
    elementVertices.push_back(element);
    element.clear();

    element.push_back(6);
    element.push_back(5);
    element.push_back(4);
    element.push_back(3);
    elementVertices.push_back(element);
  }
}

void printSamplePoints(FunctionPtr fxn, string fxnName)
{
  int rank = 0;
#ifdef HAVE_MPI
  rank     = Teuchos::GlobalMPISession::getRank();
#else
#endif
  if (rank==0)
  {
    vector< pair<double, double> > points;
    points.push_back( make_pair(0, 0) );
    points.push_back( make_pair(1, 0) );
    points.push_back( make_pair(0, 1) );
    points.push_back( make_pair(1, 1) );
    points.push_back( make_pair(-1, 0) );
    points.push_back( make_pair(0, -1) );
    points.push_back( make_pair(1, -1) );
    points.push_back( make_pair(-1, -1) );
    for (int i=0; i<points.size(); i++)
    {
      cout << fxnName << "(" << points[i].first << ", " << points[i].second << ") = ";
      cout << Function::evaluate(fxn, points[i].first, points[i].second) << endl;
    }
  }
}

int main(int argc, char *argv[])
{
  int rank = 0, numProcs = 1;
#ifdef HAVE_MPI
  // TODO: figure out the right thing to do here...
  // may want to modify argc and argv before we make the following call:
  Teuchos::GlobalMPISession mpiSession(&argc, &argv,0);
  rank=mpiSession.getRank();
  numProcs=mpiSession.getNProc();
#else
#endif
  int pToAdd = 2; // for optimal test function approximation
  bool computeRelativeErrors = true; // we'll say false when one of the exact solution components is 0

  ExactSolutionChoice exactSolnChoice = KanschatSmooth;

  bool computeGlobalConditionNumber = false; // really slow, and requires writeGlobalStiffnessMatrixToFile = true

  bool computeMaxGramConditionNumber = true; // for Gram matrices

  bool useLobattoBasis = true;

  bool useTrueTracesForVVP = true;

  bool dontImposeZeroMeanPressure = false;

  bool writeGlobalStiffnessMatrixToFile = true;  // also computes 2-norm condition number of global stiffness matrix (expensive!)

  bool useCondensedSolve = true;

  bool useCG = false;
  bool useMumps = true;

  bool useEnrichedTraces = true; // enriched traces are the right choice, mathematically speaking
  double cgTol = 1e-8;
  int cgMaxIt = 400000;
  Teuchos::RCP<Solver> cgSolver = Teuchos::rcp( new CGSolver(cgMaxIt, cgTol) );
#ifdef HAVE_MPI
#ifdef USE_MUMPS
  Teuchos::RCP<Solver> mumpsSolver = Teuchos::rcp( new MumpsSolver );
#endif
#endif

  BasisFactory::setUseEnrichedTraces(useEnrichedTraces);

  // parse args:
  int polyOrder, minLogElements, maxLogElements;
  bool useTriangles, useMultiOrder;
  NormChoice normChoice;
  string normChoiceStr;

  StokesFormulationChoice formulationType;
  string formulationTypeStr;
  parseArgs(argc, argv, polyOrder, minLogElements, maxLogElements, formulationType, useTriangles,
            useMultiOrder, normChoice, formulationTypeStr);

  if (normChoice == NaiveNorm) normChoiceStr = "naive";
  else if (normChoice == GraphNorm) normChoiceStr = "graph";
  else if (normChoice == L2Norm) normChoiceStr = "l2";
  else if (normChoice == H1ExperimentalNorm) normChoiceStr = "H^1 Experimental";
  else if (normChoice == UnitCompliantGraphNorm) normChoiceStr = "unit-compliant graph";
  else if (normChoice == UnitCompliantGraphNormQScaled) normChoiceStr = "unit-compliant graph with 1/h scaling in q";
  else normChoiceStr = "unknownNorm";

  string exactSolnChoiceStr;
  if (exactSolnChoice == KanschatSmooth)   exactSolnChoiceStr = "Kanschat Smooth";
  else if (exactSolnChoice == HDGSmooth)   exactSolnChoiceStr = "HDG Smooth (Kovasznay)";
  else if (exactSolnChoice == HDGSingular) exactSolnChoiceStr = "HDG Singular";
  else exactSolnChoiceStr = "test polynomial";

  // commenting this stuff out on the idea that relaxing conforming traces will do the trick...
  if ((exactSolnChoice==HDGSingular) && (polyOrder == 2) && useTriangles)   // just testing something...
  {
    pToAdd = 3;
  }
//  if (formulationType==VVP) {
//    // for at least the singular solution and VVP on quads, find that we get much better results
//    // with 3 than 2...
//    pToAdd = 3;
//  }
//
//  if ((formulationType==VSP) && useTriangles) {
//    // for at least the singular solution and VSP on triangles, find that we avoid some bad results
//    // with 3 rather than 2...
//    pToAdd = 3;
//  }

  bool useConformingTraces = true;
//  if (exactSolnChoice == HDGSingular) {
//    useConformingTraces = false;
//  }

  if (rank == 0)
  {
    cout << "polyOrder = " << polyOrder << endl;
    cout << "pToAdd = " << pToAdd << endl;
    cout << "formulationType = " << formulationTypeStr                  << "\n";
    cout << "useTriangles = "    << (useTriangles   ? "true" : "false") << "\n";
    cout << "test space norm: "  << normChoiceStr << "\n";
    cout << "exact solution choice: "  << exactSolnChoiceStr << "\n";
    cout << "useLobattoBasis: " << (useLobattoBasis ? "true" : "false") << "\n";
    cout << "useCondensedSolve: " << (useCondensedSolve ? "true" : "false") << "\n";
    cout << "computeGlobalConditionNumber = " << (computeGlobalConditionNumber ? "true" : "false") << "\n";
    cout << "useMumps = " << (useMumps ? "true" : "false") << "\n";
    cout << "useTrueTracesForVVP = " << (useTrueTracesForVVP ? "true" : "false") << endl;
    cout << "useConformingTraces = " << (useConformingTraces ? "true" : "false") << endl;
    cout << "dontImposeZeroMeanPressure = " << (dontImposeZeroMeanPressure ? "true" : "false") << endl;
    cout << "writeGlobalStiffnessMatrixToFile = " << (writeGlobalStiffnessMatrixToFile ? "true" : "false") << endl;
    cout << "computeMaxGramConditionNumber = " << (computeMaxGramConditionNumber ? "true" : "false") << endl;
    if (writeGlobalStiffnessMatrixToFile)
    {
      cout << "NOTE: global stiffness matrix file is written in a format that needs tweaking for MATLAB.  Delete the comment line at top, and replace the final number on the second line with a 0.\n";
    }
  }

  if ((normChoice == UnitCompliantGraphNorm) && (formulationType != VGP))
  {
    cout << "Error: unit-compliant graph norm only supported for VGP right now.\n";
    exit(1);
  }
  if ((normChoice == UnitCompliantGraphNormQScaled) && (formulationType != VGP))
  {
    cout << "Error: unit-compliant graph norm with q scaling only supported for VGP right now.\n";
    exit(1);
  }

  if (useLobattoBasis)
  {
    BasisFactory::setUseLobattoForQuadHDiv(true);
    BasisFactory::setUseLobattoForQuadHGrad(true);
  }

  double mu = 1.0;

  if (exactSolnChoice == HDGSmooth)
  {
    mu = 0.1; // what they use in their paper
  }

  Teuchos::RCP<StokesFormulation> stokesForm;

  switch (formulationType)
  {
  case DDS:
    stokesForm = Teuchos::rcp(new DDSStokesFormulation(mu));
    break;
  case DDSP:
    stokesForm = Teuchos::rcp(new DDSPStokesFormulation(mu));
    break;
  case VGP:
    stokesForm = Teuchos::rcp(new VGPStokesFormulation(mu, (normChoice==UnitCompliantGraphNorm) || (normChoice==UnitCompliantGraphNormQScaled)));
    break;
  case VGPF:
    stokesForm = Teuchos::rcp(new VGPFStokesFormulation(mu));
    break;
  case VVP:
    stokesForm = Teuchos::rcp(new VVPStokesFormulation(mu, useTrueTracesForVVP));
    break;
  case VSP:
    stokesForm = Teuchos::rcp(new VSPStokesFormulation(mu));
    break;
  case CE:
    stokesForm = Teuchos::rcp(new CEStokesFormulation(mu));
  default:
    break;
  }

  FunctionPtr u1_exact, u2_exact, p_exact;

  Teuchos::RCP<ExactSolution> mySolution;
  if (! stokesForm.get() )
  {
    cout << "\n\n ERROR: stokesForm undefined!!\n\n";
    exit(1);
  }
  else
  {
    // trying out the new ExactSolution features:
    FunctionPtr cos_y = Teuchos::rcp( new Cos_y );
    FunctionPtr sin_y = Teuchos::rcp( new Sin_y );
    FunctionPtr exp_x = Teuchos::rcp( new Exp_x );

    FunctionPtr x = Teuchos::rcp ( new Xn(1) );
    FunctionPtr x2 = Teuchos::rcp( new Xn(2) );
    FunctionPtr y2 = Teuchos::rcp( new Yn(2) );
    FunctionPtr y = Teuchos::rcp( new Yn(1) );

    SpatialFilterPtr entireBoundary = Teuchos::rcp( new SpatialFilterUnfiltered ); // SpatialFilterUnfiltered returns true everywhere

    if (exactSolnChoice == KanschatSmooth)
    {
      u1_exact = - exp_x * ( y * cos_y + sin_y );
      u2_exact = exp_x * y * sin_y;
      p_exact = 2.0 * exp_x * sin_y;
    }
    else if (exactSolnChoice == HDGSingular)
    {

      const double PI  = 3.141592653589793238462;
      const double lambda = 0.54448373678246;
      const double omega = 3.0 * PI / 2.0;
      const double lambda_plus = lambda + 1.0;
      const double lambda_minus = lambda - 1.0;
      const double omega_lambda = omega * lambda;

      FunctionPtr sin_lambda_plus_y  = Teuchos::rcp( new Sin_ay( lambda_plus ) );
      FunctionPtr sin_lambda_minus_y = Teuchos::rcp( new Sin_ay( lambda_minus) );

      FunctionPtr cos_lambda_plus_y  = Teuchos::rcp( new Cos_ay( lambda_plus ) );
      FunctionPtr cos_lambda_minus_y = Teuchos::rcp( new Cos_ay( lambda_minus) );

      FunctionPtr phi_y = (cos(omega_lambda) / lambda_plus)  * sin_lambda_plus_y  - cos_lambda_plus_y
                          - (cos(omega_lambda) / lambda_minus) * sin_lambda_minus_y + cos_lambda_minus_y;

      Teuchos::RCP<PolarizedFunction> phi_theta = Teuchos::rcp( new PolarizedFunction( phi_y ) );
      FunctionPtr phi_theta_prime = phi_theta->dtheta();
      FunctionPtr phi_theta_triple_prime = phi_theta->dtheta()->dtheta()->dtheta();

      FunctionPtr x_to_lambda = Teuchos::rcp( new Xp(lambda) );
      FunctionPtr x_to_lambda_minus = Teuchos::rcp( new Xp(lambda_minus) );
      FunctionPtr x_to_lambda_plus = Teuchos::rcp( new Xp(lambda_plus) );
      FunctionPtr r_to_lambda = Function::polarize( x_to_lambda );
      FunctionPtr r_to_lambda_minus = Function::polarize( x_to_lambda_minus );
      FunctionPtr r_to_lambda_plus = Function::polarize( x_to_lambda_plus );

      FunctionPtr cos_theta = Function::polarize(Teuchos::rcp( new Cos_y ) );
      FunctionPtr sin_theta = Function::polarize(Teuchos::rcp( new Sin_y ) );

      // define stream function:
      FunctionPtr psi = r_to_lambda_plus * (FunctionPtr) phi_theta;

//      cout << "phi (3*pi/2) = " << Function::evaluate(phi_y, 1, 3*PI/2 ) << endl;
//      cout << "phi'(3*pi/2) = " << Function::evaluate(phi_y->dy(), 1, 3*PI/2 ) << endl;
//      u1_exact = psi->dy();
//      u2_exact = - psi->dx();
      u1_exact = r_to_lambda * (  lambda_plus * sin_theta * (FunctionPtr) phi_theta + cos_theta * phi_theta_prime );
      u2_exact = r_to_lambda * ( -lambda_plus * cos_theta * (FunctionPtr) phi_theta + sin_theta * phi_theta_prime );
      p_exact = -r_to_lambda_minus * ( (lambda_plus * lambda_plus) * phi_theta_prime + phi_theta_triple_prime) / lambda_minus;

      /*if (rank==0) {
        cout << "u1(-1,0) = " << Function::evaluate(u1_exact, -1, 0) << endl;
        cout << "u2(-1,0) = " << Function::evaluate(u2_exact, -1, 0) << endl;
        cout << "u1(0,-1) = " << Function::evaluate(u1_exact, 0, -1) << endl;
        cout << "u2(0,-1) = " << Function::evaluate(u2_exact, 0, -1) << endl;
        cout << "u1(1, 0) = " << Function::evaluate(u1_exact, 1,  0) << endl;
        cout << "u2(1, 0) = " << Function::evaluate(u2_exact, 1,  0) << endl;
        cout << "u1(0, 1) = " << Function::evaluate(u1_exact, 0,  1) << endl;
        cout << "u2(0, 1) = " << Function::evaluate(u2_exact, 0,  1) << endl;
      }*/

      double p_avg = 0.0;
      // double p_avg = 9.00146834554765 / 3.0; // numerically determined: want a zero-mean pressure (3.0 being the domain measure)
      p_exact = p_exact - (FunctionPtr) Teuchos::rcp( new ConstantScalarFunction(p_avg) );

      // test code to check
      if (! checkDivergenceFree(u1_exact, u2_exact) )
      {
        cout << "WARNING: exact solution does not appear to be divergence-free.\n";
      }

      if (rank==0)
      {
        cout << "exact pressure at origin: " << Function::evaluate(p_exact, 0, 0) << endl;
      }

//      printSamplePoints(u1_exact, "u1_exact");
//      printSamplePoints(u2_exact, "u1_exact");
//
//      FunctionPtr f1 = p_exact->dx() - mu * (u1_exact->dx()->dx() + u1_exact->dy()->dy());
//      FunctionPtr f2 = p_exact->dy() - mu * (u2_exact->dx()->dx() + u2_exact->dy()->dy());
//
//      printSamplePoints(f1, "f1");
//      printSamplePoints(f2, "f2");

    }
    else if (exactSolnChoice == HDGSmooth)
    {
      // u1 = 1 - exp( lambda x ) cos (2 PI y)
      // u2 = lambda / (2 PI) exp( lambda x ) sin ( 2 PI y )
      //  p = 0.5 exp ( 2 lambda x )
      //
      // where lambda = Re / 2 - sqrt( (Re/2)^2 + (2 PI)^2 )
      //   and     Re = 1 / mu = 10.0


      const double PI  = 3.141592653589793238462;
      double Re = 1.0 / mu;
      double lambda = Re / 2 - sqrt ( (Re / 2) * (Re / 2) + (2 * PI) * (2 * PI) );

      FunctionPtr exp_lambda_x = Teuchos::rcp( new Exp_ax( lambda ) );
      FunctionPtr exp_2lambda_x = Teuchos::rcp( new Exp_ax( 2 * lambda ) );
      FunctionPtr sin_2pi_y = Teuchos::rcp( new Sin_ay( 2 * PI ) );
      FunctionPtr cos_2pi_y = Teuchos::rcp( new Cos_ay( 2 * PI ) );

      u1_exact = Function::constant(1.0) - exp_lambda_x * cos_2pi_y;
      u2_exact = (lambda / (2 * PI)) * exp_lambda_x * sin_2pi_y;

      double p_average_value = 3.41501262705947 / 4.0; // 4.0 is the measure of the domain
      p_exact = 0.5 * exp_2lambda_x - Function::constant(p_average_value); // make it have zero average
    }
    else
    {
      computeRelativeErrors = false;
      u1_exact = Function::zero();// x2;
      u2_exact = Function::zero();//-2*x*y;
      p_exact = y; // odd function: zero mean on our domain
    }

    BFPtr stokesBF = stokesForm->bf();
    if (rank==0)
      stokesBF->printTrialTestInteractions();

    mySolution = stokesForm->exactSolution(u1_exact, u2_exact, p_exact, entireBoundary);
  }

  BCPtr bc = mySolution->bc();
  if (dontImposeZeroMeanPressure)
  {
    vector<int> fieldIDs;
    stokesForm->primaryTrialIDs(fieldIDs);
    int pressureIDIndex = (formulationType==VGPF) ? 1 : 2;
    int pressureID = fieldIDs[pressureIDIndex];
    bc->removeZeroMeanConstraint(pressureID);
    // instead, use a single-point BC on pressure
    bc->addSinglePointBC(pressureID, p_exact);
  }

  Teuchos::RCP<DPGInnerProduct> ip;
  if (normChoice == GraphNorm)
  {
    if ( stokesForm.get() )
    {
      ip = stokesForm->graphNorm();
    }
    else
    {
      ip = Teuchos::rcp( new OptimalInnerProduct( mySolution->bilinearForm() ) );
    }
  }
  else if (normChoice == NaiveNorm)
  {
    if (stokesForm.get() )
    {
      ip = stokesForm->bf()->naiveNorm();
    }
    else
    {
      ip = Teuchos::rcp( new   MathInnerProduct( mySolution->bilinearForm() ) );
    }
  }
  else if (normChoice == L2Norm)
  {
    ip = stokesForm->bf()->l2Norm();
  }
  else if (normChoice == H1ExperimentalNorm)
  {
    if (formulationType != VGP)
    {
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument, "only VGP supported for the H^1 experimental norm right now.");
    }

    IPPtr myIP = Teuchos::rcp( new IP );
    VarFactory varFactory = VGPStokesFormulation::vgpVarFactory();

    VarPtr v1 = varFactory.testVar(VGP_V1_S, HGRAD);
    VarPtr v2 = varFactory.testVar(VGP_V2_S, HGRAD);
    VarPtr tau1 = varFactory.testVar(VGP_TAU1_S, HDIV);
    VarPtr tau2 = varFactory.testVar(VGP_TAU2_S, HDIV);
    VarPtr q = varFactory.testVar(VGP_Q_S, HGRAD);

    myIP->addTerm( mu * v1->grad() + tau1 ); // sigma11, sigma12
    myIP->addTerm( mu * v2->grad() + tau2 ); // sigma21, sigma22
    myIP->addTerm( v1->dx() + v2->dy() );     // pressure
    myIP->addTerm( tau1->div() );    // u1
    myIP->addTerm( tau2->div() );    // u2

    myIP->addTerm(v1);
    myIP->addTerm(v2);
    myIP->addTerm(tau1);
    myIP->addTerm(tau2);
    myIP->addTerm(q);

    ip = myIP;
  }
  else if (normChoice==UnitCompliantGraphNorm)
  {
    ip = dynamic_cast< VGPStokesFormulation* >(stokesForm.get())->scaleCompliantGraphNorm();
  }
  else if (normChoice==UnitCompliantGraphNormQScaled)
  {
    IPPtr myIP = Teuchos::rcp( new IP );
    VarFactory varFactory = VGPStokesFormulation::vgpVarFactory();
    VarPtr v1 = varFactory.testVar(VGP_V1_S, HGRAD);
    VarPtr v2 = varFactory.testVar(VGP_V2_S, HGRAD);
    VarPtr tau1 = varFactory.testVar(VGP_TAU1_S, HGRAD);
    VarPtr tau2 = varFactory.testVar(VGP_TAU2_S, HGRAD);
    VarPtr q = varFactory.testVar(VGP_Q_S, HGRAD);

    FunctionPtr h = Function::h();

    myIP->addTerm( mu * v1->dx() + tau1->x() ); // sigma11
    myIP->addTerm( mu * v1->dy() + tau1->y() ); // sigma12
    myIP->addTerm( mu * v2->dx() + tau2->x() ); // sigma21
    myIP->addTerm( mu * v2->dy() + tau2->y() ); // sigma22
    myIP->addTerm( mu * v1->dx() + mu * v2->dy() ); // pressure
    myIP->addTerm( h * tau1->div() - q->dx() ); // u1
    myIP->addTerm( h * tau2->div() - q->dy());  // u2

    myIP->addTerm( (mu / h) * v1 );
    myIP->addTerm( (mu / h) * v2 );
    myIP->addTerm( q / h );
    myIP->addTerm( tau1 );
    myIP->addTerm( tau2 );

    ip = myIP;
  }

  if (rank==0)
    ip->printInteractions();

  FieldContainer<double> quadPoints(4,2); // NOTE: quadPoints unused for HDGSingular (there, we set the mesh more manually)

  if ( exactSolnChoice != HDGSmooth )
  {
    quadPoints(0,0) = -1.0; // x1
    quadPoints(0,1) = -1.0; // y1
    quadPoints(1,0) = 1.0;
    quadPoints(1,1) = -1.0;
    quadPoints(2,0) = 1.0;
    quadPoints(2,1) = 1.0;
    quadPoints(3,0) = -1.0;
    quadPoints(3,1) = 1.0;
  }
  else
  {
    quadPoints(0,0) = -0.5; // x1
    quadPoints(0,1) =  0.0; // y1
    quadPoints(1,0) =  1.5;
    quadPoints(1,1) =  0.0;
    quadPoints(2,0) =  1.5;
    quadPoints(2,1) =  2.0;
    quadPoints(3,0) = -0.5;
    quadPoints(3,1) =  2.0;
  }

  int u1_trialID, u2_trialID, p_trialID;
  int u1_traceID, u2_traceID;
  if ( stokesForm.get() )
  {
    // don't define IDs: we don't need them
  }
  else if (formulationType == VVP)
  {
    u1_trialID = StokesVVPBilinearForm::U1;
    u2_trialID = StokesVVPBilinearForm::U2;
    p_trialID = StokesVVPBilinearForm::P;
    u1_traceID = -1; // no velocity traces available in VVP formulation
    u2_traceID = -1;
  }
  else
  {
    u1_trialID = StokesBilinearForm::U1;
    u2_trialID = StokesBilinearForm::U2;
    p_trialID =  StokesBilinearForm::P;
    u1_traceID = StokesBilinearForm::U1_HAT;
    u2_traceID = StokesBilinearForm::U2_HAT;
  }

  if ( !useMultiOrder )
  {
    HConvergenceStudy study(mySolution,
                            mySolution->bilinearForm(),
                            mySolution->ExactSolution::rhs(),
                            bc, ip,
                            minLogElements, maxLogElements,
                            polyOrder+1, pToAdd, false, useTriangles, false);
    study.setUseCondensedSolve(useCondensedSolve);
    study.setReportRelativeErrors(computeRelativeErrors);
    ostringstream stiffness_prefix;
    stiffness_prefix << "stokes_study_stiffness_k" << polyOrder << "_";
    study.setWriteGlobalStiffnessToDisk(writeGlobalStiffnessMatrixToFile, stiffness_prefix.str());
    int maxTestDegree = polyOrder + 1 + pToAdd;
    int cubatureDegreeInMesh = polyOrder + maxTestDegree;

    if (useTriangles)
    {
      int INTREPID_CUBATURE_TRI_DEFAULT_MAX_ENUM = 20;
      int cubEnrichment = INTREPID_CUBATURE_TRI_DEFAULT_MAX_ENUM - cubatureDegreeInMesh;
      study.setCubatureDegreeForExact(cubEnrichment);
    }

    if (useCG) study.setSolver(cgSolver);
    else if (useMumps)
    {
#ifdef HAVE_MPI
#ifdef USE_MUMPS
      study.setSolver(mumpsSolver);
#endif
#endif
    } // otherwise, use default solver (KLU)

    if ( exactSolnChoice != HDGSingular )
    {
      study.solve(quadPoints,useConformingTraces);
      // debug code:
//      DofOrderingPtr trialSpace = study.getSolution(0)->mesh()->getElement(0)->elementType()->trialOrderPtr;
//      cout << "trial space for cell 0 on 1x1 mesh: \n" << *trialSpace;
    }
    else
    {
      // L-shaped domain
      vector<FieldContainer<double> > vertices;
      vector< vector<int> > elementVertices;
      LShapedDomain(vertices, elementVertices, useTriangles);
      // claim from [18] is homogeneous RHS -- we're not seeing that, so as a test,
      // let's impose that.  (If we now converge, that will give a clue where the bug is...)
//      RHSPtr zeroRHS = RHS::rhs();
//
//      study = HConvergenceStudy(mySolution,
//                              mySolution->bilinearForm(),
//                              zeroRHS,
//                              mySolution->bc(), ip,
//                              minLogElements, maxLogElements,
//                              polyOrder+1, pToAdd, false, useTriangles, false);

//      cout << "WARNING: commented out the HDG singular solve (need to fix call to study).\n";
      const map< Edge, ParametricCurvePtr > edgeToCurveMap; //no curves
      MeshGeometryPtr geometry = Teuchos::rcp( new MeshGeometry(vertices, elementVertices, edgeToCurveMap) );
      study.solve(geometry,useConformingTraces);

      // don't enrich cubature if using triangles, since the cubature factory for triangles can only go so high...
      // (could be more precise about this; I'm not sure exactly where the limit is: we could enrich some)
    }
    int cubatureEnrichment = useTriangles ? 0 : 15;
    double p_integral = p_exact->integrate(study.getSolution(maxLogElements)->mesh(), cubatureEnrichment);

    if (dontImposeZeroMeanPressure)
    {
      // then we need to adjust the solutions by subtracting off the mean of the pressure
      vector<int> fieldIDs;
      stokesForm->primaryTrialIDs(fieldIDs);
      int pressureIDIndex = (formulationType==VGPF) ? 1 : 2;
      int pressureID = fieldIDs[pressureIDIndex];
      VarPtr pressure = Var::varForTrialID(pressureID, stokesForm->bf());
      double pressure_L2norm = p_exact->l2norm(study.getSolution(maxLogElements)->mesh(),cubatureEnrichment);

      for (int i=minLogElements; i<=maxLogElements; i++)
      {
        SolutionPtr soln = study.getSolution(i);
        FunctionPtr pressure_soln = Function::solution(pressure, soln);
        double pressure_integral = pressure_soln->integrate(soln->mesh());
        map<int, FunctionPtr > functionMap;
        functionMap[pressureID] = pressure_soln - pressure_integral;
        soln->projectOntoMesh(functionMap);
        double relativeError = (pressure_soln - pressure_integral - p_exact)->l2norm(soln->mesh()) / pressure_L2norm;
        cout << "relative pressure error for logElements " << i << ": "  << relativeError << endl;
      }
      // now have study recompute the errors
      study.computeErrors();
    }

    if (computeMaxGramConditionNumber)
    {
      for (int i=minLogElements; i<=maxLogElements; i++)
      {
        SolutionPtr soln = study.getSolution(i);
        ostringstream fileNameStream;
        fileNameStream << "stokesStudy_maxConditionIPMatrix_" << i << ".dat";
        IPPtr ip = Teuchos::rcp( dynamic_cast< IP* >(soln->ip().get()), false );
        bool jacobiScalingTrue = true;
        double maxConditionNumber = MeshUtilities::computeMaxLocalConditionNumber(ip, soln->mesh(), jacobiScalingTrue, fileNameStream.str());
        if (rank==0)
        {
          cout << "max jacobi-scaled Gram matrix condition number estimate for logElements " << i << ": "  << maxConditionNumber << endl;
          cout << "putative worst-conditioned Gram matrix written to: " << fileNameStream.str() << "." << endl;
        }
      }
    }

    if (writeGlobalStiffnessMatrixToFile && computeGlobalConditionNumber)
    {
      for (int i=minLogElements; i<=maxLogElements; i++)
      {
        double globalCondNum = study.computeJacobiPreconditionedConditionNumber(i);
        if (rank==0)
        {
          cout << "Jacobi-scaled global system matrix condition number for logElements " << i << ": " << globalCondNum << endl;
        }
      }
    }

    if (rank == 0)
    {
      cout << "Integral of exact pressure: " << setprecision(15) << p_integral << endl;

      cout << study.TeXErrorRateTable();
      vector<int> primaryVariables;
      stokesForm->primaryTrialIDs(primaryVariables);
      vector<int> fieldIDs,traceIDs;
      vector<string> fieldFileNames;
      stokesForm->trialIDs(fieldIDs,traceIDs,fieldFileNames);
      cout << "******** Best Approximation comparison: ********\n";
      cout << study.TeXBestApproximationComparisonTable(primaryVariables);

      ostringstream filePathPrefix;
      filePathPrefix << "stokes/" << formulationTypeStr << "_p" << polyOrder << "_velpressure";
      study.TeXBestApproximationComparisonTable(primaryVariables,filePathPrefix.str());
      filePathPrefix.str("");
      filePathPrefix << "stokes/" << formulationTypeStr << "_p" << polyOrder << "_all";
      study.TeXBestApproximationComparisonTable(fieldIDs);

      // for now, not interested in plots, etc. of individual variables.
      for (int i=0; i<fieldIDs.size(); i++)
      {
        int fieldID = fieldIDs[i];
        int traceID = traceIDs[i];
        string fieldName = fieldFileNames[i];
        ostringstream filePathPrefix;
        filePathPrefix << "stokes/" << formulationTypeStr << "_" << fieldName << "_p" << polyOrder;
        study.writeToFiles(filePathPrefix.str(),fieldID,traceID);
      }

      if (formulationType != VGPF)
      {
        for (int i=minLogElements; i<=maxLogElements; i++)
        {
          ostringstream filePath;
          int numElements = pow(2.0,i);
          filePath << "stokes/soln" << numElements << "x";
          filePath << numElements << "_p" << polyOrder << ".vtk";
          cout << "writing VTK for " << numElements << " x " << numElements << " mesh.\n";
          study.getSolution(i)->writeToVTK(filePath.str());
        }
      }
      else
      {
        cout << "formulationType = VGPF, so skipping VTK output (vector-valued fields not yet supported there).\n";
      }

      filePathPrefix.str("");
      filePathPrefix << "stokes/" << formulationTypeStr << "_p" << polyOrder << "_convDataMATLAB.dat";

      ofstream fout(filePathPrefix.str().c_str());

      for (int i=0; i<primaryVariables.size(); i++)
      {
        string convDataMATLAB = study.convergenceDataMATLAB(primaryVariables[i]);
        cout << convDataMATLAB;
        fout << convDataMATLAB;
      }
      fout.close();

      filePathPrefix.str("");
      filePathPrefix << "stokes/" << formulationTypeStr << "_p" << polyOrder << "_numDofs";
      ofstream fout2(filePathPrefix.str().c_str());
      string texGlobalDofsTable = study.TeXNumGlobalDofsTable();
      fout2 << texGlobalDofsTable;
      cout << texGlobalDofsTable;
      fout2.close();
    }
  }
  else
  {
    cout << "Generating mixed-order 16x16 mesh" << endl;
    Teuchos::RCP<Mesh> mesh = MultiOrderStudy::makeMultiOrderMesh16x16(quadPoints,
                              mySolution->bilinearForm(),
                              polyOrder+1, pToAdd,
                              useTriangles);

    Solution solution(mesh, mySolution->bc(), mySolution->ExactSolution::rhs(), ip);
    solution.solve();
    int cubDegree = 15; // for error computations
    double  pError = mySolution->L2NormOfError(solution, p_trialID,  cubDegree);
    double u1Error = mySolution->L2NormOfError(solution, u1_trialID, cubDegree);
    double u2Error = mySolution->L2NormOfError(solution, u2_trialID, cubDegree);

    string meshType = (useTriangles) ? "triangular" : "quad";

    cout << "Multi-order, 16x16 " << meshType << " mesh, pressure error: "  <<  pError << endl;
    cout << "Multi-order, 16x16 " << meshType << " mesh, u1 error: "        << u1Error << endl;
    cout << "Multi-order, 16x16 " << meshType << " mesh, u2 error: "        << u2Error << endl;
  }
}