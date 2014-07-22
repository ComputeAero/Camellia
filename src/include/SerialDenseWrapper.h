//
//  SerialDenseWrapper.h
//  Camellia-debug
//
//
//

#ifndef SerialDenseWrapper_h
#define SerialDenseWrapper_h

#include "Intrepid_FieldContainer.hpp"

#include "Epetra_SerialDenseMatrix.h"
#include "Epetra_SerialDenseVector.h"
#include "Epetra_SerialDenseSolver.h"
#include "Epetra_DataAccess.h"

#include "Epetra_SerialSymDenseMatrix.h"
#include "Epetra_SerialSpdDenseSolver.h"

#include "Teuchos_LAPACK.hpp"
#include "Teuchos_SerialDenseMatrix.hpp"
#include "Teuchos_SerialDenseVector.hpp"
#include "Teuchos_SerialQRDenseSolver.hpp"

class SerialDenseWrapper {
  static void transposeSquareMatrix(FieldContainer<double> &A) {
    int rows = A.dimension(0), cols = A.dimension(1);
    TEUCHOS_TEST_FOR_EXCEPTION(rows != cols, std::invalid_argument, "matrix not square");
    for (int i=0; i<rows; i++) {
      for (int j=0; j<i; j++) {
        double temp = A(i,j);
        A(i,j) = A(j,i);
        A(j,i) = temp;
      }
    }
  }
  static Epetra_SerialDenseMatrix convertFCToSDM(const FieldContainer<double> &A, Epetra_DataAccess CV = ::Copy){
    //  FC is row major, SDM expects column major data, so the roles of rows and columns get swapped
    // distance between rows in FC is the column length (dimension 1)
    int n = A.dimension(0);
    int m = A.dimension(1);
    double *firstEntry = (double *) &A[0]; // a bit dangerous: cast away the const.  Not dangerous if we're doing Copy, of course.
    Epetra_SerialDenseMatrix Amatrix(CV,firstEntry,m,m,n);
    return Amatrix;
  }
  static void convertSDMToFC(FieldContainer<double>& A_fc, const Epetra_SerialDenseMatrix &A){
    int n = A.M();    
    int m = A.N();
    Teuchos::Array<int> dim(2);
    dim[0] = m;
    dim[1] = n;
    double * firstEntry = (double *) &A(0,0); // again, casting away the const.  OK since we copy below.
    A_fc = FieldContainer<double>(dim,firstEntry,true); // true: copy
  }
  
  static void transposeMatrix(FieldContainer<double> &A) {
    int n = A.dimension(0);
    int m = A.dimension(1);
    if (n==m) {
      transposeSquareMatrix(A);
    } else {
      FieldContainer<double> A_copy = A;
      A.resize(m,n);
      for (int i=0; i<n; i++) {
        for (int j=0; j<m; j++) {
          A(j,i) = A_copy(i,j);
        }
      }
    }
  }
public:
  // gives X = scalarA*A+scalarB*B (overwrites A)
  static void add(FieldContainer<double> &X, const FieldContainer<double> &A, const FieldContainer<double> &B, double scalarA = 1.0, double scalarB = 1.0){
    Epetra_SerialDenseMatrix AMatrix = convertFCToSDM(A);
    Epetra_SerialDenseMatrix BMatrix = convertFCToSDM(B);
    AMatrix.Scale(scalarA);
    BMatrix.Scale(scalarB);
    AMatrix += BMatrix;
    convertSDMToFC(X,AMatrix);
  }

  static double dot(const FieldContainer<double> &a, const FieldContainer<double> &b) {
    if (((a.rank() != 1) && (a.rank() != 2)) || ((b.rank() != 1) && (b.rank() != 2))) {
      cout << "a and b must have rank 1 or 2; if rank 2, one of the two ranks' dimensions must be 1.  (I.e. a and b must both be vectors.)\n";
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument, "a and b must have rank 1 or 2; if rank 2, one of the two ranks' dimensions must be 1.  (I.e. a and b must both be vectors.)");
    }
    if (a.rank()==2) {
      if ((a.dimension(0) != 1) && (a.dimension(1) != 1)) {
        cout << "a and b must have rank 1 or 2; if rank 2, one of the two ranks' dimensions must be 1.  (I.e. a and b must both be vectors.)\n";
        TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument, "a and b must have rank 1 or 2; if rank 2, one of the two ranks' dimensions must be 1.  (I.e. a and b must both be vectors.)");
      }
    }
    if (b.rank()==2) {
      if ((b.dimension(0) != 1) && (b.dimension(1) != 1)) {
        cout << "a and b must have rank 1 or 2; if rank 2, one of the two ranks' dimensions must be 1.  (I.e. a and b must both be vectors.)\n";
        TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument, "a and b must have rank 1 or 2; if rank 2, one of the two ranks' dimensions must be 1.  (I.e. a and b must both be vectors.)");
      }
    }
    if (b.size() != a.size()) {
      cout << "a and b vectors must have the same length.\n";
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument, "a and b vectors must have the same length.");
    }
    double sum = 0.0;
    for (int i=0; i<a.size(); i++) {
      sum += a[i] * b[i];
    }
    return sum;
  }
  
  // gives X = A*B.  Must pass in 2D arrays, even for vectors! 
  static void multiply(FieldContainer<double> &X, const FieldContainer<double> &A, const FieldContainer<double> &B, char TransposeA = 'N', char TransposeB = 'N'){
    multiplyAndAdd(X,A,B,TransposeA,TransposeB,1.0,0.0);
  }
  
  static void multiplyFCByWeight(FieldContainer<double> & fc, double weight) {
    int size = fc.size();
    double *valuePtr = &fc[0]; // to make this as fast as possible, do some pointer arithmetic...
    for (int i=0; i<size; i++) {
      *valuePtr *= weight;
      valuePtr++;
    }
  }

  // wrapper for SDM multiply + add routine.  Must pass in 2D arrays, even for vectors! 
  // X = ScalarThis*X + ScalarAB*A*B
  static void multiplyAndAdd(FieldContainer<double> &X, const FieldContainer<double> &A, const FieldContainer<double> &B, char TransposeA, char TransposeB, double ScalarAB, double ScalarThis){
    int N = X.dimension(0);
    int M = X.dimension(1);
    if ((N==0) || (M==0)) {
      cout << "ERROR: empty result matrix passed in to SerialDenseWrapper::multiplyAndAdd.\n";
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument, "empty result matrix passed in to SerialDenseWrapper::multiplyAndAdd");
    }
    transposeMatrix(X); // SDMs are transposed relative to FCs
    
    Epetra_SerialDenseMatrix AMatrix = convertFCToSDM(A,::View);
    Epetra_SerialDenseMatrix BMatrix = convertFCToSDM(B,::View);
    Epetra_SerialDenseMatrix XMatrix = convertFCToSDM(X,::View);
    
    // since SDMs are transposed relative to FCs, reverse the transposal
    char transposeAMatrix = (TransposeA=='T') ? 'N' : 'T';
    char transposeBMatrix = (TransposeB=='T') ? 'N' : 'T';
    
    int success = XMatrix.Multiply(transposeAMatrix,transposeBMatrix,ScalarAB,AMatrix,BMatrix,ScalarThis);

    transposeMatrix(X); // SDMs are transposed relative to FCs
//    convertSDMToFC(X,XMatrix); // not needed when using View.
    
    if (success != 0){
      cout << "Error in SerialDenseWrapper::multiplyAndAdd with error code " << success << endl;
      
      cout << "A:\n" << A;
      cout << "B:\n" << B;
      cout << "X:\n" << X;
    }
  }
  
  static FieldContainer<double> getSubMatrix(FieldContainer<double> &A, set<unsigned> &rowIndices, set<unsigned> &colIndices,
                                             bool warnOfNonzeroOffBlockEntries = false) {
    FieldContainer<double> subMatrix(rowIndices.size(),colIndices.size());
    
    unsigned subrowIndex = 0;
    for (set<unsigned>::iterator rowIndexIt = rowIndices.begin(); rowIndexIt != rowIndices.end(); rowIndexIt++, subrowIndex++) {
      unsigned subcolIndex = 0;
      for (set<unsigned>::iterator colIndexIt = colIndices.begin(); colIndexIt != colIndices.end(); colIndexIt++, subcolIndex++) {
        subMatrix(subrowIndex,subcolIndex) = A(*rowIndexIt,*colIndexIt);
      }
    }
    
    if (warnOfNonzeroOffBlockEntries) {
      int numRows = A.dimension(0);
      int numCols = A.dimension(1);
      double tol = 1e-14;
      for (set<unsigned>::iterator rowIndexIt = rowIndices.begin(); rowIndexIt != rowIndices.end(); rowIndexIt++) {
        for (int j=0; j<numCols; j++) {
          if (colIndices.find(j) == colIndices.end()) {
            double val = A(*rowIndexIt,j);
            if (abs(val) > tol) {
              cout << "WARNING: off-block entry (" << *rowIndexIt << "," << j << ") = " << val << " is non-zero.\n";
            }
          }
        }
      }
      for (set<unsigned>::iterator colIndexIt = colIndices.begin(); colIndexIt != colIndices.end(); colIndexIt++) {
        for (int i=0; i<numRows; i++) {
          if (rowIndices.find(i) == rowIndices.end()) {
            double val = A(i,*colIndexIt);
            if (abs(val) > tol) {
              cout << "WARNING: off-block entry (" << i <<  "," << *colIndexIt << ") = " << val << " is non-zero.\n";
            }
          }
        }
      }
    }
    return subMatrix;
  }

  static void solveSystem(FieldContainer<double> &x, FieldContainer<double> &A, FieldContainer<double> &b, bool useATranspose = false) {
    // solves Ax = b, where
    // A = (N,N)
    // x, b = (N)
    if (b.rank()==1){
      b.resize(b.dimension(0),1);
      x.resize(x.dimension(0),1);
      solveSystemMultipleRHS(x, A, b, useATranspose);
      x.resize(x.dimension(0));
    }
  }

  static int solveSystemMultipleRHS(FieldContainer<double> &x, FieldContainer<double> &A, FieldContainer<double> &b, bool useATranspose = false){
    // solves Ax = b, where
    // A = (N,N)
    // x, b = (N, M)
    Epetra_SerialDenseSolver solver;
    
    int N = A.dimension(0);
    int nRHS = b.dimension(1);

    // take care of the fact that SDMs are transposed relative to FCs:
    transposeMatrix(b);
    
    if (!useATranspose) {
      transposeMatrix(A);
    }
 
    Epetra_SerialDenseMatrix AMatrix = convertFCToSDM(A);
    Epetra_SerialDenseMatrix bVectors = convertFCToSDM(b);
    Epetra_SerialDenseMatrix xVectors(N,nRHS);
   
    solver.SetMatrix(AMatrix);
    int info = solver.SetVectors(xVectors,bVectors);
    if (info!=0){
      cout << "solveSystem: failed to SetVectors with error " << info << endl;
      return info;
    }
    
    bool equilibrated = false;
    if (solver.ShouldEquilibrate()){
      solver.EquilibrateMatrix();
      solver.EquilibrateRHS();
      equilibrated = true;
    }
    
    info = solver.Solve();
    if (info!=0){
      cout << "solveSystem: failed to solve with error " << info << endl;
      return info;
    }
    
    if (equilibrated) {
      int successLocal = solver.UnequilibrateLHS();
      if (successLocal != 0) {
        cout << "solveSystem: unequilibration FAILED with error: " << successLocal << endl;
        return successLocal;
      }
    }
    
    // if we transposed A above, transpose it back here--leaving things as we found them
    if (!useATranspose) {
      transposeMatrix(A);
    }
    
    transposeMatrix(b);
    
    convertSDMToFC(x,xVectors);
    
    transposeMatrix(x);
    
    return 0;
  }
  
  static int solveSystemUsingQR(FieldContainer<double> &x, FieldContainer<double> &A, FieldContainer<double> &b, bool useATranspose = false){
    // solves Ax = b, where
    // A = (N,N)
    // x, b = (N,M)
    if ((x.rank() != 2) || (A.rank() != 2) || (b.rank() != 2)) {
      cout << "x, A, and b must each be a rank 2 FieldContainer!!\n";
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument, "x, A, and b must each be a rank 2 FieldContainer!!");
    }
    if (A.dimension(0) != A.dimension(1)) {
      cout << "A must be square!\n";
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument, "A must be square!");
    }
    if ((A.dimension(0) != b.dimension(0)) || (A.dimension(0) != x.dimension(0))) {
      cout << "x and b's first dimension must match the dimension of A!\n";
    }
    
    int N = A.dimension(0);
    int nRHS = b.dimension(1); // M
    
    Teuchos::LAPACK<int, double> lapack;
    
    int numCols = N;
    int numRows = N;
    int rowStride = numCols;
    Teuchos::SerialDenseMatrix<int, double> ATranspose( Teuchos::Copy, &A(0,0), rowStride, numCols, numRows); // transpose because SDM is column-major
    Teuchos::SerialDenseMatrix<int, double> ASDM( ATranspose, Teuchos::TRANS); // as an optimization, could avoid this declaration when useATranspose = true

    Teuchos::SerialDenseMatrix<int, double> bTranspose( Teuchos::Copy, &b(0,0), nRHS, nRHS, N); // transpose because SDM is column-major
    Teuchos::SerialDenseMatrix<int, double> bSDM( bTranspose, Teuchos::TRANS);  // transpose the RHS matrix

    Teuchos::SerialDenseMatrix<int, double> xSDM(N, nRHS);
    
    Teuchos::SerialQRDenseSolver<int,double> qrSolver;
    
    int info = 0;
    if (useATranspose) {
      qrSolver.setMatrix( Teuchos::rcp( &ATranspose, false ) );
    } else {
      qrSolver.setMatrix( Teuchos::rcp( &ASDM, false ) );
    }
    qrSolver.setVectors( Teuchos::rcp( &xSDM, false ), Teuchos::rcp( &bSDM, false ) );
    info = qrSolver.factor();
    if (info != 0) {
      std::cout << "Teuchos::SerialQRDenseSolver::factor() returned : " << info << std::endl;
      return info;
    }

    info = qrSolver.solve();
    if (info != 0) {
      std::cout << "Teuchos::SerialQRDenseSolver::solve() returned : " << info << std::endl;
      return info;
    }

    Teuchos::Array<int> dim(x.rank());
    x.dimensions(dim);
    Teuchos::SerialDenseMatrix<int, double> xSDMTranspose(xSDM, Teuchos::TRANS);
    x = FieldContainer<double>(dim, xSDMTranspose.values());
    
    return 0;
  }
  
  static int solveSPDSystemMultipleRHS(FieldContainer<double> &x, FieldContainer<double> &A_SPD, FieldContainer<double> &b){
    // solves Ax = b, where
    // A = (N,N)
    // x, b = (N)
    Epetra_SerialSpdDenseSolver solver;
    
    int N = A_SPD.dimension(0);
    int nRHS = b.dimension(1);
    
    int result = 0;
    
    Epetra_SerialSymDenseMatrix AMatrix(::Copy, &A_SPD(0,0),
                                          N, // stride -- fc stores in row-major order (a.o.t. SDM)
                                          N);
    
    Epetra_SerialDenseMatrix bVectors = convertFCToSDM(b);
    Epetra_SerialDenseMatrix xVectors(N,nRHS);
    
    solver.SetMatrix(AMatrix);
    int info = solver.SetVectors(xVectors,bVectors);
    if (info!=0){
      result = info;
      cout << "solveSPDSystemMultipleRHS: failed to SetVectors with error " << info << endl;
      return result;
    }
    
    if ( solver.ShouldEquilibrate() ) {
      solver.FactorWithEquilibration(true);
      solver.SolveToRefinedSolution(false); // false: don't use iterative refinements...
    }
    info = solver.Factor();
    if (info != 0) {
      result = info;
      cout << "solveSPDSystemMultipleRHS: Factor failed with code " << result << endl;
      return result;
    }
    
    info = solver.Solve();
    
    if (info != 0) {
      cout << "BilinearForm::optimalTestWeights: Solve FAILED with error: " << info << endl;
      result = info;
    }
    
    convertSDMToFC(x,xVectors);
    
    return result;
  }

  static double getMatrixConditionNumber(FieldContainer<double> &A) {
    Epetra_SerialDenseMatrix AMatrix = convertFCToSDM(A);
    Epetra_SerialDenseSolver solver;
    solver.SetMatrix(AMatrix); 
    double invCond;
    int result = solver.ReciprocalConditionEstimate(invCond);
    if (result == 0) // success
      return 1.0/invCond;
    else // failure
      return -1;
  }

  static void writeMatrixToMatlabFile(const string& filePath, FieldContainer<double> &A){
    int N = A.dimension(0);
    int M = A.dimension(1);
    ofstream matrixFile;
    matrixFile.open(filePath.c_str());
    
    for (int i = 0;i<N;i++){
      for (int j = 0;j<M;j++){
        matrixFile << A(i,j) << " ";
      }
      matrixFile << endl;
    }
    matrixFile.close();
  }
};

#endif
