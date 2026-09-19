#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>

#define BUFFER_SIZE 20

// Função auxiliar para verificar se um número é primo
bool eh_primo(int n) {
    if (n <= 1) return false;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    // 1. Validação do parâmetro de entrada (quantidade de números)
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <quantidade_de_numeros>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int total_numeros = atoi(argv[1]);

    // 2. Criação do Anonymous Pipe
    // fd[0] = ponta de leitura (read)
    // fd[1] = ponta de escrita (write)
    int fd[2];
    if (pipe(fd) == -1) {
        perror("Erro ao criar o pipe");
        return EXIT_FAILURE;
    }

    // 3. Criação do Processo Filho via fork()
    pid_t pid = fork();
    if (pid < 0) {
        perror("Erro no fork");
        return EXIT_FAILURE;
    }

    // ==========================================
    // CAMINHO DO FILHO (Consumidor)
    // ==========================================
    if (pid == 0) {
        // O filho só vai ler, então fechamos a ponta de escrita que ele herdou
        close(fd[1]); 

        char buffer[BUFFER_SIZE];

        while (1) {
            // Lê exatamente 20 bytes do pipe
            ssize_t bytes_lidos = read(fd[0], buffer, BUFFER_SIZE);
            if (bytes_lidos <= 0) break;

            // Converte a string recebida de volta para inteiro
            int numero = atoi(buffer);

            // Condição de parada: se receber 0, o consumidor encerra
            if (numero == 0) {
                printf("[Consumidor] Recebeu 0. Encerrando...\n");
                break;
            }

            // Verifica se é primo e imprime o resultado
            if (eh_primo(numero)) {
                printf("[Consumidor] O número %d É PRIMO!\n", numero);
            } else {
                printf("[Consumidor] O número %d não é primo.\n", numero);
            }
        }

        close(fd[0]); // Fecha a ponta de leitura ao terminar
        exit(EXIT_SUCCESS);
    }

    // ==========================================
    // CAMINHO DO PAI (Produtor)
    // ==========================================
    else {
        // O pai só vai escrever, então fechamos a ponta de leitura que o filho vai usar
        close(fd[0]); 

        srand(time(NULL)); // Semente para números aleatórios
        int n_atual = 1;   // N0 = 1 conforme o enunciado

        // Envia o primeiro número (N0)
        char buffer[BUFFER_SIZE];
        snprintf(buffer, BUFFER_SIZE, "%d", n_atual);
        write(fd[1], buffer, BUFFER_SIZE);

        // Gera e envia os próximos números
        for (int i = 1; i < total_numeros; i++) {
            int delta = (rand() % 100) + 1; // Delta entre 1 e 100
            n_atual = n_atual + delta;      // Ni = Ni-1 + delta

            snprintf(buffer, BUFFER_SIZE, "%d", n_atual);
            write(fd[1], buffer, BUFFER_SIZE);
        }

        // Após enviar a quantidade pedida, envia o 0 para sinalizar o fim
        snprintf(buffer, BUFFER_SIZE, "0");
        write(fd[1], buffer, BUFFER_SIZE);
        printf("[Produtor] Todos os números foram enviados. Encerrando...\n");

        close(fd[1]); // Fecha a ponta de escrita
        wait(NULL);   // Espera o processo filho terminar antes de fechar o programa de vez
    }

    return EXIT_SUCCESS;
}