//
//  Cell.h
//  Camellia-debug
//
//  Created by Nate Roberts on 1/22/14.
//
//

#ifndef __Camellia_debug__Cell__
#define __Camellia_debug__Cell__

#include <iostream>
#include "Shards_CellTopology.hpp"
#include "Teuchos_RCP.hpp"
#include "RefinementPattern.h"

typedef Teuchos::RCP<shards::CellTopology> CellTopoPtrLegacy;
class MeshTopology;

using namespace std;

// "cells" are geometric entities -- they do not define any kind of basis
// "elements" are cells endowed with a (local) functional discretization
class Cell {
  unsigned _cellIndex;
  CellTopoPtrLegacy _cellTopo;
  vector< unsigned > _vertices;
  vector< map< unsigned, unsigned > > _subcellPermutations; // permutation to get from local ordering to the canonical one
  
  MeshTopology* _meshTopo;
  
  // for parents:
  vector< Teuchos::RCP< Cell > > _children;
  RefinementPatternPtr _refPattern;
  
  // for children:
  Teuchos::RCP<Cell> _parent; // doesn't own memory (avoid circular reference issues)
  
  //neighbors:
  vector< pair<GlobalIndexType, unsigned> > _neighbors; // cellIndex, neighborSideIndex (which may not refer to the same side)
  /* rules for neighbors:
     - hanging node sides point to the constraining neighbor (which may not be active)
     - cells with broken neighbors point to their peer, the ancestor of the active neighbors
   */
public:
  Cell(CellTopoPtrLegacy cellTopo, const vector<unsigned> &vertices, const vector< map< unsigned, unsigned > > &subcellPermutations,
       IndexType cellIndex, MeshTopology* meshTopo);

  Teuchos::RCP<Cell> ancestralCellForSubcell(unsigned subcdim, unsigned subcord);
  
  unsigned ancestralPermutationForSubcell(unsigned subcdim, unsigned subcord);
//  unsigned ancestralPermutationForSideSubcell(unsigned sideOrdinal, unsigned subcdim, unsigned subcord);
  
  pair<unsigned, unsigned> ancestralSubcellOrdinalAndDimension(unsigned subcdim, unsigned subcord); // (subcord, subcdim) into the cell returned by ancestralCellForSubcell
  
  vector<unsigned> boundarySides();
  
  IndexType cellIndex();
  const vector< Teuchos::RCP< Cell > > &children();
  void setChildren(vector< Teuchos::RCP< Cell > > children);
  vector<IndexType> getChildIndices();
  vector< pair<IndexType, unsigned> > childrenForSide(unsigned sideOrdinal);
  vector< pair< IndexType, unsigned> > getDescendantsForSide(int sideOrdinal, bool leafNodesOnly = true);
  unsigned entityIndex(unsigned subcdim, unsigned subcord);
  vector<unsigned> getEntityIndices(unsigned subcdim);
  
  vector<unsigned> getEntityVertexIndices(unsigned subcdim, unsigned subcord);
  
  Teuchos::RCP<Cell> getParent();
  void setParent(Teuchos::RCP<Cell> parent);
  
  bool isBoundary(unsigned sideOrdinal);
  bool isParent();
  
  unsigned childOrdinal(IndexType childIndex);
  unsigned findSubcellOrdinal(unsigned subcdim, IndexType subcEntityIndex); // this is pretty brute force right now
  unsigned findSubcellOrdinalInSide(unsigned subcdim, IndexType subcEntityIndex, unsigned sideOrdinal); // this is pretty brute force right now
  
  MeshTopology* meshTopology();
  
  bool ownsSide(unsigned sideOrdinal);
  
  RefinementPatternPtr refinementPattern();
  void setRefinementPattern(RefinementPatternPtr refPattern);
  
  RefinementBranch refinementBranchForSide(unsigned sideOrdinal);
  
  RefinementBranch refinementBranchForSubcell(unsigned subcdim, unsigned subcord);
  
  unsigned subcellPermutation(unsigned d, unsigned scord);
  
  unsigned sideSubcellPermutation(unsigned sideOrdinal, unsigned sideSubcdim, unsigned sideSubcord);
  
  CellTopoPtrLegacy topology();
  
  pair<GlobalIndexType, unsigned> getNeighbor(unsigned sideOrdinal); // (neighborCellIndex, neighborSideOrdinal)
  void setNeighbor(unsigned sideOrdinal, GlobalIndexType neighborCellIndex, unsigned neighborSideOrdinal);
  
  const vector< unsigned > &vertices();
};

typedef Teuchos::RCP<Cell> CellPtr;

#endif /* defined(__Camellia_debug__Cell__) */
