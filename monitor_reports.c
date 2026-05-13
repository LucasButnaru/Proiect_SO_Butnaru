#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig) {
    keep_running = 0;
    // Notificam finalizarea
    write(STDOUT_FILENO, "END: Monitorul se inchide.\n", 27);
}

void handle_sigusr1(int sig) {
    write(STDOUT_FILENO, "MSG: Raport nou adaugat in sistem!\n", 35);
}

int main() {
    // Verificam daca ruleaza deja un monitor
    int check_fd = open(".monitor_pid", O_RDONLY);
    if (check_fd >= 0) {
        char pid_buf[32] = {0};
        read(check_fd, pid_buf, sizeof(pid_buf) - 1);
        printf("ERR: Un monitor ruleaza deja cu PID-ul %s", pid_buf); // Fara \n ca sa nu se dubleze
        fflush(stdout); // Obligatoriu pentru a trimite instant pe pipe
        close(check_fd);
        return 1;
    }

    struct sigaction sa_int;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    struct sigaction sa_usr1;
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    int fd = open(".monitor_pid", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
        char pid_str[32];
        snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
        write(fd, pid_str, strlen(pid_str));
        close(fd);
    }

    printf("START: Monitor pornit (PID %d)\n", getpid());
    fflush(stdout);

    while (keep_running) {
        pause();
    }

    unlink(".monitor_pid");
    return 0;
}






