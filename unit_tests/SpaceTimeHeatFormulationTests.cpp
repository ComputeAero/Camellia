//
//  SpaceTimeHeatFormulation
//  Camellia
//
//  Created by Nate Roberts on 11/25/14.
//
//

#include "Teuchos_UnitTestHarness.hpp"

#include "MeshFactory.h"
#include "SpaceTimeHeatFormulation.h"
#include "TypeDefs.h"

using namespace Camellia;

namespace {
  void projectExactSolution(SpaceTimeHeatFormulation &form, SolutionPtr heatSolution, FunctionPtr u) {
    double epsilon = form.epsilon();
    
    FunctionPtr sigma1, sigma2, sigma3;
    int spaceTimeDim = heatSolution->mesh()->getDimension();
    int spaceDim = spaceTimeDim - 1;
    
    sigma1 = epsilon * u->dx();
    if (spaceDim > 1) sigma2 = epsilon * u->dy();
    if (spaceDim > 2) sigma3 = epsilon * u->dz();
    
    LinearTermPtr sigma_n_lt = form.sigma_n_hat()->termTraced();
    LinearTermPtr u_lt = form.u_hat()->termTraced();
    
    map<int, FunctionPtr> exactMap;
    // fields:
    exactMap[form.u()->ID()] = u;
    exactMap[form.sigma(1)->ID()] = sigma1;
    if (spaceDim > 1) exactMap[form.sigma(2)->ID()] = sigma2;
    if (spaceDim > 2) exactMap[form.sigma(3)->ID()] = sigma3;
    
    // flux:
    // use the exact field variable solution together with the termTraced to determine the flux traced
    FunctionPtr sigma_n = sigma_n_lt->evaluate(exactMap);
    exactMap[form.sigma_n_hat()->ID()] = sigma_n;
    
    // traces:
    FunctionPtr u_hat = u_lt->evaluate(exactMap);
    exactMap[form.u_hat()->ID()] = u_hat;
    
    heatSolution->projectOntoMesh(exactMap);
  }
  
  void setupExactSolution(SpaceTimeHeatFormulation &form, FunctionPtr u,
                          MeshTopologyPtr meshTopo, int fieldPolyOrder, int delta_k) {
    double epsilon = form.epsilon();
    
    FunctionPtr sigma1, sigma2, sigma3;
    int spaceTimeDim = meshTopo->getSpaceDim();
    int spaceDim = spaceTimeDim - 1;
    
    FunctionPtr forcingFunction = SpaceTimeHeatFormulation::forcingFunction(spaceDim, epsilon, u);
    
    form.initializeSolution(meshTopo, fieldPolyOrder, delta_k, forcingFunction);
  }
  
  void testSpaceTimeHeatConsistency(int spaceDim, Teuchos::FancyOStream &out, bool &success) {
    vector<double> dimensions(spaceDim,2.0); // 2x2 square domain
    vector<int> elementCounts(spaceDim,1); // 1 x 1 mesh
    vector<double> x0(spaceDim,-1.0);
    MeshTopologyPtr spatialMeshTopo = MeshFactory::rectilinearMeshTopology(dimensions, elementCounts, x0);
    
    double t0 = 0.0, t1 = 1.0;
    MeshTopologyPtr spaceTimeMeshTopo = MeshFactory::spaceTimeMeshTopology(spatialMeshTopo, t0, t1);
    
    double epsilon = 0.1;
    int fieldPolyOrder = 2, delta_k = 1;
    
    FunctionPtr u;
    FunctionPtr x = Function::xn(1);
    FunctionPtr y = Function::yn(1);
    FunctionPtr z = Function::zn(1);
    FunctionPtr t = Function::tn(1);
    
    if (spaceDim == 1) {
      u = x * t;
    } else if (spaceDim == 2) {
      u = x * t + y;
    } else if (spaceDim == 3) {
      u = x * t + y - z;
    }
    
    bool useConformingTraces = true;
    SpaceTimeHeatFormulation form(spaceDim, useConformingTraces, epsilon);
    
    setupExactSolution(form, u, spaceTimeMeshTopo, fieldPolyOrder, delta_k);
    projectExactSolution(form, form.solution(), u);
    
    form.solution()->clearComputedResiduals();
    
    double energyError = form.solution()->energyErrorTotal();
    
    double tol = 1e-13;
    TEST_COMPARE(energyError, <, tol);
  }

  
  TEUCHOS_UNIT_TEST( SpaceTimeHeatFormulation, Consistency_1D )
  {
    // consistency test for space-time formulation with 1D space
    testSpaceTimeHeatConsistency(1, out, success);
  }
} // namespace
