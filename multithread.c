#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define MAX_ITENS 100000 // M = 10^5 conforme o enunciado

// ==========================================
// VARIÁVEIS GLOBAIS (Memória Compartilhada)
// ==========================================
int *buffer;
int N;                    // Tamanho do buffer
int in = 0;               // Índice de inserção (Produtor)
int out = 0;              // Índice de remoção (Consumidor)
int itens_consumidos = 0; // Contador de itens processados

// Novas variáveis para o histórico de ocupação
int *historico_ocupacao;
int indice_historico = 0;
int ocupacao_atual = 0;   // Quantos itens estão no buffer no momento
int tamanho_max_historico;

// ==========================================
// PRIMITIVAS DE SINCRONIZAÇÃO
// ==========================================
pthread_mutex_t mutex_buffer; 
sem_t sem_empty;              
sem_t sem_full;               

// ==========================================
// FUNÇÕES AUXILIARES
// ==========================================
bool eh_primo(int n) {
    if (n <= 1) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

// ==========================================
// THREAD: PRODUTOR
// ==========================================
void* produtor(void* arg) {
    int id = *((int*)arg);
    unsigned int seed = time(NULL) ^ (pthread_self()); 

    while (1) {
        // Aguarda espaço livre
        sem_wait(&sem_empty);
        
        pthread_mutex_lock(&mutex_buffer);
        
        // Se a meta foi atingida, aborta
        if (itens_consumidos >= MAX_ITENS) {
            pthread_mutex_unlock(&mutex_buffer);
            sem_post(&sem_empty); 
            sem_post(&sem_full);
            break;
        }
        
        // Produz e insere no buffer
        int numero = (rand_r(&seed) % 10000000) + 1;
        buffer[in] = numero;
        in = (in + 1) % N;
        
        // Atualiza a ocupação e salva no histórico
        ocupacao_atual++;
        if (indice_historico < tamanho_max_historico) {
            historico_ocupacao[indice_historico++] = ocupacao_atual;
        }
        
        pthread_mutex_unlock(&mutex_buffer);
        
        // Sinaliza novo item
        sem_post(&sem_full);
    }
    return NULL;
}

// ==========================================
// THREAD: CONSUMIDOR
// ==========================================
void* consumidor(void* arg) {
    int id = *((int*)arg);

    while (1) {
        // Aguarda ter item no buffer
        sem_wait(&sem_full);
        
        pthread_mutex_lock(&mutex_buffer);
        
        if (itens_consumidos >= MAX_ITENS) {
            pthread_mutex_unlock(&mutex_buffer);
            sem_post(&sem_empty);
            sem_post(&sem_full);
            break;
        }
        
        // Consome item do buffer
        int numero = buffer[out];
        out = (out + 1) % N;
        
        itens_consumidos++;
        int count_atual = itens_consumidos;
        
        // Atualiza a ocupação e salva no histórico
        ocupacao_atual--;
        if (indice_historico < tamanho_max_historico) {
            historico_ocupacao[indice_historico++] = ocupacao_atual;
        }
        
        pthread_mutex_unlock(&mutex_buffer);
        
        sem_post(&sem_empty);
        
        // Trabalho fora da região crítica
        bool primo = eh_primo(numero);
        // printf("[Consumidor %d] Item %d | Numero: %d -> %s\n", id, count_atual, numero, primo ? "EH PRIMO" : "nao eh primo");
        
        if (count_atual >= MAX_ITENS) {
            // Desperta as outras threads para encerramento
            for (int i = 0; i < N + 10; i++) {
                sem_post(&sem_empty);
                sem_post(&sem_full);
            }
            break;
        }
    }
    return NULL;
}

// ==========================================
// MAIN
// ==========================================
int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <tamanho_buffer_N> <num_produtores_Np> <num_consumidores_Nc>\n", argv[0]);
        return EXIT_FAILURE;
    }

    N = atoi(argv[1]);
    int Np = atoi(argv[2]);
    int Nc = atoi(argv[3]);

    if (N <= 0 || Np <= 0 || Nc <= 0) {
        fprintf(stderr, "Erro: Todos os parametros devem ser maiores que 0.\n");
        return EXIT_FAILURE;
    }

    // 1. Alocações de Memória
    buffer = (int *)malloc(N * sizeof(int));
    
    // O histórico precisa suportar M consumos + M produções + sobra para margem de erro das threads
    tamanho_max_historico = (MAX_ITENS * 2) + N + 1000;
    historico_ocupacao = (int *)malloc(tamanho_max_historico * sizeof(int));
    
    if (buffer == NULL || historico_ocupacao == NULL) {
        perror("Erro ao alocar memoria");
        return EXIT_FAILURE;
    }

    // 2. Inicialização
    pthread_mutex_init(&mutex_buffer, NULL);
    sem_init(&sem_empty, 0, N); 
    sem_init(&sem_full, 0, 0);  

    // 3. Criação das Threads
    pthread_t threads_prod[Np];
    pthread_t threads_cons[Nc];
    int id_prod[Np], id_cons[Nc];

    for (int i = 0; i < Np; i++) {
        id_prod[i] = i + 1;
        pthread_create(&threads_prod[i], NULL, produtor, &id_prod[i]);
    }
    for (int i = 0; i < Nc; i++) {
        id_cons[i] = i + 1;
        pthread_create(&threads_cons[i], NULL, consumidor, &id_cons[i]);
    }

    // 4. Join nas Threads
    for (int i = 0; i < Np; i++) pthread_join(threads_prod[i], NULL);
    for (int i = 0; i < Nc; i++) pthread_join(threads_cons[i], NULL);

    // ========================================================
    // 5. IMPRESSÃO DO HISTÓRICO DE OCUPAÇÃO AO FINAL
    // ========================================================
    printf("\n\n=== Historico de Ocupacao do Buffer ===\n");
    printf("Total de operacoes registradas (Producoes + Consumos): %d\n\n", indice_historico);
    
    // Imprime todos os valores separados por espaço
    for (int i = 0; i < indice_historico; i++) {
        printf("%d ", historico_ocupacao[i]);
    }
    printf("\n=========================================\n");
    printf("Programa concluido com sucesso.\n");

    // 6. Limpeza
    free(buffer);
    free(historico_ocupacao);
    pthread_mutex_destroy(&mutex_buffer);
    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);

    return EXIT_SUCCESS;
}