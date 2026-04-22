#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

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

// Variabile globale pentru rol si utilizator curent
char current_role[MAX_STRING];
char current_user[MAX_STRING];

// Functie pentru conversia bitilor de permisiuni intr-un string (ex: rw-rw-r--)
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


// Verifica permisiunile în functie de rol
int check_access(const char *filepath, int need_read, int need_write, int need_exec) {
	struct stat st;
	if (stat(filepath, &st) != 0) 
		return 1; // Daca nu exista, lasam operatia sa continue (va fi creat)

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

	if (need_read && !can_read) { 
		printf("Eroare: Rolul %s nu are permisiuni de citire!\n", current_role); 
		return 0; 
	}
	if (need_write && !can_write) { 
		printf("Eroare: Rolul %s nu are permisiuni de scriere!\n", current_role); 
		return 0; 
	}
	if (need_exec && !can_exec) { 
		printf("Eroare: Rolul %s nu are permisiuni de executie!\n", current_role); 
		return 0; 
	}

	return 1;
}

// Scrie în fisierul de log folosind I/O primitiv
void log_action(const char* district_id, const char* action){
	char log_path[MAX_PATH];
	snprintf(log_path, sizeof(log_path), "%s/logged_district", district_id);

	// Manager scrie, oricine citeste. Verificam manual permisiunile.
	if (!check_access(log_path, 0, 1, 0)) return;

	int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd < 0) return;

	// Setam corect permisiunile explicit cum cere enuntul
	chmod(log_path, 0644);
	char log_entry[512];
	time_t now = time(NULL);
	snprintf(log_entry, sizeof(log_entry), "[%ld] Role: %s, User: %s, Action: %s\n", now, current_role, current_user, action);
	write(fd, log_entry, strlen(log_entry));
	close(fd);
}

// Creeaza structura districtului (director + symlink)
void setup_district(const char *district_id) {
	struct stat st;
	if (stat(district_id, &st) == -1) {
	mkdir(district_id, 0750); // rwxr-x---
	}

	char report_path[MAX_PATH];
	snprintf(report_path, sizeof(report_path), "%s/reports.dat", district_id);

	// Creare reports.dat gol daca nu exista
	if (stat(report_path, &st) == -1) {
		int fd = open(report_path, O_CREAT | O_WRONLY, 0664);

		if (fd >= 0) close(fd);
	}

	// Creare symlink
	char symlink_name[MAX_PATH];

	snprintf(symlink_name, sizeof(symlink_name), "active_reports-%s", district_id);

	struct stat lst;

	if (lstat(symlink_name, &lst) == -1) {
		symlink(report_path, symlink_name);
	}	

}

//Comanda: add

void cmd_add(const char* district_id){
	setup_district(district_id);
	char report_path[MAX_PATH];
	snprintf(report_path, sizeof(report_path), "%s/reports.dat", district_id);
	
	if(!check_access(report_path, 0, 1, 0)) return;

	int fd = open(report_path, O_WRONLY | O_APPEND);

	if(fd<0) {
		perror("Eroare deschidere reports.dat!");
		return;
	}

	Report r;
	r.id = (int)time(NULL) % 10000; // ID generat simplu
	strncpy(r.inspector, current_user, MAX_STRING);
	r.lat = 67.67f; 
	r.lon = 69.69f; // Mock GPS
	strcpy(r.category, "road");
	r.severity = 2;
	r.timestamp = time(NULL);
	strcpy(r.description, "Groapa pe carosabil");

	write(fd, &r, sizeof(Report));
	close(fd);

	log_action(district_id, "add_report");
	printf("Raport adaugat cu succes in districtul %s.\n", district_id);

}

//Comanda: list
void cmd_list(const char *district_id) {
	char report_path[MAX_PATH];
	snprintf(report_path, sizeof(report_path), "%s/reports.dat", district_id);

	if (!check_access(report_path, 1, 0, 0)) return;

	struct stat st;
	if (stat(report_path, &st) == 0) {
	char perms[10];
	mode_to_string(st.st_mode, perms);
	printf("Fisier: %s | Permisiuni: %s | Dimensiune: %ld bytes | Ultimul acces: %ld\n", report_path, perms, st.st_size, st.st_mtime);
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


// Functia main
int main(int argc, char *argv[]) {
	if (argc < 6) {
	printf("Utilizare: %s --role --user --\n", argv[0]);
	return 1;
}

// Initializare
	memset(current_role, 0, sizeof(current_role));
	memset(current_user, 0, sizeof(current_user));

	int i = 1;
	while (i < 5) {
		if (strcmp(argv[i], "--role") == 0) 
			strcpy(current_role, argv[++i]);
		else if (strcmp(argv[i], "--user") == 0) 
			strcpy(current_user, argv[++i]);
	i++;
}

	char *command = argv[5];

	if (strcmp(command, "--add") == 0 && argc >= 7) {
		cmd_add(argv[6]);
	} 
	else if (strcmp(command, "--list") == 0 && argc >= 7) {
		cmd_list(argv[6]);
	} 
	else if (strcmp(command, "--remove_report") == 0 && argc >= 8) {
		cmd_remove(argv[6], atoi(argv[7]));
	} 
	else {
	printf("Comanda nerecunoscuta sau argumente lipsa.\n");
}

	return 0;
}









