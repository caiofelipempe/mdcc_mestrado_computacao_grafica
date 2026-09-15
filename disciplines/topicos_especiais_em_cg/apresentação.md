---
marp: true
theme: default
paginate: true
#header: 'Modelagem Geométrica'
#footer: 'Trabalho 0'
style: |
  section {
    background-color: #fdf6e2;
    color: #432818;
    font-family: 'Georgia', serif;
    padding: 50px;
  }
  h1 {
    color: #bb3e03;
    border-bottom: 3px solid #ee9b00;
  }
  h2, h3 {
    color: #9b2226; 
  }
  footer {
    font-size: 0.5em;
    color: #7f5539;
  }
  .alerta {
    background-color: #ffe8d6;
    border-left: 6px solid #ca6702;
    padding: 20px;
    margin-top: 20px;
    border-radius: 8px;
  }

---

# Levantamento de Temas de Pesquisa em Computação Gráfica

### Objetivo

Identificar temas de pesquisa com:

- relevância científica;
- potencial de inovação;
- fundamentação matemática sólida;
- viabilidade de implementação.

---

# Motivação

## Questão Central

É possível substituir parte da pipeline tradicional

```text
Malha
↓
Triângulos
↓
Colisão
↓
Rasterização
```

---

## Questão Central

por representações mais compactas e contínuas baseadas em:

- funções;
- primitivas analíticas;
- geometria convexa;
- representações implícitas.

---

# Temas Investigados

## Geometria Computacional

- Continuous Collision Detection (CCD)
- Support Functions
- Signed Distance Functions (SDF)
- Geometria Convexa
- Swept Volumes
---

# Temas Investigados

## Renderização

- Point-Based Rendering
- Gaussian Splatting
- Superquádricas
- Sprites Dinâmicos

---

# Simulação Física Moderna

## Principais Tendências

### Holz et al. (2025)

- simulação multifísica;
- corpos rígidos;
- fluidos;
- materiais deformáveis.

### Bacher et al. (2025)

- dinâmica baseada em quatérnions;
- maior estabilidade;
- restrições mais robustas.

### Insight

A tendência atual é aproximar física, geometria e otimização.

---

# Continuous Collision Detection

## Problema

### Tunneling

```text
t

O        |

t + Δt

         O|
```

A colisão ocorre entre frames.

---

# Continuous Collision Detection

## Estado da Arte

Principais referências:

- Redon et al. (2002)
- Tang et al. (2009)
- Coumans (2005)
- Catto (2013)

### Objetivo

Determinar:

```text
Quando ocorre a colisão?
```

e não apenas:

```text
Existe colisão?
```

---

# Support Functions e GJK

## Ideia Fundamental

Para um corpo convexo:

\[
h(u)=\max_{x\in P}(u\cdot x)
\]

Pergunta respondida:

> Qual é o ponto mais distante em uma determinada direção?

---

# Support Functions e GJK

## Por que isso é interessante?

Muitas operações passam a depender apenas da função:

\[
h(u)
\]

sem necessidade de armazenar explicitamente todos os vértices.

### Tendência Moderna

```text
Colisão
=
Otimização Convexa
```

---

# Signed Distance Functions (SDFs)

## Conceito

Uma única função fornece:

- colisão;
- distância mínima;
- penetração;
- normais.

```text
Objeto
↓
Função
↓
Geometria
```

---

# Representação Polar de Poliedros

## Hipótese Principal

Representar um poliedro por:

```text
Centro
+
Vetores Polares
```

ao invés de:

```text
Vértices
Arestas
Faces
```

---

# Representação Polar

## Vetor Polar

Para cada face:

\[
p_i = d_i n_i
\]

onde:

- \(n_i\) é a normal;
- \(d_i\) é a distância ao centro.

Cada vetor codifica:

- orientação;
- posição;
- distância.

---

# Exemplo: Cubo

```text
(+r,0,0)
(-r,0,0)

(0,+r,0)
(0,-r,0)

(0,0,+r)
(0,0,-r)
```

Nenhum vértice é necessário.

### Possível vantagem

Representação extremamente compacta.

---

# Questões de Pesquisa

1. Um poliedro pode ser reconstruído apenas pelos vetores polares?

2. É possível aproximar funções suporte diretamente?

3. Podemos construir CCD sem vértices?

4. Podemos gerar swept volumes diretamente dessa representação?

---

# Gaussian Splatting

## Tendência Atual

Substituir:

```text
Malhas
```

por:

```text
Gaussianas
```

Cada gaussiana possui:

- posição;
- orientação;
- escala;
- cor.

---

# Superquádricas e Elipsoides

## Ideia

Substituir milhares de triângulos por poucas primitivas.

```text
Cabeça  → Elipsoide
Braço   → Cápsula
Tronco  → Superquádrica
```

### Aplicações

- LOD natural;
- animação simplificada;
- sprites dinâmicos.

---

# Proposta Desejado

## Tema de Mestrado

### Detecção Contínua de Colisão Baseada em Representações Polares de Poliedros Convexos

Combina:

- Geometria Convexa;
- Support Functions;
- CCD;
- Swept Volumes;
- Simulação Física.