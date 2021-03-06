//
//  CondensedDofInterpreter.h
//  Camellia-debug
//
//  Created by Nate Roberts on 2/6/14.
//
//

#ifndef Camellia_debug_CondensedDofInterpreter_h
#define Camellia_debug_CondensedDofInterpreter_h

#include "DofInterpreter.h"
#include "Intrepid_FieldContainer.hpp"
#include "Epetra_Vector.h"
#include "Mesh.h"
#include "LagrangeConstraints.h"
#include "Epetra_SerialDenseVector.h"
#include "RHS.h"

namespace Camellia
{
/**
 Note on naming of variables:
 Essentially, the notion is that the condensed dof interpreter sits on a layer above the GDA object (owned by the Mesh),
 which sits atop individual elements.  When we use "interpreted" below we mean interpreted by the GDA.  When we say
 "global" we mean the level above the condensed dof interpreter, the dofs that actually enter the global solve.

 See Nate's dissertation, pp. 44-45 for a brief explanation of static condensation.
 **/

template <typename Scalar>
class CondensedDofInterpreter : public DofInterpreter
{
protected:
  map<GlobalIndexType, Intrepid::FieldContainer<Scalar> > _localLoadVectors;       // will be used by interpretGlobalData if _storeLocalStiffnessMatrices is true
private:
  bool _storeLocalStiffnessMatrices;
  MeshPtr _mesh; // for element type lookup, and for determination of which dofs are trace dofs
  TIPPtr<Scalar> _ip;
  TRHSPtr<Scalar> _rhs;
  LagrangeConstraints* _lagrangeConstraints;
  set<int> _uncondensibleVarIDs;
  map<GlobalIndexType, Intrepid::FieldContainer<Scalar> > _localStiffnessMatrices; // will be used by interpretGlobalData if _storeLocalStiffnessMatrices is true
  map<GlobalIndexType, Intrepid::FieldContainer<GlobalIndexType> > _localInterpretedDofIndices;       // will be used by interpretGlobalData if _storeLocalStiffnessMatrices is true

  GlobalIndexType _myGlobalDofIndexOffset;
  IndexType _myGlobalDofIndexCount;

  set<GlobalIndexType> _interpretedFluxDofIndices; // the "global" dof indices prior to condensation

  map< GlobalIndexType, map< pair<int, int>, Intrepid::FieldContainer<GlobalIndexType> > > _interpretedDofIndicesForBasis; // outer map: cellID is index.  Inner: (varID, sideOrdinal)

  map<GlobalIndexType, GlobalIndexType> _interpretedToGlobalDofIndexMap; // maps from the interpreted dof indices to the new ("outer") global dof indices (we only store the ones that are seen by the local MPI rank)

  void getSubmatrices(set<int> fieldIndices, set<int> fluxIndices,
                      const Intrepid::FieldContainer<Scalar> &K, Epetra_SerialDenseMatrix &K_field,
                      Epetra_SerialDenseMatrix &K_coupl, Epetra_SerialDenseMatrix &K_flux);

  void getSubvectors(set<int> fieldIndices, set<int> fluxIndices, const Intrepid::FieldContainer<Scalar> &b, Epetra_SerialDenseVector &b_field, Epetra_SerialDenseVector &b_flux);

  void initializeGlobalDofIndices();
  map<GlobalIndexType, GlobalIndexType> interpretedFluxMapForPartition(PartitionIndexType partition, bool storeFluxDofIndices);

  void computeAndStoreLocalStiffnessAndLoad(GlobalIndexType cellID);

  void getLocalData(GlobalIndexType cellID, Intrepid::FieldContainer<Scalar> &stiffness, Intrepid::FieldContainer<Scalar> &load, Intrepid::FieldContainer<GlobalIndexType> &interpretedDofIndices);
public:
  CondensedDofInterpreter(MeshPtr mesh, TIPPtr<Scalar> ip, TRHSPtr<Scalar> rhs, LagrangeConstraints* lagrangeConstraints, const set<int> &fieldIDsToExclude, bool storeLocalStiffnessMatrices);

  void addSolution(CondensedDofInterpreter* otherSolnDofInterpreter, Scalar weight);

  void reinitialize(); // clear stiffness matrices, etc., and rebuild global dof index map

  GlobalIndexType globalDofCount();
  set<GlobalIndexType> globalDofIndicesForPartition(PartitionIndexType rank);

  void interpretLocalData(GlobalIndexType cellID, const Intrepid::FieldContainer<Scalar> &localData,
                          Intrepid::FieldContainer<Scalar> &globalData, Intrepid::FieldContainer<GlobalIndexType> &globalDofIndices);

  // note that interpretLocalData stores the stiffness and load.  If you call this with data different from the actual stiffness and load
  // (as Solution does when imposing zero-mean constraints), it can be important to restore the actual stiffness and load afterward.
  // (see storeLoadForCell, storeStiffnessForCell)
  void interpretLocalData(GlobalIndexType cellID, const Intrepid::FieldContainer<Scalar> &localStiffnessData, const Intrepid::FieldContainer<Scalar> &localLoadData,
                          Intrepid::FieldContainer<Scalar> &globalStiffnessData, Intrepid::FieldContainer<Scalar> &globalLoadData, Intrepid::FieldContainer<GlobalIndexType> &globalDofIndices);

  virtual void interpretLocalCoefficients(GlobalIndexType cellID, const Intrepid::FieldContainer<Scalar> &localCoefficients, Epetra_MultiVector &globalCoefficients);

  void interpretLocalBasisCoefficients(GlobalIndexType cellID, int varID, int sideOrdinal, const Intrepid::FieldContainer<Scalar> &basisCoefficients,
                                       Intrepid::FieldContainer<Scalar> &globalCoefficients, Intrepid::FieldContainer<GlobalIndexType> &globalDofIndices);

  void interpretGlobalCoefficients(GlobalIndexType cellID, Intrepid::FieldContainer<Scalar> &localDofs, const Epetra_MultiVector &globalDofs);

  set<GlobalIndexType> globalDofIndicesForCell(GlobalIndexType cellID);

  GlobalIndexType condensedGlobalIndex(GlobalIndexType meshGlobalIndex); // meshGlobalIndex aka interpretedGlobalIndex

  bool varDofsAreCondensible(int varID, int sideOrdinal, DofOrderingPtr dofOrdering);

  void storeLoadForCell(GlobalIndexType cellID, const Intrepid::FieldContainer<Scalar> &load);
  void storeStiffnessForCell(GlobalIndexType cellID, const Intrepid::FieldContainer<Scalar> &stiffness);

  const Intrepid::FieldContainer<Scalar> & storedLocalLoadForCell(GlobalIndexType cellID);
  const Intrepid::FieldContainer<Scalar> & storedLocalStiffnessForCell(GlobalIndexType cellID);
};

extern template class CondensedDofInterpreter<double>;
}


#endif
