//
//  Var.cpp
//  Camellia
//
//  Created by Nathan Roberts on 3/30/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Var.h"
#include "LinearTerm.h"
#include "BF.h"

Camellia::EFunctionSpace VarFunctionSpaces::efsForSpace(Space space) {
  switch (space) {
    case HDIV:
      return Camellia::FUNCTION_SPACE_HDIV;
    case HGRAD:
      return Camellia::FUNCTION_SPACE_HGRAD;
    case HCURL:
      return Camellia::FUNCTION_SPACE_HCURL;
    case L2:
      return Camellia::FUNCTION_SPACE_HVOL;
    case CONSTANT_SCALAR:
      return Camellia::FUNCTION_SPACE_REAL_SCALAR;
      
    case HDIV_DISC:
      return Camellia::FUNCTION_SPACE_HDIV_DISC;
    case HGRAD_DISC:
      return Camellia::FUNCTION_SPACE_HGRAD_DISC;
    case HCURL_DISC:
      return Camellia::FUNCTION_SPACE_HCURL_DISC;

    case VECTOR_HGRAD:
      return Camellia::FUNCTION_SPACE_VECTOR_HGRAD;
    case VECTOR_L2:
      return Camellia::FUNCTION_SPACE_VECTOR_HVOL;
    
    case VECTOR_HGRAD_DISC:
      return Camellia::FUNCTION_SPACE_VECTOR_HGRAD_DISC;
      
//    case TENSOR_HGRAD:
      
    case HDIV_FREE:
      return Camellia::FUNCTION_SPACE_HDIV_FREE;
      
    default:
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument, "Unknown function space.");
      return Camellia::FUNCTION_SPACE_UNKNOWN;
  }
}

int VarFunctionSpaces::rankForSpace(Space space) {
  switch (space) {
    case HDIV:
      return 1;
    case HGRAD:
      return 0;
    case HCURL:
      return 1;
    case L2:
      return 0;
    case CONSTANT_SCALAR:
      return 0;
      
    case HDIV_DISC:
      return 1;
    case HGRAD_DISC:
      return 0;
    case HCURL_DISC:
      return 1;
      
    case VECTOR_HGRAD:
      return 1;
    case VECTOR_L2:
      return 1;
      
    case VECTOR_HGRAD_DISC:
      return 1;
      
      //    case TENSOR_HGRAD:
      
    case HDIV_FREE:
      return 1;
      
    default:
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument, "Unknown function space.");
      return Camellia::FUNCTION_SPACE_UNKNOWN;
  }
}

Space VarFunctionSpaces::spaceForEFS(Camellia::EFunctionSpace efs) {
  switch (efs) {
    case Camellia::FUNCTION_SPACE_HDIV:
      return HDIV;
    case Camellia::FUNCTION_SPACE_HGRAD:
      return HGRAD;
    case Camellia::FUNCTION_SPACE_HCURL:
      return HCURL;
    case Camellia::FUNCTION_SPACE_HVOL:
      return L2;
    case Camellia::FUNCTION_SPACE_REAL_SCALAR:
      return CONSTANT_SCALAR;
      
    case Camellia::FUNCTION_SPACE_HDIV_DISC:
      return HDIV_DISC;
    case Camellia::FUNCTION_SPACE_HGRAD_DISC:
      return HGRAD_DISC;
    case Camellia::FUNCTION_SPACE_HCURL_DISC:
      return HCURL_DISC;
      
    case Camellia::FUNCTION_SPACE_VECTOR_HGRAD:
      return VECTOR_HGRAD;
    case Camellia::FUNCTION_SPACE_VECTOR_HVOL:
      return VECTOR_L2;
      
    case Camellia::FUNCTION_SPACE_VECTOR_HGRAD_DISC:
      return VECTOR_HGRAD_DISC;
      
      //    case TENSOR_HGRAD:
      
    case Camellia::FUNCTION_SPACE_HDIV_FREE:
      return HDIV_FREE;
      
    default:
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument, "Unknown function space.");
      return UNKNOWN_FS;
  }
  
//  
//  if (efs == Camellia::FUNCTION_SPACE_HGRAD) {
//    return HGRAD;
//  }
//  if (efs == Camellia::FUNCTION_SPACE_HCURL) {
//    return HCURL;
//  }
//  if (efs == Camellia::FUNCTION_SPACE_HDIV) {
//    return HDIV;
//  }
//  if (efs == Camellia::FUNCTION_SPACE_HVOL) {
//    return L2;
//  }
//  if (efs == Camellia::FUNCTION_SPACE_REAL_SCALAR) {
//    return CONSTANT_SCALAR;
//  }
//  if (efs == Camellia::FUNCTION_SPACE_VECTOR_HGRAD) {
//    return VECTOR_HGRAD;
//  }
//  if (efs == Camellia::FUNCTION_SPACE_VECTOR_HVOL) {
//    return VECTOR_L2;
//  }
//  TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument, "Unknown function space.");
}

VarPtr Var::varForTrialID(int trialID, BFPtr bf) {
  Camellia::EFunctionSpace efs = bf->functionSpaceForTrial(trialID);
  Space space = spaceForEFS(efs);
  int rank = rankForSpace(space);
//  if (efs == Camellia::FUNCTION_SPACE_HGRAD) {
//    space = HGRAD;
//    rank = 0;
//  }
//  if (efs == Camellia::FUNCTION_SPACE_HCURL) {
//    space = HCURL;
//    rank = 1;
//  }
//  if (efs == Camellia::FUNCTION_SPACE_HDIV) {
//    space = HDIV;
//    rank = 1;
//  }
//  if (efs == Camellia::FUNCTION_SPACE_HDIV_FREE) {
//    space = HDIV_FREE;
//    rank = 1;
//  }
//  if (efs == Camellia::FUNCTION_SPACE_HVOL) {
//    space = L2;
//    rank = 0;
//  }
//  if (efs == Camellia::FUNCTION_SPACE_REAL_SCALAR) {
//    space = CONSTANT_SCALAR;
//    rank = 0;
//  }
//  if (efs == Camellia::FUNCTION_SPACE_VECTOR_HGRAD) {
//    space = VECTOR_HGRAD;
//    rank = 1;
//  }
//  if (efs == Camellia::FUNCTION_SPACE_VECTOR_HVOL) {
//    space = VECTOR_L2;
//    rank = 1;
//  }
  
  VarType varType;
  if (bf->isFluxOrTrace(trialID)) {
    if ((space==L2) || (space==VECTOR_L2)) {
      varType = FLUX;
    } else if ((space==HGRAD) || (space==VECTOR_HGRAD)) {
      varType = TRACE;
    } else {
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument, "unclassifiable variable");
    }
  } else {
    varType = FIELD;
  }
  
  return Teuchos::rcp(new Var(trialID, rank, "trial", OP_VALUE, space, varType));
}

Var::Var(int ID, int rank, string name, Camellia::EOperator op, Space fs, VarType varType, LinearTermPtr termTraced,
         bool definedOnTemporalInterfaces) {
  _id = ID;
  _rank = rank;
  _name = name;
  _op = op;
  _fs = fs;
  _varType = varType;
  _termTraced = termTraced;
  _definedOnTemporalInterfaces = definedOnTemporalInterfaces;
}

int Var::ID() const {
  return _id;
}

bool Var::isDefinedOnTemporalInterface() const {
  return _definedOnTemporalInterfaces;
}

const string & Var::name() const {
  return _name; 
}

bool isRightOperator(Camellia::EOperator op) { // as opposed to left
  set<int> _normalOperators;
  _normalOperators.insert(OP_CROSS_NORMAL);
  _normalOperators.insert(OP_DOT_NORMAL);
  _normalOperators.insert(OP_TIMES_NORMAL);
  _normalOperators.insert(OP_TIMES_NORMAL_X);
  _normalOperators.insert(OP_TIMES_NORMAL_Y);
  _normalOperators.insert(OP_TIMES_NORMAL_Z);
  return _normalOperators.find(op) != _normalOperators.end();
}

string Var::displayString() const {
  ostringstream varStream;
  if ( isRightOperator(_op) ) {
    varStream << _name << operatorName(_op);
  } else {
    varStream << operatorName(_op) << _name;
  }
  return varStream.str();
}

Camellia::EOperator Var::op() const {
  return _op; 
}

int Var::rank() const {  // 0 for scalar, 1 for vector, etc.
  return _rank; 
}

Space Var::space() const {
  return _fs; 
}

VarType Var::varType() const {
  return _varType; 
}

LinearTermPtr Var::termTraced() const {
  return _termTraced;
}

VarPtr Var::grad() const {
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  if (space() != VECTOR_HGRAD)
    TEUCHOS_TEST_FOR_EXCEPTION( _rank != 0, std::invalid_argument, "grad() only supported for vars of rank 0.");
  return Teuchos::rcp( new Var(_id, _rank + 1, _name, Camellia::OP_GRAD, UNKNOWN_FS, _varType ) );
}

VarPtr Var::div() const {
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  TEUCHOS_TEST_FOR_EXCEPTION( _rank != 1, std::invalid_argument, "div() only supported for vars of rank 1.");
  return Teuchos::rcp( new Var(_id, _rank - 1, _name, Camellia::OP_DIV, UNKNOWN_FS, _varType ) );
}

VarPtr Var::curl(int spaceDim) const {
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  TEUCHOS_TEST_FOR_EXCEPTION( (_rank != 0) && (_rank != 1), std::invalid_argument, "curl() can only be applied to vars of ranks 0 or 1.");
  if (spaceDim == 2) {
    int newRank = (_rank == 0) ? 1 : 0;
    return Teuchos::rcp( new Var(_id, newRank, _name, Camellia::OP_CURL, UNKNOWN_FS, _varType ) );
  } else {
    // in 3D, curl is rank-preserving.  (Vectors map to vectors.)
    return Teuchos::rcp( new Var(_id, _rank, _name, Camellia::OP_CURL, UNKNOWN_FS, _varType ) );
  }
}

VarPtr Var::dx() const {
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  TEUCHOS_TEST_FOR_EXCEPTION( _rank != 0, std::invalid_argument, "dx() only supported for vars of rank 0.");
  return Teuchos::rcp( new Var(_id, _rank, _name, Camellia::OP_DX, UNKNOWN_FS, _varType ) );
}

VarPtr Var::dy() const {
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  TEUCHOS_TEST_FOR_EXCEPTION( _rank != 0, std::invalid_argument, "dy() only supported for vars of rank 0.");
  return Teuchos::rcp( new Var(_id, _rank, _name, Camellia::OP_DY, UNKNOWN_FS, _varType ) );
}

VarPtr Var::dz() const {
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  TEUCHOS_TEST_FOR_EXCEPTION( _rank != 0, std::invalid_argument, "dz() only supported for vars of rank 0.");
  return Teuchos::rcp( new Var(_id, _rank, _name, Camellia::OP_DZ, UNKNOWN_FS, _varType ) );
}

VarPtr Var::x() const {
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  TEUCHOS_TEST_FOR_EXCEPTION( _rank != 1, std::invalid_argument, "x() only supported for vars of rank 1.");
  return Teuchos::rcp( new Var(_id, _rank-1, _name, OP_X, UNKNOWN_FS, _varType ) );
}

VarPtr Var::y() const {
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  TEUCHOS_TEST_FOR_EXCEPTION( _rank != 1, std::invalid_argument, "y() only supported for vars of rank 1.");
  return Teuchos::rcp( new Var(_id, _rank-1, _name, OP_Y, UNKNOWN_FS, _varType ) );
}

VarPtr Var::z() const {
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  TEUCHOS_TEST_FOR_EXCEPTION( _rank != 1, std::invalid_argument, "z() only supported for vars of rank 1.");
  return Teuchos::rcp( new Var(_id, _rank-1, _name, OP_Z, UNKNOWN_FS, _varType ) );
}

VarPtr Var::cross_normal() const {
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  TEUCHOS_TEST_FOR_EXCEPTION( _rank != 1, std::invalid_argument, "cross_normal() only supported for vars of rank 1.");
  return Teuchos::rcp( new Var(_id, _rank-1, _name, OP_CROSS_NORMAL, UNKNOWN_FS, _varType ) );
}

VarPtr Var::dot_normal() const {
  TEUCHOS_TEST_FOR_EXCEPTION( _rank != 1, std::invalid_argument, "dot_normal() only supported for vars of rank 1.");
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  return Teuchos::rcp( new Var(_id, _rank-1, _name, OP_DOT_NORMAL, UNKNOWN_FS, _varType ) );
}

VarPtr Var::times_normal() const {
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  VarType newType;
  if (_varType == TRACE)
    newType = FLUX;
  else if (_varType == FLUX) {
    TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument, "fluxes can't be multiplied by normal (they already implicitly contain a normal).");
  } else { // tests and field variables, restricted to the boundary, can be multiplied by normal--and are considered of the same type as before.
    newType = _varType;
  }
  return Teuchos::rcp( new Var(_id, _rank + 1, _name, OP_TIMES_NORMAL, UNKNOWN_FS, newType ) );
}

VarPtr Var::times_normal_x() const {
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  return Teuchos::rcp( new Var(_id, _rank, _name, OP_TIMES_NORMAL_X, UNKNOWN_FS, _varType ) );
}

VarPtr Var::times_normal_y() const {
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  return Teuchos::rcp( new Var(_id, _rank, _name, OP_TIMES_NORMAL_Y, UNKNOWN_FS, _varType ) );
}

VarPtr Var::times_normal_z() const {
  TEUCHOS_TEST_FOR_EXCEPTION( _op !=  Camellia::OP_VALUE, std::invalid_argument, "operators can only be applied to raw vars, not vars that have been operated on.");
  return Teuchos::rcp( new Var(_id, _rank, _name, OP_TIMES_NORMAL_Z, UNKNOWN_FS, _varType ) );
}