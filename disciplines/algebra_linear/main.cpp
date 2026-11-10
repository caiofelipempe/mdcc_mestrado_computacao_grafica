// main.cpp — testes do Método da Potência e da Potência Inversa
// Compilar: g++ -std=c++17 -O2 -Wall -Wextra main.cpp -o eigen && ./eigen
#include "linear_algebra.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

using la::Matrix;
using la::Vector;

// ---------------------------------------------------------------------------
// Gera matriz simétrica aleatória: sorteia o triângulo superior em [lo, hi]
// e espelha. A semente fixa torna o relatório reproduzível.
// ---------------------------------------------------------------------------
Matrix random_symmetric(std::size_t n, unsigned seed, double lo = -10.0, double hi = 10.0) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dist(lo, hi);
    Matrix A(n);
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = i; j < n; ++j) A(i, j) = A(j, i) = dist(gen);
    return A;
}

// ---------------------------------------------------------------------------
// Referência independente: Jacobi cíclico (só para VALIDAR o relatório —
// calcula todos os autovalores e confere se acertamos o maior/menor |λ|).
// ---------------------------------------------------------------------------
Vector jacobi_eigenvalues(Matrix A) {
    const std::size_t n = A.size();
    for (int sweep = 0; sweep < 100; ++sweep) {
        double off = 0.0;
        for (std::size_t i = 0; i < n; ++i)
            for (std::size_t j = i + 1; j < n; ++j) off += A(i, j) * A(i, j);
        if (off < 1e-30) break;

        for (std::size_t p = 0; p < n; ++p)
            for (std::size_t q = p + 1; q < n; ++q) {
                if (std::abs(A(p, q)) < 1e-300) continue;
                const double theta = (A(q, q) - A(p, p)) / (2.0 * A(p, q));
                const double t = (theta >= 0 ? 1.0 : -1.0) / (std::abs(theta) + std::sqrt(theta * theta + 1.0));
                const double c = 1.0 / std::sqrt(t * t + 1.0), s = t * c;
                for (std::size_t k = 0; k < n; ++k) { // A ← A·J
                    const double akp = A(k, p), akq = A(k, q);
                    A(k, p) = c * akp - s * akq;
                    A(k, q) = s * akp + c * akq;
                }
                for (std::size_t k = 0; k < n; ++k) { // A ← Jᵀ·A
                    const double apk = A(p, k), aqk = A(q, k);
                    A(p, k) = c * apk - s * aqk;
                    A(q, k) = s * apk + c * aqk;
                }
            }
    }
    Vector ev(n);
    for (std::size_t i = 0; i < n; ++i) ev[i] = A(i, i);
    std::sort(ev.begin(), ev.end(), [](double a, double b) { return std::abs(a) > std::abs(b); });
    return ev; // ordenados por |λ| decrescente
}

// ---------------------------------------------------------------------------
// Impressão
// ---------------------------------------------------------------------------
void rule(char c = '=', int w = 96) { std::cout << std::string(w, c) << '\n'; }

void print_matrix(const std::string& name, const Matrix& A) {
    std::cout << name << " (" << A.size() << "x" << A.size() << "):\n";
    for (std::size_t i = 0; i < A.size(); ++i) {
        std::cout << "  [";
        for (std::size_t j = 0; j < A.size(); ++j) std::cout << std::setw(9) << std::fixed << std::setprecision(4) << A(i, j);
        std::cout << " ]\n";
    }
}

void print_vector(const std::string& name, const Vector& v) {
    std::cout << "  " << name << " = [";
    for (std::size_t i = 0; i < v.size(); ++i)
        std::cout << (i ? ", " : "") << std::fixed << std::setprecision(6) << std::setw(9) << v[i];
    std::cout << " ]\n";
}

void print_result(const std::string& method, const la::EigenResult& r, double reference) {
    std::cout << "\n  >> " << method << "\n";
    std::cout << "  convergiu       : " << (r.converged ? "sim" : "NAO") << "\n";
    std::cout << "  iteracoes       : " << r.iterations << "\n";
    std::cout << "  lambda          : " << std::setprecision(12) << r.lambda << "\n";
    std::cout << "  lambda (Jacobi) : " << std::setprecision(12) << reference << "\n";
    std::cout << "  |erro lambda|   : " << std::scientific << std::setprecision(3) << std::abs(r.lambda - reference) << "\n";
    std::cout << "  ||Av - lambda v||: " << r.residual << std::fixed << "\n";
    print_vector("v", r.v);
}

// ---------------------------------------------------------------------------
struct Summary {
    std::size_t n;
    la::EigenResult pw, inv;
    double ref_max, ref_min;
};

Summary run_case(std::size_t n, unsigned seed) {
    rule();
    std::cout << "CASO " << n << "x" << n << "   (semente = " << seed << ")\n";
    rule();

    const Matrix A = random_symmetric(n, seed);
    print_matrix("A", A);
    std::cout << "  simetrica? " << (la::is_symmetric(A) ? "sim" : "nao") << "\n";

    const Vector ev = jacobi_eigenvalues(A);
    std::cout << "\n  Espectro de referencia (Jacobi, ordenado por |lambda|):\n  ";
    for (double x : ev) std::cout << std::setprecision(4) << std::setw(10) << x;
    std::cout << "\n  razao de convergencia potencia  |l2/l1|      = " << std::setprecision(4) << std::abs(ev[1] / ev[0])
              << "\n  razao de convergencia inversa   |l_n/l_(n-1)| = " << std::abs(ev[n - 1] / ev[n - 2]) << "\n";

    la::IterOptions opt;
    opt.tol = 1e-10;
    opt.max_iter = 20000;

    Summary s{n, la::power_method(A, opt), la::inverse_power_method(A, 0.0, opt), ev.front(), ev.back()};
    print_result("Metodo da Potencia (maior |lambda|)", s.pw, s.ref_max);
    print_result("Metodo da Potencia Inversa, sigma = 0 (menor |lambda|)", s.inv, s.ref_min);
    std::cout << '\n';
    return s;
}

int main() {
    std::cout << "RELATORIO — METODO DA POTENCIA E POTENCIA INVERSA (matrizes simetricas)\n"
              << "Criterio de parada: ||Av - lambda v|| <= 1e-10 * max(1, |lambda|)\n"
              << "Potencia inversa: (A - sigma I) fatorada uma vez por LDL^T\n\n";

    std::vector<Summary> all;
    unsigned seed = 42;
    for (std::size_t n : {3u, 5u, 10u}) all.push_back(run_case(n, seed++));

    rule();
    std::cout << "RESUMO\n";
    rule();
    std::cout << std::left << std::setw(7) << "n" << std::setw(15) << "lambda_max" << std::setw(8) << "iter"
              << std::setw(12) << "erro" << std::setw(15) << "lambda_min" << std::setw(8) << "iter"
              << std::setw(12) << "erro" << '\n';
    rule('-');
    for (const auto& s : all) {
        std::cout << std::left << std::setw(7) << (std::to_string(s.n) + "x" + std::to_string(s.n))
                  << std::fixed << std::setprecision(6) << std::setw(15) << s.pw.lambda << std::setw(8) << s.pw.iterations
                  << std::scientific << std::setprecision(2) << std::setw(12) << std::abs(s.pw.lambda - s.ref_max)
                  << std::fixed << std::setprecision(6) << std::setw(15) << s.inv.lambda << std::setw(8) << s.inv.iterations
                  << std::scientific << std::setprecision(2) << std::setw(12) << std::abs(s.inv.lambda - s.ref_min) << '\n';
    }
    return 0;
}