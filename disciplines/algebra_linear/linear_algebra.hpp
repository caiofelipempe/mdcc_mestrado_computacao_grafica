// linear_algebra.hpp
// ---------------------------------------------------------------------------
// Método da Potência e Método da Potência Inversa para matrizes simétricas n×n.
//
// Por que exigir simetria?
//   * O método da potência inversa precisa aplicar A⁻¹ a um vetor em cada
//     iteração. Em vez de calcular A⁻¹ explicitamente (caro e numericamente
//     pior), fatoramos A = L·D·Lᵀ UMA vez e, a cada iteração, resolvemos
//     A·w = v com duas substituições triangulares — custo O(n²) por iteração.
//   * A fatoração LDLᵀ só existe de forma tão simples porque A = Aᵀ: só lemos
//     o triângulo inferior (metade do trabalho de um LU) e, ao contrário de
//     Cholesky, ela funciona também para matrizes simétricas indefinidas
//     (autovalores negativos), que é o caso típico de uma matriz aleatória.
//   * Para A simétrica, o quociente de Rayleigh  λ ≈ vᵀAv / vᵀv  converge
//     quadraticamente mais rápido que a razão de componentes.
//
// Header-only, C++17, sem dependências.
// ---------------------------------------------------------------------------
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace la {

using Vector = std::vector<double>;

// ---------------------------------------------------------------------------
// Matriz quadrada densa, armazenamento row-major.
// ---------------------------------------------------------------------------
class Matrix {
public:
    explicit Matrix(std::size_t n = 0, double value = 0.0) : n_(n), data_(n * n, value) {}

    static Matrix identity(std::size_t n) {
        Matrix I(n);
        for (std::size_t i = 0; i < n; ++i) I(i, i) = 1.0;
        return I;
    }

    std::size_t size() const { return n_; }

    double&       operator()(std::size_t i, std::size_t j)       { return data_[i * n_ + j]; }
    const double& operator()(std::size_t i, std::size_t j) const { return data_[i * n_ + j]; }

private:
    std::size_t n_;
    std::vector<double> data_;
};

// ---------------------------------------------------------------------------
// Operações básicas
// ---------------------------------------------------------------------------
inline double dot(const Vector& a, const Vector& b) {
    double s = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) s += a[i] * b[i];
    return s;
}

inline double norm2(const Vector& v) { return std::sqrt(dot(v, v)); }

inline Vector operator*(const Matrix& A, const Vector& x) {
    const std::size_t n = A.size();
    Vector y(n, 0.0);
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j) y[i] += A(i, j) * x[j];
    return y;
}

inline void normalize(Vector& v) {
    const double nv = norm2(v);
    if (nv == 0.0) throw std::runtime_error("normalize: vetor nulo");
    for (double& x : v) x /= nv;
}

// Norma de Frobenius — usada como escala para tolerâncias relativas.
inline double frobenius(const Matrix& A) {
    double s = 0.0;
    for (std::size_t i = 0; i < A.size(); ++i)
        for (std::size_t j = 0; j < A.size(); ++j) s += A(i, j) * A(i, j);
    return std::sqrt(s);
}

inline bool is_symmetric(const Matrix& A, double tol = 1e-12) {
    const double scale = std::max(1.0, frobenius(A));
    for (std::size_t i = 0; i < A.size(); ++i)
        for (std::size_t j = i + 1; j < A.size(); ++j)
            if (std::abs(A(i, j) - A(j, i)) > tol * scale) return false;
    return true;
}

// Resíduo ‖A·v − λ·v‖₂ — mede quão bom é o par (λ, v).
inline double eigen_residual(const Matrix& A, double lambda, const Vector& v) {
    Vector r = A * v;
    for (std::size_t i = 0; i < r.size(); ++i) r[i] -= lambda * v[i];
    return norm2(r);
}

// Fixa o sinal do autovetor (componente de maior módulo positiva), para que
// resultados de métodos diferentes sejam comparáveis lado a lado.
inline void fix_sign(Vector& v) {
    std::size_t k = 0;
    for (std::size_t i = 1; i < v.size(); ++i)
        if (std::abs(v[i]) > std::abs(v[k])) k = i;
    if (v[k] < 0.0)
        for (double& x : v) x = -x;
}

// ---------------------------------------------------------------------------
// Fatoração LDLᵀ de matriz simétrica:  A = L·D·Lᵀ
//   L: triangular inferior com diagonal unitária
//   D: diagonal (guardada como vetor)
// Lê apenas o triângulo inferior de A. Sem pivoteamento: falha (lança) se
// aparecer um pivô ~0, o que indica A singular ou mal condicionada nessa
// ordem — com shift σ igual a um autovalor exato, por exemplo.
// ---------------------------------------------------------------------------
struct LDLT {
    Matrix L;
    Vector D;

    explicit LDLT(const Matrix& A) : L(Matrix::identity(A.size())), D(A.size(), 0.0) {
        const std::size_t n = A.size();
        const double eps = 1e-14 * std::max(1.0, frobenius(A));

        for (std::size_t j = 0; j < n; ++j) {
            double d = A(j, j);
            for (std::size_t k = 0; k < j; ++k) d -= L(j, k) * L(j, k) * D[k];
            if (std::abs(d) < eps)
                throw std::runtime_error("LDLT: pivo nulo (matriz singular ou shift sobre um autovalor)");
            D[j] = d;

            for (std::size_t i = j + 1; i < n; ++i) {
                double s = A(i, j);
                for (std::size_t k = 0; k < j; ++k) s -= L(i, k) * L(j, k) * D[k];
                L(i, j) = s / d;
            }
        }
    }

    // Resolve A·x = b:  L·y = b  →  z = D⁻¹·y  →  Lᵀ·x = z.   Custo O(n²).
    Vector solve(const Vector& b) const {
        const std::size_t n = D.size();
        Vector x(b);
        for (std::size_t i = 0; i < n; ++i)              // L·y = b
            for (std::size_t k = 0; k < i; ++k) x[i] -= L(i, k) * x[k];
        for (std::size_t i = 0; i < n; ++i) x[i] /= D[i]; // D·z = y
        for (std::size_t i = n; i-- > 0;)                 // Lᵀ·x = z
            for (std::size_t k = i + 1; k < n; ++k) x[i] -= L(k, i) * x[k];
        return x;
    }
};

// ---------------------------------------------------------------------------
// Resultado de um método iterativo de autovalor
// ---------------------------------------------------------------------------
struct EigenResult {
    double lambda = 0.0;   // autovalor estimado
    Vector v;              // autovetor unitário (sinal normalizado)
    int iterations = 0;
    bool converged = false;
    double residual = 0.0; // ‖A·v − λ·v‖₂
};

struct IterOptions {
    double tol = 1e-10;    // critério: ‖A·v − λ·v‖ ≤ tol·max(1, |λ|)
    int max_iter = 10000;
    Vector v0 = {};        // chute inicial; vazio → (1, 1, ..., 1)
};

namespace detail {

inline Vector initial_vector(const IterOptions& opt, std::size_t n) {
    Vector v = opt.v0.empty() ? Vector(n, 1.0) : opt.v0;
    if (v.size() != n) throw std::invalid_argument("v0 com dimensao errada");
    normalize(v);
    return v;
}

inline void require_symmetric(const Matrix& A, const char* who) {
    if (A.size() == 0) throw std::invalid_argument(std::string(who) + ": matriz vazia");
    if (!is_symmetric(A)) throw std::invalid_argument(std::string(who) + ": matriz deve ser simetrica");
}

// Núcleo comum aos dois métodos. `step` mapeia v ↦ (próximo vetor, não
// normalizado): A·v na potência, (A − σI)⁻¹·v na potência inversa.
// O critério de parada usa o resíduo do par de Rayleigh, e não a variação de
// v, porque se λ < 0 o vetor troca de sinal a cada iteração na potência
// direta e a "variação de v" nunca iria a zero.
template <class Step>
EigenResult iterate(const Matrix& A, const IterOptions& opt, Step step) {
    EigenResult r;
    Vector v = initial_vector(opt, A.size());

    for (r.iterations = 1; r.iterations <= opt.max_iter; ++r.iterations) {
        v = step(v);
        normalize(v);

        const Vector Av = A * v;
        r.lambda = dot(v, Av);                 // quociente de Rayleigh (‖v‖ = 1)

        Vector res = Av;
        for (std::size_t i = 0; i < res.size(); ++i) res[i] -= r.lambda * v[i];
        r.residual = norm2(res);

        if (r.residual <= opt.tol * std::max(1.0, std::abs(r.lambda))) {
            r.converged = true;
            break;
        }
    }
    if (!r.converged) r.iterations = opt.max_iter;

    fix_sign(v);
    r.v = std::move(v);
    return r;
}

} // namespace detail

// ---------------------------------------------------------------------------
// Método da Potência
//   v_{k+1} = A·v_k / ‖A·v_k‖
// Converge para o autovetor do autovalor de MAIOR módulo |λ₁|, com taxa
// |λ₂/λ₁|. Se |λ₁| = |λ₂| (ex.: λ e −λ) não converge — o resultado volta com
// converged = false.
// ---------------------------------------------------------------------------
inline EigenResult power_method(const Matrix& A, const IterOptions& opt = {}) {
    detail::require_symmetric(A, "power_method");
    return detail::iterate(A, opt, [&](const Vector& v) { return A * v; });
}

// ---------------------------------------------------------------------------
// Método da Potência Inversa (com deslocamento σ)
//   (A − σI)·w = v_k,   v_{k+1} = w / ‖w‖
// Os autovalores de (A − σI)⁻¹ são 1/(λᵢ − σ), então o dominante corresponde
// ao autovalor de A MAIS PRÓXIMO de σ. Com σ = 0 (padrão) → menor |λ|.
// A matriz (A − σI) é fatorada uma única vez via LDLᵀ.
// ---------------------------------------------------------------------------
inline EigenResult inverse_power_method(const Matrix& A, double shift = 0.0,
                                        const IterOptions& opt = {}) {
    detail::require_symmetric(A, "inverse_power_method");

    Matrix S = A;
    for (std::size_t i = 0; i < S.size(); ++i) S(i, i) -= shift;
    const LDLT fact(S);

    return detail::iterate(A, opt, [&](const Vector& v) { return fact.solve(v); });
}

} // namespace la