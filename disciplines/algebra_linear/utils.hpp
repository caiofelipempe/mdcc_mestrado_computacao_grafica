#include <random>
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <stdexcept>

// ============================================================
// CLASSE UTILS PARA OPERACOES DE ALGEBRA LINEAR
// ============================================================
class Utils {
public:
    // Enumeracao para tipo de pivotacao
    enum class PivotType {
        PARTIAL,
        TOTAL
    };

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
            int maxRow = k;
            double maxVal = std::abs(matrix[k][k]);
            
            for (int i = k + 1; i < n; ++i) {
                if (std::abs(matrix[i][k]) > maxVal) {
                    maxVal = std::abs(matrix[i][k]);
                    maxRow = i;
                }
            }
            
            if (maxVal < 1e-9) {
                return false;
            }
            
            if (maxRow != k) {
                std::swap(matrix[k], matrix[maxRow]);
                std::swap(vectorB[k], vectorB[maxRow]);
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
    
    // Eliminacao de Gauss com Pivotamento Total
    static bool solveTotalPivot(
        const std::vector<std::vector<double>>& A,
        const std::vector<double>& b,
        std::vector<double>& x
    ) {
        int n = A.size();
        
        auto matrix = A;
        auto vectorB = b;
        
        std::vector<int> perm(n);
        std::iota(perm.begin(), perm.end(), 0);
        
        for (int k = 0; k < n - 1; ++k) {
            int maxRow = k;
            int maxCol = k;
            double maxVal = std::abs(matrix[k][k]);
            
            for (int i = k; i < n; ++i) {
                for (int j = k; j < n; ++j) {
                    if (std::abs(matrix[i][j]) > maxVal) {
                        maxVal = std::abs(matrix[i][j]);
                        maxRow = i;
                        maxCol = j;
                    }
                }
            }
            
            if (maxVal < 1e-9) {
                return false;
            }
            
            if (maxRow != k) {
                std::swap(matrix[k], matrix[maxRow]);
                std::swap(vectorB[k], vectorB[maxRow]);
            }
            
            if (maxCol != k) {
                for (int i = 0; i < n; ++i) {
                    std::swap(matrix[i][k], matrix[i][maxCol]);
                }
                std::swap(perm[k], perm[maxCol]);
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
            x[perm[i]] = xSolved[i];
        }
        
        return true;
    }
    
    // Metodo de Gauss-Jordan com suporte a pivotacao parcial e total
    static bool solveGaussJordan(
        const std::vector<std::vector<double>>& A,
        const std::vector<double>& b,
        std::vector<double>& x,
        PivotType pivotType = PivotType::PARTIAL
    ) {
        int n = A.size();
        
        // Copia a matriz e o vetor para nao modificar os originais
        auto matrix = A;
        auto vectorB = b;
        
        // Vetor de permutacao para pivotacao total
        std::vector<int> permutation(n);
        std::iota(permutation.begin(), permutation.end(), 0);
        
        try {
            for (int k = 0; k < n; ++k) {
                // --- Pivotacao ---
                if (pivotType == PivotType::PARTIAL) {
                    // Pivotacao Parcial: encontra o maior elemento na coluna k
                    int maxRow = k;
                    double maxVal = std::abs(matrix[k][k]);
                    
                    for (int i = k + 1; i < n; ++i) {
                        if (std::abs(matrix[i][k]) > maxVal) {
                            maxVal = std::abs(matrix[i][k]);
                            maxRow = i;
                        }
                    }
                    
                    // Troca as linhas se necessario
                    if (maxRow != k) {
                        std::swap(matrix[k], matrix[maxRow]);
                        std::swap(vectorB[k], vectorB[maxRow]);
                    }
                    
                } else if (pivotType == PivotType::TOTAL) {
                    // Pivotacao Total: encontra o maior elemento na submatriz restante
                    int maxRow = k;
                    int maxCol = k;
                    double maxVal = std::abs(matrix[k][k]);
                    
                    for (int i = k; i < n; ++i) {
                        for (int j = k; j < n; ++j) {
                            if (std::abs(matrix[i][j]) > maxVal) {
                                maxVal = std::abs(matrix[i][j]);
                                maxRow = i;
                                maxCol = j;
                            }
                        }
                    }
                    
                    // Troca as linhas se necessario
                    if (maxRow != k) {
                        std::swap(matrix[k], matrix[maxRow]);
                        std::swap(vectorB[k], vectorB[maxRow]);
                    }
                    
                    // Troca as colunas se necessario
                    if (maxCol != k) {
                        for (int i = 0; i < n; ++i) {
                            std::swap(matrix[i][k], matrix[i][maxCol]);
                        }
                        std::swap(permutation[k], permutation[maxCol]);
                    }
                }
                
                // Verifica se o pivô e zero
                double pivot = matrix[k][k];
                if (std::abs(pivot) < 1e-9) {
                    return false; // Matriz singular
                }
                
                // Normaliza a linha do pivô
                for (int j = k; j < n; ++j) {
                    matrix[k][j] /= pivot;
                }
                vectorB[k] /= pivot;
                
                // Zera os elementos acima e abaixo do pivô
                for (int i = 0; i < n; ++i) {
                    if (i != k) {
                        double factor = matrix[i][k];
                        for (int j = k; j < n; ++j) {
                            matrix[i][j] -= factor * matrix[k][j];
                        }
                        vectorB[i] -= factor * vectorB[k];
                    }
                }
            }
            
            // Ajusta a ordem das solucoes se foi feita pivotacao total
            x.assign(n, 0.0);
            if (pivotType == PivotType::TOTAL) {
                for (int i = 0; i < n; ++i) {
                    x[permutation[i]] = vectorB[i];
                }
            } else {
                x = vectorB;
            }
            
            return true;
            
        } catch (const std::exception&) {
            return false;
        }
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
        
        A.assign(size, std::vector<double>(size, 0.0));
        b.assign(size, 0.0);
        
        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                A[i][j] = dis(gen);
            }
        }
        
        std::vector<double> x_sol(size);
        for (int i = 0; i < size; ++i) {
            x_sol[i] = dis(gen);
        }
        
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