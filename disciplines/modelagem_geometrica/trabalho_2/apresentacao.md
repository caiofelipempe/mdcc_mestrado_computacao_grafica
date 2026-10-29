---
marp: true
theme: default
paginate: true
size: 16:9
style: |
  section { font-size: 26px; }
  section.lead h1 { font-size: 52px; }
  h1 { color: #1f4e8c; }
  h2 { color: #1f4e8c; }
  code { font-size: 0.85em; }
  pre { font-size: 0.72em; }
  table { font-size: 0.8em; }
  .small { font-size: 0.8em; }
---

<!-- _class: lead -->

# Modelador CSG com Raycast de Quads

Modelagem Geométrica — Trabalho 02
C++20 · GLFW · OpenGL · ImGui

---

## O que é

Um modelador de sólidos por **CSG** que desenha **sem gerar malha do sólido**.

- O sólido é uma árvore binária: folhas são primitivas, nós internos são união, interseção e diferença
- A cada frame, a câmera lança raios contra a árvore e cada acerto vira um quad
- Dá para editar a árvore, mexer na câmera e salvar/abrir em texto

---

## Estrutura do código

| Módulo | O que faz |
|---|---|
| `csg.hpp` | Árvore CSG, pertinência, raycast, serializador e parser |
| `main.cpp` (`Trabalho02`) | Cena editável, UI, câmera, geração dos quads |
| `shape.hpp` | Esfera, bloco, cubo, cilindro, cone, cápsula, tetraedro |
| `Rot3f` / `Vec3f` | Rotação por quaternion |

`SceneNode` (modelo mutável da UI) → `buildCsg()` → `CsgNode` (árvore usada no cálculo).

---

## Árvore CSG

```cpp
using CsgPtr = std::unique_ptr<CsgNode>;
enum class CsgOp { Union, Intersection, Difference };

struct CsgPrimitive { std::unique_ptr<Shape> shape; Vec3f origin; Rot3f rotator; };
struct CsgOperation { CsgOp op; CsgPtr left; CsgPtr right; };   // Difference = left - right

class CsgNode { using Data = std::variant<CsgPrimitive, CsgOperation>; ... };
```

- Nó é um `variant` de folha ou operação, lido com `std::visit`
- `unique_ptr` para a posse; construção com `csgPrimitive`, `csgUnion`, `csgIntersection`, `csgDifference`

---

## Primitivas

Cada shape fica no **espaço local**, centrado na origem, com o eixo Y como eixo de revolução.

| Primitiva | Região |
|---|---|
| Esfera | $x^2+y^2+z^2 \le r^2$ |
| Bloco / Cubo | $\lvert x\rvert\le h_x,\ \lvert y\rvert\le h_y,\ \lvert z\rvert\le h_z$ |
| Cilindro | $x^2+z^2\le r^2$ e $\lvert y\rvert\le h/2$ |
| Cone | $(x^2+z^2)\,h^2 \le r^2(h/2-y)^2$ e $\lvert y\rvert\le h/2$ |
| Cápsula | distância ao segmento do eixo $\le r$ |
| Tetraedro regular | 4 semiespaços $n\cdot p \ge -s$ |

---

## `contains(p)`

1. Leva o ponto do mundo para o local: `(p - origin).rotated(q.conjugated())`
2. Testa a primitiva (`detail::inside`)
3. Nos nós internos, combina os filhos:

| Operação | Regra |
|---|---|
| União | `l \|\| r` |
| Interseção | `l && r` |
| Diferença | `l && !r` |

---

## Raycast: o raio vira lista de intervalos

Para um raio $o + t\,d$, cada nó devolve os intervalos de $t$ em que o raio está **dentro** do sólido:

```cpp
struct Interval { float t0, t1; Vec3f n0, n1; };  // entrada, saída e normais externas
using Intervals = std::vector<Interval>;          // ordenados e disjuntos
```

- Operações booleanas sobre intervalos dão intervalos, então a conta fecha
- `raycast()` pega o primeiro intervalo com $t_1>0$ e devolve um `RayHit`

---

## Primitivas como $At^2 + Bt + C \le 0$

```cpp
template <class Grad>
Intervals quadraticLE0(float A, float B, float C, Grad grad);
```

- `grad(t)` dá o gradiente no ponto de contato, que é a **normal externa**
- Trata caso linear ($A\approx 0$), discriminante negativo e intervalos infinitos
- Semiespaço é o caso $A=0$: bloco e tetraedro são interseção de semiespaços
- Cilindro = laje em Y ∩ cilindro infinito. Cone = laje ∩ cone duplo
- Cápsula = (laje ∩ cilindro) ∪ esfera de cada ponta

---

## Álgebra de intervalos

- **`unite`:** ordena por `t0` e funde o que se sobrepõe
- **`intersect`:** varredura com dois ponteiros; entra no maior `t0`, sai no menor `t1`
- **`subtract`:** recorta A pelos intervalos de B; onde a fronteira vem de B, a normal vira $-n_B$

```cpp
if (cut.t0 > cur.t0) out.push_back({cur.t0, cut.t0, cur.n0, cut.n0 * -1.0f});
cur.t0 = cut.t1;
cur.n0 = cut.n1 * -1.0f;
```

---

## Por que cada shape guarda as normais de entrada e saída

Cada primitiva devolve `{t0, t1, n0, n1}`. A normal viaja junto com o `t` pela árvore.

- **Não dá para recalcular na raiz:** depois de combinar os nós só sobram `t0` e `t1`, e ninguém sabe qual primitiva definiu a fronteira
- **Entrada (`n0`):** vira `normalEnter` e é a `lightNormal` do quad
- **Saída (`n1`):** a álgebra precisa dela. Na diferença, a saída de B vira a entrada de A − B com sinal trocado (`cur.n0 = cut.n1 * -1`)
- Em união e interseção vence a normal de quem define o menor `t0` / maior `t1`

| Shape | Normal no ponto de contato |
|---|---|
| Semiespaço (bloco, tetraedro) | $n$ constante |
| Esfera | $p - c$ |
| Cilindro (lateral) | $(x,\ 0,\ z)$ |
| Cone | $(x,\ m^2(h/2-y),\ z)$ |

<span class="small">Com $t$ infinito, ou origem do raio já dentro, a normal é zero e o quad usa $-d$. As normais voltam do local para o mundo junto com a rotação.</span>

---

![bg left:58% fit](img/raycast_uniao.png)

## Raycast na união

- Os intervalos de A e B se sobrepõem e viram **um só**
- Entra onde A entra (menor `t0`) e sai onde B sai (maior `t1`)
- As normais vêm de quem define cada ponta: entrada de A, saída de B

---

![bg left:58% fit](img/raycast_intersecao.png)

## Raycast na interseção

- Só sobra o trecho **comum** a A e B
- Entra no maior `t0` (entrada de B) e sai no menor `t1` (saída de A)
- A normal de cada ponta é a do shape que "manda" naquela ponta

---

![bg left:58% fit](img/raycast_diferenca.png)

## Raycast na diferença

- O trecho de B é **cortado** de A
- Sobra de A só o que vem antes de B: `[t0 de A, t0 de B]`
- Na ponta do corte a normal **inverte**: a saída do resultado usa $-n_B$ da entrada de B, apontando para dentro de B

---

![bg left:58% fit](img/raycast_diferenca_contraria.png)

## Raycast na diferença contrário

- Aqui o raio entra pelo lado de B, o objeto que subtrai
- B é cortado de A: o resultado começa onde B termina (cur.t0 = cut.t1)
- A normal de entrada é a saída de B invertida (cut.n1 * -1): aponta para dentro de B, que é o lado de fora do resultado
- A saída é a de A, sem mudança

---

## Visão geral do raycast

![h:470 center](img/raycast_quads_visao_geral.svg)

<span class="small">Um raio por pixel da grade reduzida. Quem acerta vira um quad voltado para a câmera, no ponto de entrada.</span>

---

## Transformações

- O raio vai para o espaço local: `(o - origin).rotated(inverse)` e `d.rotated(inverse)`
- A transformação é rígida, então **$t$ não muda** e os intervalos já valem no mundo
- As normais voltam ao mundo com a rotação direta
- A rotação é um quaternion (`Rot3f`); a UI edita em graus e `rotatorOf()` converte

---

## Geração dos quads

A cada frame (`onUpdate` → `rebuildMesh`):

1. Grade de `w × h` raios, com $w = \text{largura}/\text{escala}$ (escala de 1 a 8)
2. Direção de cada pixel a partir de `forward`, `right`, `up` e do FOV
3. Se acertou: ponto $= o + t_{enter}\,d$
4. Quad virado para a câmera, com tamanho proporcional a $z_{in}$
5. Dois triângulos com `lightNormal` = normal de entrada (se a origem está dentro, usa $-d$)

Os raios saem da câmera atual, então a malha é refeita quando ela se move.

---

## Volume aproximado

Cada pixel cobre um tronco de pirâmide entre $z_{in}$ e $z_{out}$ ($z = t\cos\theta$):

$$V_{pixel} = \text{pixelW}\cdot\text{pixelH}\cdot\frac{z_{out}^3 - z_{in}^3}{3}$$

- A soma de todos os pixels aparece em *Estatísticas*
- Só usa o **primeiro intervalo** de cada raio, então subestima sólidos com mais de uma passagem (ex.: diferença que abre um vazio)

---

## Salvar e abrir em texto

```text
U(Es(Ps(0,0,0),Tr(0,0,0,1),3),D(Cx(Ps(2,0,0),Tr(0,0,0,1),5,4,3),Ci(Ps(2,0,0),Tr(0,0,0,1),1,6)))
```

- `U`, `I`, `D` = união, interseção, diferença. `Ps` = posição. `Tr` = quaternion (x,y,z,w)
- Tags `Es`, `Cx`, `Cb`, `Ci`, `Cn`, `Cp`, `Te` ficam numa tabela (`kShapeSpecs`)
- Parser de descida recursiva que informa a posição do erro; `CsgNode::parse` devolve `Result<CsgPtr, std::string>`
- `to_chars` / `from_chars` garantem ida e volta exata. Arquivo inválido não mexe na cena

---

## Interface

- **Outliner:** seleciona nós; botão direito adiciona operação + primitiva, muda a operação ou remove o nó
- **Propriedades** em abas: *Cena* (limites, resolução, exibição, estatísticas), *Nó* (transformação, forma, trocar A e B), *Câmera* (FOV, near/far, sensibilidades)
- **Arquivo:** Abrir / Salvar `.txt`
- **Câmera:** botão direito orbita, scroll dá zoom, WASD + Q/E move

---

## Decisões

- `variant` + `visit` em vez de hierarquia de nós com virtuais
- `std::derived_from<Shape>` em `csgPrimitive`: só aceita shape válido, sem fábrica
- Erro de parse como valor (`Result`), sem exceção na UI
- `ShapeSpec` concentra tag, aridade e construtor de cada primitiva
- Custo por frame: `w · h · (nós da árvore)`; a escala de resolução controla

---

## Limitações

- `dynamic_cast` repetido em `inside`, `shapeIntervals` e na serialização
- Os quads dependem da câmera e da resolução; não são uma superfície extraída do sólido
- Sem aceleração (BVH, AABB por nó) para descartar raios
- Volume só considera o primeiro intervalo de cada raio
- Tudo em `float`, com tolerâncias fixas em `quadraticLE0`

---

<!-- _class: lead -->

# Resumo

CSG como **árvore + álgebra de intervalos sobre raios**.
O mesmo mecanismo dá visualização, normais e volume aproximado.