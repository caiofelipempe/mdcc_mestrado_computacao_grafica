#pragma once

#include <vector>
#include <string>

class LinearSystemSolver
{
public:
    using Matrix = std::vector<std::vector<double>>;
    using Vector = std::vector<double>;

    // ==========================================
    // TAREFA 01: Eliminação de Gauss
    // ==========================================
    static Vector gaussPartialPivot(Matrix A, Vector b);
    static Vector gaussTotalPivot(Matrix A, Vector b);

    // ==========================================
    // TAREFA 02: Método de Gauss-Jordan
    // ==========================================
    static Vector gaussJordanPartialPivot(Matrix A, Vector b);
    static Vector gaussJordanTotalPivot(Matrix A, Vector b);

    // ==========================================
    // TAREFA 03: Decomposição LU (sem permutação) e Solução
    // ==========================================
    static void luDecomposition(const Matrix& A, Matrix& L, Matrix& U);
    static Vector solveLU(const Matrix& L, const Matrix& U, const Vector& b);

    // ==========================================
    // TAREFA 04: RREF, Posto e Dimensão do Espaço Nulo
    // ==========================================
    struct RREFResult
    {
        Matrix rrefMatrix;
        int rank;
        int nullity;
    };
    static RREFResult computeRREF(Matrix A);

    // ==========================================
    // TAREFA 05: Decomposição de Cholesky (S * S^T)
    // ==========================================
    static Matrix choleskyDecomposition(const Matrix& A);

    // ==========================================
    // TAREFA 06: Ortogonalização de Gram-Schmidt e Extensão
    // ==========================================
    static Matrix extendBasis(const Matrix& vectors, int m);
    static Matrix gramSchmidtOrthonormalize(const Matrix& basis);

    // ==========================================
    // TAREFA 07: Mínimos Quadrados e Regressão Linear
    // ==========================================
    static Vector leastSquares(const Matrix& A, const Vector& b);
    struct RegressionResult
    {
        double slope;
        double intercept;
    };
    static RegressionResult linearRegression(const Vector& x, const Vector& y);

    // ==========================================
    // TAREFA 08: Métodos de Potência (Regular, Inverso, Deslocamento)
    // ==========================================
    struct PowerResult
    {
        double eigenvalue;
        Vector eigenvector;
        int iterations;
    };
    static PowerResult powerMethod(const Matrix& A, Vector x0, double eps, int maxIter = 1000);
    static PowerResult inversePowerMethod(const Matrix& A, Vector x0, double eps, int maxIter = 1000);
    static PowerResult shiftedPowerMethod(const Matrix& A, Vector x0, double eps, double mu, int maxIter = 1000);

    // ==========================================
    // TAREFA 09: Método de Householder
    // ==========================================
    struct HouseholderResult
    {
        Matrix outputMatrix;
        Matrix accumulatedH;
    };
    static HouseholderResult householderTransform(const Matrix& A, bool printSteps = true);

    // ==========================================
    // TAREFA 10: Decomposição QR
    // ==========================================
    struct QRResult
    {
        Matrix Q;
        Matrix R;
    };
    static QRResult qrDecomposition(const Matrix& A);

    // ==========================================
    // TAREFA 11: Autovalores e Autovetores via Método QR
    // ==========================================
    struct EigenResult
    {
        Vector eigenvalues;
        Matrix eigenvectors;
    };
    static EigenResult qrEigenSymmetric(const Matrix& A, double eps);
    static EigenResult qrEigenNonSymmetric(const Matrix& A, double eps);

    // ==========================================
    // TAREFA 12: Decomposição SVD
    // ==========================================
    struct SVDResult
    {
        Matrix U;
        Matrix Sigma;
        Matrix V;
    };
    static SVDResult svdDecomposition(const Matrix& A);

    // ==========================================
    // Utilitários de Apoio
    // ==========================================
    static Matrix multiplyMatrices(const Matrix& A, const Matrix& B);
    static Vector multiplyMatrixVector(const Matrix& A, const Vector& v);
    static Matrix transposeMatrix(const Matrix& A);
    static double vectorNorm(const Vector& v);
    static void printMatrix(const Matrix& A, const std::string& name = "");
    static void printVector(const Vector& v, const std::string& name = "");

private:
    static void swapRows(Matrix& A, Vector& b, int r1, int r2);
    static void swapColumns(Matrix& A, std::vector<int>& permutation, int c1, int c2);
};