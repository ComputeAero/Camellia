//
//  PoissonFormulation.h
//  Camellia
//
//  Created by Nate Roberts on 10/16/14.
//
//

#ifndef Camellia_PoissonFormulation_h
#define Camellia_PoissonFormulation_h

#include "VarFactory.h"
#include "BF.h"

class PoissonFormulation {
  BFPtr _poissonBF;
  int _spaceDim;

  static const string S_PHI;
  static const string S_PSI;

  static const string S_PHI_HAT;
  static const string S_PSI_N_HAT;

  static const string S_Q;
  static const string S_TAU;
public:
  PoissonFormulation(int spaceDim, bool useConformingTraces);
  
  BFPtr bf();
  
  // field variables:
  VarPtr phi();
  VarPtr psi();
  
  // traces:
  VarPtr psi_n_hat();
  VarPtr phi_hat();
  
  // test variables:
  VarPtr q();
  VarPtr tau();
};

#endif