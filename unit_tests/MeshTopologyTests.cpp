//
//  MeshTopologyTests
//  Camellia
//
//  Created by Nate Roberts on 12/8/14.
//
//

#include "Teuchos_UnitTestHarness.hpp"

#include "CamelliaCellTools.h"
#include "MeshTopology.h"
#include "PoissonFormulation.h"

#include "MeshFactory.h"

using namespace Camellia;
using namespace Intrepid;

namespace
{
void testConstraints( MeshTopologyPtr mesh, unsigned entityDim, map<unsigned,pair<IndexType,unsigned> > &expectedConstraints, Teuchos::FancyOStream &out, bool &success)
{

  string meshName = "mesh";

  // check constraints for entities belonging to active cells
  set<unsigned> activeCells = mesh->getActiveCellIndices();

  for (set<unsigned>::iterator cellIt = activeCells.begin(); cellIt != activeCells.end(); cellIt++)
  {
    unsigned cellIndex = *cellIt;
    CellPtr cell = mesh->getCell(cellIndex);
    vector<unsigned> entitiesForCell = cell->getEntityIndices(entityDim);
    for (vector<unsigned>::iterator entityIt = entitiesForCell.begin(); entityIt != entitiesForCell.end(); entityIt++)
    {
      unsigned entityIndex = *entityIt;

      pair<IndexType,unsigned> constrainingEntity = mesh->getConstrainingEntity(entityDim, entityIndex);

      unsigned constrainingEntityIndex = constrainingEntity.first;
      unsigned constrainingEntityDim = constrainingEntity.second;
      if ((constrainingEntityIndex==entityIndex) && (constrainingEntityDim == entityDim))
      {
        // then we should expect not to have an entry in expectedConstraints:
        if (expectedConstraints.find(entityIndex) != expectedConstraints.end())
        {
          cout << "Expected entity constraint is not imposed in " << meshName << ".\n";
          cout << "Expected " << CamelliaCellTools::entityTypeString(entityDim) << " " << entityIndex << " to be constrained by ";
          cout << CamelliaCellTools::entityTypeString(expectedConstraints[entityIndex].second) << " " << expectedConstraints[entityIndex].first << endl;
          cout << CamelliaCellTools::entityTypeString(entityDim) << " " << entityIndex << " vertices:\n";
          mesh->printEntityVertices(entityDim, entityIndex);
          cout << CamelliaCellTools::entityTypeString(expectedConstraints[entityIndex].second) << " " << expectedConstraints[entityIndex].first << " vertices:\n";
          mesh->printEntityVertices(entityDim, expectedConstraints[entityIndex].first);
          success = false;
        }
      }
      else
      {
        if (expectedConstraints.find(entityIndex) == expectedConstraints.end())
        {
          cout << "Unexpected entity constraint is imposed in " << meshName << ".\n";

          string entityType;
          if (entityDim==0)
          {
            entityType = "Vertex ";
          }
          else if (entityDim==1)
          {
            entityType = "Edge ";
          }
          else if (entityDim==2)
          {
            entityType = "Face ";
          }
          else if (entityDim==3)
          {
            entityType = "Volume ";
          }
          string constrainingEntityType;
          if (constrainingEntityDim==0)
          {
            constrainingEntityType = "Vertex ";
          }
          else if (constrainingEntityDim==1)
          {
            constrainingEntityType = "Edge ";
          }
          else if (constrainingEntityDim==2)
          {
            constrainingEntityType = "Face ";
          }
          else if (constrainingEntityDim==3)
          {
            constrainingEntityType = "Volume ";
          }

          cout << entityType << entityIndex << " unexpectedly constrained by " << constrainingEntityType << constrainingEntityIndex << endl;
          cout << entityType << entityIndex << " vertices:\n";
          mesh->printEntityVertices(entityDim, entityIndex);
          cout << constrainingEntityType << constrainingEntityIndex << " vertices:\n";
          mesh->printEntityVertices(constrainingEntityDim, constrainingEntityIndex);
          success = false;
        }
        else
        {
          unsigned expectedConstrainingEntity = expectedConstraints[entityIndex].first;
          if (expectedConstrainingEntity != constrainingEntityIndex)
          {
            cout << "The constraining entity is not the expected one in " << meshName << ".\n";
            cout << "Expected " << CamelliaCellTools::entityTypeString(entityDim) << " " << entityIndex << " to be constrained by ";
            cout << CamelliaCellTools::entityTypeString(expectedConstraints[entityIndex].second) << " " << expectedConstrainingEntity;
            cout << "; was constrained by " << constrainingEntityIndex << endl;
            cout << CamelliaCellTools::entityTypeString(entityDim) << " " << entityIndex << " vertices:\n";
            mesh->printEntityVertices(entityDim, entityIndex);
            cout << CamelliaCellTools::entityTypeString(expectedConstraints[entityIndex].second) << " " << expectedConstrainingEntity << " vertices:\n";
            mesh->printEntityVertices(entityDim, expectedConstrainingEntity);
            cout << CamelliaCellTools::entityTypeString(constrainingEntityDim) << " " << constrainingEntityIndex << " vertices:\n";
            mesh->printEntityVertices(constrainingEntityDim, constrainingEntityIndex);
            success = false;
          }
        }
      }
    }
  }
}

TEUCHOS_UNIT_TEST( MeshTopology, InitialMeshEntitiesActiveCellCount)
{
  // one easy way to create a quad mesh topology is to use MeshFactory

  int spaceDim = 2;
  bool conformingTraces = false;
  PoissonFormulation formulation(spaceDim, conformingTraces);
  BFPtr bf = formulation.bf();

  int H1Order = 1;

  int horizontalElements = 1;
  int verticalElements = 1;
  double width = 1.0, height = 1.0;
  MeshPtr mesh = MeshFactory::quadMesh(bf, H1Order, width, height, horizontalElements, verticalElements); // creates a 1-cell mesh
  MeshTopologyPtr meshTopo = mesh->getTopology();

  unsigned sideDim = meshTopo->getSpaceDim() - 1;

  // uniform refinement --> none of the parent cells sides should have any active cells
  CellPtr cell = meshTopo->getCell(0);
  int sideCount = cell->getSideCount();
  for (int sideOrdinal=0; sideOrdinal<sideCount; sideOrdinal++)
  {
    IndexType sideEntityIndex = cell->entityIndex(sideDim, sideOrdinal);

    vector< pair<unsigned,unsigned> > activeCellInfoForSide = meshTopo->getActiveCellIndices(sideDim, sideEntityIndex);

    TEST_EQUALITY(activeCellInfoForSide.size(), 1);
    TEST_EQUALITY(meshTopo->getActiveCellCount(sideDim, sideEntityIndex), activeCellInfoForSide.size());
  }

  int vertexDim = 0;
  int vertexCount = cell->topology()->getNodeCount();
  for (int vertexOrdinal=0; vertexOrdinal<vertexCount; vertexOrdinal++)
  {
    IndexType vertexEntityIndex = cell->entityIndex(vertexDim, vertexOrdinal);

    vector< pair<unsigned,unsigned> > activeCellInfoForVertex = meshTopo->getActiveCellIndices(vertexDim, vertexEntityIndex);

    TEST_EQUALITY(activeCellInfoForVertex.size(), 1);
    TEST_EQUALITY(meshTopo->getActiveCellCount(vertexDim, vertexEntityIndex), activeCellInfoForVertex.size());
  }

  // repeat, but now with a 2x1 mesh:
  horizontalElements = 2;
  verticalElements = 1;
  mesh = MeshFactory::quadMesh(bf, H1Order, width, height, horizontalElements, verticalElements); // creates a 1-cell mesh
  meshTopo = mesh->getTopology();

  for (int d=0; d<=sideDim; d++)
  {
    int entityCount = meshTopo->getEntityCount(d);
    for (IndexType entityIndex=0; entityIndex < entityCount; entityIndex++)
    {
      vector< pair<unsigned,unsigned> > activeCellInfoForEntity = meshTopo->getActiveCellIndices(d, entityIndex);
      TEST_ASSERT((activeCellInfoForEntity.size() == 1) || (activeCellInfoForEntity.size() == 2));
      TEST_EQUALITY(meshTopo->getActiveCellCount(d, entityIndex), activeCellInfoForEntity.size());

      if (d==sideDim)
      {
        vector< IndexType > sides = meshTopo->getSidesContainingEntity(sideDim, entityIndex);
        TEST_EQUALITY(sides.size(), 1);
        TEST_EQUALITY(sides[0], entityIndex);
      }
    }
  }
}

TEUCHOS_UNIT_TEST( MeshTopology, DeactivateCellOnRefinement)
{
  // one easy way to create a quad mesh topology is to use MeshFactory

  int spaceDim = 2;
  bool conformingTraces = false;
  PoissonFormulation formulation(spaceDim, conformingTraces);
  BFPtr bf = formulation.bf();

  int H1Order = 1;
  MeshPtr mesh = MeshFactory::quadMesh(bf, H1Order);
  MeshTopologyPtr meshTopo = mesh->getTopology();

  set<GlobalIndexType> cellsToRefine;
  cellsToRefine.insert(0);

  mesh->hRefine(cellsToRefine, RefinementPattern::regularRefinementPatternQuad());

  unsigned sideDim = meshTopo->getSpaceDim() - 1;

  // uniform refinement --> none of the parent cells sides should have any active cells
  CellPtr cell = meshTopo->getCell(0);
  int sideCount = cell->getSideCount();
  for (int sideOrdinal=0; sideOrdinal<sideCount; sideOrdinal++)
  {
    IndexType sideEntityIndex = cell->entityIndex(sideDim, sideOrdinal);

    vector< pair<unsigned,unsigned> > activeCellInfoForSide = meshTopo->getActiveCellIndices(sideDim, sideEntityIndex);

    TEST_EQUALITY(activeCellInfoForSide.size(), 0);
  }
}

TEUCHOS_UNIT_TEST( MeshTopology, ConstrainingSideAncestryUniformMesh)
{
  // one easy way to create a quad mesh topology is to use MeshFactory

  int spaceDim = 2;
  bool conformingTraces = false;
  PoissonFormulation formulation(spaceDim, conformingTraces);
  BFPtr bf = formulation.bf();

  int H1Order = 1;
  MeshPtr mesh = MeshFactory::quadMesh(bf, H1Order);
  MeshTopologyPtr meshTopo = mesh->getTopology();

  set<GlobalIndexType> cellsToRefine;
  cellsToRefine.insert(0);

  mesh->hRefine(cellsToRefine, RefinementPattern::regularRefinementPatternQuad());

  // everything should be compatible at this point

  set<GlobalIndexType> activeCellIDs = mesh->cellIDsInPartition();

  unsigned sideDim = meshTopo->getSpaceDim() - 1;

  for (set<GlobalIndexType>::iterator cellIDIt = activeCellIDs.begin(); cellIDIt != activeCellIDs.end(); cellIDIt++)
  {
    CellPtr cell = meshTopo->getCell(*cellIDIt);
    int sideCount = cell->getSideCount();
    for (int sideOrdinal=0; sideOrdinal<sideCount; sideOrdinal++)
    {
      IndexType sideEntityIndex = cell->entityIndex(sideDim, sideOrdinal);

      vector< pair<unsigned,unsigned> > constrainingSideAncestry = meshTopo->getConstrainingSideAncestry(sideEntityIndex);

      TEST_EQUALITY(constrainingSideAncestry.size(), 0);
    }
  }
}

TEUCHOS_UNIT_TEST(MeshTopology, GetEntityGeneralizedParent_LineRefinement)
{
  int spaceDim = 1;
  MeshTopologyPtr meshTopo = Teuchos::rcp( new MeshTopology(spaceDim) );

  CellTopoPtr lineTopo = CellTopology::line();

  vector< vector<double> > lineVertices;
  double xLeft = 0.0;
  double xRight = 1.0;
  lineVertices.push_back(vector<double>(1, xLeft));
  lineVertices.push_back(vector<double>(1, xRight));

  meshTopo->addCell(lineTopo, lineVertices); // cell from 0 to 1

  int cellIndex = 0;
  CellPtr cell = meshTopo->getCell(cellIndex);

  RefinementPatternPtr refPattern = RefinementPattern::regularRefinementPattern(lineTopo);

  meshTopo->refineCell(cellIndex, refPattern);

  double xMiddle = (xLeft + xRight) / 2.0;

  unsigned vertexOrdinal;
  meshTopo->getVertexIndex(vector<double>(1,xMiddle), vertexOrdinal);

  int vertexDim = 0, lineDim = 1;
  pair<IndexType, unsigned> generalizedParent = meshTopo->getEntityGeneralizedParent(vertexDim, vertexOrdinal);

  TEST_EQUALITY(generalizedParent.first, cellIndex);
  TEST_EQUALITY(generalizedParent.second, lineDim);

  // try same for a child *cell*
  CellPtr childCell = meshTopo->getCell(cellIndex)->children()[0];
  generalizedParent = meshTopo->getEntityGeneralizedParent(spaceDim, childCell->cellIndex());

  TEST_EQUALITY(generalizedParent.first, cellIndex);
  TEST_EQUALITY(generalizedParent.second, lineDim);
}

TEUCHOS_UNIT_TEST(MeshTopology, GetRootMeshTopology)
{
  int k = 1;
  int H1Order = k + 1;
  int delta_k = 1;

  int spaceDim = 2;
  bool conformingTraces = false;
  PoissonFormulation formulation(spaceDim, conformingTraces);
  BFPtr bf = formulation.bf();

  BFPtr bilinearForm = bf;

  int rootMeshNumCells = 2;
  double width = 1.0;

  vector<double> dimensions;
  vector<int> elementCounts;
  vector<double> x0(spaceDim,0.0);
  for (int d=0; d<spaceDim; d++)
  {
    dimensions.push_back(width);
    elementCounts.push_back(rootMeshNumCells);
  }
  MeshPtr originalMesh = MeshFactory::rectilinearMesh(bf, dimensions, elementCounts, H1Order, delta_k, x0);
  MeshPtr mesh = MeshFactory::rectilinearMesh(bf, dimensions, elementCounts, H1Order, delta_k, x0);

  // get a sample cellTopo:
  CellTopoPtr cellTopo = mesh->getTopology()->getCell(0)->topology();
  RefinementPatternPtr refPattern = RefinementPattern::regularRefinementPattern(cellTopo);

  int numUniformRefinements = 3;
  for (int i=0; i<numUniformRefinements; i++)
  {
    set<GlobalIndexType> activeCellIDs = mesh->getActiveCellIDs();
    mesh->RefinementObserver::hRefine(activeCellIDs, refPattern);
  }

  MeshTopologyPtr rootMeshTopology = mesh->getTopology()->getRootMeshTopology();
  MeshTopologyPtr originalMeshTopology = originalMesh->getTopology();

  TEST_EQUALITY(rootMeshTopology->cellCount(), originalMeshTopology->cellCount());

  std::set<IndexType> rootCellIndices = rootMeshTopology->getActiveCellIndices();
  std::set<IndexType> originalCellIndices = originalMeshTopology->getActiveCellIndices();

  // compare sets:
  std::set<IndexType>::iterator rootCellIt = rootCellIndices.begin(), originalCellIt = originalCellIndices.begin();
  for (int i=0; i<rootMeshTopology->cellCount(); i++)
  {
    TEST_EQUALITY(*rootCellIt, *originalCellIt);
    rootCellIt++;
    originalCellIt++;
  }

  for (std::set<IndexType>::iterator cellIDIt = rootCellIndices.begin(); cellIDIt != rootCellIndices.end(); cellIDIt++)
  {
    CellPtr originalCell = originalMeshTopology->getCell(*cellIDIt);
    CellPtr rootCell = rootMeshTopology->getCell(*cellIDIt);

    vector<unsigned> originalVertexIndices = originalCell->vertices();
    vector<unsigned> rootVertexIndices = rootCell->vertices();

    TEST_EQUALITY(originalVertexIndices.size(), rootVertexIndices.size());

    TEST_COMPARE_ARRAYS(originalVertexIndices, rootVertexIndices);

    for (int i=0; i<originalVertexIndices.size(); i++)
    {
      vector<double> originalVertex = originalMeshTopology->getVertex(originalVertexIndices[i]);
      vector<double> rootVertex = rootMeshTopology->getVertex(rootVertexIndices[i]);

      TEST_COMPARE_FLOATING_ARRAYS(originalVertex, rootVertex, 1e-15);
    }
  }
}

TEUCHOS_UNIT_TEST( MeshTopology, UnrefinedSpaceTimeMeshTopologyIsUnconstrained )
{
  // to start with, just a single-cell space-time MeshTopology
  CellTopoPtr spaceTopo = CellTopology::line();
  int spaceDim = spaceTopo->getDimension();
  MeshTopologyPtr spatialMeshTopo = Teuchos::rcp( new MeshTopology(spaceDim) );

  FieldContainer<double> cellNodes(spaceTopo->getNodeCount(),spaceTopo->getDimension());
  CamelliaCellTools::refCellNodesForTopology(cellNodes, spaceTopo);
  vector<vector<double>> cellVerticesVector;
  CamelliaCellTools::pointsVectorFromFC(cellVerticesVector, cellNodes);
  spatialMeshTopo->addCell(spaceTopo, cellVerticesVector);

  double t0 = 0.0, t1 = 1.0;
  MeshTopologyPtr spaceTimeMeshTopo = MeshFactory::spaceTimeMeshTopology(spatialMeshTopo, t0, t1);

  for (int d=0; d<spaceTimeMeshTopo->getSpaceDim(); d++)
  {
    map<unsigned,pair<IndexType,unsigned> > expectedConstraints;
    testConstraints(spaceTimeMeshTopo, d, expectedConstraints, out, success);
  }
}
} // namespace
