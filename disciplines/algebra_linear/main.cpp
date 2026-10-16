#include "linear_system_solver.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>

using Matrix = LinearSystemSolver::Matrix;
using Vector = LinearSystemSolver::Vector;

double matrixError(
    const Matrix& A,
    const Matrix& B)
{
    double err = 0.0;

    for (size_t i = 0; i < A.size(); ++i)
    {
        for (size_t j = 0; j < A[0].size(); ++j)
        {
            err = std::max(
                err,
                std::abs(A[i][j] - B[i][j]));
        }
    }

    return err;
}

double vectorError(
    const Vector& a,
    const Vector& b)
{
    double err = 0.0;

    for (size_t i = 0; i < a.size(); ++i)
    {
        err = std::max(
            err,
            std::abs(a[i] - b[i]));
    }

    return err;
}

Matrix identity(int n)
{
    Matrix I(
        n,
        Vector(n, 0.0));

    for (int i = 0; i < n; ++i)
    {
        I[i][i] = 1.0;
    }

    return I;
}

int main()
{
    std::cout
        << std::fixed
        << std::setprecision(6);

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<double>
        dist(-5.0, 5.0);

    std::normal_distribution<double>
        noise(0.0, 0.2);

    std::vector<int> tamanhos =
    {
        3,
        4,
        5,
        10
    };

    std::cout
        << "====================================================\n";
    std::cout
        << " INICIANDO TESTES, MATRIZES E RESULTADOS (01 - 12)\n";
    std::cout
        << "====================================================\n";

    for (int n : tamanhos)
    {
        std::cout
            << "\n====================================================\n";
        std::cout
            << ">>> TAMANHO DAS MATRIZES: N = "
            << n
            << " <<<\n";
        std::cout
            << "====================================================\n\n";

        // =====================================================
        // MATRIZES BASE
        // =====================================================

        Matrix A(
            n,
            Vector(n));

        Matrix M(
            n,
            Vector(n));

        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
            {
                A[i][j] = dist(gen);
                M[i][j] = dist(gen);
            }
        }

        Matrix Mt =
            LinearSystemSolver::transposeMatrix(M);

        Matrix A_sym =
            LinearSystemSolver::multiplyMatrices(
                Mt,
                M);

        for (int i = 0; i < n; ++i)
        {
            A_sym[i][i] += n;
        }

        LinearSystemSolver::printMatrix(
            A,
            "Matriz A");

        // =====================================================
        // TAREFA 01
        // =====================================================

        Vector xTrue(n);

        for (int i = 0; i < n; ++i)
        {
            xTrue[i] = i + 1;
        }

        Vector b =
            LinearSystemSolver::multiplyMatrixVector(
                A,
                xTrue);

        auto xGauss =
            LinearSystemSolver::gaussPartialPivot(
                A,
                b);

        std::cout
            << "[Tarefa 01] Gauss\n";

        LinearSystemSolver::printVector(
            xGauss,
            "Solucao");

        std::cout
            << "Erro: "
            << vectorError(
                   xTrue,
                   xGauss)
            << "\n\n";

        // =====================================================
        // TAREFA 02
        // =====================================================

        auto xJordan =
            LinearSystemSolver::
            gaussJordanTotalPivot(
                A,
                b);

        std::cout
            << "[Tarefa 02] Gauss-Jordan\n";

        LinearSystemSolver::printVector(
            xJordan,
            "Solucao");

        std::cout
            << "Erro: "
            << vectorError(
                   xTrue,
                   xJordan)
            << "\n\n";

        // =====================================================
        // TAREFA 03
        // =====================================================

        Matrix L;
        Matrix U;

        LinearSystemSolver::
        luDecomposition(
            A,
            L,
            U);

        auto LU =
            LinearSystemSolver::
            multiplyMatrices(
                L,
                U);

        std::cout
            << "[Tarefa 03] LU\n";

        LinearSystemSolver::printMatrix(
            L,
            "L");

        LinearSystemSolver::printMatrix(
            U,
            "U");

        LinearSystemSolver::printMatrix(
            LU,
            "L*U");

        std::cout
            << "Erro LU: "
            << matrixError(
                   A,
                   LU)
            << "\n\n";

        // =====================================================
        // TAREFA 04
        // =====================================================

        auto singular = A;

        if (n > 2)
        {
            for (int j = 0; j < n; ++j)
            {
                singular[2][j] =
                    singular[0][j]
                    + singular[1][j];
            }
        }

        auto rref =
            LinearSystemSolver::
            computeRREF(
                singular);

        std::cout
            << "[Tarefa 04] RREF\n";

        LinearSystemSolver::printMatrix(
            singular,
            "Matriz Singular");

        LinearSystemSolver::printMatrix(
            rref.rrefMatrix,
            "RREF");

        std::cout
            << "Rank = "
            << rref.rank
            << "\n";

        std::cout
            << "Nullity = "
            << rref.nullity
            << "\n\n";

        // =====================================================
        // TAREFA 05
        // =====================================================

        auto S =
            LinearSystemSolver::
            choleskyDecomposition(
                A_sym);

        auto ST =
            LinearSystemSolver::
            transposeMatrix(
                S);

        auto ArecChol =
            LinearSystemSolver::
            multiplyMatrices(
                S,
                ST);

        std::cout
            << "[Tarefa 05] Cholesky\n";

        LinearSystemSolver::printMatrix(
            S,
            "S");

        std::cout
            << "Erro Cholesky = "
            << matrixError(
                   A_sym,
                   ArecChol)
            << "\n\n";

        // =====================================================
        // TAREFA 06
        // =====================================================

        Matrix basis;

        for (int i = 0; i < n; ++i)
        {
            Vector v(n);

            for (int j = 0; j < n; ++j)
            {
                v[j] = dist(gen);
            }

            basis.push_back(v);
        }

        auto ortho =
            LinearSystemSolver::
            gramSchmidtOrthonormalize(
                basis);

        std::cout
            << "[Tarefa 06] Gram-Schmidt\n";

        LinearSystemSolver::printMatrix(
            basis,
            "Base Original");

        LinearSystemSolver::printMatrix(
            ortho,
            "Base Ortonormal");

        // =====================================================
        // TAREFA 07
        // =====================================================

        Vector xReg(50);
        Vector yReg(50);

        for (int i = 0; i < 50; ++i)
        {
            xReg[i] = i;

            yReg[i] =
                2.0 * i
                + 1.0
                + noise(gen);
        }

        auto reg =
            LinearSystemSolver::
            linearRegression(
                xReg,
                yReg);

        std::cout
            << "[Tarefa 07] Regressao\n";

        std::cout
            << "Slope = "
            << reg.slope
            << "\n";

        std::cout
            << "Intercept = "
            << reg.intercept
            << "\n\n";

        // =====================================================
        // TAREFA 08
        // =====================================================

        Vector x0(
            n,
            1.0);

        auto power =
            LinearSystemSolver::
            powerMethod(
                A_sym,
                x0,
                1e-8,
                1000);

        std::cout
            << "[Tarefa 08] Potencia\n";

        std::cout
            << "Maior autovalor = "
            << power.eigenvalue
            << "\n\n";

        // =====================================================
        // TAREFA 09
        // =====================================================

        auto house =
            LinearSystemSolver::
            householderTransform(
                A_sym,
                false);

        std::cout
            << "[Tarefa 09] Householder\n";

        LinearSystemSolver::printMatrix(
            house.outputMatrix,
            "Tridiagonal");

        // =====================================================
        // TAREFA 10
        // =====================================================

        auto qr =
            LinearSystemSolver::
            qrDecomposition(
                A);

        auto QR =
            LinearSystemSolver::
            multiplyMatrices(
                qr.Q,
                qr.R);

        std::cout
            << "[Tarefa 10] QR\n";

        LinearSystemSolver::printMatrix(
            qr.Q,
            "Q");

        LinearSystemSolver::printMatrix(
            qr.R,
            "R");

        std::cout
            << "Erro QR = "
            << matrixError(
                   A,
                   QR)
            << "\n\n";

        // =====================================================
        // TAREFA 11
        // =====================================================

        auto eigen =
            LinearSystemSolver::
            qrEigenSymmetric(
                A_sym,
                1e-8);

        std::cout
            << "[Tarefa 11] Autovalores\n";

        LinearSystemSolver::printVector(
            eigen.eigenvalues,
            "Eigenvalues");

        // =====================================================
        // TAREFA 12
        // =====================================================

        auto svd =
            LinearSystemSolver::
            svdDecomposition(
                A);

        Matrix Vt =
            LinearSystemSolver::
            transposeMatrix(
                svd.V);

        Matrix US =
            LinearSystemSolver::
            multiplyMatrices(
                svd.U,
                svd.Sigma);

        Matrix Arec =
            LinearSystemSolver::
            multiplyMatrices(
                US,
                Vt);

        std::cout
            << "[Tarefa 12] SVD\n";

        LinearSystemSolver::printMatrix(
            svd.U,
            "U");

        LinearSystemSolver::printMatrix(
            svd.Sigma,
            "Sigma");

        LinearSystemSolver::printMatrix(
            svd.V,
            "V");

        std::cout
            << "Erro SVD = "
            << matrixError(
                   A,
                   Arec)
            << "\n\n";
    }

    return 0;
}