#!/usr/bin/env python3
"""
fix_paragraphs.py
Adiciona uma linha em branco extra ao final de cada parágrafo no .tex.
Em LaTeX, múltiplas linhas em branco consecutivas são equivalentes a uma,
então isso é seguro mesmo dentro de ambientes como table/figure.

Uso:
    python3 fix_paragraphs.py          # processa tcc_pqc.tex no mesmo diretório
    python3 fix_paragraphs.py outro.tex
"""

import re
import sys
from pathlib import Path

def fix(filepath: Path) -> None:
    content = filepath.read_text(encoding='utf-8')

    # 1. Normaliza: reduz 3+ newlines consecutivos para exatamente 2
    #    (evita acumular caso o script rode mais de uma vez)
    content = re.sub(r'\n{3,}', '\n\n', content)

    # 2. Substitui cada quebra de parágrafo (linha em branco) por duas linhas em branco
    content = content.replace('\n\n', '\n\n\n')

    filepath.write_text(content, encoding='utf-8')
    print(f"OK: {filepath.name} — {content.count(chr(10))} linhas no total.")

if __name__ == '__main__':
    target = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent / 'tcc_pqc.tex'
    if not target.exists():
        print(f"ERRO: arquivo não encontrado: {target}")
        sys.exit(1)
    fix(target)
