---
marp: true
theme: default
paginate: true
header: 'Modelagem Geométrica'
footer: 'Trabalho 0'
style: |
  section {
    background-color: #111827;
    color: #f9fafb;
    font-family: 'Georgia', serif;
    padding: 50px;
  }

  h1 {
    color: #22d3ee;
    border-bottom: 3px solid #06b6d4;
  }

  h2,
  h3 {
    color: #67e8f9;
  }

  footer {
    font-size: 0.5em;
    color: #94a3b8;
  }

  header {
    color: #94a3b8;
  }

  .alerta {
    background-color: #1e293b;
    border-left: 6px solid #22d3ee;
    padding: 20px;
    margin-top: 20px;
    border-radius: 8px;
  }
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