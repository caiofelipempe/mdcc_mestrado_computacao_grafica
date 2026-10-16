#include "linear_system_solver.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <iomanip>

using Matrix = LinearSystemSolver::Matrix;
using Vector = LinearSystemSolver::Vector;

void LinearSystemSolver::swapRows(Matrix &A, Vector &b, int r1, int r2)
{
    std::swap(A[r1], A[r2]);
    std::swap(b[r1], b[r2]);
}

void LinearSystemSolver::swapColumns(Matrix &A, std::vector<int> &permutation, int c1, int c2)
{
    for (auto &row : A)
    {
        std::swap(row[c1], row[c2]);
    }
    std::swap(permutation[c1], permutation[c2]);
}

// ==========================================
// TAREFA 01
// ==========================================
Vector LinearSystemSolver::gaussPartialPivot(Matrix A, Vector b)
{
    const int n = static_cast<int>(A.size());
    for (int k = 0; k < n - 1; ++k)
    {
        int pivot = k;
        for (int i = k + 1; i < n; ++i)
        {
            if (std::fabs(A[i][k]) > std::fabs(A[pivot][k]))
                pivot = i;
        }
        if (std::fabs(A[pivot][k]) < 1e-12)
            throw std::runtime_error("Matriz singular.");
        if (pivot != k)
            swapRows(A, b, pivot, k);

        for (int i = k + 1; i < n; ++i)
        {
            double m = A[i][k] / A[k][k];
            for (int j = k; j < n; ++j)
                A[i][j] -= m * A[k][j];
            b[i] -= m * b[k];
        }
    }
    Vector x(n);
    for (int i = n - 1; i >= 0; --i)
    {
        double sum = b[i];
        for (int j = i + 1; j < n; ++j)
            sum -= A[i][j] * x[j];
        x[i] = sum / A[i][i];
    }
    return x;
}

Vector LinearSystemSolver::gaussTotalPivot(Matrix A, Vector b)
{
    const int n = static_cast<int>(A.size());
    std::vector<int> permutation(n);
    for (int i = 0; i < n; ++i)
        permutation[i] = i;

    for (int k = 0; k < n - 1; ++k)
    {
        int pivotRow = k, pivotCol = k;
        double maxValue = 0.0;
        for (int i = k; i < n; ++i)
        {
            for (int j = k; j < n; ++j)
            {
                if (std::fabs(A[i][j]) > maxValue)
                {
                    maxValue = std::fabs(A[i][j]);
                    pivotRow = i;
                    pivotCol = j;
                }
            }
        }
        if (maxValue < 1e-12)
            throw std::runtime_error("Matriz singular.");
        if (pivotRow != k)
            swapRows(A, b, pivotRow, k);
        if (pivotCol != k)
            swapColumns(A, permutation, pivotCol, k);

        for (int i = k + 1; i < n; ++i)
        {
            double m = A[i][k] / A[k][k];
            for (int j = k; j < n; ++j)
                A[i][j] -= m * A[k][j];
            b[i] -= m * b[k];
        }
    }
    Vector x(n);
    for (int i = n - 1; i >= 0; --i)
    {
        double sum = b[i];
        for (int j = i + 1; j < n; ++j)
            sum -= A[i][j] * x[j];
        x[i] = sum / A[i][i];
    }
    Vector reordered(n);
    for (int i = 0; i < n; ++i)
        reordered[permutation[i]] = x[i];
    return reordered;
}

// ==========================================
// TAREFA 02
// ==========================================
Vector LinearSystemSolver::gaussJordanPartialPivot(Matrix A, Vector b)
{
    const int n = static_cast<int>(A.size());
    for (int k = 0; k < n; ++k)
    {
        int pivot = k;
        for (int i = k + 1; i < n; ++i)
        {
            if (std::fabs(A[i][k]) > std::fabs(A[pivot][k]))
                pivot = i;
        }
        if (std::fabs(A[pivot][k]) < 1e-12)
            throw std::runtime_error("Matriz singular.");
        if (pivot != k)
            swapRows(A, b, pivot, k);

        double diag = A[k][k];
        for (int j = 0; j < n; ++j)
            A[k][j] /= diag;
        b[k] /= diag;

        for (int i = 0; i < n; ++i)
        {
            if (i == k)
                continue;
            double factor = A[i][k];
            for (int j = 0; j < n; ++j)
                A[i][j] -= factor * A[k][j];
            b[i] -= factor * b[k];
        }
    }
    return b;
}

Vector LinearSystemSolver::gaussJordanTotalPivot(Matrix A, Vector b)
{
    const int n = static_cast<int>(A.size());
    std::vector<int> permutation(n);
    for (int i = 0; i < n; ++i)
        permutation[i] = i;

    for (int k = 0; k < n; ++k)
    {
        int pivotRow = k, pivotCol = k;
        double maxValue = 0.0;
        for (int i = k; i < n; ++i)
        {
            for (int j = k; j < n; ++j)
            {
                if (std::fabs(A[i][j]) > maxValue)
                {
                    maxValue = std::fabs(A[i][j]);
                    pivotRow = i;
                    pivotCol = j;
                }
            }
        }
        if (maxValue < 1e-12)
            throw std::runtime_error("Matriz singular.");
        if (pivotRow != k)
            swapRows(A, b, pivotRow, k);
        if (pivotCol != k)
            swapColumns(A, permutation, pivotCol, k);

        double diag = A[k][k];
        for (int j = 0; j < n; ++j)
            A[k][j] /= diag;
        b[k] /= diag;

        for (int i = 0; i < n; ++i)
        {
            if (i == k)
                continue;
            double factor = A[i][k];
            for (int j = 0; j < n; ++j)
                A[i][j] -= factor * A[k][j];
            b[i] -= factor * b[k];
        }
    }
    Vector reordered(n);
    for (int i = 0; i < n; ++i)
        reordered[permutation[i]] = b[i];
    return reordered;
}

// ==========================================
// TAREFA 03
// ==========================================
void LinearSystemSolver::luDecomposition(const Matrix &A, Matrix &L, Matrix &U)
{
    int n = static_cast<int>(A.size());
    L = Matrix(n, Vector(n, 0.0));
    U = Matrix(n, Vector(n, 0.0));

    for (int i = 0; i < n; ++i)
    {
        L[i][i] = 1.0;
        for (int j = i; j < n; ++j)
        {
            double sum = 0.0;
            for (int k = 0; k < i; ++k)
                sum += L[i][k] * U[k][j];
            U[i][j] = A[i][j] - sum;
        }
        for (int j = i + 1; j < n; ++j)
        {
            double sum = 0.0;
            for (int k = 0; k < i; ++k)
                sum += L[j][k] * U[k][i];
            if (std::abs(U[i][i]) < 1e-12)
                throw std::runtime_error("Pivô zero na LU.");
            L[j][i] = (A[j][i] - sum) / U[i][i];
        }
    }
}

Vector LinearSystemSolver::solveLU(const Matrix &L, const Matrix &U, const Vector &b)
{
    int n = static_cast<int>(b.size());
    Vector y(n, 0.0);
    for (int i = 0; i < n; ++i)
    {
        double sum = 0.0;
        for (int j = 0; j < i; ++j)
            sum += L[i][j] * y[j];
        y[i] = b[i] - sum;
    }
    Vector x(n, 0.0);
    for (int i = n - 1; i >= 0; --i)
    {
        double sum = 0.0;
        for (int j = i + 1; j < n; ++j)
            sum += U[i][j] * x[j];
        x[i] = (y[i] - sum) / U[i][i];
    }
    return x;
}

// ==========================================
// TAREFA 04
// ==========================================
LinearSystemSolver::RREFResult LinearSystemSolver::computeRREF(Matrix A)
{
    int m = static_cast<int>(A.size());
    int n = static_cast<int>(A[0].size());
    int lead = 0, rank = 0;

    for (int r = 0; r < m; ++r)
    {
        if (lead >= n)
            break;
        int i = r;
        while (std::abs(A[i][lead]) < 1e-12)
        {
            i++;
            if (i == m)
            {
                i = r;
                lead++;
                if (lead == n)
                    goto end_rref;
            }
        }
        std::swap(A[r], A[i]);
        double val = A[r][lead];
        for (int j = 0; j < n; ++j)
            A[r][j] /= val;

        for (int i = 0; i < m; ++i)
        {
            if (i != r)
            {
                double factor = A[i][lead];
                for (int j = 0; j < n; ++j)
                    A[i][j] -= factor * A[r][j];
            }
        }
        lead++;
    }
end_rref:
    for (int i = 0; i < m; ++i)
    {
        bool nonZero = false;
        for (int j = 0; j < n; ++j)
        {
            if (std::abs(A[i][j]) > 1e-12)
            {
                nonZero = true;
                break;
            }
        }
        if (nonZero)
            rank++;
    }
    return {A, rank, n - rank};
}

// ==========================================
// TAREFA 05
// ==========================================
Matrix LinearSystemSolver::choleskyDecomposition(const Matrix &A)
{
    int n = static_cast<int>(A.size());
    Matrix S(n, Vector(n, 0.0));
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j <= i; ++j)
        {
            double sum = 0.0;
            for (int k = 0; k < j; ++k)
                sum += S[i][k] * S[j][k];
            if (i == j)
            {
                double val = A[i][i] - sum;
                if (val < 0.0)
                {
                    std::cout << "A Matriz não é positiva definida.\n";
                    throw std::runtime_error("Matriz não positiva definida.");
                }
                S[i][j] = std::sqrt(val);
            }
            else
            {
                S[i][j] = (A[i][j] - sum) / S[j][j];
            }
        }
    }
    return S;
}

// ==========================================
// TAREFA 06
// ==========================================
Matrix LinearSystemSolver::extendBasis(const Matrix &vectors, int m)
{
    Matrix basis = vectors;
    int n = static_cast<int>(vectors.size());
    for (int i = 0; i < m && n < m; ++i)
    {
        Vector candidate(m, 0.0);
        candidate[i] = 1.0;
        Matrix test = basis;
        test.push_back(candidate);
        if (computeRREF(test).rank > n)
        {
            basis.push_back(candidate);
            n++;
        }
    }
    return basis;
}

Matrix LinearSystemSolver::gramSchmidtOrthonormalize(const Matrix &basis)
{
    int n = static_cast<int>(basis.size());
    int m = static_cast<int>(basis[0].size());
    Matrix ortho(n, Vector(m, 0.0));
    for (int i = 0; i < n; ++i)
    {
        ortho[i] = basis[i];
        for (int j = 0; j < i; ++j)
        {
            double dotNum = 0.0, dotDen = 0.0;
            for (int k = 0; k < m; ++k)
            {
                dotNum += basis[i][k] * ortho[j][k];
                dotDen += ortho[j][k] * ortho[j][k];
            }
            double proj = (dotDen != 0.0) ? (dotNum / dotDen) : 0.0;
            for (int k = 0; k < m; ++k)
                ortho[i][k] -= proj * ortho[j][k];
        }
        double norm = vectorNorm(ortho[i]);
        if (norm > 1e-12)
        {
            for (int k = 0; k < m; ++k)
                ortho[i][k] /= norm;
        }
    }
    return ortho;
}

// ==========================================
// TAREFA 07
// ==========================================
Vector LinearSystemSolver::leastSquares(const Matrix &A, const Vector &b)
{
    Matrix At = transposeMatrix(A);
    Matrix AtA = multiplyMatrices(At, A);
    Vector Atb = multiplyMatrixVector(At, b);
    Matrix L, U;
    luDecomposition(AtA, L, U);
    return solveLU(L, U, Atb);
}

LinearSystemSolver::RegressionResult LinearSystemSolver::linearRegression(const Vector &x, const Vector &y)
{
    int m = static_cast<int>(x.size());
    Matrix A(m, Vector(2, 1.0));
    for (int i = 0; i < m; ++i)
        A[i][1] = x[i];
    Vector coeffs = leastSquares(A, y);
    return {coeffs[1], coeffs[0]};
}

// ==========================================
// TAREFA 08
// ==========================================
LinearSystemSolver::PowerResult LinearSystemSolver::powerMethod(
    const Matrix &A,
    Vector x0,
    double eps,
    int maxIter)
{
    Vector x = x0;

    double normX = vectorNorm(x);

    if (normX < 1e-12)
    {
        throw std::runtime_error("Vetor inicial nulo.");
    }

    for (double &v : x)
    {
        v /= normX;
    }

    double lambda = 0.0;
    int iter = 0;

    for (; iter < maxIter; ++iter)
    {
        Vector y = multiplyMatrixVector(A, x);

        double normY = vectorNorm(y);

        if (normY < 1e-12)
        {
            throw std::runtime_error("Norma nula durante o método da potência.");
        }

        for (double &v : y)
        {
            v /= normY;
        }

        Vector Ay = multiplyMatrixVector(A, y);

        double nextLambda = 0.0;

        for (size_t i = 0; i < y.size(); ++i)
        {
            nextLambda += y[i] * Ay[i];
        }

        if (std::abs(nextLambda - lambda) < eps)
        {
            lambda = nextLambda;
            x = y;
            break;
        }

        lambda = nextLambda;
        x = y;
    }

    return {lambda, x, iter};
}

LinearSystemSolver::PowerResult LinearSystemSolver::inversePowerMethod(
    const Matrix& A,
    Vector x0,
    double eps,
    int maxIter)
{
    Matrix L, U;

    luDecomposition(A, L, U);

    Vector x = x0;

    double normX = vectorNorm(x);

    for (double& v : x)
    {
        v /= normX;
    }

    double lambda = 0.0;
    int iter = 0;

    for (; iter < maxIter; ++iter)
    {
        Vector y = solveLU(L, U, x);

        double normY = vectorNorm(y);

        for (double& v : y)
        {
            v /= normY;
        }

        Vector Ay = multiplyMatrixVector(A, y);

        double rayleigh = 0.0;

        for (size_t i = 0; i < y.size(); ++i)
        {
            rayleigh += y[i] * Ay[i];
        }

        if (std::abs(rayleigh - lambda) < eps)
        {
            lambda = rayleigh;
            x = y;
            break;
        }

        lambda = rayleigh;
        x = y;
    }

    return { lambda, x, iter };
}

LinearSystemSolver::PowerResult LinearSystemSolver::shiftedPowerMethod(const Matrix &A, Vector x0, double eps, double mu, int maxIter)
{
    int n = static_cast<int>(A.size());
    Matrix A_shifted = A;
    for (int i = 0; i < n; ++i)
        A_shifted[i][i] -= mu;
    auto res = powerMethod(A_shifted, x0, eps, maxIter);
    res.eigenvalue += mu;
    return res;
}

// ==========================================
// TAREFA 09
// ==========================================
LinearSystemSolver::HouseholderResult LinearSystemSolver::householderTransform(const Matrix &A, bool printSteps)
{
    int n = static_cast<int>(A.size());
    Matrix H_acc(n, Vector(n, 0.0));
    for (int i = 0; i < n; ++i)
        H_acc[i][i] = 1.0;
    Matrix curr = A;
    if (printSteps)
        printMatrix(curr, "Matriz Original (Householder)");

    for (int k = 0; k < n - 2; ++k)
    {
        Vector x(n - k - 1, 0.0);
        for (int i = 0; i < n - k - 1; ++i)
            x[i] = curr[k + 1 + i][k];
        double normX = vectorNorm(x);
        if (normX < 1e-12)
            continue;
        double alpha = (x[0] > 0) ? -normX : normX;
        Vector u = x;
        u[0] -= alpha;
        double normU = vectorNorm(u);
        if (normU > 1e-12)
            for (auto &val : u)
                val /= normU;

        Matrix P_sub(n - k - 1, Vector(n - k - 1, 0.0));
        for (int i = 0; i < n - k - 1; ++i)
            for (int j = 0; j < n - k - 1; ++j)
                P_sub[i][j] = (i == j ? 1.0 : 0.0) - 2.0 * u[i] * u[j];

        Matrix P(n, Vector(n, 0.0));
        for (int i = 0; i < n; ++i)
            P[i][i] = 1.0;
        for (int i = 0; i < n - k - 1; ++i)
            for (int j = 0; j < n - k - 1; ++j)
                P[k + 1 + i][k + 1 + j] = P_sub[i][j];

        curr = multiplyMatrices(multiplyMatrices(P, curr), P);
        H_acc = multiplyMatrices(P, H_acc);

        if (printSteps)
        {
            std::cout << "--- Passo " << k + 1 << " ---\n";
            printMatrix(P, "Matriz de Householder P");
            printMatrix(curr, "Matriz Modificada");
        }
    }
    return {curr, H_acc};
}

// ==========================================
// TAREFA 10
// ==========================================
LinearSystemSolver::QRResult LinearSystemSolver::qrDecomposition(const Matrix &A)
{
    int n = static_cast<int>(A.size());
    Matrix cols(n, Vector(n, 0.0));
    for (int j = 0; j < n; ++j)
        for (int i = 0; i < n; ++i)
            cols[j][i] = A[i][j];

    Matrix ortho = gramSchmidtOrthonormalize(cols);
    Matrix Q(n, Vector(n, 0.0));
    for (int j = 0; j < n; ++j)
        for (int i = 0; i < n; ++i)
            Q[i][j] = ortho[j][i];

    Matrix R = multiplyMatrices(transposeMatrix(Q), A);
    return {Q, R};
}

// ==========================================
// TAREFA 11
// ==========================================
LinearSystemSolver::EigenResult LinearSystemSolver::qrEigenSymmetric(const Matrix &A, double eps)
{
    int n = static_cast<int>(A.size());
    Matrix Ak = A;
    Matrix Q_acc(n, Vector(n, 0.0));
    for (int i = 0; i < n; ++i)
        Q_acc[i][i] = 1.0;

    for (int iter = 0; iter < 100; ++iter)
    {
        auto [Q, R] = qrDecomposition(Ak);
        Ak = multiplyMatrices(R, Q);
        Q_acc = multiplyMatrices(Q_acc, Q);
        double offDiag = 0.0;
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                if (i != j)
                    offDiag += std::abs(Ak[i][j]);
        if (offDiag < eps)
            break;
    }
    Vector evals(n);
    for (int i = 0; i < n; ++i)
        evals[i] = Ak[i][i];
    return {evals, Q_acc};
}

LinearSystemSolver::EigenResult LinearSystemSolver::qrEigenNonSymmetric(const Matrix &A, double eps)
{
    auto [tri, H] = householderTransform(A, false);
    auto res = qrEigenSymmetric(tri, eps);
    res.eigenvectors = multiplyMatrices(H, res.eigenvectors);
    return res;
}

// ==========================================
// TAREFA 12
// ==========================================
LinearSystemSolver::SVDResult
LinearSystemSolver::svdDecomposition(const Matrix& A)
{
    const int m = static_cast<int>(A.size());
    const int n = static_cast<int>(A[0].size());

    Matrix At = transposeMatrix(A);
    Matrix AtA = multiplyMatrices(At, A);

    auto eigen = qrEigenSymmetric(AtA, 1e-9);

    Vector eigenvalues = eigen.eigenvalues;
    Matrix Vraw = eigen.eigenvectors;

    std::vector<int> permutation(n);

    for (int i = 0; i < n; ++i)
    {
        permutation[i] = i;
    }

    std::sort(
        permutation.begin(),
        permutation.end(),
        [&](int a, int b)
        {
            return eigenvalues[a] > eigenvalues[b];
        });

    Vector sortedEigenvalues(n);
    Matrix V(n, Vector(n, 0.0));

    for (int k = 0; k < n; ++k)
    {
        sortedEigenvalues[k] = eigenvalues[permutation[k]];

        for (int i = 0; i < n; ++i)
        {
            V[i][k] = Vraw[i][permutation[k]];
        }
    }

    Matrix Sigma(m, Vector(n, 0.0));
    Matrix U(m, Vector(m, 0.0));

    int r = std::min(m, n);

    for (int i = 0; i < r; ++i)
    {
        double lambda = std::max(0.0, sortedEigenvalues[i]);

        double sigma = std::sqrt(lambda);

        Sigma[i][i] = sigma;

        if (sigma < 1e-12)
        {
            continue;
        }

        Vector vi(n);

        for (int j = 0; j < n; ++j)
        {
            vi[j] = V[j][i];
        }

        Vector ui = multiplyMatrixVector(A, vi);

        for (int j = 0; j < m; ++j)
        {
            U[j][i] = ui[j] / sigma;
        }
    }

    if (r > 0)
    {
        Matrix Ucols(r);

        for (int col = 0; col < r; ++col)
        {
            Ucols[col].resize(m);

            for (int row = 0; row < m; ++row)
            {
                Ucols[col][row] = U[row][col];
            }
        }

        Matrix Uortho = gramSchmidtOrthonormalize(Ucols);

        for (int col = 0; col < r; ++col)
        {
            for (int row = 0; row < m; ++row)
            {
                U[row][col] = Uortho[col][row];
            }
        }
    }

    return { U, Sigma, V };
}

// ==========================================
// Utilitários
// ==========================================
Matrix LinearSystemSolver::multiplyMatrices(const Matrix &A, const Matrix &B)
{
    int rA = static_cast<int>(A.size()), cA = static_cast<int>(A[0].size());
    int rB = static_cast<int>(B.size()), cB = static_cast<int>(B[0].size());
    Matrix C(rA, Vector(cB, 0.0));
    for (int i = 0; i < rA; ++i)
        for (int j = 0; j < cB; ++j)
            for (int k = 0; k < cA; ++k)
                C[i][j] += A[i][k] * B[k][j];
    return C;
}

Vector LinearSystemSolver::multiplyMatrixVector(const Matrix &A, const Vector &v)
{
    int r = static_cast<int>(A.size()), c = static_cast<int>(A[0].size());
    Vector res(r, 0.0);
    for (int i = 0; i < r; ++i)
        for (int j = 0; j < c; ++j)
            res[i] += A[i][j] * v[j];
    return res;
}

Matrix LinearSystemSolver::transposeMatrix(const Matrix &A)
{
    int r = static_cast<int>(A.size()), c = static_cast<int>(A[0].size());
    Matrix At(c, Vector(r, 0.0));
    for (int i = 0; i < r; ++i)
        for (int j = 0; j < c; ++j)
            At[j][i] = A[i][j];
    return At;
}

double LinearSystemSolver::vectorNorm(const Vector &v)
{
    double sum = 0.0;
    for (double val : v)
        sum += val * val;
    return std::sqrt(sum);
}

void LinearSystemSolver::printMatrix(const Matrix &A, const std::string &name)
{
    if (!name.empty())
        std::cout << "=== " << name << " ===\n";
    for (const auto &row : A)
    {
        for (double val : row)
            std::cout << std::setw(10) << std::fixed << std::setprecision(4) << val << " ";
        std::cout << "\n";
    }
    std::cout << "\n";
}

void LinearSystemSolver::printVector(const Vector &v, const std::string &name)
{
    if (!name.empty())
        std::cout << "=== " << name << " ===\n";
    for (double val : v)
        std::cout << std::setw(10) << std::fixed << std::setprecision(4) << val << " ";
    std::cout << "\n\n";
}