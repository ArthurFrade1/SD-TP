#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

// Handler para o primeiro sinal (SIGUSR1)
void trata_sinal_1(int sig) {
    printf("\n[Handler] Sinal %d (SIGUSR1) interceptado! O programa continua executando.\n", sig);
}

// Handler para o segundo sinal (SIGUSR2)
void trata_sinal_2(int sig) {
    printf("\n[Handler] Sinal %d (SIGUSR2) interceptado! O programa continua executando.\n", sig);
}

// Handler para o terceiro sinal (SIGTERM) - Este irá encerrar o programa
void trata_sinal_3(int sig) {
    printf("\n[Handler] Sinal %d (SIGTERM) interceptado! Encerrando o processo de forma segura...\n", sig);
    exit(0);
}

int main(int argc, char *argv[]) {
    // Verifica os argumentos de linha de comando
    if (argc != 2 || (strcmp(argv[1], "busy") != 0 && strcmp(argv[1], "blocking") != 0)) {
        fprintf(stderr, "Uso: %s <busy|blocking>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Registra os signal handlers para capturar os sinais escolhidos
    signal(SIGUSR1, trata_sinal_1);
    signal(SIGUSR2, trata_sinal_2);
    signal(SIGTERM, trata_sinal_3);

    printf("==== Receptor de Sinais Iniciado ====\n");
    printf("Meu PID para receber sinais e: %d\n", getpid());
    printf("Modo de espera configurado : %s\n", argv[1]);
    printf("-------------------------------------\n");
    printf("Sinais suportados:\n");
    printf("  %d -> SIGUSR1 (Apenas avisa)\n", SIGUSR1);
    printf("  %d -> SIGUSR2 (Apenas avisa)\n", SIGUSR2);
    printf("  %d -> SIGTERM (Encerra o programa)\n\n", SIGTERM);

    // Lógica de espera
    if (strcmp(argv[1], "busy") == 0) {
        printf("Iniciando Busy Wait (isso vai consumir 100%% de uma thread da CPU)...\n");
        // Busy wait: O loop fica girando infinitamente sem liberar a CPU
        while (1) {
            // Não faz nada, apenas queima ciclo de clock
        }
    } else {
        printf("Iniciando Blocking Wait (o processo vai dormir e nao consumira CPU)...\n");
        // Blocking wait: O processo é suspenso até que um sinal chegue
        while (1) {
            pause(); // Aguarda a entrega de um sinal
        }
    }

    return EXIT_SUCCESS;
}