#include<time.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

struct log{
	
	char dateTime[26];
	char ip[16];	
	char command[10];
	char name[32]; 
	int registro;
      
};


int main(){

	struct log *log;
	FILE *fplogs;
	time_t current_time;
	char c_time_string[26];	

	log = malloc(sizeof(struct log));

	fplogs = fopen("serverDogs.log", "r");

	
	while(fread(log,sizeof(struct log),1,fplogs) == 1){	
	printf("command: %s\n", log->command);
	printf("dateTime: %s\n", log->dateTime);	
	printf("ip: %s\n", log->ip);
	printf("name: %s\n", log->name);
	printf("registro: %i\n", log->registro);
	}
	fclose(fplogs); 
}

