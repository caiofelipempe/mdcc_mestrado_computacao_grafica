# Build Manager Script

Script Python para gerenciar o ciclo de compilação, configuração e limpeza do projeto CMake com integração ao vcpkg.

## Pré-requisitos

- Python 3.7+
- CMake
- Compilador C/C++ (GCC, Clang ou MSVC)

## Uso

### Comandos Principais

# Configurar o projeto (Release por padrão)
python build.py configure

# Compilar o projeto (executa configure automaticamente se necessário)
python build.py build

# Limpar e reconfigurar/recompilar do zero
python build.py rebuild

# Remover completamente o diretório de build
python build.py clean-all

### Opções de Compilação

# Configurar em modo Debug
python build.py configure --build-type Debug

# Outros tipos suportados: Release, RelWithDebInfo, MinSizeRel
python build.py configure --build-type RelWithDebInfo

## Exemplos Práticos

# Fluxo inicial de compilação
python build.py build

# Reconstrução completa (rebuild) em Debug
python build.py rebuild --build-type Debug

# Limpeza total do ambiente de build
python build.py clean-all

## Parâmetros da CLI

- command: configure, build, rebuild, clean-all (Obrigatório) - Ação principal do ciclo de build
- --build-type: Debug, Release, RelWithDebInfo, MinSizeRel (Padrão: Release) - Tipo da build gerada pelo CMake

## Estrutura de Diretórios

Após a execução, a seguinte estrutura é gerada automaticamente:

projeto/
├── build/             # Diretório de artefatos (gerado automaticamente)
├── CMakeLists.txt
├── build.py           # Script de gerenciamento
└── ...