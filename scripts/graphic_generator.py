"""
Le todos os arquivos data/results-*.json e gera graficos de barras, 
comparando os algoritmos de escalonamento em cada cenario.
Os graficos sao salvos em graphs/<metrica>.png.
"""


import glob
import json
import math
import os

import matplotlib.pyplot as plt
import numpy as np

DATA_DIR = "data"
OUTPUT_DIR = "data"


# Rotulos para os cenarios.
# Se um cenario nao estiver, o nome bruto do JSON e usado.
SCENARIO_LABELS = {
    "cenario_1_aleatorio_equilibrado": "Aleatório\nequilibrado",
    "cenario_2_io_bound": "I/O-bound",
    "cenario_3_cpu_bound": "CPU-bound",
    "cenario_4_prioridades_desbalanceadas": "Prioridades\ndesbalanceadas",
}


# Metricas que serao plotadas
METRICS = {
    "turnaround": ("Turnaround Médio", "unidades de tempo"),
    "slowdown": ("Slowdown Médio", "razão (adimensional)"),
    "context_switches": ("Trocas de Contexto", "quantidade"),
    "total_time": ("Tempo Total de Simulação", "unidades de tempo"),
    "jain_index": ("Índice de Jain do Slowdown", "0 a 1"),
}


# Uma cor fixa por algoritmo deixa os graficos consistentes entre si.
ALGORITHM_COLORS = {
    "FCFS": "#121AAC",
    "Round Robin": "#BF0ADF",
    "Priority": "#009E64",
    "Personalizado": "#CDCD01",
}




def ci95(mean, std, n):
    """Calcula a margem do Intervalo de Confianca de 95%."""
    if n <= 1:
        return 0.0
    return 1.96 * std / math.sqrt(n)


def load_all_results(data_dir=DATA_DIR):
    """
    Carrega todos os data/results-*.json e retorna um dicionario
        { nome_do_algoritmo: {cenario: {...}, ...}, ... }
    """
    results = {}
    pattern = os.path.join(data_dir, "results-*.json")
    for filepath in sorted(glob.glob(pattern)):
        with open(filepath, encoding="utf-8") as f:
            payload = json.load(f)
        algorithm_name = payload["algorithm"]
        results[algorithm_name] = payload["scenarios"]
    return results


def get_scenario_order(results):
    """
    Usa a ordem de cenarios do primeiro algoritmo carregado como
    referencia, mas so mantem cenarios presentes em todos os
    algoritmos (para a comparacao ficar justa).
    """
    algorithms = list(results.keys())
    if not algorithms:
        return []


    scenario_sets = [set(results[algo].keys()) for algo in algorithms]
    common_scenarios = set.intersection(*scenario_sets)


    first_algo_order = list(results[algorithms[0]].keys())
    return [s for s in first_algo_order if s in common_scenarios]




def plot_metric(results, metric_key, scenario_order, output_dir=OUTPUT_DIR):
    metric_name, unit = METRICS[metric_key]
    algorithms = list(results.keys())
    n_algorithms = len(algorithms)
    n_scenarios = len(scenario_order)


    x = np.arange(n_scenarios)
    width = 0.8 / max(n_algorithms, 1)


    fig, ax = plt.subplots(figsize=(10, 6))


    for i, algorithm in enumerate(algorithms):
        means, errors = [], []
        for scenario in scenario_order:
            scenario_data = results[algorithm][scenario]
            metric_data = scenario_data["metrics"][metric_key]
            n_seeds = scenario_data["num_seeds"]


            means.append(metric_data["mean"])
            errors.append(ci95(metric_data["mean"], metric_data["std"], n_seeds))


        offset = (i - (n_algorithms - 1) / 2) * width
        color = ALGORITHM_COLORS.get(algorithm)
        ax.bar(
            x + offset,
            means,
            width,
            yerr=errors,
            capsize=4,
            label=algorithm,
            color=color,
        )


    ax.set_xticks(x)
    ax.set_xticklabels([SCENARIO_LABELS.get(s, s) for s in scenario_order])
    ax.set_ylabel(f"{metric_name} ({unit})")
    ax.set_title(f"{metric_name} por cenário e algoritmo (média ± IC95%)")
    ax.legend(title="Algoritmo")
    ax.grid(axis="y", linestyle="--", alpha=0.4)
    ax.set_axisbelow(True)
    fig.tight_layout()


    os.makedirs(output_dir, exist_ok=True)
    out_path = os.path.join(output_dir, f"{metric_key}.png")
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"[ok] gráfico salvo em: {out_path}")




def plot_all_metrics_grid(results, scenario_order, output_dir=OUTPUT_DIR):
    """
    Gera tambem uma figura unica com todas as metricas lado a lado,
    util para colocar no relatorio/slides.
    """
    metric_keys = list(METRICS.keys())
    n_metrics = len(metric_keys)
    n_cols = 2
    n_rows = math.ceil(n_metrics / n_cols)


    fig, axes = plt.subplots(n_rows, n_cols, figsize=(14, 5 * n_rows))
    axes = np.array(axes).reshape(-1)


    algorithms = list(results.keys())
    n_algorithms = len(algorithms)
    x = np.arange(len(scenario_order))
    width = 0.8 / max(n_algorithms, 1)


    for m_idx, metric_key in enumerate(metric_keys):
        ax = axes[m_idx]
        metric_name, unit = METRICS[metric_key]


        for i, algorithm in enumerate(algorithms):
            means, errors = [], []
            for scenario in scenario_order:
                scenario_data = results[algorithm][scenario]
                metric_data = scenario_data["metrics"][metric_key]
                n_seeds = scenario_data["num_seeds"]
                means.append(metric_data["mean"])
                errors.append(ci95(metric_data["mean"], metric_data["std"], n_seeds))


            offset = (i - (n_algorithms - 1) / 2) * width
            color = ALGORITHM_COLORS.get(algorithm)
            ax.bar(x + offset, means, width, yerr=errors, capsize=3,
                   label=algorithm, color=color)


        ax.set_xticks(x)
        ax.set_xticklabels([SCENARIO_LABELS.get(s, s) for s in scenario_order],
                            fontsize=8)
        ax.set_ylabel(unit, fontsize=9)
        ax.set_title(metric_name, fontsize=11)
        ax.grid(axis="y", linestyle="--", alpha=0.4)
        ax.set_axisbelow(True)


    # remove eixos vazios (caso n_metrics seja impar)
    for j in range(n_metrics, len(axes)):
        fig.delaxes(axes[j])


    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper center", ncol=n_algorithms,
               bbox_to_anchor=(0.5, 1.02), title="Algoritmo")
    fig.tight_layout()


    os.makedirs(output_dir, exist_ok=True)
    out_path = os.path.join(output_dir, "todas_as_metricas.png")
    fig.savefig(out_path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"[ok] gráfico salvo em: {out_path}")




# Main para testar os gráficos
def main():
    results = load_all_results()
    if not results:
        print(f"Nenhum arquivo 'results-*.json' encontrado em '{DATA_DIR}/'.")
        return


    scenario_order = get_scenario_order(results)
    if not scenario_order:
        print("Os algoritmos carregados não têm cenários em comum.")
        return


    print(f"Algoritmos carregados: {', '.join(results.keys())}")
    print(f"Cenários em comum: {', '.join(scenario_order)}\n")


    for metric_key in METRICS:
        plot_metric(results, metric_key, scenario_order)


    plot_all_metrics_grid(results, scenario_order)




if __name__ == "__main__":
    main()