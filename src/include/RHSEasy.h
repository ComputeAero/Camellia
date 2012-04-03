//
//  RHSEasy.h
//  Camellia
//
//  Created by Nathan Roberts on 3/30/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef Camellia_RHSEasy_h
#define Camellia_RHSEasy_h

#include "RHS.h"
#include "LinearTerm.h"

class RHSEasy : public RHS {
  vector< LinearTermPtr > _terms;
  set<int> _testIDs;
public:
  void addTerm( LinearTermPtr rhsTerm );
  void addTerm( VarPtr v );
  
  // at a conceptual/design level, this method isn't necessary
  bool nonZeroRHS(int testVarID);
  
  void integrateAgainstStandardBasis(FieldContainer<double> &rhsVector, 
                                     Teuchos::RCP<DofOrdering> testOrdering, 
                                     BasisCachePtr basisCache);};

#endif