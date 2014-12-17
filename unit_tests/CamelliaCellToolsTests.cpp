//
//  CamelliaCellToolsTests.cpp
//  Camellia
//
//  Created by Nate Roberts on 11/18/14.
//
//

#include "Teuchos_UnitTestHarness.hpp"
#include "Teuchos_UnitTestHelpers.hpp"

#include "Intrepid_FieldContainer.hpp"

#include "Intrepid_CellTools.hpp"

#include "Shards_CellTopology.hpp"

#include "CellTopology.h"

#include "CamelliaCellTools.h"

using namespace Camellia;

namespace {
  vector< CellTopoPtr > getShardsTopologies() {
    vector< CellTopoPtr > shardsTopologies;

    shardsTopologies.push_back(CellTopology::point());
    shardsTopologies.push_back(CellTopology::line());
    shardsTopologies.push_back(CellTopology::quad());
    shardsTopologies.push_back(CellTopology::triangle());
    shardsTopologies.push_back(CellTopology::hexahedron());
    //  shardsTopologies.push_back(CellTopology::tetrahedron()); // tetrahedron not yet supported by permutation
    return shardsTopologies;
  }
  
  TEUCHOS_UNIT_TEST( CamelliaCellTools, MapToPhysicalFrame)
  {
    // particularly important to try it out on some topologies defined in terms of tensor products
    // main use case is with tensorial degree equal to 1 (for space-time), but might be worth trying with tensorial degree 2 and 3, too
    
    // to begin, just a very simple test that *nodes* are mapped appropriately

    std::vector< CellTopoPtr > shardsTopologies = getShardsTopologies();
    
    for (int tensorialDegree=0; tensorialDegree<2; tensorialDegree++) {
      for (int topoOrdinal = 0; topoOrdinal < shardsTopologies.size(); topoOrdinal++) {
        CellTopoPtr shardsTopo = shardsTopologies[topoOrdinal];
        
        if (shardsTopo->getDimension() == 0) continue; // don't bother testing point topology
        
        CellTopoPtr topo = CellTopology::cellTopology(shardsTopo->getShardsTopology(), tensorialDegree);
        
        FieldContainer<double> refNodes(topo->getNodeCount(),topo->getDimension());
        
        CamelliaCellTools::refCellNodesForTopology(refNodes, topo);
        FieldContainer<double> physicalNodes(1,topo->getNodeCount(),topo->getDimension());
        
        // to make the physical cell, multiply node by 1/2/3 in x/y/z and add 1/4/9
        for (int node=0; node < topo->getNodeCount(); node++) {
          for (int d=0; d<topo->getDimension(); d++) {
            physicalNodes(0,node,d) = refNodes(node,d) * d + d * d;
//            physicalNodes(0,node,d) = refNodes(node,d);
          }
        }
//        cout << "WARNING: in CamelliaCellToolsTests, temporarily replaced the physical nodes with the reference nodes (i.e. just testing the identity mapping right now).\n";
        
        FieldContainer<double> mappedNodes(1,topo->getNodeCount(),topo->getDimension());
        CamelliaCellTools::mapToPhysicalFrame(mappedNodes, refNodes, physicalNodes, topo);
        
        TEST_COMPARE_FLOATING_ARRAYS(physicalNodes, mappedNodes, 1e-15);
        
      }
    }
  }
  
  TEUCHOS_UNIT_TEST( CamelliaCellTools, SetJacobianForSimpleShardsTopologies )
  {
    std::vector< CellTopoPtr > shardsTopologies = getShardsTopologies();
    
    for (int topoOrdinal = 0; topoOrdinal < shardsTopologies.size(); topoOrdinal++) {
      CellTopoPtr topo = shardsTopologies[topoOrdinal];
      
      int spaceDim = topo->getDimension();
      
      if (spaceDim == 0) continue; // don't bother testing point topology
      
      FieldContainer<double> refCellNodes(topo->getNodeCount(),spaceDim);
      
      CamelliaCellTools::refCellNodesForTopology(refCellNodes, topo);
      
      FieldContainer<double> cellNodes = refCellNodes;
      cellNodes.resize(1,cellNodes.dimension(0),cellNodes.dimension(1));
      int numPoints = refCellNodes.dimension(0);
      FieldContainer<double> jacobianCamellia(1,numPoints,spaceDim,spaceDim);
      FieldContainer<double> jacobianShards(1,numPoints,spaceDim,spaceDim);
      CamelliaCellTools::setJacobian(jacobianCamellia, refCellNodes, cellNodes, topo);
      Intrepid::CellTools<double>::setJacobian(jacobianShards, refCellNodes, cellNodes, topo->getShardsTopology());
      
      TEST_COMPARE_FLOATING_ARRAYS(jacobianShards, jacobianCamellia, 1e-15);
          
      for (int i=0; i<cellNodes.size(); i++) {
        cellNodes[i] /= 2.0;
      }
      CamelliaCellTools::setJacobian(jacobianCamellia, refCellNodes, cellNodes, topo);
      Intrepid::CellTools<double>::setJacobian(jacobianShards, refCellNodes, cellNodes, topo->getShardsTopology());
      
      TEST_COMPARE_FLOATING_ARRAYS(jacobianShards, jacobianCamellia, 1e-15);
    }
  }
  
  TEUCHOS_UNIT_TEST( CamelliaCellTools, PermutedReferenceCellPoints )
  {
    // to begin, just a very simple test that *nodes* are permuted appropriately
    
    shards::CellTopology quad_4(shards::getCellTopologyData<shards::Quadrilateral<4> >() );

    FieldContainer<double> refPoints(quad_4.getNodeCount(),quad_4.getDimension());
    FieldContainer<double> quadNodesPermuted(quad_4.getNodeCount(),quad_4.getDimension());
    
    int permutationCount = quad_4.getNodePermutationCount();
    
    CamelliaCellTools::refCellNodesForTopology(refPoints, quad_4);
    
    FieldContainer<double> permutedRefPoints(quad_4.getNodeCount(),quad_4.getDimension());
    
    for (int permutation=0; permutation<permutationCount; permutation++) {
      CamelliaCellTools::refCellNodesForTopology(quadNodesPermuted, quad_4, permutation);
      
      CamelliaCellTools::permutedReferenceCellPoints(quad_4, permutation, refPoints, permutedRefPoints);
      
      // expect permutedRefPoints = quadNodesPermuted
      
      for (int nodeOrdinal=0; nodeOrdinal < quadNodesPermuted.dimension(0); nodeOrdinal++) {
        for (int d=0; d<quadNodesPermuted.dimension(1); d++) {
          TEST_FLOATING_EQUALITY(quadNodesPermuted(nodeOrdinal,d), permutedRefPoints(nodeOrdinal,d), 1e-15);
        }
      }
    }
  }
  
  TEUCHOS_UNIT_TEST( CamelliaCellTools, RefCellPointsForTopology ) {
    // just check that the version that takes a Camellia CellTopology matches
    // the one that takes a shards CellTopology
    
    std::vector< CellTopoPtr > shardsTopologies = getShardsTopologies();
    
    for (int topoOrdinal = 0; topoOrdinal < shardsTopologies.size(); topoOrdinal++) {
      CellTopoPtr topo = shardsTopologies[topoOrdinal];
      
      if (topo->getDimension() == 0) continue; // don't bother testing point topology
      
      FieldContainer<double> refCellNodesShards(topo->getNodeCount(),topo->getDimension());
      FieldContainer<double> refCellNodesCamellia(topo->getNodeCount(),topo->getDimension());
      int permutationCount;
      if (topo->getDimension() <= 2) {
        permutationCount = topo->getNodePermutationCount();
      } else {
        permutationCount = 1; // shards doesn't provide permutations for 3D objects
      }
      
      for (int permutation=0; permutation<permutationCount; permutation++) {
        CamelliaCellTools::refCellNodesForTopology(refCellNodesShards, topo->getShardsTopology(), permutation);
        CamelliaCellTools::refCellNodesForTopology(refCellNodesCamellia, topo, permutation);
        
        TEST_COMPARE_FLOATING_ARRAYS(refCellNodesShards, refCellNodesCamellia, 1e-15);
        
        if (!success) {
          cout << "Test failure (set breakpoint here) \n";
        }
      }
    }
  }
  
  TEUCHOS_UNIT_TEST( CamelliaCellTools, RefCellPointsForTensorTopology ) {
    // just check that the version that takes a Camellia CellTopology of tensorial degree 1 matches:
    // shardsTopo refNodes + t = -1
    // shardsTopo refNodes + t =  1
    // in that order...
    
    std::vector< CellTopoPtr > shardsTopologies = getShardsTopologies();
    
    for (int topoOrdinal = 0; topoOrdinal < shardsTopologies.size(); topoOrdinal++) {
      CellTopoPtr shardsTopo = shardsTopologies[topoOrdinal];
      
      int tensorialDegree = 1;
      CellTopoPtr topo = CellTopology::cellTopology(shardsTopo->getShardsTopology(), tensorialDegree);
      
      FieldContainer<double> refCellNodesShards(shardsTopo->getNodeCount(),shardsTopo->getDimension());
      FieldContainer<double> refCellNodesCamellia(topo->getNodeCount(),topo->getDimension());
      
      int permutation=0;
      CamelliaCellTools::refCellNodesForTopology(refCellNodesShards, shardsTopo, permutation);
      CamelliaCellTools::refCellNodesForTopology(refCellNodesCamellia, topo, permutation);
      
      FieldContainer<double> refCellNodesExpected(topo->getNodeCount(),topo->getDimension());
      
      if (shardsTopo->getDimension() == 0) {
       // special case: point x line = line
        CellTopoPtr line = CellTopology::line();
        CamelliaCellTools::refCellNodesForTopology(refCellNodesExpected, line);
      } else {
        for (int node=0; node<topo->getNodeCount(); node++) {
          int shardsNode = node % shardsTopo->getNodeCount();
          for (int d=0; d<topo->getDimension(); d++) {
            if (d < shardsTopo->getDimension())
              refCellNodesExpected(node,d) = refCellNodesShards(shardsNode,d);
            else
              if (shardsNode == node) refCellNodesExpected(node,d) = -1;
              else refCellNodesExpected(node,d) = 1;
          }
        }
      }
      
      TEST_COMPARE_FLOATING_ARRAYS(refCellNodesExpected, refCellNodesCamellia, 1e-15);
      
      if (!success) {
        cout << "Test failure (set breakpoint here) \n";
      }
    }
  }
} // namespace