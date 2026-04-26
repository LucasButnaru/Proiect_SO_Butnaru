#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#define MAX_PATH 256
#define MAX_STRING 64

// Structura pentru raport
typedef struct {
    int id;
    char inspector[MAX_STRING];
    float lat;
    float lon;
    char category[MAX_STRING];
    int severity;
    time_t timestamp;
    char description[256];
} Report;

// Variabile globale pentru rol și utilizator curent
char current_role[MAX_STRING];
char current_user[MAX_STRING];

// Funcție pentru conversia biților de permisiuni într-un string (ex: rw-rw-r--)
void mode_to_string(mode_t mode, char *str) {
    strcpy(str, "---------");
    if (mode & S_IRUSR) str[0] = 'r';
    if (mode & S_IWUSR) str[1] = 'w';
    if (mode & S_IXUSR) str[2] = 'x';
    if (mode & S_IRGRP) str[3] = 'r';
    if (mode & S_IWGRP) str[4] = 'w';
    if (mode & S_IXGRP) str[5] = 'x';
    if (mode & S_IROTH) str[6] = 'r';
    if (mode & S_IWOTH) str[7] = 'w';
    if (mode & S_IXOTH) str[8] = 'x';
}

// Verifică permisiunile în funcție de rol
int check_access(const char *filepath, int need_read, int need_write, int need_exec) {
    struct stat st;
    if (stat(filepath, &st) != 0) return 1; // Dacă nu există, lăsăm operația să continue (va fi creat)

    int can_read = 0, can_write = 0, can_exec = 0;

    if (strcmp(current_role, "manager") == 0) { // Manager = Owner
        can_read = (st.st_mode & S_IRUSR);
        can_write = (st.st_mode & S_IWUSR);
        can_exec = (st.st_mode & S_IXUSR);
    } else if (strcmp(current_role, "inspector") == 0) { // Inspector = Group
        can_read = (st.st_mode & S_IRGRP);
        can_write = (st.st_mode & S_IWGRP);
        can_exec = (st.st_mode & S_IXGRP);
    }

    if (need_read && !can_read) { printf("Eroare: Rolul %s nu are permisiuni de citire!\n", current_role); return 0; }
    if (need_write && !can_write) { printf("Eroare: Rolul %s nu are permisiuni de scriere!\n", current_role); return 0; }
    if (need_exec && !can_exec) { printf("Eroare: Rolul %s nu are permisiuni de executie!\n", current_role); return 0; }

    return 1;
}

// Scrie în fișierul de log folosind I/O primitiv
void log_action(const char *district_id, const char *action) {
    char log_path[MAX_PATH];
    snprintf(log_path, sizeof(log_path), "%s/logged_district", district_id);

    // Manager scrie, oricine citește. Verificăm manual permisiunile.
    if (!check_access(log_path, 0, 1, 0)) return;

    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return;
    
    // Setăm corect permisiunile explicit cum cere enunțul
    chmod(log_path, 0644);

    char log_entry[512];
    time_t now = time(NULL);
    snprintf(log_entry, sizeof(log_entry), "[%ld] Role: %s, User: %s, Action: %s\n", now, current_role, current_user, action);
    write(fd, log_entry, strlen(log_entry));
    close(fd);
}

// Creează structura districtului (director + symlink + fisiere)
void setup_district(const char *district_id) {
    struct stat st;
    
    // 1. Creare director
    if (stat(district_id, &st) == -1) {
        mkdir(district_id, 0750); // rwxr-x---
    }

    // 2. Creare reports.dat
    char report_path[MAX_PATH];
    snprintf(report_path, sizeof(report_path), "%s/reports.dat", district_id);
    if (stat(report_path, &st) == -1) {
        int fd = open(report_path, O_CREAT | O_WRONLY, 0664);
        if (fd >= 0) close(fd);
        chmod(report_path, 0664); // Setare explicita
    }

    // 3. Creare district.cfg
    char config_path[MAX_PATH];
    snprintf(config_path, sizeof(config_path), "%s/district.cfg", district_id);
    if (stat(config_path, &st) == -1) {
        int fd = open(config_path, O_CREAT | O_WRONLY, 0640);
        if (fd >= 0) {
            const char *default_cfg = "severity_threshold=2\n";
            write(fd, default_cfg, strlen(default_cfg));
            close(fd);
        }
        chmod(config_path, 0640); // Setare explicita
    }

    // 4. Creare symlink
    char symlink_name[MAX_PATH];
    snprintf(symlink_name, sizeof(symlink_name), "active_reports-%s", district_id);
    struct stat lst;
    if (lstat(symlink_name, &lst) == -1) {
        symlink(report_path, symlink_name);
    }
}

// Comanda: add
void cmd_add(const char *district_id) {
    setup_district(district_id);
    char report_path[MAX_PATH];
    snprintf(report_path, sizeof(report_path), "%s/reports.dat", district_id);

    // Verificam permisiunile
    if (!check_access(report_path, 0, 1, 0)) return;

    int fd = open(report_path, O_WRONLY | O_APPEND);
    if (fd < 0) { perror("Eroare deschidere reports.dat"); return; }

    // Liste pentru generarea de date aleatoare
    const char* categorii[] = {"road", "sanitation", "lighting", "parks", "water"};
    const char* descrieri[] = {
        "Groapa pe carosabil", 
        "Gunoi neridicat pe trotuar", 
        "Stalp de iluminat defect", 
        "Banca rupta in parc", 
        "Scurgere de apa pe strada"
    };
    
    // Alegem un index la intamplare (între 0 și 4)
    int rand_idx = rand() % 5;

    Report r;
    r.id = rand() % 100000; // ID aleatoriu între 0 și 99999
    strncpy(r.inspector, current_user, MAX_STRING);
    
    // Generam coordonate GPS aleatoare (simuland o zona apropiata de Timisoara)
    r.lat = 45.70f + ((float)rand() / RAND_MAX) * 0.1f; 
    r.lon = 21.20f + ((float)rand() / RAND_MAX) * 0.1f;
    
    // Atribuim valorile aleatoare alese
    strcpy(r.category, categorii[rand_idx]);
    r.severity = (rand() % 5) + 1; // Severitate între 1 și 5
    r.timestamp = time(NULL);      // Momentul adaugarii raportului
    strcpy(r.description, descrieri[rand_idx]);

    // Scriem in fisier
    write(fd, &r, sizeof(Report));
    close(fd);

    log_action(district_id, "add_report");
    printf("Raport adaugat cu succes in districtul %s (ID: %d | Cat: %s | Sev: %d).\n", 
           district_id, r.id, r.category, r.severity);
}

// Comanda: list
void cmd_list(const char *district_id) {
    char report_path[MAX_PATH];
    snprintf(report_path, sizeof(report_path), "%s/reports.dat", district_id);

    if (!check_access(report_path, 1, 0, 0)) return;

    struct stat st;
    if (stat(report_path, &st) == 0) {
        char perms[10];
        mode_to_string(st.st_mode, perms);
        printf("Fisier: %s | Permisiuni: %s | Dimensiune: %ld bytes | Ultimul acces: %ld\n", 
               report_path, perms, st.st_size, st.st_mtime);
    }

    int fd = open(report_path, O_RDONLY);
    if (fd < 0) return;

    Report r;
    printf("\nRaportari curente:\n");
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        printf("ID: %d | Categorie: %s | Severitate: %d | Inspector: %s\n", r.id, r.category, r.severity, r.inspector);
    }
    close(fd);
    log_action(district_id, "list_reports");
}

// Comanda: remove_report
void cmd_remove(const char *district_id, int target_id) {
    if (strcmp(current_role, "manager") != 0) {
        printf("Eroare: Doar managerii pot sterge rapoarte.\n");
        return;
    }

    char report_path[MAX_PATH];
    snprintf(report_path, sizeof(report_path), "%s/reports.dat", district_id);

    int fd = open(report_path, O_RDWR);
    if (fd < 0) return;

    Report r;
    off_t pos = 0;
    int found = 0;

    // Găsim raportul
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if (r.id == target_id) {
            found = 1;
            break;
        }
        pos += sizeof(Report);
    }

    if (!found) {
        printf("Raportul %d nu a fost gasit.\n", target_id);
        close(fd);
        return;
    }

    // Shiftam inregistrarile (cum e specificat cu lseek si ftruncate)
    off_t read_pos = pos + sizeof(Report);
    off_t write_pos = pos;

    while (1) {
        lseek(fd, read_pos, SEEK_SET);
        ssize_t bytes_read = read(fd, &r, sizeof(Report));
        if (bytes_read <= 0) break; // Am ajuns la final

        lseek(fd, write_pos, SEEK_SET);
        write(fd, &r, sizeof(Report));

        read_pos += sizeof(Report);
        write_pos += sizeof(Report);
    }

    ftruncate(fd, write_pos); // Trunchiem fisierul
    close(fd);

    printf("Raportul %d a fost sters.\n", target_id);
    log_action(district_id, "remove_report");
}

// Returneaza 1 in caz de succes, 0 daca formatul este invalid
int parse_condition(const char *input, char *field, char *op, char *value) {
    // Cautam prima aparitie a caracterului ':'
    const char *colon1 = strchr(input, ':');
    if (colon1 == NULL) return 0; // Format gresit

    // Cautam a doua aparitie a caracterului ':', incepand de dupa prima
    const char *colon2 = strchr(colon1 + 1, ':');
    if (colon2 == NULL) return 0; // Format gresit

    // Extragem 'field' (copiem de la inceput pana la primul ':')
    size_t field_len = colon1 - input;
    strncpy(field, input, field_len);
    field[field_len] = '\0'; // Adaugam terminatorul de sir

    // Extragem 'operator' (copiem intre cele doua ':')
    size_t op_len = colon2 - (colon1 + 1);
    strncpy(op, colon1 + 1, op_len);
    op[op_len] = '\0';

    // Extragem 'value' (copiem tot ce este dupa al doilea ':')
    strcpy(value, colon2 + 1);

    return 1;
}

// Returneaza 1 daca raportul satisface conditia, 0 in caz contrar
int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int val = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity == val;
        if (strcmp(op, "!=") == 0) return r->severity != val;
        if (strcmp(op, "<") == 0) return r->severity < val;
        if (strcmp(op, "<=") == 0) return r->severity <= val;
        if (strcmp(op, ">") == 0) return r->severity > val;
        if (strcmp(op, ">=") == 0) return r->severity >= val;
    } 
    else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->category, value) != 0;
    } 
    else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->inspector, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->inspector, value) != 0;
    } 
    else if (strcmp(field, "timestamp") == 0) {
        time_t val = (time_t)atol(value);
        if (strcmp(op, "==") == 0) return r->timestamp == val;
        if (strcmp(op, "!=") == 0) return r->timestamp != val;
        if (strcmp(op, "<") == 0) return r->timestamp < val;
        if (strcmp(op, "<=") == 0) return r->timestamp <= val;
        if (strcmp(op, ">") == 0) return r->timestamp > val;
        if (strcmp(op, ">=") == 0) return r->timestamp >= val;
    }
    
    return 0; // Camp necunoscut sau operator care nu este suportat
}

// Comanda: filter
void cmd_filter(int argc, char *argv[], int start_arg_idx) {
    const char *district_id = argv[start_arg_idx];
    char report_path[MAX_PATH];
    snprintf(report_path, sizeof(report_path), "%s/reports.dat", district_id);

    // Verificam permisiunile inainte de orice operatie
    if (!check_access(report_path, 1, 0, 0)) return;

    // Deschidem reports.dat pentru citire
    int fd = open(report_path, O_RDONLY);
    if (fd < 0) {
        perror("Eroare la deschiderea fisierului reports.dat");
        return;
    }

    Report r;
    int num_conditions = argc - (start_arg_idx + 1);
    int reports_found = 0;

    printf("\n--- Rezultate filtrare pentru '%s' ---\n", district_id);
    
    // Citim inregistrarile una cate una cu read()
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        int matches_all = 1; // Presupunem initial ca raportul respecta toate conditiile
        
        // Parcurgem fiecare conditie primita in linia de comanda
        for (int i = 0; i < num_conditions; i++) {
            char field[32], op[4], value[64];
            const char *current_condition = argv[start_arg_idx + 1 + i];

            // Parsam conditia curenta cu functia AI
            if (!parse_condition(current_condition, field, op, value)) {
                printf("Eroare de sintaxa la conditia: %s\n", current_condition);
                matches_all = 0;
                break; // Daca e scrisa gresit, oprim verificarea pt acest raport
            }

            // Testam raportul impotriva conditiei parsate cu functia AI
            if (!match_condition(&r, field, op, value)) {
                matches_all = 0; // A picat conditia (avem operator logic AND implicit intre ele)
                break;           // Nu mai are rost sa verificam restul conditiilor
            }
        }

        // Printam raportul doar daca TOATE conditiile au returnat 1
        if (matches_all) {
            printf("ID: %d | Categorie: %s | Severitate: %d | Inspector: %s | Timp: %ld\n", 
                   r.id, r.category, r.severity, r.inspector, r.timestamp);
            reports_found++;
        }
    }

    if (reports_found == 0) {
        printf("Niciun raport nu corespunde criteriilor cautate.\n");
    }

    close(fd);

    // Inregistram actiunea în log asa cum cere faza 1
    log_action(district_id, "filter");
}

// Functia main cu parsarea argumentelor
int main(int argc, char *argv[]) {
    // Verificam numărul minim de argumente pentru a avea role, user și o comandă
    if (argc < 6) {
        printf("Utilizare: %s --role <role> --user <user> --<command> <args...>\n", argv[0]);
        return 1;
    }

    srand(time(NULL));

    // Initializare variabile globale
    memset(current_role, 0, sizeof(current_role));
    memset(current_user, 0, sizeof(current_user));
    
    // Extragem rolul și utilizatorul din argumente
    int i = 1;
    while (i < 5) {
        if (strcmp(argv[i], "--role") == 0) strcpy(current_role, argv[++i]);
        else if (strcmp(argv[i], "--user") == 0) strcpy(current_user, argv[++i]);
        i++;
    }

    // Comanda este al 6-lea argument (index 5)
    char *command = argv[5];

    // Rutam executia către functia potrivita
    if (strcmp(command, "--add") == 0 && argc >= 7) {
        cmd_add(argv[6]);
        
    } else if (strcmp(command, "--list") == 0 && argc >= 7) {
        cmd_list(argv[6]);
        
    } else if (strcmp(command, "--remove_report") == 0 && argc >= 8) {
        cmd_remove(argv[6], atoi(argv[7]));
        
    } else if (strcmp(command, "--filter") == 0 && argc >= 8) {
        // argc >= 8 ne asigura ca avem districtul (argv[6]) și MACAR o condiție (argv[7])
        // Trimitem argc și argv la functie pentru a parcurge restul conditiilor (argv[7], argv[8], etc.)
        cmd_filter(argc, argv, 6);
        
    } else {
        printf("Eroare: Comanda nerecunoscuta sau argumente lipsa.\n");
    }

    return 0;
}