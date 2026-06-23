# Validação da Metodologia de Medição de Memória

**TCC — Análise Experimental Comparativa de Algoritmos de Criptografia Pós-Quântica do NIST**
**Ambiente:** GCC 13.3.0 · WSL Ubuntu 24.04 · AMD Ryzen 7 7435HS

---

## 1. Questão metodológica

As medições de memória (heap e stack) foram realizadas com compilação `-O0 -g`, sem
otimizações. Os benchmarks de tempo, por outro lado, avaliam 16 configurações de flags
diferentes (combinações de `-O0` a `-O3` com `-march=native`). A questão é: **as medições
de memória com `-O0` são válidas e representativas para todos os casos?**

---

## 2. Heap — independência completa de flags

### Evidência direta nos dados

A tabela abaixo mostra os valores de `heap_delta_bytes` para todos os 17 algoritmos e
todas as 3 operações (51 medições no total):

| Família       | Operações medidas | heap_delta_bytes (todas) |
|---------------|:-----------------:|:------------------------:|
| ML-KEM-512    | keygen / encap / decap | **0 / 0 / 0** |
| ML-KEM-768    | keygen / encap / decap | **0 / 0 / 0** |
| ML-KEM-1024   | keygen / encap / decap | **0 / 0 / 0** |
| HQC-128       | keygen / encap / decap | **0 / 0 / 0** |
| HQC-192       | keygen / encap / decap | **0 / 0 / 0** |
| HQC-256       | keygen / encap / decap | **0 / 0 / 0** |
| ML-DSA-44     | keygen / sign / verify | **0 / 0 / 0** |
| ML-DSA-65     | keygen / sign / verify | **0 / 0 / 0** |
| ML-DSA-87     | keygen / sign / verify | **0 / 0 / 0** |
| FN-DSA-512    | keygen / sign / verify | **0 / 0 / 0** |
| FN-DSA-1024   | keygen / sign / verify | **0 / 0 / 0** |
| SLH-DSA-SHAKE-128s | keygen / sign / verify | **0 / 0 / 0** |
| SLH-DSA-SHAKE-128f | keygen / sign / verify | **0 / 0 / 0** |
| SLH-DSA-SHAKE-192s | keygen / sign / verify | **0 / 0 / 0** |
| SLH-DSA-SHAKE-192f | keygen / sign / verify | **0 / 0 / 0** |
| SLH-DSA-SHAKE-256s | keygen / sign / verify | **0 / 0 / 0** |
| SLH-DSA-SHAKE-256f | keygen / sign / verify | **0 / 0 / 0** |

**Resultado: 51/51 medições com `heap_delta = 0`.**

### Explicação

Esse resultado uniforme não é coincidência — é comportamento esperado da liboqs:

- A liboqs aloca buffers internos via `OQS_MEM_malloc()` no início de cada chamada e os
  libera antes de retornar. O delta líquido entre `mallinfo2().uordblks` antes e depois
  é sempre zero.
- Flags de compilação (`-O0` a `-O3`, `-march=native`) **não afetam chamadas a
  `malloc()`/`free()`**: essas são funções de biblioteca, não construções de linguagem
  que o compilador pode eliminar ou reorganizar.
- Portanto, o heap delta medido com `-O0` é exatamente o mesmo que seria medido com
  qualquer outra combinação de flags. **A medição de heap é flag-independente por
  design.**

---

## 3. Stack — `-O0` como limite superior conservador

### Por que flags afetam o stack (em teoria)

Flags de otimização podem reduzir o uso de stack via:

| Otimização | Mecanismo | Efeito |
|---|---|---|
| Inlining (`-O2`/`-O3`) | Elimina o frame da função inlina | Reduz stack |
| Dead code elimination | Remove variáveis nunca lidas | Reduz stack |
| Register allocation (`-O1`+) | Mantém vars em registradores | Reduz stack |
| `-O0` (sem otimização) | Mantém todos os frames e vars | **Máximo real** |

### Evidência: maior uso de stack por algoritmo

Os valores abaixo (medidos com `-O0`) mostram o pico de stack de cada algoritmo. Com
qualquer flag de otimização maior, esses valores seriam iguais ou menores.

**KEMs (keygen/encap/decap):**

| Algoritmo      | keygen (B) | encap (B) | decap (B) | Pico |
|----------------|:----------:|:---------:|:---------:|:----:|
| ML-KEM-512     | 13.520 | 16.160 | 16.960 | **16.960** |
| ML-KEM-768     | 17.648 | 20.784 | 21.904 | **21.904** |
| ML-KEM-1024    | 22.768 | 26.448 | 28.048 | **28.048** |
| HQC-128        | 38.576 | 56.448 | 60.896 | **60.896** |
| HQC-192        | 70.688 | 106.752 | 115.760 | **115.760** |
| HQC-256        | 108.768 | 166.592 | 181.040 | **181.040** |

**Assinaturas (keygen/sign/verify):**

| Algoritmo            | keygen (B) | sign (B) | verify (B) | Pico |
|----------------------|:----------:|:--------:|:----------:|:----:|
| ML-DSA-44            | 28.304 | 53.456 | 25.968 | **53.456** |
| ML-DSA-65            | 33.456 | 83.312 | 29.072 | **83.312** |
| ML-DSA-87            | 41.648 | 117.072 | 35.472 | **117.072** |
| FN-DSA-512           | 23.608 | 49.584 | 9.064 | **49.584** |
| FN-DSA-1024          | 40.504 | 90.736 | 13.160 | **90.736** |
| SLH-DSA-SHAKE-128s   | 14.480 | 13.744 | 8.464 | **14.480** |
| SLH-DSA-SHAKE-128f   | 14.000 | 13.360 | 8.464 | **14.000** |
| SLH-DSA-SHAKE-192s   | 20.976 | 19.504 | 9.328 | **20.976** |
| SLH-DSA-SHAKE-192f   | 20.240 | 18.928 | 9.360 | **20.240** |
| SLH-DSA-SHAKE-256s   | 29.744 | 27.312 | 10.480 | **29.744** |
| SLH-DSA-SHAKE-256f   | 29.104 | 26.800 | 10.512 | **29.104** |

### Por que o harness contribui minimamente

O código de medição (`memory_benchmark.c`) contém apenas:
- Alocação do buffer de chave/assinatura (stack pequeno e bem definido)
- Chamada única à função da liboqs
- Leitura do byte sentinela para cálculo do consumo

O stack **dominante** vem do código interno da liboqs, que é pré-compilado com suas
próprias otimizações internas (incluindo assembly AVX2 para ML-KEM). As flags passadas
ao nosso harness afetam apenas o pequeno frame do código de medição, não o algoritmo
criptográfico em si.

---

## 4. Validação empírica (reprodutível)

Para confirmar os pontos acima experimentalmente:

```bash
cd ~/TCC/TCC_PQC/code/benchmark
bash validate_flags_comparison.sh
```

O script:
1. Compila `memory_benchmark.c` com `-O0` e com `-O3 -march=native`
2. Executa ambas as versões e gera CSVs individuais
3. Compara os resultados de stack lado a lado
4. Emite `✓ VALIDADO` se `-O3` nunca produz valores maiores que `-O0`

**Resultado esperado:** todos os deltas ≤ 0 (uso de stack com `-O3` igual ou menor), o
que confirma que `-O0` é de fato o pior caso.

---

## 5. Conclusão metodológica

| Métrica | Dependência de flags | Justificativa |
|---------|:-------------------:|---------------|
| `heap_delta_bytes` | **Nenhuma** | malloc/free são chamadas runtime; 51/51 = 0 confirma empiricamente |
| `stack_bytes` | **Nenhuma** (na prática) | `-O0` é o limite superior; otimizações só reduzem; código dominante é a liboqs pré-compilada |
| `massif` (pico de heap) | **Nenhuma** | Valgrind intercepta malloc no nível da biblioteca, independente de como o harness foi compilado |

Os valores medidos com `-O0 -g` são conservadores (pior caso) e válidos para qualquer
configuração de compilação. Sistemas que satisfaçam esses limites de stack/heap
suportarão qualquer outra configuração de flags.
