---
marp: true
theme: default
paginate: true
math: katex
style: |
  section {
    background-color: #111827;
    color: #f9fafb;
    font-family: 'Georgia', serif;
    font-size: 26px;
    padding: 50px;
  }
  h1 {
    color: #22d3ee;
    border-bottom: 3px solid #06b6d4;
    padding-bottom: 6px;
  }
  h2, h3 { color: #67e8f9; }
  strong { color: #fbbf24; }
  footer { font-size: 0.5em; color: #94a3b8; }
  header { color: #94a3b8; }
  blockquote {
    background-color: #1e293b;
    border-left: 6px solid #22d3ee;
    padding: 12px 20px;
    border-radius: 8px;
    color: #e2e8f0;
  }
  table { font-size: 0.78em; margin: 0 auto; }
  th { background-color: #0e7490; color: #f9fafb; }
  td { background-color: #1e293b; }
  code { color: #67e8f9; background-color: #1e293b; }
  pre { background-color: #0b1220; border-radius: 8px; }
  section.titulo { text-align: center; justify-content: center; }
  section.titulo h1 { border-bottom: none; font-size: 1.8em; }
---

<!-- _class: titulo -->

# Tópicos Especiais em CG

## Colisão contínua de corpos rígidos representados por faces

---

# Representação Geométrica

Como representar um corpo rígido?

Abordagens tradicionais:

- Meshes
- Convex Hulls
- Esferas
- Cápsulas
- Caixas

Proposta:

Representar a geometria através de faces.

---

# Face Como Plano

Cada face é representada por:

$$
\pi=(w,x,y,z)
$$

onde:

- $x,y,z$ definem a orientação da face
- $w$ define sua posição

Equação do plano:

$$
w+xX+yY+zZ=0
$$

---

# Faces, Não Pontos

Em uma mesh tradicional:

```text
vértices
↓
arestas
↓
faces
```

Nesta proposta:

```text
faces
↓
arestas
↓
vértices
```

As faces tornam-se o elemento fundamental.

---

# Interpretação de um Plano

O elemento

$$
\pi=(w,x,y,z)
$$

não representa um ponto.

Ele representa uma face infinita.

- $(x,y,z)$ define a normal
- $w$ desloca o plano

---

# Exemplo

Considere:

$$
\pi=(-2,1,0,0)
$$

Temos:

$$
-2 + X = 0
$$

Logo:

$$
X=2
$$

---

# De Plano Para Equação

Dado:

$$
\pi=(w,x,y,z)
$$

obtemos diretamente:

$$
w+xX+yY+zZ=0
$$

O plano já está completamente descrito.

---

# Interseção de Planos

Em 3D:

- dois planos → reta
- três planos → ponto

Assim:

```text
faces
↓
arestas
↓
vértices
```

surgem naturalmente.

---

# Arestas Surgem Naturalmente

$$
\pi_1 \cap \pi_2
\rightarrow
L
$$

```text
Plano 1
    ╲
     ╲
      ╲
       ╲
        ╳──── reta
       ╱
      ╱
Plano 2
```

Não armazenamos arestas.

---

# Vértices Surgem Naturalmente

$$
\pi_1
\cap
\pi_2
\cap
\pi_3
=
P
$$

```text
3 faces
↓
1 vértice
```

Não armazenamos vértices.

---

# Exemplo: Canto de um Cubo

Considere:

$$
-1+X=0
$$

$$
-1+Y=0
$$

$$
-1+Z=0
$$

A interseção produz:

$$
(1,1,1)
$$

---

# Construção de um Cubo

Faces:

$$
(-1,1,0,0)
$$

$$
(-1,-1,0,0)
$$

$$
(-1,0,1,0)
$$

$$
(-1,0,-1,0)
$$

$$
(-1,0,0,1)
$$

$$
(-1,0,0,-1)
$$

---

# O Que é Armazenado?

Mesh tradicional:

```text
Vértices
Arestas
Triângulos
```

Proposta:

```text
Planos
```

Todo o resto é derivado.

---

# Filosofia da Representação

Tradicional:

```text
Pontos geram faces
```

Proposta:

```text
Faces geram pontos
```

A geometria passa a ser descrita por restrições espaciais.

---

# Face Como Restrição

Cada plano define:

$$
w+xX+yY+zZ=0
$$

e um semiespaço:

$$
w+xX+yY+zZ \le 0
$$

---

# Corpo Como Interseção

Um corpo é definido por:

$$
\pi_1,\pi_2,\ldots,\pi_n
$$

Equivalentemente:

$$
B
=
\bigcap_i
\left\{
X :
w_i+x_iX+y_iY+z_iZ \le 0
\right\}
$$

---

# Exemplo: Quadrado

Faces:

$$
(-1,1,0,0)
$$

$$
(-1,-1,0,0)
$$

$$
(-1,0,1,0)
$$

$$
(-1,0,-1,0)
$$

---

# Restrições

As faces produzem:

$$
-1+X\le0
$$

$$
-1-X\le0
$$

$$
-1+Y\le0
$$

$$
-1-Y\le0
$$

---

# Região Obtida

```text
+---------+
|         |
|         |
|         |
+---------+
```

Quadrado centrado na origem.

---

# Exemplo: Cubo

Faces:

$$
(-1,1,0,0)
$$

$$
(-1,-1,0,0)
$$

$$
(-1,0,1,0)
$$

$$
(-1,0,-1,0)
$$

$$
(-1,0,0,1)
$$

$$
(-1,0,0,-1)
$$

---

# Sem Triangulação

Não armazenamos:

- vértices
- arestas
- triângulos

Apenas planos.

---

# Regiões Não Fechadas

Nem toda geometria precisa ser um poliedro fechado.

Podemos representar regiões infinitas.

---

# Chão Infinito

Plano:

$$
(0,0,1,0)
$$

Equação:

$$
Y=0
$$

Semiespaço:

$$
Y\le0
$$

---

# Parede Infinita

Plano:

$$
(-1,1,0,0)
$$

Equação:

$$
X=1
$$

Semiespaço:

$$
X\le1
$$

---

# Corredor Infinito

Planos:

$$
(-1,1,0,0)
$$

$$
(-1,-1,0,0)
$$

Produzem:

$$
-1\le X\le1
$$

---

# Sala Sem Teto

Cinco faces:

```text
+X
-X
+Y
+Z
-Z
```

Produzem:

```text
┌───────┐
│       │
│       │
└───────┘

aberta para cima
```

---

# Estrutura de Dados

```cpp
struct Plane
{
    float w;
    float x;
    float y;
    float z;
};

struct ConvexRegion
{
    std::vector<Plane> faces;
};
```

---

# Corpo Rígido

```cpp
struct RigidBody
{
    Motor motor;
    Twist twist;

    ConvexRegion region;
};
```

---

# Movimento da Geometria

Cada plano move-se por:

$$
\pi_i(t)
=
M(t)\pi_iM^{-1}(t)
$$

Portanto:

$$
Region(t)
=
M(t)\,Region\,M^{-1}(t)
$$

---

# Vantagens

✅ Representação compacta

✅ Sem triangulação

✅ Compatível com PGA

✅ Regiões infinitas

✅ Regiões abertas

✅ Corpo definido por restrições

---

# Arquitetura Proposta

```text
Planos
↓
Região Convexa
↓
Motor
↓
Movimento Contínuo
↓
Distância
↓
TOI
↓
Colisão
```

---