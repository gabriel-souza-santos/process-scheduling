# Simulador de Escalonamento de Processos
 
Simulador de escalonamento de processos desenvolvido para o projeto da Unidade 3 de Sistemas Operacionais. Gera cargas de trabalho controladas por seed, executa algoritmos clássicos de escalonamento (FCFS, Round Robin, Prioridade) e um algoritmo próprio, e compara os resultados com base em métricas quantitativas (turnaround médio, trocas de contexto e índice de Jain sobre o slowdown).
 
## Estrutura do repositório
 
```
.
├── config    # arquivos JSON de configuração dos cenários de simulação
├── data      # resultados gerados pela simulação (JSON)
├── docs      # documentação do projeto (modelagem de E/S, troca de contexto, algoritmo próprio etc.)
├── include   # cabeçalhos (.h)
├── scripts   # scripts em Python para geração de gráficos a partir de data/
└── src       # arquivos fonte (.c)
```
 
## Requisitos
 
- CMake >= 3.16
- Compilador C (GCC ou Clang)
- Python >= 3.10 (para os scripts de `scripts/`)
- Dependências Python: `pandas`, `matplotlib`, `numpy`, `scipy` (temporário, pode mudar)
```bash
  pip install -r scripts/requirements.txt
```
 
## Como compilar e executar
 
Recomenda-se criar uma pasta `build` para compilação:
 
```bash
cmake -S . -B build
cmake --build build
./build/bin/process_scheduling
```
