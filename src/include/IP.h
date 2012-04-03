//
//  IP.h
//  Camellia
//
//  Created by Nathan Roberts on 3/30/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef Camellia_IP_h
#define Camellia_IP_h

#include "DPGInnerProduct.h"
#include "LinearTerm.h"
#include "Var.h"

class IP : public DPGInnerProduct {
  vector< LinearTermPtr > _linearTerms;
public:
  IP();
  
  // if the terms are a1, a2, ..., then the inner product is (a1,a1) + (a2,a2) + ... 
  void addTerm( LinearTermPtr a);
  void addTerm( VarPtr v );
  
  void computeInnerProductMatrix(FieldContainer<double> &innerProduct, 
                                 Teuchos::RCP<DofOrdering> dofOrdering,
                                 Teuchos::RCP<BasisCache> basisCache);
  
  void operators(int testID1, int testID2, 
                 vector<IntrepidExtendedTypes::EOperatorExtended> &testOp1,
                 vector<IntrepidExtendedTypes::EOperatorExtended> &testOp2);
  
  void printInteractions();
};

#endif