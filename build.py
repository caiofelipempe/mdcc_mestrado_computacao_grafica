#!/usr/bin/env python3
"""
Script de automação para configuração, build e limpeza de projetos CMake com suporte a vcpkg.
"""

import os
import sys
import subprocess
import argparse
import shutil
import platform
from pathlib import Path
from typing import List, Optional


class BuildManager:
    def __init__(self, project_root: Optional[Path] = None, build_type: str = "Release"):
        self.project_root = Path(project_root or os.getcwd()).resolve()
        self.build_dir = self.project_root / "build"
        self.build_type = build_type

    def _check_cmake_installed(self) -> bool:
        """Verifica se o CMake está disponível no PATH."""
        if shutil.which("cmake") is None:
            print("❌ Erro: 'cmake' não foi encontrado no PATH do sistema.")
            return False
        return True

    def run_command(self, cmd: List[str], cwd: Optional[Path] = None) -> bool:
        """Executa um comando no terminal com logs formatados."""
        cwd = cwd or self.project_root
        print(f"\n📁 Diretório: {cwd}")
        print(f"🔄 Executando: {' '.join(cmd)}\n")

        try:
            subprocess.run(cmd, cwd=cwd, check=True)
            return True
        except subprocess.CalledProcessError as e:
            print(f"❌ Erro ao executar comando (Código {e.returncode}): {e}")
            return False
        except Exception as e:
            print(f"❌ Falha inesperada: {e}")
            return False

    def _get_vcpkg_path(self) -> Optional[Path]:
        """Resolve o caminho das dependências instaladas pelo vcpkg com base no SO."""
        base_vcpkg = self.project_root / "vcpkg_installed"
        if not base_vcpkg.exists():
            return None

        os_map = {
            "Linux": "x64-linux",
            "Windows": "x64-windows",
            "Darwin": "x64-osx"
        }
        target_triplet = os_map.get(platform.system())
        
        if target_triplet:
            triplet_path = base_vcpkg / target_triplet
            if triplet_path.exists():
                return triplet_path

        return base_vcpkg

    def configure(self) -> bool:
        """Configura o projeto executando a geração do CMake."""
        if not self._check_cmake_installed():
            return False

        self.build_dir.mkdir(parents=True, exist_ok=True)
        print(f"✅ Diretório de build garantido em: {self.build_dir}")

        cmake_args = [
            "cmake",
            "-B", str(self.build_dir),
            "-S", str(self.project_root),
            f"-DCMAKE_BUILD_TYPE={self.build_type}",
        ]

        vcpkg_path = self._get_vcpkg_path()
        if vcpkg_path:
            cmake_args.append(f"-DCMAKE_PREFIX_PATH={vcpkg_path}")

        return self.run_command(cmake_args)

    def build(self) -> bool:
        """Compila o projeto configurado usando múltiplos núcleos."""
        if not self._check_cmake_installed():
            return False

        if not self.build_dir.exists():
            print("⚠️  Diretório de build não encontrado. Executando configuração primeiro...")
            if not self.configure():
                return False

        cpu_jobs = str(os.cpu_count() or 4)
        cmd = [
            "cmake",
            "--build", str(self.build_dir),
            "--config", self.build_type,
            "-j", cpu_jobs
        ]
        return self.run_command(cmd)

    def clean_all(self) -> bool:
        """Remove completamente o diretório de build."""
        if self.build_dir.exists():
            print(f"🗑️  Removendo diretório de build: {self.build_dir}")
            try:
                shutil.rmtree(self.build_dir)
                print("✅ Diretório de build removido com sucesso.")
            except OSError as e:
                print(f"❌ Erro ao remover diretório: {e}")
                return False
        else:
            print("✅ O diretório de build já não existe.")
        return True

    def rebuild(self) -> bool:
        """Realiza limpa completa, reconfiguração e compilação do projeto."""
        print("🔄 Iniciando reconstrução total (rebuild)...")
        if self.clean_all() and self.configure():
            return self.build()
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Gerenciador de Build para Projetos C/C++ com CMake",
        formatter_class=argparse.RawTextHelpFormatter,
        epilog="""\
Exemplos de uso:
  python build.py configure                     # Configura o projeto em modo Release
  python build.py build                         # Compila o projeto (configura se necessário)
  python build.py rebuild                       # Limpa a pasta build, reconfigura e compila
  python build.py clean-all                     # Deleta o diretório de build
  python build.py configure --build-type Debug  # Configura o projeto em modo Debug
"""
    )

    parser.add_argument(
        "command",
        choices=["configure", "build", "clean-all", "rebuild"],
        help="Comando a ser executado no ciclo de build"
    )

    parser.add_argument(
        "--build-type",
        choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"],
        default="Release",
        help="Tipo de build do CMake. Padrão: Release"
    )

    args = parser.parse_args()

    manager = BuildManager(build_type=args.build_type)

    commands_map = {
        "configure": manager.configure,
        "build": manager.build,
        "clean-all": manager.clean_all,
        "rebuild": manager.rebuild,
    }

    action = commands_map.get(args.command)
    success = action() if action else False

    if success:
        print("\n✨ Operação concluída com sucesso!")
        sys.exit(0)
    else:
        print("\n❌ Falha na execução da operação.")
        sys.exit(1)


if __name__ == "__main__":
    main()