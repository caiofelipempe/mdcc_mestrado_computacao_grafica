// linear_algebra.c
#include "linear_algebra.h"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Criação e destruição
// ---------------------------------------------------------------------------

Matrix *matrix_create(size_t rows, size_t cols)
{
    if (rows == 0 || cols == 0 || rows > SIZE_MAX / cols)
        return NULL;

    Matrix *m = malloc(sizeof *m);
    if (!m)
        return NULL;

    m->data = calloc(rows * cols, sizeof *m->data);
    if (!m->data)
    {
        free(m);
        return NULL;
    }

    m->rows = rows;
    m->cols = cols;
    return m;
}

Matrix *matrix_identity(size_t n)
{
    Matrix *m = matrix_create(n, n);
    if (!m)
        return NULL;

    for (size_t i = 0; i < n; ++i)
        matrix_set(m, i, i, 1.0);
    return m;
}

Matrix *matrix_copy(const Matrix *m)
{
    Matrix *c = matrix_create(m->rows, m->cols);
    if (!c)
        return NULL;

    memcpy(c->data, m->data, m->rows * m->cols * sizeof *m->data);
    return c;
}

void matrix_destroy(Matrix *m)
{
    if (!m)
        return;
    free(m->data);
    free(m);
}

// ---------------------------------------------------------------------------
// Preenchimento e geradores aleatórios
// ---------------------------------------------------------------------------

double random_uniform(double lo, double hi)
{
    return lo + (hi - lo) * ((double)rand() / (double)RAND_MAX);
}

void matrix_fill_random(Matrix *m, double lo, double hi)
{
    for (size_t k = 0; k < m->rows * m->cols; ++k)
        m->data[k] = random_uniform(lo, hi);
}

Matrix *matrix_random_diagonal(size_t n, double lo, double hi)
{
    Matrix *m = matrix_create(n, n);
    if (!m)
        return NULL;

    for (size_t i = 0; i < n; ++i)
        matrix_set(m, i, i, random_uniform(lo, hi));
    return m;
}

Matrix *matrix_random_upper_triangular(size_t n, double lo, double hi)
{
    Matrix *m = matrix_create(n, n);
    if (!m)
        return NULL;

    for (size_t i = 0; i < n; ++i)
        for (size_t j = i; j < n; ++j)
            matrix_set(m, i, j, random_uniform(lo, hi));
    return m;
}

Matrix *matrix_random_lower_triangular(size_t n, double lo, double hi)
{
    Matrix *m = matrix_create(n, n);
    if (!m)
        return NULL;

    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j <= i; ++j)
            matrix_set(m, i, j, random_uniform(lo, hi));
    return m;
}

Matrix *matrix_random_symmetric(size_t n, double lo, double hi)
{
    Matrix *m = matrix_create(n, n);
    if (!m)
        return NULL;

    for (size_t i = 0; i < n; ++i)
        for (size_t j = i; j < n; ++j)
        {
            const double v = random_uniform(lo, hi);
            matrix_set(m, i, j, v);
            matrix_set(m, j, i, v);
        }
    return m;
}

// Produto interno das colunas a e b de m.
static double column_dot(const Matrix *m, size_t a, size_t b)
{
    double s = 0.0;
    for (size_t i = 0; i < m->rows; ++i)
        s += matrix_get(m, i, a) * matrix_get(m, i, b);
    return s;
}

// Subtrai da coluna j sua projeção sobre a coluna k (supõe a coluna k unitária).
static void column_remove_projection(Matrix *m, size_t j, size_t k)
{
    const double p = column_dot(m, j, k);
    for (size_t i = 0; i < m->rows; ++i)
        *matrix_at(m, i, j) -= p * matrix_get(m, i, k);
}

Matrix *matrix_random_orthogonal(size_t n)
{
    Matrix *q = matrix_create(n, n);
    if (!q)
        return NULL;

    matrix_fill_random(q, -1.0, 1.0);

    for (size_t j = 0; j < n; ++j)
    {
        double norm = 0.0;

        // Uma coluna aleatória é quase sempre linearmente independente das anteriores;
        // se cair (quase) no espaço delas, sorteia de novo.
        do
        {
            // Duas passadas de Gram-Schmidt modificado ("twice is enough") para
            // manter a ortogonalidade perto da precisão de máquina.
            for (int pass = 0; pass < 2; ++pass)
                for (size_t k = 0; k < j; ++k)
                    column_remove_projection(q, j, k);

            norm = sqrt(column_dot(q, j, j));

            if (norm < 1e-8)
                for (size_t i = 0; i < n; ++i)
                    matrix_set(q, i, j, random_uniform(-1.0, 1.0));
        } while (norm < 1e-8);

        for (size_t i = 0; i < n; ++i)
            *matrix_at(q, i, j) /= norm;
    }

    return q;
}

// ---------------------------------------------------------------------------
// Verificações
// ---------------------------------------------------------------------------

bool matrix_is_square(const Matrix *m)
{
    return m->rows == m->cols;
}

bool matrix_is_diagonal(const Matrix *m, double tol)
{
    for (size_t i = 0; i < m->rows; ++i)
        for (size_t j = 0; j < m->cols; ++j)
            if (i != j && fabs(matrix_get(m, i, j)) > tol)
                return false;
    return true;
}

bool matrix_is_upper_triangular(const Matrix *m, double tol)
{
    for (size_t i = 1; i < m->rows; ++i)
        for (size_t j = 0; j < i && j < m->cols; ++j)
            if (fabs(matrix_get(m, i, j)) > tol)
                return false;
    return true;
}

bool matrix_is_lower_triangular(const Matrix *m, double tol)
{
    for (size_t i = 0; i < m->rows; ++i)
        for (size_t j = i + 1; j < m->cols; ++j)
            if (fabs(matrix_get(m, i, j)) > tol)
                return false;
    return true;
}

bool matrix_is_symmetric(const Matrix *m, double tol)
{
    if (!matrix_is_square(m))
        return false;

    for (size_t i = 0; i < m->rows; ++i)
        for (size_t j = i + 1; j < m->cols; ++j)
            if (fabs(matrix_get(m, i, j) - matrix_get(m, j, i)) > tol)
                return false;
    return true;
}

bool matrix_is_orthogonal(const Matrix *m, double tol)
{
    if (!matrix_is_square(m))
        return false;

    // (QᵀQ)(a, b) é o produto interno das colunas a e b; deve ser δ(a, b).
    for (size_t a = 0; a < m->cols; ++a)
        for (size_t b = a; b < m->cols; ++b)
        {
            const double expected = (a == b) ? 1.0 : 0.0;
            if (fabs(column_dot(m, a, b) - expected) > tol)
                return false;
        }
    return true;
}

// ---------------------------------------------------------------------------
// Operações básicas
// ---------------------------------------------------------------------------

Matrix *matrix_transpose(const Matrix *m)
{
    Matrix *t = matrix_create(m->cols, m->rows);
    if (!t)
        return NULL;

    for (size_t i = 0; i < m->rows; ++i)
        for (size_t j = 0; j < m->cols; ++j)
            matrix_set(t, j, i, matrix_get(m, i, j));
    return t;
}

Matrix *matrix_multiply(const Matrix *a, const Matrix *b)
{
    if (a->cols != b->rows)
        return NULL;

    Matrix *c = matrix_create(a->rows, b->cols);
    if (!c)
        return NULL;

    // Ordem i-k-j: percorre b e c por linha (acesso contíguo).
    for (size_t i = 0; i < a->rows; ++i)
        for (size_t k = 0; k < a->cols; ++k)
        {
            const double aik = matrix_get(a, i, k);
            for (size_t j = 0; j < b->cols; ++j)
                *matrix_at(c, i, j) += aik * matrix_get(b, k, j);
        }
    return c;
}

void matrix_mul_vec(const Matrix *m, const double *x, double *y)
{
    for (size_t i = 0; i < m->rows; ++i)
    {
        double s = 0.0;
        for (size_t j = 0; j < m->cols; ++j)
            s += matrix_get(m, i, j) * x[j];
        y[i] = s;
    }
}

double matrix_frobenius_norm(const Matrix *m)
{
    double s = 0.0;
    for (size_t k = 0; k < m->rows * m->cols; ++k)
        s += m->data[k] * m->data[k];
    return sqrt(s);
}

void matrix_print(const Matrix *m, const char *name)
{
    printf("%s (%zux%zu):\n", name ? name : "", m->rows, m->cols);
    for (size_t i = 0; i < m->rows; ++i)
    {
        printf("  [");
        for (size_t j = 0; j < m->cols; ++j)
            printf(" %10.5f", matrix_get(m, i, j));
        printf(" ]\n");
    }
}

// --- vetores (uso interno) ---

static double vec_dot(const double *a, const double *b, size_t n)
{
    double s = 0.0;
    for (size_t i = 0; i < n; ++i)
        s += a[i] * b[i];
    return s;
}

static double vec_norm(const double *v, size_t n)
{
    return sqrt(vec_dot(v, v, n));
}

// Normaliza v. Retorna false se v for nulo.
static bool vec_normalize(double *v, size_t n)
{
    const double norm = vec_norm(v, n);
    if (norm == 0.0)
        return false;
    for (size_t i = 0; i < n; ++i)
        v[i] /= norm;
    return true;
}

// Fixa o sinal (componente de maior módulo positiva) para resultados comparáveis.
static void vec_fix_sign(double *v, size_t n)
{
    size_t k = 0;
    for (size_t i = 1; i < n; ++i)
        if (fabs(v[i]) > fabs(v[k]))
            k = i;
    if (v[k] < 0.0)
        for (size_t i = 0; i < n; ++i)
            v[i] = -v[i];
}

// Maior |a(i, j)|, base das tolerâncias de singularidade.
static double max_abs(const Matrix *m)
{
    double r = 0.0;
    for (size_t k = 0; k < m->rows * m->cols; ++k)
        r = fmax(r, fabs(m->data[k]));
    return r;
}

// Pivôs com módulo até este valor são tratados como zero (matriz singular).
static double singular_threshold(const Matrix *m)
{
    return (double)m->rows * DBL_EPSILON * max_abs(m);
}

// ---------------------------------------------------------------------------
// Fatorações
// ---------------------------------------------------------------------------

// Constrói em v (tamanho len) o vetor unitário de Householder que leva x em
// (alpha, 0, ..., 0), com alpha = −sinal(x₀)·‖x‖ (evita cancelamento).
// Retorna false se x já é nulo (nada a refletir).
static bool householder_vector(const double *x, double *v, size_t len)
{
    const double norm = vec_norm(x, len);
    if (norm == 0.0)
        return false;

    const double alpha = (x[0] >= 0.0) ? -norm : norm;
    for (size_t i = 0; i < len; ++i)
        v[i] = x[i];
    v[0] -= alpha;
    return vec_normalize(v, len);
}

// H = I − 2·v·vᵀ atuando nas linhas first..first+len−1 de m (m ← H·m).
static void householder_apply_rows(Matrix *m, const double *v, size_t first, size_t len)
{
    for (size_t j = 0; j < m->cols; ++j)
    {
        double s = 0.0;
        for (size_t i = 0; i < len; ++i)
            s += v[i] * matrix_get(m, first + i, j);
        for (size_t i = 0; i < len; ++i)
            *matrix_at(m, first + i, j) -= 2.0 * v[i] * s;
    }
}

// H atuando nas colunas first..first+len−1 de m (m ← m·H).
static void householder_apply_cols(Matrix *m, const double *v, size_t first, size_t len)
{
    for (size_t i = 0; i < m->rows; ++i)
    {
        double s = 0.0;
        for (size_t j = 0; j < len; ++j)
            s += v[j] * matrix_get(m, i, first + j);
        for (size_t j = 0; j < len; ++j)
            *matrix_at(m, i, first + j) -= 2.0 * v[j] * s;
    }
}

bool matrix_qr(const Matrix *a, Matrix **q_out, Matrix **r_out)
{
    const size_t m = a->rows, n = a->cols;
    Matrix *q = matrix_identity(m);
    Matrix *r = matrix_copy(a);
    double *x = malloc(m * sizeof *x);
    double *v = malloc(m * sizeof *v);

    if (!q || !r || !x || !v)
    {
        matrix_destroy(q);
        matrix_destroy(r);
        free(x);
        free(v);
        return false;
    }

    // Hₖ···H₀·A = R  ⇒  A = (H₀···Hₖ)·R = Q·R.
    const size_t steps = (m - 1 < n) ? m - 1 : n;
    for (size_t k = 0; k < steps; ++k)
    {
        const size_t len = m - k;
        for (size_t i = 0; i < len; ++i)
            x[i] = matrix_get(r, k + i, k);

        if (!householder_vector(x, v, len))
            continue;

        householder_apply_rows(r, v, k, len);
        householder_apply_cols(q, v, k, len);

        // Abaixo da diagonal sobra só erro de arredondamento.
        for (size_t i = k + 1; i < m; ++i)
            matrix_set(r, i, k, 0.0);
    }

    free(x);
    free(v);
    *q_out = q;
    *r_out = r;
    return true;
}

bool matrix_tridiagonalize(const Matrix *a, Matrix **q_out, Matrix **t_out)
{
    if (!matrix_is_square(a))
        return false;

    const size_t n = a->rows;
    Matrix *q = matrix_identity(n);
    Matrix *t = matrix_copy(a);
    double *x = malloc(n * sizeof *x);
    double *v = malloc(n * sizeof *v);

    if (!q || !t || !x || !v)
    {
        matrix_destroy(q);
        matrix_destroy(t);
        free(x);
        free(v);
        return false;
    }

    // Para cada coluna k, zera t(k+2.., k) com Hₖ atuando nas linhas/colunas k+1..n−1.
    // A semelhança t ← Hₖ·t·Hₖ preserva os autovalores e a simetria.
    for (size_t k = 0; k + 2 < n; ++k)
    {
        const size_t first = k + 1, len = n - first;
        for (size_t i = 0; i < len; ++i)
            x[i] = matrix_get(t, first + i, k);

        if (!householder_vector(x, v, len))
            continue;

        householder_apply_rows(t, v, first, len);
        householder_apply_cols(t, v, first, len);
        householder_apply_cols(q, v, first, len);
    }

    // Limpa o arredondamento fora da faixa tridiagonal e restaura a simetria exata.
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < i; ++j)
        {
            if (i - j > 1)
                matrix_set(t, i, j, 0.0);
            matrix_set(t, j, i, matrix_get(t, i, j));
        }

    free(x);
    free(v);
    *q_out = q;
    *t_out = t;
    return true;
}

// LU com pivoteamento parcial: P·A = L·U, guardados juntos em lu (L com diagonal
// unitária implícita, abaixo da diagonal; U na diagonal e acima). perm[i] é a linha
// de A que foi para a posição i.
typedef struct
{
    Matrix *lu;
    size_t *perm;
} LU;

static void lu_destroy(LU *f)
{
    matrix_destroy(f->lu);
    free(f->perm);
    f->lu = NULL;
    f->perm = NULL;
}

// Retorna false se A for singular (ou faltar memória).
static bool lu_factor(const Matrix *a, LU *f)
{
    const size_t n = a->rows;
    f->lu = matrix_copy(a);
    f->perm = malloc(n * sizeof *f->perm);
    if (!f->lu || !f->perm)
    {
        lu_destroy(f);
        return false;
    }

    const double eps = singular_threshold(a);
    Matrix *m = f->lu;

    for (size_t i = 0; i < n; ++i)
        f->perm[i] = i;

    for (size_t k = 0; k < n; ++k)
    {
        // Pivô: maior |m(i, k)| na coluna, para limitar o crescimento dos multiplicadores.
        size_t p = k;
        for (size_t i = k + 1; i < n; ++i)
            if (fabs(matrix_get(m, i, k)) > fabs(matrix_get(m, p, k)))
                p = i;

        if (fabs(matrix_get(m, p, k)) <= eps)
        {
            lu_destroy(f);
            return false;
        }

        if (p != k)
        {
            for (size_t j = 0; j < n; ++j)
            {
                const double tmp = matrix_get(m, k, j);
                matrix_set(m, k, j, matrix_get(m, p, j));
                matrix_set(m, p, j, tmp);
            }
            const size_t tp = f->perm[k];
            f->perm[k] = f->perm[p];
            f->perm[p] = tp;
        }

        const double pivot = matrix_get(m, k, k);
        for (size_t i = k + 1; i < n; ++i)
        {
            const double l = matrix_get(m, i, k) / pivot;
            matrix_set(m, i, k, l);
            for (size_t j = k + 1; j < n; ++j)
                *matrix_at(m, i, j) -= l * matrix_get(m, k, j);
        }
    }
    return true;
}

// Resolve A·x = b com a fatoração: L·y = P·b, depois U·x = y.
static void lu_solve(const LU *f, const double *b, double *x)
{
    const Matrix *m = f->lu;
    const size_t n = m->rows;

    for (size_t i = 0; i < n; ++i)
    {
        double s = b[f->perm[i]];
        for (size_t k = 0; k < i; ++k)
            s -= matrix_get(m, i, k) * x[k];
        x[i] = s;
    }
    for (size_t i = n; i-- > 0;)
    {
        double s = x[i];
        for (size_t k = i + 1; k < n; ++k)
            s -= matrix_get(m, i, k) * x[k];
        x[i] = s / matrix_get(m, i, i);
    }
}

// ---------------------------------------------------------------------------
// Inversa
// ---------------------------------------------------------------------------

Matrix *matrix_inverse_diagonal(const Matrix *m)
{
    const size_t n = m->rows;
    const double eps = singular_threshold(m);

    Matrix *inv = matrix_create(n, n);
    if (!inv)
        return NULL;

    for (size_t i = 0; i < n; ++i)
    {
        const double d = matrix_get(m, i, i);
        if (fabs(d) <= eps)
        {
            matrix_destroy(inv);
            return NULL;
        }
        matrix_set(inv, i, i, 1.0 / d);
    }
    return inv;
}

Matrix *matrix_inverse_orthogonal(const Matrix *m)
{
    return matrix_transpose(m);
}

Matrix *matrix_inverse_upper_triangular(const Matrix *m)
{
    const size_t n = m->rows;
    const double eps = singular_threshold(m);

    for (size_t i = 0; i < n; ++i)
        if (fabs(matrix_get(m, i, i)) <= eps)
            return NULL;

    Matrix *inv = matrix_create(n, n);
    if (!inv)
        return NULL;

    // A inversa também é triangular superior. Coluna j: resolve U·x = eⱼ só nas
    // linhas 0..j (as demais são zero), por substituição regressiva.
    for (size_t j = 0; j < n; ++j)
        for (size_t i = j + 1; i-- > 0;)
        {
            double s = (i == j) ? 1.0 : 0.0;
            for (size_t k = i + 1; k <= j; ++k)
                s -= matrix_get(m, i, k) * matrix_get(inv, k, j);
            matrix_set(inv, i, j, s / matrix_get(m, i, i));
        }
    return inv;
}

Matrix *matrix_inverse_lower_triangular(const Matrix *m)
{
    const size_t n = m->rows;
    const double eps = singular_threshold(m);

    for (size_t i = 0; i < n; ++i)
        if (fabs(matrix_get(m, i, i)) <= eps)
            return NULL;

    Matrix *inv = matrix_create(n, n);
    if (!inv)
        return NULL;

    // A inversa também é triangular inferior. Coluna j: resolve L·x = eⱼ só nas
    // linhas j..n−1, por substituição progressiva.
    for (size_t j = 0; j < n; ++j)
        for (size_t i = j; i < n; ++i)
        {
            double s = (i == j) ? 1.0 : 0.0;
            for (size_t k = j; k < i; ++k)
                s -= matrix_get(m, i, k) * matrix_get(inv, k, j);
            matrix_set(inv, i, j, s / matrix_get(m, i, i));
        }
    return inv;
}

Matrix *matrix_inverse_spd(const Matrix *m)
{
    const size_t n = m->rows;
    const double eps = singular_threshold(m);

    // Cholesky: m = L·Lᵀ. Um pivô ≤ 0 indica que m não é definida positiva.
    Matrix *l = matrix_create(n, n);
    if (!l)
        return NULL;

    for (size_t j = 0; j < n; ++j)
    {
        double d = matrix_get(m, j, j);
        for (size_t k = 0; k < j; ++k)
            d -= matrix_get(l, j, k) * matrix_get(l, j, k);

        if (d <= eps)
        {
            matrix_destroy(l);
            return NULL;
        }

        const double ljj = sqrt(d);
        matrix_set(l, j, j, ljj);

        for (size_t i = j + 1; i < n; ++i)
        {
            double s = matrix_get(m, i, j);
            for (size_t k = 0; k < j; ++k)
                s -= matrix_get(l, i, k) * matrix_get(l, j, k);
            matrix_set(l, i, j, s / ljj);
        }
    }

    // m⁻¹ = (L·Lᵀ)⁻¹ = L⁻ᵀ·L⁻¹, reaproveitando a inversa triangular.
    Matrix *linv = matrix_inverse_lower_triangular(l);
    Matrix *linv_t = linv ? matrix_transpose(linv) : NULL;
    Matrix *inv = linv_t ? matrix_multiply(linv_t, linv) : NULL;

    matrix_destroy(l);
    matrix_destroy(linv);
    matrix_destroy(linv_t);
    if (!inv)
        return NULL;

    // Simetria exata (o produto acumula arredondamentos diferentes em (i, j) e (j, i)).
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < i; ++j)
        {
            const double avg = 0.5 * (matrix_get(inv, i, j) + matrix_get(inv, j, i));
            matrix_set(inv, i, j, avg);
            matrix_set(inv, j, i, avg);
        }
    return inv;
}

Matrix *matrix_inverse_general(const Matrix *m)
{
    const size_t n = m->rows;
    LU f;
    if (!lu_factor(m, &f))
        return NULL;

    Matrix *inv = matrix_create(n, n);
    double *e = calloc(n, sizeof *e);
    double *x = malloc(n * sizeof *x);

    if (inv && e && x)
    {
        for (size_t j = 0; j < n; ++j)
        {
            e[j] = 1.0;
            lu_solve(&f, e, x);
            e[j] = 0.0;
            for (size_t i = 0; i < n; ++i)
                matrix_set(inv, i, j, x[i]);
        }
    }
    else
    {
        matrix_destroy(inv);
        inv = NULL;
    }

    free(e);
    free(x);
    lu_destroy(&f);
    return inv;
}

Matrix *matrix_inverse(const Matrix *m, double tol)
{
    if (!matrix_is_square(m))
        return NULL;

    // Da estrutura mais barata para a mais cara. A diagonal vem antes das triangulares
    // porque toda diagonal também é triangular.
    if (matrix_is_diagonal(m, tol))
        return matrix_inverse_diagonal(m);
    if (matrix_is_upper_triangular(m, tol))
        return matrix_inverse_upper_triangular(m);
    if (matrix_is_lower_triangular(m, tol))
        return matrix_inverse_lower_triangular(m);
    if (matrix_is_orthogonal(m, tol))
        return matrix_inverse_orthogonal(m);

    if (matrix_is_symmetric(m, tol))
    {
        // Cholesky só funciona se for definida positiva; se não for, cai no caso geral.
        Matrix *inv = matrix_inverse_spd(m);
        if (inv)
            return inv;
    }

    return matrix_inverse_general(m);
}

// ---------------------------------------------------------------------------
// Autovalores e autovetores
// ---------------------------------------------------------------------------

void eigen_pair_destroy(EigenPair *p)
{
    free(p->vector);
    p->vector = NULL;
}

void eigen_decomposition_destroy(EigenDecomposition *d)
{
    free(d->values);
    matrix_destroy(d->vectors);
    d->values = NULL;
    d->vectors = NULL;
}

// ‖A·v − λ·v‖₂, usando work (tamanho n) como espaço auxiliar.
static double eigen_residual(const Matrix *a, double lambda, const double *v, double *work)
{
    const size_t n = a->rows;
    matrix_mul_vec(a, v, work);
    double s = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        const double r = work[i] - lambda * v[i];
        s += r * r;
    }
    return sqrt(s);
}

double eigen_decomposition_residual(const Matrix *a, const EigenDecomposition *d)
{
    const size_t n = d->n;
    double *v = malloc(n * sizeof *v);
    double *work = malloc(n * sizeof *work);
    double worst = INFINITY;

    if (v && work)
    {
        worst = 0.0;
        for (size_t j = 0; j < n; ++j)
        {
            for (size_t i = 0; i < n; ++i)
                v[i] = matrix_get(d->vectors, i, j);
            worst = fmax(worst, eigen_residual(a, d->values[j], v, work));
        }
    }

    free(v);
    free(work);
    return worst;
}

// --- potência e potência inversa ---

// Um passo do método: out = (operador)·in.
typedef void (*EigenStep)(const void *ctx, const double *in, double *out);

static void step_power(const void *ctx, const double *in, double *out)
{
    matrix_mul_vec((const Matrix *)ctx, in, out);
}

static void step_inverse_power(const void *ctx, const double *in, double *out)
{
    lu_solve((const LU *)ctx, in, out);
}

// Núcleo comum: v ← step(v)/‖step(v)‖ e λ = vᵀ·A·v (quociente de Rayleigh). O critério
// usa o resíduo, e não a variação de v, porque com λ < 0 o vetor troca de sinal a cada
// iteração e a variação nunca iria a zero.
static EigenPair eigen_iterate(const Matrix *a, const double *v0, double tol, int max_iter,
                               EigenStep step, const void *ctx)
{
    const size_t n = a->rows;
    EigenPair r = {.n = n};

    double *v = malloc(n * sizeof *v);
    double *w = malloc(n * sizeof *w);
    double *av = malloc(n * sizeof *av);
    if (!v || !w || !av)
        goto fail;

    for (size_t i = 0; i < n; ++i)
        v[i] = v0 ? v0[i] : 1.0;
    if (!vec_normalize(v, n))
        goto fail;

    for (r.iterations = 1; r.iterations <= max_iter; ++r.iterations)
    {
        step(ctx, v, w);

        // A·v = 0 só acontece na potência: v já é autovetor de λ = 0.
        if (vec_normalize(w, n))
        {
            double *tmp = v;
            v = w;
            w = tmp;
        }

        matrix_mul_vec(a, v, av);
        r.value = vec_dot(v, av, n);
        r.residual = eigen_residual(a, r.value, v, w);

        if (r.residual <= tol * fmax(1.0, fabs(r.value)))
        {
            r.converged = true;
            break;
        }
    }
    if (!r.converged)
        r.iterations = max_iter;

    vec_fix_sign(v, n);
    r.vector = v;
    free(w);
    free(av);
    return r;

fail:
    free(v);
    free(w);
    free(av);
    r.vector = NULL;
    return r;
}

EigenPair eigen_power(const Matrix *a, const double *v0, double tol, int max_iter)
{
    if (!matrix_is_square(a))
        return (EigenPair){0};
    return eigen_iterate(a, v0, tol, max_iter, step_power, a);
}

EigenPair eigen_inverse_power(const Matrix *a, double shift, const double *v0, double tol,
                              int max_iter)
{
    if (!matrix_is_square(a))
        return (EigenPair){0};

    // Os autovalores de (A − σI)⁻¹ são 1/(λᵢ − σ): o dominante é o λ mais próximo de σ.
    Matrix *s = matrix_copy(a);
    if (!s)
        return (EigenPair){0};
    for (size_t i = 0; i < s->rows; ++i)
        *matrix_at(s, i, i) -= shift;

    LU f;
    const bool ok = lu_factor(s, &f);
    matrix_destroy(s);
    if (!ok)
        return (EigenPair){0};

    EigenPair r = eigen_iterate(a, v0, tol, max_iter, step_inverse_power, &f);
    lu_destroy(&f);
    return r;
}

// --- métodos para todos os autopares (simétricas) ---

// Raiz da soma dos quadrados fora da diagonal.
static double off_diagonal_norm(const Matrix *m)
{
    double s = 0.0;
    for (size_t i = 0; i < m->rows; ++i)
        for (size_t j = 0; j < m->cols; ++j)
            if (i != j)
                s += matrix_get(m, i, j) * matrix_get(m, i, j);
    return sqrt(s);
}

// Monta o resultado a partir da matriz diagonalizada d e dos vetores v (assume a posse
// de v): autovalores = diagonal de d, ordenados de forma crescente junto com as colunas.
static EigenDecomposition decomposition_from(const Matrix *d, Matrix *v, int iterations,
                                             bool converged)
{
    const size_t n = d->rows;
    EigenDecomposition r = {.n = n, .iterations = iterations, .converged = converged};

    r.values = malloc(n * sizeof *r.values);
    if (!r.values)
    {
        matrix_destroy(v);
        return (EigenDecomposition){0};
    }
    for (size_t i = 0; i < n; ++i)
        r.values[i] = matrix_get(d, i, i);

    // Ordenação por seleção trocando também as colunas de v.
    for (size_t i = 0; i < n; ++i)
    {
        size_t k = i;
        for (size_t j = i + 1; j < n; ++j)
            if (r.values[j] < r.values[k])
                k = j;
        if (k == i)
            continue;

        const double tv = r.values[i];
        r.values[i] = r.values[k];
        r.values[k] = tv;
        for (size_t row = 0; row < n; ++row)
        {
            const double t = matrix_get(v, row, i);
            matrix_set(v, row, i, matrix_get(v, row, k));
            matrix_set(v, row, k, t);
        }
    }

    // Sinal fixo de cada autovetor (coluna).
    for (size_t j = 0; j < n; ++j)
    {
        size_t k = 0;
        for (size_t i = 1; i < n; ++i)
            if (fabs(matrix_get(v, i, j)) > fabs(matrix_get(v, k, j)))
                k = i;
        if (matrix_get(v, k, j) < 0.0)
            for (size_t i = 0; i < n; ++i)
                matrix_set(v, i, j, -matrix_get(v, i, j));
    }

    r.vectors = v;
    return r;
}

// Tolerância de simetria proporcional à escala da matriz.
static bool is_symmetric_scaled(const Matrix *a)
{
    return matrix_is_symmetric(a, 1e-12 * fmax(1.0, matrix_frobenius_norm(a)));
}

EigenDecomposition eigen_qr_iteration(const Matrix *a, double tol, int max_iter)
{
    if (!is_symmetric_scaled(a))
        return (EigenDecomposition){0};

    const double target = tol * fmax(1.0, matrix_frobenius_norm(a));
    Matrix *ak = matrix_copy(a);
    Matrix *v = matrix_identity(a->rows);
    if (!ak || !v)
        goto fail;

    // Aₖ₊₁ = Rₖ·Qₖ = Qₖᵀ·Aₖ·Qₖ é semelhante a Aₖ e tende a ficar diagonal; o produto
    // dos Qₖ acumula os autovetores.
    int it = 0;
    bool converged = off_diagonal_norm(ak) <= target;
    while (!converged && it < max_iter)
    {
        Matrix *q, *r;
        if (!matrix_qr(ak, &q, &r))
            goto fail;

        Matrix *next = matrix_multiply(r, q);
        Matrix *vq = matrix_multiply(v, q);
        matrix_destroy(q);
        matrix_destroy(r);
        if (!next || !vq)
        {
            matrix_destroy(next);
            matrix_destroy(vq);
            goto fail;
        }

        matrix_destroy(ak);
        matrix_destroy(v);
        ak = next;
        v = vq;

        ++it;
        converged = off_diagonal_norm(ak) <= target;
    }

    EigenDecomposition res = decomposition_from(ak, v, it, converged);
    matrix_destroy(ak);
    return res;

fail:
    matrix_destroy(ak);
    matrix_destroy(v);
    return (EigenDecomposition){0};
}

EigenDecomposition eigen_jacobi(const Matrix *a, double tol, int max_sweeps)
{
    if (!is_symmetric_scaled(a))
        return (EigenDecomposition){0};

    const size_t n = a->rows;
    const double target = tol * fmax(1.0, matrix_frobenius_norm(a));
    Matrix *m = matrix_copy(a);
    Matrix *v = matrix_identity(n);
    if (!m || !v)
    {
        matrix_destroy(m);
        matrix_destroy(v);
        return (EigenDecomposition){0};
    }

    int sweep = 0;
    bool converged = off_diagonal_norm(m) <= target;
    while (!converged && sweep < max_sweeps)
    {
        for (size_t p = 0; p < n; ++p)
            for (size_t q = p + 1; q < n; ++q)
            {
                const double apq = matrix_get(m, p, q);
                if (apq == 0.0)
                    continue;

                // Rotação J(p, q, θ) escolhida para zerar a(p, q) em Jᵀ·A·J. t = tg θ é a
                // menor raiz de t² + 2·θ·t − 1 = 0 (rotação de no máximo 45°, mais estável).
                const double theta = (matrix_get(m, q, q) - matrix_get(m, p, p)) / (2.0 * apq);
                const double t = (theta >= 0.0 ? 1.0 : -1.0) / (fabs(theta) + sqrt(theta * theta + 1.0));
                const double c = 1.0 / sqrt(t * t + 1.0), s = t * c;

                for (size_t k = 0; k < n; ++k) // A ← A·J
                {
                    const double akp = matrix_get(m, k, p), akq = matrix_get(m, k, q);
                    matrix_set(m, k, p, c * akp - s * akq);
                    matrix_set(m, k, q, s * akp + c * akq);
                }
                for (size_t k = 0; k < n; ++k) // A ← Jᵀ·A
                {
                    const double apk = matrix_get(m, p, k), aqk = matrix_get(m, q, k);
                    matrix_set(m, p, k, c * apk - s * aqk);
                    matrix_set(m, q, k, s * apk + c * aqk);
                }
                for (size_t k = 0; k < n; ++k) // V ← V·J
                {
                    const double vkp = matrix_get(v, k, p), vkq = matrix_get(v, k, q);
                    matrix_set(v, k, p, c * vkp - s * vkq);
                    matrix_set(v, k, q, s * vkp + c * vkq);
                }

                // Os elementos zerados ficam exatamente zero.
                matrix_set(m, p, q, 0.0);
                matrix_set(m, q, p, 0.0);
            }

        ++sweep;
        converged = off_diagonal_norm(m) <= target;
    }

    EigenDecomposition res = decomposition_from(m, v, sweep, converged);
    matrix_destroy(m);
    return res;
}

// Rotação de Givens P = [c s; −s c] nas linhas/colunas k e k+1: t ← P·t·Pᵀ, v ← v·Pᵀ.
static void givens_similarity(Matrix *t, Matrix *v, size_t k, double c, double s)
{
    const size_t n = t->rows;
    for (size_t j = 0; j < n; ++j)
    {
        const double a = matrix_get(t, k, j), b = matrix_get(t, k + 1, j);
        matrix_set(t, k, j, c * a + s * b);
        matrix_set(t, k + 1, j, -s * a + c * b);
    }
    for (size_t i = 0; i < n; ++i)
    {
        const double a = matrix_get(t, i, k), b = matrix_get(t, i, k + 1);
        matrix_set(t, i, k, c * a + s * b);
        matrix_set(t, i, k + 1, -s * a + c * b);
    }
    for (size_t i = 0; i < n; ++i)
    {
        const double a = matrix_get(v, i, k), b = matrix_get(v, i, k + 1);
        matrix_set(v, i, k, c * a + s * b);
        matrix_set(v, i, k + 1, -s * a + c * b);
    }
}

// Um passo QR implícito com deslocamento de Wilkinson no bloco tridiagonal [lo, hi].
// Equivale a T − μI = QR, T ← RQ + μI, mas sem formar Q: a primeira rotação vem da
// coluna deslocada e as seguintes "perseguem" o elemento fora da faixa até sair do bloco.
static void tridiagonal_qr_step(Matrix *t, Matrix *v, size_t lo, size_t hi)
{
    // Wilkinson: autovalor do bloco 2×2 do canto mais próximo de t(hi, hi).
    const double a = matrix_get(t, hi - 1, hi - 1);
    const double b = matrix_get(t, hi, hi - 1);
    const double d = matrix_get(t, hi, hi);
    const double delta = 0.5 * (a - d);
    const double sign = (delta >= 0.0) ? 1.0 : -1.0;
    const double mu = d - b * b / (delta + sign * hypot(delta, b));

    double x = matrix_get(t, lo, lo) - mu;
    double z = matrix_get(t, lo + 1, lo);

    for (size_t k = lo; k < hi; ++k)
    {
        const double r = hypot(x, z);
        const double c = (r == 0.0) ? 1.0 : x / r;
        const double s = (r == 0.0) ? 0.0 : z / r;

        givens_similarity(t, v, k, c, s);

        // A rotação k zerou o bulge t(k+1, k−1); deixa exatamente zero.
        if (k > lo)
        {
            matrix_set(t, k + 1, k - 1, 0.0);
            matrix_set(t, k - 1, k + 1, 0.0);
        }

        // A rotação gera um novo bulge em t(k+2, k), eliminado pela rotação seguinte.
        if (k + 1 < hi)
        {
            x = matrix_get(t, k + 1, k);
            z = matrix_get(t, k + 2, k);
        }
    }
}

EigenDecomposition eigen_householder_qr(const Matrix *a, double tol, int max_iter)
{
    if (!is_symmetric_scaled(a))
        return (EigenDecomposition){0};

    // 1) A = Q·T·Qᵀ com T tridiagonal: cada passo QR passa a custar O(n) na faixa, e não
    //    uma fatoração QR completa O(n³).
    Matrix *v, *t;
    if (!matrix_tridiagonalize(a, &v, &t))
        return (EigenDecomposition){0};

    // 2) QR implícito com deflação: quando t(i, i−1) fica desprezível em relação aos
    //    vizinhos da diagonal, o problema se separa em dois blocos independentes.
    const size_t n = a->rows;
    const double floor_tol = tol * fmax(1.0, matrix_frobenius_norm(a));
    int it = 0;
    size_t hi = n - 1;

    while (hi > 0 && it < max_iter)
    {
        const double sub = fabs(matrix_get(t, hi, hi - 1));
        const double diag = fabs(matrix_get(t, hi - 1, hi - 1)) + fabs(matrix_get(t, hi, hi));
        if (sub <= tol * diag || sub <= floor_tol)
        {
            matrix_set(t, hi, hi - 1, 0.0);
            matrix_set(t, hi - 1, hi, 0.0);
            --hi;
            continue;
        }

        // Início do bloco não reduzido que termina em hi.
        size_t lo = hi - 1;
        while (lo > 0)
        {
            const double e = fabs(matrix_get(t, lo, lo - 1));
            const double dd = fabs(matrix_get(t, lo - 1, lo - 1)) + fabs(matrix_get(t, lo, lo));
            if (e <= tol * dd || e <= floor_tol)
            {
                matrix_set(t, lo, lo - 1, 0.0);
                matrix_set(t, lo - 1, lo, 0.0);
                break;
            }
            --lo;
        }

        tridiagonal_qr_step(t, v, lo, hi);
        ++it;
    }

    EigenDecomposition res = decomposition_from(t, v, it, hi == 0);
    matrix_destroy(t);
    return res;
}
