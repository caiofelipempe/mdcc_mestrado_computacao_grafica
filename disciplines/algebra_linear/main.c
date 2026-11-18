// main.c — relatório dos métodos de autovalores/autovetores em matrizes simétricas aleatórias.
//
// Uso:
//   algebra_linear                 → tamanhos padrão (3, 5 e 10)
//   algebra_linear 4 8 20          → tamanhos escolhidos na linha de comando
//   algebra_linear > relatorio.txt → salva o relatório
//
// Os demais parâmetros (semente, tolerância, limites de iteração, intervalo dos valores
// sorteados) ficam nas constantes abaixo.
#include "linear_algebra.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ---------------------------------------------------------------------------
// Configuração
// ---------------------------------------------------------------------------

static const size_t DEFAULT_SIZES[] = {3, 5, 10};

#define SEED 42u          // semente do rand(): o relatório é reproduzível
#define VALUE_MIN (-10.0) // elementos sorteados uniformemente em [VALUE_MIN, VALUE_MAX]
#define VALUE_MAX 10.0
#define TOL 1e-10         // tolerância dos critérios de parada
#define MAX_ITER 20000    // limite de iterações (potência, potência inversa, QR, Householder+QR)
#define MAX_SWEEPS 100    // limite de varreduras do Jacobi

#define MAX_SIZES 32

// ---------------------------------------------------------------------------
// Métodos que calculam todos os autopares
// ---------------------------------------------------------------------------

typedef EigenDecomposition (*FullMethod)(const Matrix *a, double tol, int max_iter);

typedef struct
{
    const char *name;
    FullMethod run;
    int max_iter;
} FullMethodInfo;

static const FullMethodInfo FULL_METHODS[] = {
    {"Iteracao QR", eigen_qr_iteration, MAX_ITER},
    {"Jacobi", eigen_jacobi, MAX_SWEEPS},
    {"Householder", eigen_householder_qr, MAX_ITER},
};

#define N_FULL (sizeof FULL_METHODS / sizeof FULL_METHODS[0])
#define REFERENCE 2 // índice em FULL_METHODS usado como referência dos autovalores
#define N_METHODS (N_FULL + 2) // + potência e potência inversa

// Uma linha do resumo final.
typedef struct
{
    const char *name;
    bool ok; // o método rodou (false: erro de alocação ou matriz inválida)
    bool converged;
    int iterations;
    double ms;
    double error;    // diferença máxima para os autovalores de referência
    double residual; // maior ‖A·v − λ·v‖
} Row;

typedef struct
{
    size_t n;
    Row rows[N_METHODS];
} CaseSummary;

// ---------------------------------------------------------------------------
// Impressão
// ---------------------------------------------------------------------------

static void rule(char c)
{
    for (int i = 0; i < 100; ++i)
        putchar(c);
    putchar('\n');
}

static void print_values(const char *label, const double *v, size_t n)
{
    printf("  %-20s [", label);
    for (size_t i = 0; i < n; ++i)
        printf(" %12.6f", v[i]);
    printf(" ]\n");
}

static void print_matrix_indented(const Matrix *m)
{
    for (size_t i = 0; i < m->rows; ++i)
    {
        printf("    [");
        for (size_t j = 0; j < m->cols; ++j)
        {
            // Arredondamentos como -1e-17 apareceriam como "-0.00000".
            const double x = matrix_get(m, i, j);
            printf(" %9.5f", fabs(x) < 5e-6 ? 0.0 : x);
        }
        printf(" ]\n");
    }
}

static double elapsed_ms(clock_t start)
{
    return 1000.0 * (double)(clock() - start) / CLOCKS_PER_SEC;
}

// Lista cada autopar: λᵢ e a coluna i de V.
static void print_pairs(const double *values, const Matrix *v)
{
    const size_t n = v->rows;
    printf("  autopares (lambda_i, v_i):\n");
    for (size_t j = 0; j < n; ++j)
    {
        printf("    lambda_%-3zu = %14.8f   v_%-3zu = [", j + 1, values[j], j + 1);
        for (size_t i = 0; i < n; ++i)
            printf(" %9.5f", matrix_get(v, i, j));
        printf(" ]\n");
    }
}

// Vᵀ·A·V: deve ser diagonal com os autovalores na diagonal.
static void print_diagonalized(const Matrix *a, const Matrix *v)
{
    Matrix *vt = matrix_transpose(v);
    Matrix *vta = vt ? matrix_multiply(vt, a) : NULL;
    Matrix *d = vta ? matrix_multiply(vta, v) : NULL;

    if (d)
    {
        printf("  V^T A V (diagonalizada):\n");
        print_matrix_indented(d);
    }

    matrix_destroy(vt);
    matrix_destroy(vta);
    matrix_destroy(d);
}

// max |(VᵀV − I)(i, j)|: quanto os autovetores se afastam de uma base ortonormal.
static double orthogonality_error(const Matrix *v)
{
    Matrix *vt = matrix_transpose(v);
    Matrix *p = vt ? matrix_multiply(vt, v) : NULL;
    double e = INFINITY;

    if (p)
    {
        e = 0.0;
        for (size_t i = 0; i < p->rows; ++i)
            for (size_t j = 0; j < p->cols; ++j)
                e = fmax(e, fabs(matrix_get(p, i, j) - (i == j ? 1.0 : 0.0)));
    }

    matrix_destroy(vt);
    matrix_destroy(p);
    return e;
}

// ---------------------------------------------------------------------------
// Um caso n×n
// ---------------------------------------------------------------------------

static void report_full(const Matrix *a, const FullMethodInfo *m, const EigenDecomposition *d,
                        const double *ref, double ms, Row *row)
{
    const size_t n = a->rows;
    *row = (Row){.name = m->name, .ms = ms};

    printf("\n  >> %s\n", m->name);
    if (!d->values)
    {
        printf("  ERRO: o metodo nao pode ser aplicado\n");
        return;
    }

    row->ok = true;
    row->converged = d->converged;
    row->iterations = d->iterations;
    row->residual = eigen_decomposition_residual(a, d);
    row->error = 0.0;
    for (size_t i = 0; i < n; ++i)
        row->error = fmax(row->error, fabs(d->values[i] - ref[i]));

    printf("  convergiu            : %s\n", d->converged ? "sim" : "NAO");
    printf("  %-21s: %d\n", m->run == eigen_jacobi ? "varreduras" : "iteracoes", d->iterations);
    printf("  tempo                : %.3f ms\n", ms);
    printf("  max ||Av - lambda v||: %.3e\n", row->residual);
    printf("  max |V^T V - I|      : %.3e\n", orthogonality_error(d->vectors));
    if (m != &FULL_METHODS[REFERENCE])
        printf("  max |lambda - ref|   : %.3e\n", row->error);
    print_values("autovalores", d->values, n);
    printf("  V (autovetores nas colunas):\n");
    print_matrix_indented(d->vectors);
    print_diagonalized(a, d->vectors);
    print_pairs(d->values, d->vectors);
}

static void report_pair(const Matrix *a, const char *name, const EigenPair *p, double expected,
                        double ms, Row *row)
{
    *row = (Row){.name = name, .ms = ms};

    printf("\n  >> %s\n", name);
    if (!p->vector)
    {
        printf("  ERRO: o metodo nao pode ser aplicado (matriz singular para o deslocamento?)\n");
        return;
    }

    row->ok = true;
    row->converged = p->converged;
    row->iterations = p->iterations;
    row->residual = p->residual;
    row->error = fabs(p->value - expected);

    printf("  convergiu            : %s\n", p->converged ? "sim" : "NAO");
    printf("  iteracoes            : %d\n", p->iterations);
    printf("  tempo                : %.3f ms\n", ms);
    printf("  lambda               : %.12f\n", p->value);
    printf("  lambda (referencia)  : %.12f\n", expected);
    printf("  |lambda - ref|       : %.3e\n", row->error);
    printf("  ||Av - lambda v||    : %.3e\n", p->residual);
    print_values("autovetor v", p->vector, p->n);

    // A·v e λ·v lado a lado: devem coincidir.
    double *av = malloc(p->n * sizeof *av);
    double *lv = malloc(p->n * sizeof *lv);
    if (av && lv)
    {
        matrix_mul_vec(a, p->vector, av);
        for (size_t i = 0; i < p->n; ++i)
            lv[i] = p->value * p->vector[i];
        print_values("A v", av, p->n);
        print_values("lambda v", lv, p->n);
    }
    free(av);
    free(lv);
}

static bool run_case(size_t n, CaseSummary *summary)
{
    summary->n = n;

    rule('=');
    printf("CASO %zux%zu\n", n, n);
    rule('=');

    Matrix *a = matrix_random_symmetric(n, VALUE_MIN, VALUE_MAX);
    if (!a)
        return false;

    printf("  A:\n");
    print_matrix_indented(a);
    printf("  simetrica? %s\n", matrix_is_symmetric(a, 0.0) ? "sim" : "nao");

    // Métodos completos. A referência roda primeiro para servir de comparação aos demais.
    EigenDecomposition decomp[N_FULL] = {0};
    double ms[N_FULL] = {0};
    for (size_t k = 0; k < N_FULL; ++k)
    {
        const clock_t start = clock();
        decomp[k] = FULL_METHODS[k].run(a, TOL, FULL_METHODS[k].max_iter);
        ms[k] = elapsed_ms(start);
    }

    const EigenDecomposition *ref = &decomp[REFERENCE];
    if (!ref->values)
    {
        printf("  ERRO: o metodo de referencia (%s) falhou\n", FULL_METHODS[REFERENCE].name);
        for (size_t k = 0; k < N_FULL; ++k)
            eigen_decomposition_destroy(&decomp[k]);
        matrix_destroy(a);
        return false;
    }

    // Maior e menor |λ| da referência, esperados na potência e na potência inversa.
    size_t imax = 0, imin = 0;
    for (size_t i = 1; i < n; ++i)
    {
        if (fabs(ref->values[i]) > fabs(ref->values[imax]))
            imax = i;
        if (fabs(ref->values[i]) < fabs(ref->values[imin]))
            imin = i;
    }

    printf("\n  Espectro de referencia (%s, crescente):\n", FULL_METHODS[REFERENCE].name);
    print_values("lambda", ref->values, n);

    // Razões que governam a convergência da potência (|λ₂/λ₁|) e da inversa (|λₙ/λₙ₋₁|).
    if (n > 1)
    {
        double second_max = 0.0, second_min = INFINITY;
        for (size_t i = 0; i < n; ++i)
        {
            if (i != imax)
                second_max = fmax(second_max, fabs(ref->values[i]));
            if (i != imin)
                second_min = fmin(second_min, fabs(ref->values[i]));
        }
        printf("  razao de convergencia potencia |l2/l1|          = %.4f\n",
               second_max / fabs(ref->values[imax]));
        printf("  razao de convergencia inversa  |l_n/l_(n-1)|    = %.4f\n",
               fabs(ref->values[imin]) / second_min);
    }

    for (size_t k = 0; k < N_FULL; ++k)
        report_full(a, &FULL_METHODS[k], &decomp[k], ref->values, ms[k], &summary->rows[k]);

    clock_t start = clock();
    EigenPair pw = eigen_power(a, NULL, TOL, MAX_ITER);
    double pw_ms = elapsed_ms(start);

    start = clock();
    EigenPair inv = eigen_inverse_power(a, 0.0, NULL, TOL, MAX_ITER);
    double inv_ms = elapsed_ms(start);

    report_pair(a, "Potencia (maior |lambda|)", &pw, ref->values[imax], pw_ms,
                &summary->rows[N_FULL]);
    report_pair(a, "Potencia inversa, sigma = 0 (menor |lambda|)", &inv, ref->values[imin], inv_ms,
                &summary->rows[N_FULL + 1]);
    printf("\n");

    eigen_pair_destroy(&pw);
    eigen_pair_destroy(&inv);
    for (size_t k = 0; k < N_FULL; ++k)
        eigen_decomposition_destroy(&decomp[k]);
    matrix_destroy(a);
    return true;
}

static void print_summary(const CaseSummary *cases, size_t count)
{
    rule('=');
    printf("RESUMO\n");
    rule('=');
    printf("%-7s %-46s %-5s %8s %10s %12s %12s\n", "n", "metodo", "conv", "iter", "tempo(ms)",
           "erro lambda", "residuo");
    rule('-');

    for (size_t c = 0; c < count; ++c)
    {
        char size[32];
        snprintf(size, sizeof size, "%zux%zu", cases[c].n, cases[c].n);

        for (size_t k = 0; k < N_METHODS; ++k)
        {
            const Row *r = &cases[c].rows[k];
            if (!r->ok)
            {
                printf("%-7s %-46s %s\n", k == 0 ? size : "", r->name, "ERRO");
                continue;
            }

            char error[16];
            if (k == REFERENCE)
                snprintf(error, sizeof error, "(ref)");
            else
                snprintf(error, sizeof error, "%.2e", r->error);

            printf("%-7s %-46s %-5s %8d %10.3f %12s %12.2e\n", k == 0 ? size : "", r->name,
                   r->converged ? "sim" : "NAO", r->iterations, r->ms, error, r->residual);
        }
        if (c + 1 < count)
            rule('-');
    }
}

// ---------------------------------------------------------------------------

int main(int argc, char **argv)
{
    size_t sizes[MAX_SIZES];
    size_t count = 0;

    if (argc > 1)
    {
        for (int i = 1; i < argc && count < MAX_SIZES; ++i)
        {
            char *end;
            const long n = strtol(argv[i], &end, 10);
            if (*end != '\0' || n < 1)
            {
                fprintf(stderr, "tamanho invalido: %s\n", argv[i]);
                return 1;
            }
            sizes[count++] = (size_t)n;
        }
    }
    else
    {
        for (size_t i = 0; i < sizeof DEFAULT_SIZES / sizeof DEFAULT_SIZES[0]; ++i)
            sizes[count++] = DEFAULT_SIZES[i];
    }

    srand(SEED);

    printf("RELATORIO — AUTOVALORES E AUTOVETORES DE MATRIZES SIMETRICAS ALEATORIAS\n");
    printf("Elementos uniformes em [%.1f, %.1f], semente %u\n", VALUE_MIN, VALUE_MAX, SEED);
    printf("Tolerancia %.0e; ate %d iteracoes (%d varreduras no Jacobi)\n", TOL, MAX_ITER,
           MAX_SWEEPS);
    printf("Criterios: potencia/inversa  ||Av - lambda v|| <= tol * max(1, |lambda|)\n");
    printf("           QR/Jacobi/Householder+QR  fora da diagonal <= tol * ||A||_F\n");
    printf("Referencia dos autovalores: %s\n\n", FULL_METHODS[REFERENCE].name);

    CaseSummary cases[MAX_SIZES];
    size_t done = 0;
    for (size_t i = 0; i < count; ++i)
        if (run_case(sizes[i], &cases[done]))
            ++done;

    print_summary(cases, done);
    return done == count ? 0 : 1;
}
