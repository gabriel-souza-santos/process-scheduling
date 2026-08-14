#include "simulation.h"

static void process_arrivals(Simulation *simulation);
static void process_cpu(Simulation *simulation);
static void process_io(Simulation *simulation);
static void schedule_process(Simulation *simulation);

void simulation_init(Simulation *simulation, Process *processes,
                     unsigned int total_processes) {
  simulation->current_time = 0;
  simulation->total_processes = total_processes;
  simulation->finished_processes = 0;
  simulation->processes = processes;

  /*
   * A inicialização da CPU e da fila de prontos
   * deve ser feita pelos respectivos módulos.
   */
}

void simulation_run(Simulation *simulation) {
  while (!simulation_finished(simulation)) {
    simulation_tick(simulation);
  }
}

void simulation_tick(Simulation *simulation) {
  /*
   * 1. Coloca na fila os processos
   *    que chegaram neste instante.
   */
  process_arrivals(simulation);

  /*
   * 2. Atualiza os processos bloqueados
   *    esperando por E/S.
   */
  process_io(simulation);

  /*
   * 3. Executa o processo atualmente
   *    presente na CPU.
   */
  process_cpu(simulation);

  /*
   * 4. Caso a CPU esteja livre, escolhe
   *    um processo da fila de prontos.
   */
  schedule_process(simulation);

  /*
   * 5. Avança o relógio da simulação.
   */
  simulation->current_time++;
}

int simulation_finished(const Simulation *simulation) {
  return simulation->finished_processes >= simulation->total_processes;
}

/*
 * Verifica processos que chegaram no instante atual
 * e os coloca na fila de prontos.
 */
static void process_arrivals(Simulation *simulation) {
  for (unsigned int i = 0; i < simulation->total_processes; i++) {

    Process *process = &simulation->processes[i];

    if (process->arrival_time != simulation->current_time)
      continue;

    /*
     * O processo acabou de chegar.
     * Sua entrada na fila será feita pelo
     * módulo ready_queue.
     */
    ready_queue_push(&simulation->ready_queue, process);
  }
}

/*
 * Atualiza processos que estão realizando E/S.
 */
static void process_io(Simulation *simulation) {
  /*
   * A implementação depende de como o projeto
   * representa os processos bloqueados e as
   * operações de E/S.
   *
   * O motor apenas reserva esta etapa no ciclo.
   */
}

/*
 * Executa uma unidade de tempo do processo
 * atualmente na CPU.
 */
static void process_cpu(Simulation *simulation) {
  /*
   * A implementação depende da estrutura CPU
   * e do modelo de rajadas definido pelo projeto.
   */
}

/*
 * Escolhe o próximo processo quando a CPU
 * estiver disponível.
 */
static void schedule_process(Simulation *simulation) {
  /*
   * A política FCFS, Prioridade ou Round Robin
   * pertence ao módulo da fila/escalonador.
   *
   * O motor apenas solicita o próximo processo.
   */

  if (simulation->cpu.running != NULL)
    return;

  if (ready_queue_is_empty(&simulation->ready_queue))
    return;

  simulation->cpu.running = ready_queue_pop(&simulation->ready_queue);
}
