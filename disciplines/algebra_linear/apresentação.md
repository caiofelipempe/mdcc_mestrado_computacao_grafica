---
marp: true
theme: default
paginate: true
---

# Projeto de Álgebra Linear em C++

Implementação de algoritmos clássicos de Métodos Numéricos em C++

**Caio Felipe de Moura Peixoto**

---

# Objetivo

Desenvolver implementações próprias para:

- Sistemas lineares
- Fatorações matriciais
- Ortogonalização
- Autovalores e autovetores
- Mínimos quadrados
- SVD

---

# Conteúdo Implementado

1. Eliminação de Gauss
2. Gauss-Jordan
3. LU
4. RREF
5. Cholesky
6. Gram-Schmidt

---

# Conteúdo Implementado

7. Mínimos Quadrados
8. Método da Potência
9. Householder
10. QR
11. QR Eigen
12. SVD

---

# Eliminação de Gauss

Resolve:

```text
Ax = b
```

Etapas:

- Escolha do pivô
- Eliminação progressiva
- Substituição regressiva

---

# Eliminação de Gauss

Métodos implementados:

```cpp
gaussPartialPivot()
gaussTotalPivot()
```

Pivoteamento:

- Parcial → troca linhas
- Total → troca linhas e colunas

---

# Gauss-Jordan

Transformação:

```text
[A | b]
↓
[I | x]
```

Características:

- Elimina acima e abaixo do pivô
- Produz solução diretamente

---

# Gauss-Jordan

Métodos:

```cpp
gaussJordanPartialPivot()
gaussJordanTotalPivot()
```

Resultado:

```text
x
```

é obtido sem substituição regressiva.

---

# Decomposição LU

Objetivo:

```text
A = LU
```

Onde:

```text
L → triangular inferior
U → triangular superior
```

---

# Decomposição LU

Métodos:

```cpp
luDecomposition()
solveLU()
```

Resolução:

```text
Ly = b
Ux = y
```

---

# RREF

Método:

```cpp
computeRREF()
```

Calcula:

- Forma escalonada reduzida
- Rank
- Nullity

---

# RREF

Exemplo:

```text
linha3 = linha1 + linha2
```

Resultado:

```text
rank = n - 1
nullity = 1
```

---

# Decomposição de Cholesky

Aplicável a matrizes:

```text
SPD
```

(Simétricas Positivas Definidas)

Objetivo:

```text
A = SSᵀ
```

---

# Decomposição de Cholesky

Cálculo:

```text
Sii = √(Aii − soma)
```

Aplicações:

- Sistemas SPD
- Estatística
- Otimização

---

# Gram-Schmidt

Objetivo:

Transformar uma base qualquer em:

```text
Base Ortonormal
```

---

# Gram-Schmidt

Propriedades:

```text
qi·qj = 0
```

```text
||qi|| = 1
```

Aplicações:

- QR
- Projeções
- Computação gráfica

---

# Mínimos Quadrados

Resolve:

```text
Ax ≈ b
```

Quando não existe solução exata.

---

# Mínimos Quadrados

Equações normais:

```text
(AᵀA)x = Aᵀb
```

Método:

```cpp
leastSquares()
```

---

# Regressão Linear

Modelo:

```text
y = ax + b
```

Método:

```cpp
linearRegression()
```

---

# Regressão Linear

Teste utilizado:

```text
y = 2x + 1 + ruído
```

Resultado esperado:

```text
a ≈ 2
b ≈ 1
```

---

# Método da Potência

Objetivo:

Encontrar o maior autovalor da matriz.

Métodos:

```cpp
powerMethod()
inversePowerMethod()
shiftedPowerMethod()
```

---

# Método da Potência

Iteração:

```text
x(k+1)=Ax(k)
```

Estimativa:

```text
λ = xᵀAx
```

(Quociente de Rayleigh)

---

# Householder

Objetivo:

Transformar uma matriz simétrica em:

```text
Tridiagonal
```

---

# Householder

Reflexão:

```text
P = I − 2uuᵀ
```

Transformação:

```text
A' = PAP
```

---

# Decomposição QR

Objetivo:

```text
A = QR
```

Onde:

```text
Q → ortogonal
R → triangular superior
```

---

# Decomposição QR

Implementação:

1. Gram-Schmidt
2. Construção de Q
3. Cálculo de R

```text
R = QᵀA
```

---

# Método QR para Autovalores

Objetivo:

Encontrar:

- Autovalores
- Autovetores

---

# Método QR para Autovalores

Iteração:

```text
A = QR
A' = RQ
```

Repetindo até convergência.

---

# Método QR para Autovalores

Após convergência:

```text
Autovalores
=
Diagonal(A)
```

Autovetores:

```text
Acúmulo dos Q
```

---

# SVD

Objetivo:

```text
A = UΣVᵀ
```

Uma das decomposições mais importantes da Álgebra Linear.

---

# SVD

Primeiro:

```text
AᵀA
```

Depois:

- Autovalores
- Autovetores

---

# SVD

Construção de:

```text
V
```

Valores singulares:

```text
σ = √λ
```

---

# SVD

Cálculo de:

```text
u = Av / σ
```

Posteriormente:

```text
Gram-Schmidt
```

para ortonormalização.

---

# Estratégia de Testes

Foram executados testes para:

```text
N = 3
N = 4
N = 5
N = 10
```

---

# Validação

Gauss:

```text
b = Ax
```

recuperando:

```text
x conhecido
```

---

# Validação

LU:

```text
A ≈ LU
```

Cholesky:

```text
A ≈ SSᵀ
```

QR:

```text
A ≈ QR
```

---

# Validação

SVD:

```text
A ≈ UΣVᵀ
```

Método da Potência:

```text
comparado com QR
```

---

# Resultados

```text
Erro Gauss = 0
Erro LU = 0
Erro Cholesky = 0
Erro QR = 0
Erro SVD = 0
```

---

# Principais Aprendizados

- Resolução de sistemas lineares
- Fatorações matriciais
- Ortogonalização
- Métodos espectrais
- SVD

---

# Conclusão

Foram implementados e validados:

✅ 12 algoritmos clássicos

✅ Testes para múltiplas dimensões

✅ Validação numérica por reconstrução

✅ Implementação completa em C++

---

# Obrigado