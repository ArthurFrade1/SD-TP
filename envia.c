#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    // Verifica se os parâmetros foram passados corretamente
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <PID_DESTINO> <NUMERO_DO_SINAL>\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t pid = atoi(argv[1]);
    int sig = atoi(argv[2]);

    // O sinal 0 é um sinal especial. Ele não é enviado, mas o sistema 
    // realiza a checagem de erros. Serve para verificar se o processo existe.
    if (kill(pid, 0) == -1) {
        if (errno == ESRCH) {
            fprintf(stderr, "Erro: O processo %d não existe.\n", pid);
        } else if (errno == EPERM) {
            fprintf(stderr, "Erro: Sem permissão para enviar sinal ao processo %d.\n", pid);
        } else {
            perror("Erro ao verificar o processo");
        }
        return EXIT_FAILURE;
    }

    // Se passou da verificação, envia o sinal real
    if (kill(pid, sig) == -1) {
        perror("Erro ao enviar o sinal");
        return EXIT_FAILURE;
    }

    printf("Sucesso: Sinal %d enviado para o processo %d.\n", sig, pid);
    return EXIT_SUCCESS;
}