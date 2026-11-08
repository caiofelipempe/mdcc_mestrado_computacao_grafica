---
marp: true
theme: default
paginate: true
math: katex
style: |
  section {
    background-color: #1a1d12;
    color: #d8d4c4;
    font-family: 'Georgia', serif;
    font-size: 26px;
    padding: 50px;
  }
  h1 {
    color: #d4af37;
    border-bottom: 3px solid #cd7f32;
    padding-bottom: 6px;
  }
  h2, h3 { color: #a3b86c; }
  strong { color: #d9904f; }
  footer { font-size: 0.5em; color: #a8a89a; }
  header { color: #a8a89a; }
  blockquote {
    background-color: #252a18;
    border-left: 6px solid #d4af37;
    padding: 12px 20px;
    border-radius: 8px;
    color: #e8e4d4;
  }
  table { font-size: 0.72em; margin: 0 auto; }
  th { background-color: #556b2f; color: #d4af37; }
  td { background-color: #252a18; color: #d8d4c4; }
  code { color: #c0c0c0; background-color: #252a18; }
  pre { background-color: #12150d; border-radius: 8px; }
  section.titulo { text-align: center; justify-content: center; }
  section.titulo h1 { border-bottom: none; font-size: 1.8em; }
  ul { margin-top: 0.4em; }
  p { margin: 0.4em 0; }
---

<!-- _class: titulo -->
<!-- _paginate: false -->

# Tópicos Especiais em Computação Gráfica

### Trabalho 3

---

## Agenda

1. **Motivação:** por que Geometric Algebra (GA)?
2. **Bloco I — Fundamentos e livros-texto** — refs [2], [9], [10], [20], [6]
3. **Bloco II — Geometria projetiva e cinemática** — refs [3], [4], [13], [16], [14]
4. **Bloco III — Dinâmica em PGA** — ref [1]
5. **Bloco IV — Conformal, quádricas e otimização** — refs [5], [7], [8], [15]
6. **Bloco V — Implementações e indústria** — refs [19], [11], [12], [17], [18]
7. **Síntese, tendências e conclusões**

---

## Por que Geometric Algebra?

> A GA — ou álgebra de Clifford — unifica vetores, complexos, quaterniões e álgebra exterior em um único formalismo.

- **Representação unificada:** pontos, retas, planos, rotações e translações vivem no mesmo espaço algébrico.
- **Sem matrizes 4×4:** operadores geométricos são **versores** e **rotores**, com composição por produto geométrico.

---

## Por que GA? (cont.)

- **Dimensionalidade agnóstica:** a formulação se generaliza naturalmente para $n$ dimensões.
- **Estabilidade numérica:** elimina conversões entre representações e reduz casos especiais.

---

<!-- _class: titulo -->

# Bloco I
## Fundamentos e livros-texto

---

## [2] Dorst, Fontijne & Mann (2009) — *GA for Computer Science*

| Dado | Informação |
|------|------------|
| **Autores** | Leo Dorst, Daniel Fontijne, Stephen Mann |
| **Veículo** | Morgan Kaufmann, 2009 |
| **Tipo** | Livro-texto de referência |
| **Escopo** | GA para programadores de CG e *entertainment computing* |

**O que propõe:** primeiro livro dedicado a apresentar a GA como alternativa prática à álgebra linear, com abordagem **orientada a objetos** para implementar operadores geométricos.

---

## [2] Dorst, Fontijne & Mann (2009) — *(cont.)*

**Uso industrial:** formação de equipes de P&D que substituem stacks matriciais por um formalismo unificado em *engines* gráficas, VR e robótica.

**Conclusão dos autores:** a GA simplifica a expressão e a implementação de algoritmos geométricos; o custo de aprendizado é compensado pela **clareza conceitual** e pela redução de casos especiais.

---

## [9] Browne (2012) — *Grassmann Algebra, Vol. 1*

| Dado | Informação |
|------|------------|
| **Autor** | John Browne |
| **Veículo** | Barnard Publishing, 2012 |
| **Tipo** | Livro (fundamentos) |
| **Tema** | Álgebra exterior como sistema computacional para geometria |

**O que propõe:** apresenta a **álgebra de Grassmann** (precursora histórica da GA) com notação e ferramentas algorítmicas para *multivetores* e *versores*.

---

## [9] Browne (2012) — *(cont.)*

**Uso industrial:** ponte incremental para equipes que já usam álgebra exterior em CAD, visão e robótica.

**Conclusão:** a obra estabelece a base sobre a qual a PGA moderna foi construída, mostrando que muitos conceitos já estavam presentes na álgebra de Grassmann.

---

## [10] Dorst (2020) — *Correspondência Notacional Browne ↔ PGA*

| Dado | Informação |
|------|------------|
| **Autor** | Leo Dorst |
| **Veículo** | Nota apendicular em bivector.net, 2020 |
| **Tipo** | Nota técnica |

**O que propõe:** mapeia explicitamente a notação e as operações da álgebra de Grassmann de Browne para a **Plane-based Geometric Algebra (PGA)**.

**Uso industrial:** referência para migração de bibliotecas de álgebra exterior para PGA (ex.: Klein), reduzindo *onboarding* e erros de interpretação.

**Conclusão:** a correspondência é **direta e sistemática**, validando a PGA como evolução natural da álgebra de Grassmann.

---

## [20] Dorst (2002) — *The Inner Products of GA*

| Dado | Informação |
|------|------------|
| **Autor** | Leo Dorst |
| **Veículo** | *Applications of GA in CS and Engineering*, Birkhäuser, 2002 |
| **Tipo** | Artigo de fundamentação |

**O que propõe:** analisa as diferentes definições de **produto interno** em GA (escalar, contração à esquerda/direita, produto de Hestenes) e mostra que a escolha afeta a consistência das operações derivadas.

---

## [20] Dorst (2002) — *(cont.)*

**Uso industrial:** fundamental para implementações corretas de bibliotecas de GA; muitos *bugs* em geometria computacional vêm do uso inconsistente de produtos internos.

**Conclusão:** a escolha do produto interno deve ser guiada pela **consistência geométrica**, não pela conveniência algébrica.

---

## [6] Doran (2020) — *Euclidean Geometry and GA*

| Dado | Informação |
|------|------------|
| **Autor** | Chris Doran |
| **Veículo** | Blogpost, Cambridge, junho de 2020 |
| **Tipo** | Divulgação técnica |

**O que propõe:** apresenta a GA como linguagem natural para a **geometria euclidiana**, mostrando como reflexões, rotações, translações e movimentos rígidos emergem diretamente dos produtos geométricos.

**Uso industrial:** material introdutório para engenheiros e cientistas que desejam reformular problemas euclidianos clássicos em GA.

**Conclusão:** a GA não é apenas uma notação alternativa — é uma **reformulação conceitual** que torna a geometria euclidiana mais transparente e algorítmica.

---

<!-- _class: titulo -->

# Bloco II
## Geometria projetiva e cinemática de corpos rígidos

---

## [3] Selig (2000) — *Clifford Algebra of Points, Lines and Planes*

| Dado | Informação |
|------|------------|
| **Autor** | J. M. Selig |
| **Veículo** | *Robotica*, vol. 18, pp. 545–556, 2000 |
| **Tipo** | Artigo de periódico |

**O que propõe:** descreve a **álgebra de Clifford do grupo de movimentos** de corpos rígidos, com pontos, retas e planos como **elementos homogêneos**. Deriva incidência, distância, *meet* e *join* diretamente na álgebra.

---

## [3] Selig (2000) — *(cont.)*

**Uso industrial:**
- Robótica de montagem (*peg-in-hole*) com modelagem de tolerâncias.
- Visão estéreo: geometria epipolar compacta.
- CG: visibilidade e *clipping* de primitivas lineares.

**Conclusão:** a álgebra de Clifford fornece um **arcabouço unificado** para pontos, retas e planos, eliminando múltiplas representações.

---

## [4] Dorst (2016) — *3D Oriented Projective Geometry*

| Dado | Informação |
|------|------------|
| **Autor** | Leo Dorst |
| **Veículo** | *Adv. Appl. Clifford Algebras*, vol. 26, pp. 1137–1172, 2016 |
| **Tipo** | Artigo de periódico |

**O que propõe:** representa transformações projetivas 3D por **rotores em \( \mathbb{R}^{3,3} \)** (espaço das retas orientadas). Introduz uma **reflexão orientada** sobre retas e compara com Klawitter e Goldman et al.

---

## [4] Dorst (2016) — *(cont.)*

**Uso industrial:**
- CAD e *shape matching*: transformações projetivas orientadas.
- Visão 3D: estimação de pose a partir de linhas.
- AR: composição contínua de cenas.

**Conclusão:** a representação por versores de $\mathbb{R}^{3,3}$ é **mais expressiva e eficiente** do que abordagens não orientadas.

---

## [13] Gunn (2011) — *Cayley–Klein Geometries*

| Dado | Informação |
|------|------------|
| **Autor** | Charles G. Gunn |
| **Veículo** | Tese de doutorado, TU Berlin, 2011 |
| **Tipo** | Tese |

**O que propõe:** formula a **mecânica de corpos rígidos** em espaços de curvatura constante (euclidiano, elíptico, hiperbólico) usando as **geometrias de Cayley–Klein**, unificando cinemática e dinâmica.

---

## [13] Gunn (2011) — *(cont.)*

**Uso industrial:**
- Simulação física em ambientes não euclidianos (VR com distorções).
- Robótica em variedades.
- Visualização científica.

**Conclusão:** as geometrias de Cayley–Klein fornecem um **arcabouço unificado**, e a GA é a linguagem natural para expressar essa unificação nos três espaços clássicos.

---

## [16] Fontijne & Dorst (2011) — *Reconstructing Motions through Reflections*

| Dado | Informação |
|------|------------|
| **Autores** | Daniel Fontijne, Leo Dorst |
| **Veículo** | *Guide to GA in Practice*, Springer, 2011, pp. 63–78 |
| **Tipo** | Capítulo de livro |

**O que propõe:** novo algoritmo para reconstruir um **movimento de corpo rígido** a partir de correspondências exatas de pontos, construindo uma sequência de **reflexões** em GA, com fórmula compacta e rápida.

---

## [16] Fontijne & Dorst (2011) — *(cont.)*

**Uso industrial:**
- Registro 3D e *shape matching* (escaneamento, inspeção).
- Robótica: calibração *hand-eye*.
- AR: pose de marcadores em tempo real.

**Conclusão:** a abordagem por reflexões é **numericamente estável e eficiente**, dispensando SVD e outras decomposições custosas.

---

## [14] Dorst & Valkenburg (2011) — *Square Root and Logarithm of Rotors*

| Dado | Informação |
|------|------------|
| **Autores** | Leo Dorst, Robert Valkenburg |
| **Veículo** | *Guide to GA in Practice*, Springer, 2011, pp. 81–104 |
| **Tipo** | Capítulo de livro |

**O que propõe:** fórmulas explícitas para **raiz quadrada e logaritmo de rotores** em CGA 3D, via **decomposição polar**. O logaritmo é essencial para interpolação suave e velocidades angulares.

---

## [14] Dorst & Valkenburg (2011) — *(cont.)*

**Uso industrial:**
- Animação: interpolação de rotações com aceleração contínua.
- Simulação de robôs: trajetórias suaves em espaço de juntas.
- Visão: *averaging* de rotações.

**Conclusão:** a decomposição polar permite calcular raízes e logaritmos de forma **robusta e eficiente**, viabilizando aplicações em tempo real.

---

<!-- _class: titulo -->

# Bloco III
## Dinâmica e física em PGA

---

## [1] Dorst & De Keninck (2022) — *May the Forque Be with You*

| Dado | Informação |
|------|------------|
| **Autores** | Leo Dorst, Steven De Keninck |
| **Veículo** | bivector.net, 2022 (guia técnico) |
| **Tipo** | Guia técnico / artigo estendido |

**O que propõe:** aplica a PGA à **dinâmica de corpos rígidos** em qualquer dimensão. Introduz o conceito de **“forque”** (*force + torque*) como um bivetor unificado.

---

## [1] Dorst & De Keninck (2022) — *(cont.)*

**Uso industrial:**
- Motores de física para jogos e simulação.
- Robótica: dinâmica de manipuladores e veículos.
- Simulação de multidões e corpos deformáveis.

**Conclusão:** a PGA permite uma **unificação completa entre aspectos lineares e angulares** da dinâmica, madura para substituir formalismos vetoriais tradicionais.

---

<!-- _class: titulo -->

# Bloco IV
## Geometria conformal, quádricas e otimização

---

## [5] Breuils et al. (2018) — *QCGA em \( \mathbb{R}^{9,6} \)*

| Dado | Informação |
|------|------------|
| **Autores** | S. Breuils, V. Nozick, A. Sugimoto, E. Hitzer |
| **Veículo** | *Adv. Appl. Clifford Algebras*, vol. 28, 2018 |
| **Tipo** | Artigo de periódico |

**O que propõe:** introduz a **Quadric Conformal GA (QCGA)** em $\mathbb{R}^{9,6}$, capaz de construir **quádricas gerais** (elipsoides, paraboloides, hiperboloides, cilindros) a partir de pontos de controle via produto externo.

---

## [5] Breuils et al. (2018) — *(cont.)*

**Uso industrial:**
- CAD: modelagem e manipulação de quádricas.
- Visão: detecção e ajuste de superfícies em nuvens de pontos.
- CG: *ray tracing* de quádricas com transformações versoriais.

**Conclusão:** a QCGA fornece **representação unificada** para quádricas, com construção, transformação e interseção inteiramente dentro da álgebra.

---

## [7] De Keninck & Dorst (2019) — *GA Levenberg–Marquardt*

| Dado | Informação |
|------|------------|
| **Autores** | Steven De Keninck, Leo Dorst |
| **Veículo** | *Advances in Computer Graphics* (CGI 2019), Springer, pp. 511–522 |
| **Tipo** | Artigo de conferência |

**O que propõe:** implementação **livre de matrizes** do **Levenberg–Marquardt** em GA. Jacobianas e operações matriciais são substituídas por **produtos geométricos e contrações**.

---

## [7] De Keninck & Dorst (2019) — *(cont.)*

**Uso industrial:**
- *Bundle adjustment* e calibração de câmeras.
- Registro de nuvens de pontos (ICP e variantes).
- Estimação de parâmetros cinemáticos/dinâmicos em robótica.

**Conclusão:** a versão em GA é **compacta, expressiva e numericamente competitiva**, facilitando a incorporação de restrições geométricas.

---

## [8] De Keninck & Dorst (2020) — *Hyperwedge*

| Dado | Informação |
|------|------------|
| **Autores** | Steven De Keninck, Leo Dorst |
| **Veículo** | *Advances in Computer Graphics* (CGI 2020), Springer, pp. 549–554 |
| **Tipo** | Artigo de conferência |

**O que propõe:** resolve o problema da **construção direta de elementos geométricos** em álgebras de dimensão $N$ via produto externo de $N-1$ pontos, com sérios problemas numéricos em CGA e QCGA com *float* 64 bits.

---

## [8] De Keninck & Dorst (2020) — *(cont.)*

**Uso industrial:**
- Bibliotecas de GA: esferas, círculos, planos e quádricas com precisão.
- Visão: ajuste de modelos em altas dimensões.
- CG: construção eficiente em *shaders* e GPU.

**Conclusão:** o *hyperwedge* otimizado resolve um **gargalo numérico crítico** e torna viáveis aplicações em álgebras de alta dimensão.

---

## [15] Hitzer & Ichikawa (2013) — *Crystallographic Subperiodic Groups*

| Dado | Informação |
|------|------------|
| **Autores** | Eckhard Hitzer, Daisuke Ichikawa |
| **Veículo** | *Adv. Appl. Clifford Algebras*, vol. 23, pp. 887–906, 2013 |
| **Tipo** | Artigo de periódico |

**O que propõe:** representa **grupos cristalográficos subperiódicos** (camadas, cadeias, grupos pontuais) usando **versores e rotores** da álgebra de Clifford.

---

## [15] Hitzer & Ichikawa (2013) — *(cont.)*

**Uso industrial:**
- Cristalografia computacional e classificação de estruturas.
- Ciência dos materiais: nanotubos, filmes finos, interfaces.
- Visualização científica.

**Conclusão:** a GA fornece **representação compacta e algébrica** para grupos subperiódicos, facilitando a implementação computacional.

---

<!-- _class: titulo -->

# Bloco V
## Implementações, bibliotecas e aplicações industriais

---

## [19] Ong (2020) — *jeremyong/klein: v2.2.1*

| Dado | Informação |
|------|------------|
| **Autor** | Jeremy Ong (com S. De Keninck e Faaux) |
| **Veículo** | Zenodo, 2020 (software) |
| **Tipo** | Biblioteca C++ de código aberto |

**O que propõe:** **Klein** é uma implementação especializada e **otimizada com SIMD** da PGA 3D ($\mathcal{P}(\mathbb{R}^*_{3,0,1})$). A v2.2.1 oferece suporte a C++11/14.

---

## [19] Ong (2020) — *(cont.)*

**Uso industrial:**
- Motores gráficos e de jogos: rotações, translações, planos e pontos sem *overhead* de matrizes 4×4.
- Robótica e visão: integração em *pipelines* C++.
- Prototipagem: código aberto, bem documentado.

**Conclusão:** Klein demonstra que a PGA é **viável em produção**, com desempenho competitivo e API expressiva.

---

## [11] De Keninck & Gunn (2019) — *PGA Cheat Sheets*

| Dado | Informação |
|------|------------|
| **Autores** | Steven De Keninck, Charles G. Gunn |
| **Veículo** | Material do curso SIGGRAPH 2019 (bivector.net) |
| **Tipo** | Material de referência rápida |

**O que propõe:** duas folhas de referência para **PGA 3D e 2D**, cobrindo pontos, retas, planos, produtos geométricos, contrações, *meet/join*, rotores, motores e transformações comuns.

**Uso industrial:** apoio a desenvolvedores que implementam PGA em software comercial; padroniza a notação dentro das equipes.

**Conclusão:** as *cheat sheets* consolidam a PGA como ferramenta **prática e pronta para uso**.

---

## [12] De Keninck & Dorst (2020) — *PGA: Clean Up Your Mesh!*

| Dado | Informação |
|------|------------|
| **Autores** | Steven De Keninck, Leo Dorst |
| **Veículo** | Em preparação, 2020 |
| **Tipo** | Artigo em desenvolvimento |

**O que propõe:** embora o conteúdo completo não estivesse disponível, o título indica aplicação de PGA ao **processamento e limpeza de malhas poligonais**, usando operações geométricas exatas (*meet*, *join*, interseção de planos).

**Potencial industrial:** *pipeline* de assets (correção de auto-interseções, buracos, normais), impressão 3D (reparo de modelos) e CAD (validação geométrica).

**Conclusão esperada:** a PGA oferece abordagem **robusta e conceitualmente limpa** para *cleanup* de malhas.

---

## [17] Doran, Hestenes, Sommen & Van Acker (1993) — *Lie Groups as Spin Groups*

| Dado | Informação |
|------|------------|
| **Autores** | C. Doran, D. Hestenes, F. Sommen, N. Van Acker |
| **Veículo** | *J. Math. Phys.*, vol. 34, no. 8, pp. 3642–3669, 1993 |
| **Tipo** | Artigo de periódico |

**O que propõe:** demonstra que **grupos de Lie clássicos** podem ser representados como **grupos de spin** em GA. Rotações, translações e outras transformações contínuas tornam-se **versores**.

---

## [17] Doran et al. (1993) — *(cont.)*

**Uso industrial:**
- Robótica: representação de $SE(3)$ e $SO(3)$ com versores.
- Visão: otimização em variedades.
- Física e gráficos: simulação de sistemas com simetria contínua.

**Conclusão:** a identificação Lie ↔ Spin **simplifica a teoria de representações** e é um marco teórico que sustenta muitas aplicações práticas posteriores.

---

## [18] Stolfi (1991) — *Oriented Projective Geometry*

| Dado | Informação |
|------|------------|
| **Autor** | Jorge Stolfi |
| **Veículo** | Academic Press, 1991 |
| **Tipo** | Livro |

**O que propõe:** desenvolve a **geometria projetiva orientada** como arcabouço unificado para CG e geometria computacional, com pontos, retas e planos **orientados** e operações consistentes de incidência, interseção e transformação.

---

## [18] Stolfi (1991) — *(cont.)*

**Uso industrial:**
- Geometria computacional robusta.
- CG: *clipping* e faces visíveis.
- CAD: modelagem de sólidos e booleanas.

**Conclusão:** a geometria projetiva orientada é **precursora conceitual da PGA** e fornece base sólida para operações geométricas em CG.

---

## Síntese: o que os trabalhos têm em comum

| Dimensão | Padrão observado |
|----------|------------------|
| **Unificação** | Substituição de múltiplas representações (matrizes, quaterniões, vetores) por uma única álgebra |
| **Eficiência** | Vários artigos mostram GA **mais rápida e estável** que abordagens matriciais, com otimizações (SIMD, *hyperwedge*) |
| **Aplicabilidade** | Do fundamento teórico a bibliotecas prontas para produção (Klein), otimização (LM) e dinâmica (PGA) |
| **Generalidade** | PGA e QCGA formulam problemas em **dimensões arbitrárias** |

---

## Lacunas e direções futuras

- **Adoção em larga escala:** poucas bibliotecas comerciais maduras; Klein é um passo importante, mas faltam ferramentas de *debug*, *profiling* e integração com *engines*.
- **Educação:** a curva de aprendizado da GA é íngreme; livros e *cheat sheets* ajudam, mas falta formação sistemática.

---

## Lacunas e direções futuras (cont.)

- **Hardware:** GA pode se beneficiar de aceleradores dedicados (GPU/FPGA); o artigo *Hyperwedge* mostra que precisão numérica é um desafio tratável.
- **Integração com ML:** potencial inexplorado em redes neurais geométricas e *differentiable rendering*.

---

## Conclusões finais

> A GA deixou de ser curiosidade matemática para se tornar **tecnologia viável e vantajosa** em CG, robótica e visão computacional.

- **Maturidade teórica:** representações compactas e unificadas para uma ampla gama de objetos geométricos.
- **Viabilidade prática:** bibliotecas otimizadas e algoritmos estáveis tornam a GA competitiva com abordagens tradicionais.

---

## Conclusões finais (cont.)

- **Impacto industrial:** aplicações em simulação física, CAD, robótica, visão 3D e cristalografia já são realidade ou estão em estágio avançado.

**Tendência:** a GA se consolidará no *toolkit* de CG à medida que a complexidade geométrica crescer e a demanda por soluções unificadas aumentar.

---

<!-- _class: titulo -->
<!-- _paginate: false -->

# Obrigado!

**Tópicos Especiais em Computação Gráfica**