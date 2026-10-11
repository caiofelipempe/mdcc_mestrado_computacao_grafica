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
  section.denso { font-size: 22px; }
  section.refs { font-size: 19px; }
---

<!-- _class: titulo -->

# Topicos Especiais em CG

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

# Face Como Vetor

Cada face é representada por:

$$
s = nd
$$

onde:

- $n$ é a normal da face
- $d$ é a distância da face à origem

Armazenamos apenas:

$$
s
$$

---

# Faces, Não Pontos

Em uma mesh tradicional armazenamos:

```text
vértices
↓
arestas
↓
faces
```

Em algebra Geométrica:

```text
faces
↓
arestas
↓
vértices
```

As faces tornam-se o elemento fundamental.

---

# Interpretação de um Vetor

O vetor

$$
s = nd
$$

não representa um ponto do sólido.

Ele representa uma face.

```text
Origem
  │
  │─────► s
  │
```

A direção define a orientação da face.

O comprimento define sua distância à origem.

---

# De Vetor Para Plano

Dado:

$$
s
$$

extraímos:

$$
n=\frac{s}{|s|}
$$

$$
d=|s|
$$

Obtendo a equação:

$$
n\cdot x=d
$$

Assim cada vetor gera um plano.

---

# Exemplo

Considere:

$$
s=(2,0,0)
$$

Então:

$$
n=(1,0,0)
$$

$$
d=2
$$

Plano gerado:

$$
x=2
$$

---

# Arestas Surgem Naturalmente

Em 3D,

a interseção de dois planos gera uma reta.

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

As arestas surgem da geometria.

---

# Vértices Surgem Naturalmente

Em 3D,

a interseção de três planos gera um ponto.

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

Eles são reconstruídos quando necessário.

---

# Exemplo: Canto de um Cubo

Considere:

$$
x=1
$$

$$
y=1
$$

$$
z=1
$$

A interseção produz:

$$
(1,1,1)
$$

que é um vértice do cubo.

---

# Construção de um Cubo

Faces:

$$
x=1
$$

$$
x=-1
$$

$$
y=1
$$

$$
y=-1
$$

$$
z=1
$$

$$
z=-1
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
Faces
```

Todo o resto é derivado.

---

# Filosofia da Representação

Na representação tradicional:

```text
Pontos geram faces
```

Nesta proposta:

```text
Faces geram pontos
```

A geometria passa a ser descrita por restrições espaciais em vez de amostras da superfície.

---

# Exemplo

Considere:

$$
s=(2,0,0)
$$

Temos:

$$
n=(1,0,0)
$$

$$
d=2
$$

Logo a face está localizada em:

$$
x=2
$$

---

# Face Como Restrição

Cada face define:

$$
n \cdot x = d
$$

e o semiespaço:

$$
n \cdot x \le d
$$

Todo ponto que satisfaz a restrição pertence à região.

---

# Corpo Como Interseção

Um corpo é definido por várias faces:

$$
s_1,s_2,\ldots,s_n
$$

Equivalentemente:

$$
B
=
\bigcap_i
\left\{
x :
n_i\cdot x\le d_i
\right\}
$$

O corpo surge da interseção das restrições.

---

# Exemplo: Quadrado

Faces:

$$
(1,0)
$$

$$
(-1,0)
$$

$$
(0,1)
$$

$$
(0,-1)
$$

---

# Restrições

As faces geram:

$$
x \le 1
$$

$$
x \ge -1
$$

$$
y \le 1
$$

$$
y \ge -1
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

Um quadrado centrado na origem.

---

# Exemplo: Cubo

Um cubo pode ser descrito por apenas seis faces:

```text
(+X)
(-X)

(+Y)
(-Y)

(+Z)
(-Z)
```

Sem necessidade de armazenar:

- vértices
- arestas
- triângulos

---

# Regiões Não Fechadas

Nem toda geometria precisa ser um poliedro fechado.

A região pode permanecer aberta.

---

# Chão Infinito

Uma única face:

$$
(0,1,0)
$$

define:

$$
y \le 1
$$

Visualmente:

```text
      ar

-------------------

      chão
```

---

# Parede Infinita

Uma face:

$$
(1,0,0)
$$

define:

$$
x \le 1
$$

Visualmente:

```text
|
|
| espaço
|
|
```

---

# Corredor Infinito

Duas faces:

$$
(1,0,0)
$$

$$
(-1,0,0)
$$

geram:

$$
-1 \le x \le 1
$$

Visualmente:

```text
|
| corredor
|
| infinito
|
```

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

produzem:

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
struct ConvexRegion
{
    std::vector<Vec3> faces;
};
```

Cada vetor armazena:

```cpp
face = normal * distância
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

O motor move todas as faces:

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

✅ Não depende de mesh

✅ Funciona para volumes fechados

✅ Funciona para pisos infinitos

✅ Funciona para paredes infinitas

✅ Compatível com PGA

---

# Arquitetura Proposta

```text
Faces
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

# Linha de Pesquisa

```text
PGA
↓
Motores
↓
Faces Convexas
↓
Regiões Geométricas
↓
CCD
↓
Geometric Physics Engine
```