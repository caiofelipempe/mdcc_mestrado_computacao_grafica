#include <random>

// ============================================================
// CLASSE UTILS PARA OPERACOES DE ALGEBRA LINEAR
// ============================================================
class LinearAlgebraUtils {
public:
    // Eliminacao de Gauss com Pivotamento Total
    static bool solveTotalPivot(
        const std::vector<std::vector<double>>& A,
        const std::vector<double>& b,
        std::vector<double>& x
    ) {
        int n = A.size();
        auto matrix = A;
        auto vectorB = b;
        
        std::vector<int> varOrder(n);
        std::iota(varOrder.begin(), varOrder.end(), 0);
        
        for (int k = 0; k < n - 1; ++k) {
            int pivotRow = k;
            int pivotCol = k;
            double maxVal = std::abs(matrix[k][k]);
            
            for (int i = k; i < n; ++i) {
                for (int j = k; j < n; ++j) {
                    if (std::abs(matrix[i][j]) > maxVal) {
                        maxVal = std::abs(matrix[i][j]);
                        pivotRow = i;
                        pivotCol = j;
                    }
                }
            }
            
            if (maxVal < 1e-9) {
                return false;
            }
            
            if (pivotRow != k) {
                std::swap(matrix[k], matrix[pivotRow]);
                std::swap(vectorB[k], vectorB[pivotRow]);
            }
            
            if (pivotCol != k) {
                for (int i = 0; i < n; ++i) {
                    std::swap(matrix[i][k], matrix[i][pivotCol]);
                }
                std::swap(varOrder[k], varOrder[pivotCol]);
            }
            
            for (int i = k + 1; i < n; ++i) {
                double factor = matrix[i][k] / matrix[k][k];
                for (int j = k; j < n; ++j) {
                    matrix[i][j] -= factor * matrix[k][j];
                }
                vectorB[i] -= factor * vectorB[k];
            }
        }
        
        if (std::abs(matrix[n-1][n-1]) < 1e-9) {
            return false;
        }
        
        std::vector<double> xSolved(n, 0.0);
        for (int i = n - 1; i >= 0; --i) {
            double sum = 0.0;
            for (int j = i + 1; j < n; ++j) {
                sum += matrix[i][j] * xSolved[j];
            }
            xSolved[i] = (vectorB[i] - sum) / matrix[i][i];
        }
        
        x.assign(n, 0.0);
        for (int i = 0; i < n; ++i) {
            x[varOrder[i]] = xSolved[i];
        }
        
        return true;
    }
    
    // Eliminacao de Gauss com Pivotamento Parcial
    static bool solvePartialPivot(
        const std::vector<std::vector<double>>& A,
        const std::vector<double>& b,
        std::vector<double>& x
    ) {
        int n = A.size();
        auto matrix = A;
        auto vectorB = b;
        
        for (int k = 0; k < n - 1; ++k) {
            int pivotRow = k;
            double maxVal = std::abs(matrix[k][k]);
            
            for (int i = k + 1; i < n; ++i) {
                if (std::abs(matrix[i][k]) > maxVal) {
                    maxVal = std::abs(matrix[i][k]);
                    pivotRow = i;
                }
            }
            
            if (maxVal < 1e-9) {
                return false;
            }
            
            if (pivotRow != k) {
                std::swap(matrix[k], matrix[pivotRow]);
                std::swap(vectorB[k], vectorB[pivotRow]);
            }
            
            for (int i = k + 1; i < n; ++i) {
                double factor = matrix[i][k] / matrix[k][k];
                for (int j = k; j < n; ++j) {
                    matrix[i][j] -= factor * matrix[k][j];
                }
                vectorB[i] -= factor * vectorB[k];
            }
        }
        
        if (std::abs(matrix[n-1][n-1]) < 1e-9) {
            return false;
        }
        
        x.assign(n, 0.0);
        for (int i = n - 1; i >= 0; --i) {
            double sum = 0.0;
            for (int j = i + 1; j < n; ++j) {
                sum += matrix[i][j] * x[j];
            }
            x[i] = (vectorB[i] - sum) / matrix[i][i];
        }
        
        return true;
    }
    
    // Calcula o erro de A*x - b
    static double computeError(
        const std::vector<std::vector<double>>& A,
        const std::vector<double>& b,
        const std::vector<double>& x
    ) {
        int n = A.size();
        double maxError = 0.0;
        
        for (int i = 0; i < n; ++i) {
            double sum = 0.0;
            for (int j = 0; j < n; ++j) {
                sum += A[i][j] * x[j];
            }
            double error = std::abs(sum - b[i]);
            maxError = std::max(maxError, error);
        }
        
        return maxError;
    }
    
    // Gera um sistema aleatorio com solucao conhecida
    static void generateRandomSystem(
        std::vector<std::vector<double>>& A,
        std::vector<double>& b,
        int size,
        double minVal = -10.0,
        double maxVal = 10.0
    ) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dis(minVal, maxVal);
        
        // Redimensiona a matriz e o vetor
        A.assign(size, std::vector<double>(size, 0.0));
        b.assign(size, 0.0);
        
        // Gera a matriz aleatoria
        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                A[i][j] = dis(gen);
            }
        }
        
        // Gera solucao conhecida
        std::vector<double> x_sol(size);
        for (int i = 0; i < size; ++i) {
            x_sol[i] = dis(gen);
        }
        
        // Calcula b = A * x_sol
        for (int i = 0; i < size; ++i) {
            double sum = 0.0;
            for (int j = 0; j < size; ++j) {
                sum += A[i][j] * x_sol[j];
            }
            b[i] = sum;
        }
    }
    
    // Inicializa uma matriz identidade
    static void identityMatrix(std::vector<std::vector<double>>& A, int size) {
        A.assign(size, std::vector<double>(size, 0.0));
        for (int i = 0; i < size; ++i) {
            A[i][i] = 1.0;
        }
    }
};