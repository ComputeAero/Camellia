// NOTE: This is deprecated by the conversion to Tpetra

#include "StandardAssembler.h"

// Epetra includes
#ifdef HAVE_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif

#include <Teuchos_GlobalMPISession.hpp>
//#include "BilinearFormUtility.h"
#include "SerialDenseWrapper.h"
#include "ml_epetra_utils.h"
#include "ml_epetra_preconditioner.h"
#include "BC.h"
#include "Epetra_Import.h"
#include "Epetra_Vector.h"

#include "Solver.h"
#include "EpetraExt_RowMatrixOut.h"
#include "EpetraExt_MultiVectorOut.h"

#include "Mesh.h"
#include "Solution.h"
#include "RHS.h"

using namespace Intrepid;

namespace Camellia
{
Epetra_Map StandardAssembler::getPartMap()
{
  int rank = Teuchos::GlobalMPISession::getRank();
#ifdef HAVE_MPI
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif
  MeshPtr mesh = _solution->mesh();
  set<GlobalIndexType> myGlobalIndicesSet = mesh->globalDofIndicesForPartition(rank);
  return _solution->getPartitionMap(rank,myGlobalIndicesSet, mesh->numGlobalDofs(),0,&Comm);
}

Epetra_FECrsMatrix StandardAssembler::initializeMatrix()
{
  Epetra_DataAccess copy = (Epetra_DataAccess) 0;
  Epetra_FECrsMatrix matrix(copy, getPartMap(),_solution->mesh()->rowSizeUpperBound());
  return matrix;
}
Epetra_FEVector StandardAssembler::initializeVector()
{
  Epetra_FEVector vector(getPartMap());
  return vector;
}

FieldContainer<double> StandardAssembler::getSubVector(Epetra_FEVector u, ElementPtr elem)
{
  int numTrialDofs = numTrialDofsForElem(elem);
  DofOrderingPtr trialOrdering = elem->elementType()->trialOrderPtr;
  MeshPtr mesh = _solution->mesh();
  FieldContainer<double> u_K(numTrialDofs,1);
  for (int localDofIndex=0; localDofIndex<numTrialDofs; localDofIndex++)
  {
    int globalDofIndex = mesh->globalDofIndex(elem->cellID(), localDofIndex);
    u_K(localDofIndex,0) = (*u(0))[globalDofIndex];
  }
  return u_K;
}

void StandardAssembler::getElemData(ElementPtr elem, FieldContainer<double> &K, FieldContainer<double> &f)
{
  int numTrialDofs = numTrialDofsForElem(elem);

  FieldContainer<double> B = getOverdeterminedStiffness(elem);
  FieldContainer<double> Rv = getIPMatrix(elem);
  FieldContainer<double> l = getRHS(elem);
  FieldContainer<double> RvB(B.dimension(0),B.dimension(1));

  SerialDenseWrapper::solveSystemMultipleRHS(RvB,Rv,B); // solve for optimal test functions
  //  FieldContainer<double> BtRvB(numTrialDofs,numTrialDofs),BtRvl(numTrialDofs,1);
  K.resize(numTrialDofs,numTrialDofs);
  f.resize(numTrialDofs);
  SerialDenseWrapper::multiply(K,RvB,B,'T','N');
  SerialDenseWrapper::multiply(f,RvB,l,'T','N');
}

//Teuchos::RCP<Epetra_LinearProblem> StandardAssembler::assembleProblem(){
//Epetra_FECrsMatrix StandardAssembler::assembleProblem(){
void StandardAssembler::assembleProblem(Epetra_FECrsMatrix &globalStiffMatrix, Epetra_FEVector &rhsVector)
{
  MeshPtr mesh = _solution->mesh();

  vector<ElementPtr> elems = mesh->activeElements();
  for (vector<ElementPtr>::iterator elemIt = elems.begin(); elemIt!=elems.end(); elemIt++)
  {
    ElementPtr elem = *elemIt;
    int numTrialDofs = numTrialDofsForElem(elem);
    FieldContainer<double> BtRvB(numTrialDofs,numTrialDofs),BtRvl(numTrialDofs,1);
    getElemData(elem,BtRvB,BtRvl);
    // apply filter(s) (e.g. penalty method, preconditioners, etc.)
    if (_solution->filter().get())
    {
      BtRvB.resize(1,numTrialDofs,numTrialDofs);
      _solution->filter()->filter(BtRvB,BtRvl,BasisCache::basisCacheForCell(mesh,elem->cellID()),mesh,_solution->bc());
      BtRvB.resize(numTrialDofs,numTrialDofs);
    }

    FieldContainer<int> globalDofIndices(numTrialDofs);
    // we have the same local-to-global map for both rows and columns
    for (int i=0; i<numTrialDofs; i++)
    {
      globalDofIndices(i) = mesh->globalDofIndex(elem->cellID(),i);
    }

    globalStiffMatrix.InsertGlobalValues(numTrialDofs,&globalDofIndices(0),numTrialDofs,&globalDofIndices(0),&BtRvB(0,0));
    rhsVector.SumIntoGlobalValues(numTrialDofs,&globalDofIndices(0),&BtRvl(0));

  }
  globalStiffMatrix.GlobalAssemble();
  rhsVector.GlobalAssemble();
  // apply BCs
  applyBCs(globalStiffMatrix,rhsVector);

  globalStiffMatrix.GlobalAssemble();
  rhsVector.GlobalAssemble();
}

void StandardAssembler::applyBCs(Epetra_FECrsMatrix &globalStiffMatrix, Epetra_FEVector &rhsVector)
{
  int rank = Teuchos::GlobalMPISession::getRank();
#ifdef HAVE_MPI
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif
  MeshPtr mesh = _solution->mesh();
  set<GlobalIndexType> myGlobalIndicesSet = mesh->globalDofIndicesForPartition(rank);
  Epetra_Map partMap = _solution->getPartitionMap(rank,myGlobalIndicesSet, mesh->numGlobalDofs(),0,&Comm);

  FieldContainer<GlobalIndexType> bcGlobalIndices;
  FieldContainer<double> bcGlobalValues;
  mesh->boundary().bcsToImpose(bcGlobalIndices,bcGlobalValues,*(_solution->bc()), myGlobalIndicesSet, NULL, NULL);
  int numBCs = bcGlobalIndices.size();

  Epetra_MultiVector v(partMap,1);
  v.PutScalar(0.0);
  for (int i = 0; i < numBCs; i++)
  {
    v.ReplaceGlobalValue((long long)bcGlobalIndices(i), 0, bcGlobalValues(i));
  }

  Epetra_MultiVector rhsDirichlet(partMap,1);
  globalStiffMatrix.Apply(v,rhsDirichlet);

  // Update right-hand side
  rhsVector.Update(-1.0,rhsDirichlet,1.0);

  if (numBCs != 0)
  {
    int err = rhsVector.ReplaceGlobalValues(numBCs,(long long*)&bcGlobalIndices(0),&bcGlobalValues(0));
    if (err != 0)
    {
      cout << "ERROR: rhsVector.ReplaceGlobalValues(): some indices non-local...\n";
    }
  }
  // Zero out rows and columns of stiffness matrix corresponding to Dirichlet edges and add one to diagonal.
  FieldContainer<int> bcLocalIndices(bcGlobalIndices.dimension(0));
  for (int i=0; i<bcGlobalIndices.dimension(0); i++)
  {
    bcLocalIndices(i) = globalStiffMatrix.LRID((long long)bcGlobalIndices(i));
  }
  if (numBCs == 0)
  {
    ML_Epetra::Apply_OAZToMatrix(NULL, 0, globalStiffMatrix);
  }
  else
  {
    ML_Epetra::Apply_OAZToMatrix(&bcLocalIndices(0), numBCs, globalStiffMatrix);
  }
}

void StandardAssembler::distributeDofs(Epetra_FEVector lhsVector)
{
  int rank = Teuchos::GlobalMPISession::getRank();

#ifdef HAVE_MPI
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif
  MeshPtr mesh = _solution->mesh();
  set<GlobalIndexType> myGlobalIndicesSet = mesh->globalDofIndicesForPartition(rank);
  Epetra_Map partMap = _solution->getPartitionMap(rank,myGlobalIndicesSet, mesh->numGlobalDofs(),0,&Comm);

  lhsVector.GlobalAssemble();

  // Import solution onto current processor
  int numNodesGlobal = partMap.NumGlobalElements();
  Epetra_Map     solnMap(numNodesGlobal, numNodesGlobal, 0, Comm);
  Epetra_Import  solnImporter(solnMap, partMap);
  Epetra_Vector  solnCoeff(solnMap);
  solnCoeff.Import(lhsVector, solnImporter, Insert);

  // copy the dof coefficients into our data structure
  // get ALL element types (not just ours)-- this is a global import that we should get rid of eventually...
  vector<ElementPtr> elems = mesh->activeElements();
  for (vector<ElementPtr>::iterator elemIt = elems.begin(); elemIt!=elems.end(); elemIt++)
  {
    ElementPtr elem = *(elemIt);
    int cellID = elem->cellID();
    int numDofs = numTrialDofsForElem(elem);
    FieldContainer<double> elemDofs(numDofs);
    for (int dofIndex=0; dofIndex<numDofs; dofIndex++)
    {
      int globalIndex = mesh->globalDofIndex(cellID, dofIndex);
      elemDofs(dofIndex) = solnCoeff[globalIndex];
    }
    _solution->setSolnCoeffsForCellID(elemDofs, cellID);
  }

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                              HELPER ROUTINES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


FieldContainer<double> StandardAssembler::getRHS(ElementPtr elem)
{
  MeshPtr mesh = _solution->mesh();
  int cellID = elem->cellID();
  int cubatureEnrichmentDegree = _solution->cubatureEnrichmentDegree();
  ElementTypePtr elemTypePtr = elem->elementType();
  DofOrderingPtr testOrderingPtr = elemTypePtr->testOrderPtr;
  int numTestDofs = testOrderingPtr->totalDofs();
  FieldContainer<double> rhsVector(1,numTestDofs);
  BasisCachePtr basisCache = BasisCache::basisCacheForCell(mesh, cellID, false, cubatureEnrichmentDegree); // should testVsTest be true?
  _solution->rhs()->integrateAgainstStandardBasis(rhsVector, testOrderingPtr, basisCache);
  rhsVector.resize(numTestDofs,1);
  return rhsVector;
}

// gets rectangular stiffness matrix - BilinearFormUtility::computeStiffnessMatrix(...) longer version
FieldContainer<double> StandardAssembler::getOverdeterminedStiffness(ElementPtr elem)
{
  MeshPtr mesh = _solution->mesh();
  int cellID = elem->cellID();
  ElementTypePtr elemTypePtr = elem->elementType();
  DofOrderingPtr testOrderingPtr = elemTypePtr->testOrderPtr;
  int numTestDofs = testOrderingPtr->totalDofs();
  DofOrderingPtr trialOrderingPtr = elemTypePtr->trialOrderPtr;
  int numTrialDofs = trialOrderingPtr->totalDofs();
  int cubatureEnrichmentDegree = _solution->cubatureEnrichmentDegree();

  FieldContainer<double> B(1,numTestDofs,numTrialDofs);
  //BilinearFormUtility::computeStiffnessMatrixForCell(B, mesh, cellID);
  BasisCachePtr basisCache = BasisCache::basisCacheForCell(mesh,cellID,false,cubatureEnrichmentDegree);
  FieldContainer<double> cellSideParities  = mesh->cellSideParitiesForCell(cellID);
  mesh->bilinearForm()->stiffnessMatrix(B, elemTypePtr, cellSideParities, basisCache);

  //  B.initialize(0.0);
  B.resize(numTestDofs,numTrialDofs);
  return B;
}


FieldContainer<double> StandardAssembler::getIPMatrix(ElementPtr elem)
{
  MeshPtr mesh = _solution->mesh();
  int cellID = elem->cellID();
  int cubatureEnrichmentDegree = _solution->cubatureEnrichmentDegree();
  ElementTypePtr elemTypePtr = elem->elementType();
  DofOrderingPtr testOrderingPtr = elemTypePtr->testOrderPtr;
  int numTestDofs = testOrderingPtr->totalDofs();
  BasisCachePtr ipBasisCache = BasisCache::basisCacheForCell(mesh, cellID, true, cubatureEnrichmentDegree);

  //  BasisCachePtr ipBasisCache = Teuchos::rcp(new BasisCache(elemTypePtr,mesh,true, cubatureEnrichmentDegree));
  FieldContainer<double> ipMatrix(1,numTestDofs,numTestDofs);

  _solution->ip()->computeInnerProductMatrix(ipMatrix,testOrderingPtr, ipBasisCache);
  ipMatrix.resize(numTestDofs,numTestDofs);
  return ipMatrix;
}

int StandardAssembler::numTestDofsForElem(ElementPtr elem)
{
  ElementTypePtr elemTypePtr = elem->elementType();
  DofOrderingPtr testOrderingPtr = elemTypePtr->testOrderPtr;
  return testOrderingPtr->totalDofs();
}

int StandardAssembler::numTrialDofsForElem(ElementPtr elem)
{
  ElementTypePtr elemTypePtr = elem->elementType();
  DofOrderingPtr trialOrderingPtr = elemTypePtr->trialOrderPtr;
  return trialOrderingPtr->totalDofs();
}
}
