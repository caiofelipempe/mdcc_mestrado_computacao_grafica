// linear_algebra.h
// ---------------------------------------------------------------------------
// Matriz densa rows×cols (row-major) e geradores/verificadores de matrizes
// especiais: diagonal, triangular, simétrica e ortogonal.
//
// Os geradores aleatórios usam rand(); chame srand(semente) antes para obter
// resultados reproduzíveis.
// ---------------------------------------------------------------------------
#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    double *data; // rows*cols elementos, row-major: a(i, j) = data[i*cols + j]
    size_t rows;
    size_t cols;
} Matrix;

// Acesso ao elemento (i, j). Sem verificação de limites.
static inline double *matrix_at(Matrix *m, size_t i, size_t j)
{
    return &m->data[i * m->cols + j];
}

static inline double matrix_get(const Matrix *m, size_t i, size_t j)
{
    return m->data[i * m->cols + j];
}

static inline void matrix_set(Matrix *m, size_t i, size_t j, double value)
{
    m->data[i * m->cols + j] = value;
}

// ---------------------------------------------------------------------------
// Criação e destruição
// ---------------------------------------------------------------------------

// Cria matriz rows×cols zerada. Retorna NULL se rows ou cols for 0 ou se faltar memória.
Matrix *matrix_create(size_t rows, size_t cols);

// Cria a identidade n×n.
Matrix *matrix_identity(size_t n);

// Cria uma cópia de m.
Matrix *matrix_copy(const Matrix *m);

// Libera a matriz (aceita NULL).
void matrix_destroy(Matrix *m);

// ---------------------------------------------------------------------------
// Preenchimento e geradores aleatórios (valores uniformes em [lo, hi])
// ---------------------------------------------------------------------------

// Número aleatório uniforme em [lo, hi].
double random_uniform(double lo, double hi);

// Preenche todos os elementos de m com valores aleatórios.
void matrix_fill_random(Matrix *m, double lo, double hi);

// Diagonal n×n: diagonal aleatória, demais elementos zero.
Matrix *matrix_random_diagonal(size_t n, double lo, double hi);

// Triangular superior n×n: aleatória para j >= i, zero abaixo da diagonal.
Matrix *matrix_random_upper_triangular(size_t n, double lo, double hi);

// Triangular inferior n×n: aleatória para j <= i, zero acima da diagonal.
Matrix *matrix_random_lower_triangular(size_t n, double lo, double hi);

// Simétrica n×n: sorteia o triângulo superior e espelha.
Matrix *matrix_random_symmetric(size_t n, double lo, double hi);

// Ortogonal n×n (QᵀQ = I): Gram-Schmidt modificado, com reortogonalização,
// aplicado às colunas de uma matriz aleatória.
Matrix *matrix_random_orthogonal(size_t n);

// ---------------------------------------------------------------------------
// Verificações. tol é a tolerância absoluta para considerar um valor zero
// (use 0 para exigir igualdade exata).
// ---------------------------------------------------------------------------

bool matrix_is_square(const Matrix *m);

// a(i, j) = 0 para i != j.
bool matrix_is_diagonal(const Matrix *m, double tol);

// a(i, j) = 0 para i > j.
bool matrix_is_upper_triangular(const Matrix *m, double tol);

// a(i, j) = 0 para i < j.
bool matrix_is_lower_triangular(const Matrix *m, double tol);

// Quadrada e a(i, j) = a(j, i).
bool matrix_is_symmetric(const Matrix *m, double tol);

// Quadrada e QᵀQ = I.
bool matrix_is_orthogonal(const Matrix *m, double tol);

// ---------------------------------------------------------------------------
// Operações básicas
// ---------------------------------------------------------------------------

// Transposta (nova matriz).
Matrix *matrix_transpose(const Matrix *m);

// Produto a·b (nova matriz). NULL se as dimensões não forem compatíveis.
Matrix *matrix_multiply(const Matrix *a, const Matrix *b);

// y = m·x, com x de tamanho cols e y de tamanho rows.
void matrix_mul_vec(const Matrix *m, const double *x, double *y);

// Norma de Frobenius.
double matrix_frobenius_norm(const Matrix *m);

// Imprime a matriz em stdout.
void matrix_print(const Matrix *m, const char *name);

// ---------------------------------------------------------------------------
// Fatorações
// ---------------------------------------------------------------------------

// QR por reflexões de Householder: a (m×n) = q (m×m ortogonal) · r (m×n triangular
// superior). Retorna false se faltar memória.
bool matrix_qr(const Matrix *a, Matrix **q, Matrix **r);

// Tridiagonalização de Householder de uma matriz simétrica: a = q · t · qᵀ, com t
// tridiagonal simétrica e q ortogonal. Retorna false se a não for quadrada ou faltar memória.
bool matrix_tridiagonalize(const Matrix *a, Matrix **q, Matrix **t);

// ---------------------------------------------------------------------------
// Inversa
//
// matrix_inverse identifica a estrutura da matriz e chama o método mais barato que
// se aplica. Os métodos específicos SUPÕEM a estrutura (não verificam) e retornam
// NULL se a matriz for singular. tol é a tolerância das verificações de estrutura.
// ---------------------------------------------------------------------------

// Escolhe o método: diagonal → triangular → ortogonal → simétrica definida positiva
// (Cholesky) → geral (LU com pivoteamento parcial). NULL se não for quadrada ou for singular.
Matrix *matrix_inverse(const Matrix *m, double tol);

// Diagonal: inverte cada elemento da diagonal. O(n).
Matrix *matrix_inverse_diagonal(const Matrix *m);

// Ortogonal: a inversa é a transposta. O(n²).
Matrix *matrix_inverse_orthogonal(const Matrix *m);

// Triangular superior: substituição regressiva, coluna a coluna. O(n³/3).
Matrix *matrix_inverse_upper_triangular(const Matrix *m);

// Triangular inferior: substituição progressiva, coluna a coluna. O(n³/3).
Matrix *matrix_inverse_lower_triangular(const Matrix *m);

// Simétrica definida positiva: m = L·Lᵀ (Cholesky), m⁻¹ = L⁻ᵀ·L⁻¹.
// NULL se a matriz não for definida positiva.
Matrix *matrix_inverse_spd(const Matrix *m);

// Geral: P·m = L·U (pivoteamento parcial) e resolve m·x = eⱼ para cada coluna.
Matrix *matrix_inverse_general(const Matrix *m);

// ---------------------------------------------------------------------------
// Autovalores e autovetores
//
// Critérios de parada:
//   * potência / potência inversa: ‖A·v − λ·v‖ ≤ tol·max(1, |λ|);
//   * QR, Jacobi e Householder+QR: elementos fora da diagonal ≤ tol·‖A‖_F.
// ---------------------------------------------------------------------------

// Um autopar (λ, v), com v unitário e com a componente de maior módulo positiva.
typedef struct
{
    double value;
    double *vector; // n elementos; NULL em caso de erro
    size_t n;
    int iterations;
    bool converged;
    double residual; // ‖A·v − λ·v‖₂
} EigenPair;

// Todos os autopares de uma matriz simétrica, em ordem crescente de autovalor.
// A coluna j de vectors é o autovetor (unitário) de values[j].
typedef struct
{
    double *values; // n elementos; NULL em caso de erro
    Matrix *vectors;
    size_t n;
    int iterations; // iterações (QR) ou varreduras (Jacobi)
    bool converged;
} EigenDecomposition;

void eigen_pair_destroy(EigenPair *p);
void eigen_decomposition_destroy(EigenDecomposition *d);

// Maior ‖A·vⱼ − λⱼ·vⱼ‖₂ entre os autopares da decomposição.
double eigen_decomposition_residual(const Matrix *a, const EigenDecomposition *d);

// Método da potência: autovalor de MAIOR módulo. v0 é o chute inicial (NULL → (1, ..., 1)).
// Converge com taxa |λ₂/λ₁|; não converge se |λ₁| = |λ₂| com λ₁ ≠ λ₂ (ex.: λ e −λ).
EigenPair eigen_power(const Matrix *a, const double *v0, double tol, int max_iter);

// Método da potência inversa com deslocamento: autovalor MAIS PRÓXIMO de shift
// (shift = 0 → menor |λ|). (A − shift·I) é fatorada uma vez (LU); falha (vector = NULL)
// se shift for exatamente um autovalor.
EigenPair eigen_inverse_power(const Matrix *a, double shift, const double *v0, double tol,
                              int max_iter);

// Iteração QR sem deslocamento: Aₖ = Qₖ·Rₖ, Aₖ₊₁ = Rₖ·Qₖ, V ← V·Qₖ. Só para simétricas.
// Lenta quando há autovalores de módulo próximo; não converge se |λᵢ| = |λⱼ| com λᵢ ≠ λⱼ.
EigenDecomposition eigen_qr_iteration(const Matrix *a, double tol, int max_iter);

// Jacobi cíclico: rotações de Givens que zeram cada a(p, q) fora da diagonal. Só para
// simétricas. max_sweeps limita as varreduras completas.
EigenDecomposition eigen_jacobi(const Matrix *a, double tol, int max_sweeps);

// Householder + QR: tridiagonaliza com Householder e aplica QR implícito com
// deslocamento de Wilkinson e deflação na tridiagonal. Só para simétricas.
EigenDecomposition eigen_householder_qr(const Matrix *a, double tol, int max_iter);
