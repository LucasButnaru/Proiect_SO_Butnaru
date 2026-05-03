#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

// Variabilă globală pentru a ține programul activ
volatile sig_atomic_t keep_running = 1;

// Handler pentru SIGINT (Ctrl+C)
void handle_sigint(int sig) {
    keep_running = 0;
    // Semnalam iesirea; printf nu e 100% sigur in handlere de semnal, dar e cerut pentru afișare
    write(STDOUT_FILENO, "\n[Monitor] S-a primit SIGINT. Se inchide programul...\n", 55);
}

// Handler pentru SIGUSR1 (Notificare raport nou)
void handle_sigusr1(int sig) {
    write(STDOUT_FILENO, "[Monitor] Notificare: Un nou raport a fost adaugat in sistem!\n", 63);
}

int main() {
    // 1. Configurarea handler-elor folosind DOAR sigaction() (regula din PDF)
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

    // 2. Crearea fisierului .monitor_pid și scrierea PID-ului
    int fd = open(".monitor_pid", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Eroare la crearea .monitor_pid");
        return 1;
    }
    
    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
    write(fd, pid_str, strlen(pid_str));
    close(fd);

    printf("[Monitor] Pornit cu succes cu PID %d. Astept semnale...\n", getpid());

    // 3. Bucla infinita de asteptare a semnalelor
    while (keep_running) {
        pause(); // Pune procesul "la somn" pana primeste un semnal (evită consumul de CPU)
    }

    // 4. Cleanup la inchidere (SIGINT)
    unlink(".monitor_pid");
    printf("[Monitor] Fisierul .monitor_pid a fost sters. La revedere!\n");

    return 0;
}