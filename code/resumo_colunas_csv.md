# Descrição das Colunas dos CSVs de Memória

## `memory_results_tcc.csv`

Gerado pelo programa `memory_benchmark.c`. Cada linha representa uma operação de um algoritmo, medida via **stack painting** e **mallinfo2**.

| Coluna | O que representa |
|---|---|
| `algoritmo` | Nome do algoritmo no padrão NIST (ex: `FN-DSA-512`, `SLH-DSA-SHAKE-128s`) |
| `nome_liboqs` | Nome interno usado pela liboqs (ex: `Falcon-512`, `SPHINCS+-SHAKE-128s-simple`) |
| `operacao` | Operação executada: `keygen`, `encap`/`sign` ou `decap`/`verify` |
| `heap_delta_bytes` | Diferença de heap antes e depois da operação, medida via `mallinfo2()`. Sempre zero — a liboqs aloca e libera todas as estruturas dentro de cada chamada, sem deixar alocações persistentes |
| `stack_bytes` | **Coluna principal.** Uso real de pilha medido pela técnica de *stack painting*: bytes efetivamente utilizados do buffer sentinela de 4 MB alocado como pilha da thread POSIX |
| `rss_before_kb` | Memória residente do processo (RSS) antes da operação, lida em `/proc/self/status` (KB) |
| `rss_after_kb` | Memória residente do processo (RSS) após a operação (KB) |
| `rss_delta_kb` | Diferença de RSS (depois − antes). Também zero na prática, pois o buffer de stack já está mapeado antes da medição |
| `sucesso` | `1` se a operação criptográfica foi executada com sucesso, `0` se houve falha |

### Como a medição de stack funciona

1. Um buffer de 4 MB é preenchido com o byte sentinela `0xAB`.
2. Uma thread POSIX é criada apontando esse buffer como sua pilha (`pthread_attr_setstack`).
3. A operação criptográfica é executada dentro da thread.
4. Após o retorno, o buffer é varrido do endereço mais baixo para o mais alto.
5. O primeiro byte diferente de `0xAB` delimita o limite de uso; o consumo real é calculado como o complemento em relação ao tamanho total do buffer.

---

## `massif_results_heap_tcc.csv`

Gerado pelo script `parse_massif.py` a partir dos arquivos `.massif` produzidos pelo **Valgrind**. Cada linha representa o pico de heap de uma operação de um algoritmo, medido com `valgrind --tool=massif`.

| Coluna | O que representa |
|---|---|
| `algoritmo` | Nome do algoritmo no padrão NIST (ex: `ML-KEM-512`, `SLH-DSA-SHAKE-256f`) |
| `nome_liboqs` | Nome interno usado pela liboqs (ex: `ML-KEM-512`, `SPHINCS+-SHAKE-256f-simple`) |
| `operacao` | Operação executada: `keygen`, `encap`/`sign` ou `decap`/`verify` |
| `heap_peak_bytes` | **Coluna principal.** Pico de heap em bytes durante a execução, extraído do campo `mem_heap_B` do arquivo `.massif` gerado pelo Valgrind |
| `heap_peak_kb` | Mesmo valor convertido para KB (÷ 1024) |
| `heap_peak_mb` | Mesmo valor convertido para MB (÷ 1.048.576) |
| `total_peak_bytes` | Pico total de memória incluindo overhead de inicialização do Valgrind e da liboqs |
| `total_peak_kb` | Mesmo valor em KB |

### Como a medição de heap funciona

1. O programa auxiliar `massif_single.c` executa **exatamente uma chamada** de uma operação por invocação (sem laços), para que o Valgrind perfile o heap daquela operação isolada.
2. O script `run_massif_all.sh` automatiza a execução para todos os algoritmos e operações, salvando um arquivo `.massif` por combinação.
3. O script `parse_massif.py` lê cada `.massif`, extrai o valor de pico de `mem_heap_B` e consolida tudo no CSV.

> **Nota:** o delta líquido de heap é sempre zero porque a liboqs libera todas as alocações transitórias ao término de cada operação. O Valgrind massif captura o **pico durante a execução**, que é o valor relevante para análise.

---

## Observação sobre nomes de algoritmos

A versão do liboqs utilizada ainda adota nomes internos pré-padronização. Ambos os programas incluem uma tabela de mapeamento que traduz os nomes FIPS/NIST para os identificadores aceitos pela API:

| Nome NIST (TCC) | Nome liboqs |
|---|---|
| FN-DSA-512 | Falcon-512 |
| FN-DSA-1024 | Falcon-1024 |
| SLH-DSA-SHAKE-128s | SPHINCS+-SHAKE-128s-simple |
| SLH-DSA-SHAKE-128f | SPHINCS+-SHAKE-128f-simple |
| SLH-DSA-SHAKE-192s | SPHINCS+-SHAKE-192s-simple |
| SLH-DSA-SHAKE-192f | SPHINCS+-SHAKE-192f-simple |
| SLH-DSA-SHAKE-256s | SPHINCS+-SHAKE-256s-simple |
| SLH-DSA-SHAKE-256f | SPHINCS+-SHAKE-256f-simple |
| ML-KEM-*, ML-DSA-*, HQC-* | (inalterado) |
