#include "linear_system_solver.hpp"
#include <iostream>
#include <random>
#include <iomanip>

int main()
{
    // Configura fluxo de saída para console com formatação decimal limpa
    std::cout << std::fixed << std::setprecision(4);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(-5.0, 5.0);

    std::vector<int> tamanhos = {3, 4, 5, 10};

    std::cout << "====================================================\n";
    std::cout << " INICIANDO TESTES, MATRIZES E RESULTADOS (01 - 12)\n";
    std::cout << "====================================================\n\n";

    for (int n : tamanhos)
    {
        std::string separator = "\n====================================================\n";
        std::string header = separator + " >>> TAMANHO DAS MATRIZES: N = " + std::to_string(n) + " <<<\n" + separator + "\n";
        
        std::cout << header;

        // Geração de matrizes e vetores aleatórios
        LinearSystemSolver::Matrix A(n, LinearSystemSolver::Vector(n));
        LinearSystemSolver::Matrix A_sym(n, LinearSystemSolver::Vector(n));
        LinearSystemSolver::Vector b(n);
        LinearSystemSolver::Vector x0(n, 1.0);
        
        LinearSystemSolver::Vector x_reg(n);
        LinearSystemSolver::Vector y_reg(n);

        for (int i = 0; i < n; ++i)
        {
            b[i] = dist(gen);
            x_reg[i] = static_cast<double>(i + 1);
            y_reg[i] = 2.0 * x_reg[i] + dist(gen) * 0.1;
            for (int j = 0; j < n; ++j)
            {
                A[i][j] = dist(gen);
            }
        }

        // Matriz simétrica positiva definida para Cholesky e Autovalores
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j <= i; ++j)
            {
                double val = dist(gen);
                A_sym[i][j] = val;
                A_sym[j][i] = val;
            }
            A_sym[i][i] += 25.0; // Garante dominância diagonal positiva
        }

        // Impressão da Matriz Original A
        std::cout << "--- Matriz Original A (" << n << "x" << n << ") ---\n";
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j)
            {
                std::cout << std::setw(9) << A[i][j] << " ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";

        // 1 & 2. Sistemas Lineares (Gauss & Gauss-Jordan)
        try {
            auto x_gauss = LinearSystemSolver::gaussPartialPivot(A, b);
            std::cout << "[Tarefa 01] Solução por Eliminação de Gauss (Pivoteamento Parcial):\n";
            for (double val : x_gauss) {
                std::cout << std::setw(9) << val << " ";
            }
            std::cout << "\n\n";

            auto x_gj = LinearSystemSolver::gaussJordanTotalPivot(A, b);
            std::cout << "[Tarefa 02] Solução por Gauss-Jordan (Pivoteamento Total):\n";
            for (double val : x_gj) {
                std::cout << std::setw(9) << val << " ";
            }
            std::cout << "\n\n";
        } catch (const std::exception& e) {
            std::cout << "[Tarefas 01/02] Erro: " << e.what() << "\n";
        }

        // 3. Decomposição LU
        try {
            LinearSystemSolver::Matrix L, U;
            LinearSystemSolver::luDecomposition(A, L, U);
            std::cout << "[Tarefa 03] Decomposição LU (A = L * U):\n";
            
            std::cout << "Matriz L (Triangular Inferior):\n";
            for (const auto& row : L) {
                for (double val : row) { std::cout << std::setw(9) << val << " "; }
                std::cout << "\n";
            }
            
            std::cout << "Matriz U (Triangular Superior):\n";
            for (const auto& row : U) {
                for (double val : row) { std::cout << std::setw(9) << val << " "; }
                std::cout << "\n";
            }
            std::cout << "\n";
        } catch (const std::exception& e) {
            std::cout << "[Tarefa 03] Exceção LU: " << e.what() << "\n\n";
        }

        // 4. RREF, Posto e Nulo
        auto rrefRes = LinearSystemSolver::computeRREF(A);
        std::cout << "[Tarefa 04] RREF, Posto e Nulo:\n"
                  << " -> Posto (Rank): " << rrefRes.rank << "\n"
                  << " -> Dimensão do Espaço Nulo (Nullity): " << rrefRes.nullity << "\n";
        
        std::cout << "Matriz RREF:\n";
        for (const auto& row : rrefRes.rrefMatrix) {
            for (double val : row) { std::cout << std::setw(9) << val << " "; }
            std::cout << "\n";
        }
        std::cout << "\n";

        // 5. Cholesky
        try {
            auto S = LinearSystemSolver::choleskyDecomposition(A_sym);
            std::cout << "[Tarefa 05] Decomposição de Cholesky (S * S^T):\n";
            for (const auto& row : S) {
                for (double val : row) { std::cout << std::setw(9) << val << " "; }
                std::cout << "\n";
            }
            std::cout << "\n";
        } catch (const std::exception& e) {
            std::cout << "[Tarefa 05] Parada: " << e.what() << "\n\n";
        }

        // 6. Gram-Schmidt
        LinearSystemSolver::Matrix basis;
        for (int i = 0; i < std::min(2, n); ++i)
        {
            LinearSystemSolver::Vector v(n, 0.0);
            v[i] = 1.0;
            basis.push_back(v);
        }
        auto extended = LinearSystemSolver::extendBasis(basis, n);
        auto orthonormal = LinearSystemSolver::gramSchmidtOrthonormalize(extended);
        std::cout << "[Tarefa 06] Gram-Schmidt (Base Ortonormal Gerada):\n";
        for (const auto& row : orthonormal) {
            for (double val : row) { std::cout << std::setw(9) << val << " "; }
            std::cout << "\n";
        }
        std::cout << "\n";

        // 7. Regressão Linear
        auto reg = LinearSystemSolver::linearRegression(x_reg, y_reg);
        std::cout << "[Tarefa 07] Regressão Linear por Mínimos Quadrados:\n"
                  << " -> Equação da Reta: y = " << reg.slope << " * x + " << reg.intercept << "\n\n";

        // 8. Método de Potência
        try {
            auto powRes = LinearSystemSolver::powerMethod(A_sym, x0, 1e-4, 300);
            std::cout << "[Tarefa 08] Método de Potência Regular:\n"
                      << " -> Maior Autovalor: " << powRes.eigenvalue << "\n"
                      << " -> Iterações: " << powRes.iterations << "\n\n";
        } catch (const std::exception& e) {
            std::cout << "[Tarefa 08] Erro: " << e.what() << "\n\n";
        }

        // 9. Householder
        auto houseRes = LinearSystemSolver::householderTransform(A_sym, false);
        std::cout << "[Tarefa 09] Transformação de Householder (Matriz Tridiagonal):\n";
        for (const auto& row : houseRes.outputMatrix) {
            for (double val : row) { std::cout << std::setw(9) << val << " "; }
            std::cout << "\n";
        }
        std::cout << "\n";

        // 10. Decomposição QR
        auto qr = LinearSystemSolver::qrDecomposition(A);
        std::cout << "[Tarefa 10] Decomposição QR (A = Q * R):\n";
        
        std::cout << "Matriz Q (Ortogonal):\n";
        for (const auto& row : qr.Q) {
            for (double val : row) { std::cout << std::setw(9) << val << " "; }
            std::cout << "\n";
        }
        
        std::cout << "Matriz R (Triangular Superior):\n";
        for (const auto& row : qr.R) {
            for (double val : row) { std::cout << std::setw(9) << val << " "; }
            std::cout << "\n";
        }
        std::cout << "\n";

        // 11. Autovalores QR
        auto eigenRes = LinearSystemSolver::qrEigenSymmetric(A_sym, 1e-4);
        std::cout << "[Tarefa 11] Autovalores via Método QR:\n";
        for (double val : eigenRes.eigenvalues) {
            std::cout << std::setw(9) << val << " ";
        }
        std::cout << "\n\n";

        // 12. Decomposição SVD
        auto svd = LinearSystemSolver::svdDecomposition(A);
        std::cout << "[Tarefa 12] Decomposição SVD (U, Sigma, V):\n";
        
        std::cout << "Matriz U:\n";
        for (const auto& row : svd.U) {
            for (double val : row) { std::cout << std::setw(9) << val << " "; }
            std::cout << "\n";
        }
        
        std::cout << "Matriz Sigma:\n";
        for (const auto& row : svd.Sigma) {
            for (double val : row) { std::cout << std::setw(9) << val << " "; }
            std::cout << "\n";
        }
        
        std::cout << "Matriz V:\n";
        for (const auto& row : svd.V) {
            for (double val : row) { std::cout << std::setw(9) << val << " "; }
            std::cout << "\n";
        }
        std::cout << "\n====================================================\n\n";
    }

    std::cout << "CONCLUÍDO! Todos os testes foram executados com saída direcionada apenas para o console.\n";

    return 0;
}