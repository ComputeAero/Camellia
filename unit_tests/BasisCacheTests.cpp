//
//  BasisCacheTests.cpp
//  Camellia
//
//  Created by Nate Roberts on 12/11/14.
//
//

#include "Teuchos_UnitTestHarness.hpp"

#include "BasisCache.h"

#include "CamelliaCellTools.h"
#include "CellTopology.h"

using namespace Camellia;

namespace {
  FieldContainer<double> unitCubeNodes() {
    CellTopoPtr hex = CellTopology::hexahedron();
    // for now, let's use the reference cell.  (Jacobian should be the identity.)
    FieldContainer<double> refCubePoints(hex->getNodeCount(), hex->getDimension());
    CamelliaCellTools::refCellNodesForTopology(refCubePoints, hex);

    FieldContainer<double> cubePoints = refCubePoints;
    for (int i=0; i<cubePoints.size(); i++) {
      cubePoints[i] = (cubePoints[i]+1) / 2;
    }
    return cubePoints;
  }
  
  TEUCHOS_UNIT_TEST( BasisCache, LineCubature )
  {
    int cubDegree = 1;
    double x0 = 0, x1 = 1;
    BasisCachePtr basisCache = BasisCache::basisCache1D(x0, x1, cubDegree);
    
    const FieldContainer<double>* cubWeights = &basisCache->getCubatureWeights();
    const FieldContainer<double>* physicalCubaturePoints = &basisCache->getPhysicalCubaturePoints();
    
    // try a line m x + b; the integral on (0,1) should be m / 2 + b.
    double m = 2.0, b = -1;
    double expected_integral = m / 2.0 + b;
    
    double integral = 0;
    for (int i=0; i<cubWeights->size(); i++) {
      double x = (*physicalCubaturePoints)(0,i,0);
      double f_val = m * x + b;
      integral += (*cubWeights)(i) * f_val;
    }
    
    TEST_FLOATING_EQUALITY(expected_integral, integral, 1e-15);
  }
  
  TEUCHOS_UNIT_TEST(BasisCache, Jacobian3D)
  {
    
    CellTopoPtr hex = CellTopology::hexahedron();
    // for now, let's use the reference cell.  (Jacobian should be the identity.)
    FieldContainer<double> refCubePoints(hex->getNodeCount(), hex->getDimension());
    CamelliaCellTools::refCellNodesForTopology(refCubePoints, hex);
    
    // small upgrade: unit cube
    //  FieldContainer<double> cubePoints = unitCubeNodes();
    
    int numCells = 1;
    refCubePoints.resize(numCells,8,3); // first argument is cellIndex; we'll just have 1
    
    Teuchos::RCP<shards::CellTopology> hexTopoPtr;
    hexTopoPtr = Teuchos::rcp(new shards::CellTopology(shards::getCellTopologyData<shards::Hexahedron<8> >() ));
    
    int spaceDim = 3;
    int cubDegree = 2;
    
    shards::CellTopology hexTopo(shards::getCellTopologyData<shards::Hexahedron<8> >() );
    
    FieldContainer<double> physicalCellNodes = refCubePoints;
    physicalCellNodes.resize(numCells,hexTopo.getVertexCount(),spaceDim);
    BasisCache hexBasisCache( physicalCellNodes, hexTopo, cubDegree);
    
    FieldContainer<double> referenceToReferenceJacobian = hexBasisCache.getJacobian();
    int numPoints = referenceToReferenceJacobian.dimension(1);
    
    FieldContainer<double> kronecker(numCells,numPoints,spaceDim,spaceDim);
    for (int cellIndex=0; cellIndex<numCells; cellIndex++) {
      for (int ptIndex=0; ptIndex<numPoints; ptIndex++) {
        for (int d1=0; d1<spaceDim; d1++) {
          for (int d2=0; d2<spaceDim; d2++) {
            kronecker(cellIndex,ptIndex,d1,d2) = (d1==d2) ? 1 : 0;
          }
        }
      }
    }
    
    double maxDiff = 0;
    double tol = 1e-15;
    for (int valOrdinal=0; valOrdinal<referenceToReferenceJacobian.size(); valOrdinal++) {
      double diff = abs(kronecker[valOrdinal]-referenceToReferenceJacobian[valOrdinal]);
      TEST_ASSERT( diff < tol );
      maxDiff = max(maxDiff,diff);
    }
    TEST_ASSERT(maxDiff < tol);
    
    if (maxDiff >= tol) {
      cout << "identity map doesn't have identity Jacobian.\n";
      cout << "maxDiff = " << maxDiff << endl;
      
      cout << "referenceToReferenceJacobian:\n" << referenceToReferenceJacobian ;
    }

    
//    TEST_COMPARE_FLOATING_ARRAYS(kronecker, referenceToReferenceJacobian, 1e-15);
    
//    if (! fcsAgree(kronecker, referenceToReferenceJacobian, tol, maxDiff) ) {
//      cout << "identity map doesn't have identity Jacobian.\n";
//      cout << "maxDiff = " << maxDiff << endl;
//      success = false;
//    }
    
    physicalCellNodes = unitCubeNodes();
    physicalCellNodes.resize(numCells, hexTopo.getVertexCount(), hexTopo.getDimension());
    hexBasisCache = BasisCache( physicalCellNodes, hexTopo, cubDegree );
    FieldContainer<double> halfKronecker = kronecker;
    BilinearForm::multiplyFCByWeight(halfKronecker, 0.5);
    
    maxDiff = 0;
    FieldContainer<double> referenceToUnitCubeJacobian = hexBasisCache.getJacobian();
    for (int valOrdinal=0; valOrdinal<referenceToUnitCubeJacobian.size(); valOrdinal++) {
      double diff = abs(halfKronecker[valOrdinal]-referenceToUnitCubeJacobian[valOrdinal]);
      TEST_ASSERT( diff < tol );
      maxDiff = max(maxDiff,diff);
    }
    TEST_ASSERT(maxDiff < tol);
    
    if (maxDiff >= tol) {
      cout << "map to unit cube doesn't have the expected half-identity Jacobian.\n";
      cout << "maxDiff = " << maxDiff << endl;
    }
    
//    TEST_COMPARE_FLOATING_ARRAYS(halfKronecker, referenceToUnitCubeJacobian, 1e-15);

//    if (! fcsAgree(halfKronecker, referenceToUnitCubeJacobian, tol, maxDiff) ) {
//      cout << "map to unit cube doesn't have the expected half-identity Jacobian.\n";
//      cout << "maxDiff = " << maxDiff << endl;
//      success = false;
//    }
  }
  
  TEUCHOS_UNIT_TEST( BasisCache, SetRefCellPoints )
  {
    double tol = 1e-15;
      
    // test setting ref points on the side cache
    CellTopoPtr cellTopo = CellTopology::quad();
    int numSides = cellTopo->getSideCount();
    
    int cubDegree = 2;
    shards::CellTopology shardsTopo = cellTopo->getShardsTopology();
    BasisCachePtr basisCache = Teuchos::rcp( new BasisCache(shardsTopo, cubDegree, true) ); // true: create side cache, too
    
    FieldContainer<double> unitQuadNodes(1,cellTopo->getNodeCount(), cellTopo->getDimension());
    unitQuadNodes(0,0,0) = 0.0;
    unitQuadNodes(0,0,1) = 0.0;
    unitQuadNodes(0,1,0) = 1.0;
    unitQuadNodes(0,1,1) = 0.0;
    unitQuadNodes(0,2,0) = 1.0;
    unitQuadNodes(0,2,1) = 1.0;
    unitQuadNodes(0,3,0) = 0.0;
    unitQuadNodes(0,3,1) = 1.0;
    
    basisCache->setPhysicalCellNodes(unitQuadNodes, vector<GlobalIndexType>(), true);
    
    for (int sideOrdinal=0; sideOrdinal < numSides; sideOrdinal++)
    {
      BasisCachePtr sideBasisCache = basisCache->getSideBasisCache(sideOrdinal);
      FieldContainer<double> refCellPoints = sideBasisCache->getRefCellPoints();
      FieldContainer<double> physicalCubaturePointsExpected = sideBasisCache->getPhysicalCubaturePoints();
      //    cout << "side " << sideIndex << " ref cell points:\n" << refCellPoints;
      //    cout << "side " << sideIndex << " physCubature points:\n" << physicalCubaturePointsExpected;
      sideBasisCache->setRefCellPoints(refCellPoints);
      FieldContainer<double> physicalCubaturePointsActual = sideBasisCache->getPhysicalCubaturePoints();
      
      double maxDiff = 0;
      for (int valOrdinal=0; valOrdinal<physicalCubaturePointsActual.size(); valOrdinal++) {
        double diff = abs(physicalCubaturePointsExpected[valOrdinal]-physicalCubaturePointsActual[valOrdinal]);
        TEST_ASSERT( diff < tol );
        double maxDiff = max(maxDiff,diff);
      }
      TEST_ASSERT(maxDiff < tol);
      
      if (maxDiff >= tol) {
        cout << "After resetting refCellPoints, physical cubature points are different in side basis cache.\n";
        cout << "maxDiff = " << maxDiff << endl;
      }

    }
      // TODO: test quad
      
      // test hexahedron
      int numCells = 1;
      int numPoints = 1;
      int spaceDim = 3;
    
      shards::CellTopology hexTopo(shards::getCellTopologyData<shards::Hexahedron<8> >() );
      FieldContainer<double> physicalCellNodes = unitCubeNodes();
      physicalCellNodes.resize(numCells,hexTopo.getVertexCount(),spaceDim);
      BasisCache hexBasisCache( physicalCellNodes, hexTopo, cubDegree);
      
      FieldContainer<double> refCellPointsHex(numPoints,spaceDim);
      refCellPointsHex(0,0) = 0.0;
      refCellPointsHex(0,1) = 0.0;
      refCellPointsHex(0,2) = 0.0;
      
      FieldContainer<double> physicalPointsExpected(numCells,numPoints,spaceDim);
      physicalPointsExpected(0,0,0) = 0.5;
      physicalPointsExpected(0,0,1) = 0.5;
      physicalPointsExpected(0,0,2) = 0.5;
      
      hexBasisCache.setRefCellPoints(refCellPointsHex);
      
      FieldContainer<double> physicalPointsActual = hexBasisCache.getPhysicalCubaturePoints();
    
    double maxDiff = 0;
    for (int valOrdinal=0; valOrdinal<physicalPointsActual.size(); valOrdinal++) {
      double diff = abs(physicalPointsExpected[valOrdinal]-physicalPointsActual[valOrdinal]);
      TEST_ASSERT( diff < tol );
      double maxDiff = max(maxDiff,diff);
    }
    TEST_ASSERT(maxDiff < tol);
    
    if (maxDiff >= tol) {
      cout << "physical points don't match expected for hexahedron.";
      cout << "maxDiff = " << maxDiff << endl;
    }
  }
} // namespace