#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>


void create_district(char *name){
	if(mkdir(name, 0750) == -1){
		perror("mkdir");
		return;
	}
	
	char path[100];
	
	sprintf(path, "%s/reports.dat", name);
	int fd1 = open(path, O_CREAT | O_RDWR, 0660);
	close(fd1);
	
	sprintf(path, "%s/district.cfg", name);
	int fd2 = open(path, O_CREAT | O_RDWR, 0640);
	
	write(fd2, "threshold=2\n", 12);
	close(fd2);
	
	sprintf(path, "%s/logged_district", name);
	int fd3 = open(path, O_CREAT | O_RDWR, 0640);
	close(fd3);
	
	printf("District %s created!\n", name);
}



typedef struct {
	int id;
	char inspector[50];
	float latitude;
	float longitude;
	char category[20];
	int severity;
	time_t timestamp;
	char description[100];
} Raport;

bool check_role(char *text[])
{
	if(strcmp(text, "inspector")==0 || strcmp(text, "manager")==0)
		return true;
	else
		return false;

}

int main(int argc, char *argv[]){

    char *role = NULL;
	char *command = NULL;
	char *district = NULL;
	for(int i = 1; i<argc; i++){
		if(strcmp(argv[i], "--role") == 0){
			role = argv[i+1];
		}
		else if(strcmp(argv[i], "--add") == 0){
			district = argv[i+1];
			command = "add";
		}
	}
	
	if(check_role(role)==false || command == NULL){
		printf("Usage Error!\n");
		return 1;
	}
	
	if(strcmp(command, "add") == 0){
		create_district(district);
	}

    return 0;
}







