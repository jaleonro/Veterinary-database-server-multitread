#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 3535
#define MAXLEN 32

int clientFd;
int r;


void readString(int socket, char *buff, int msgSize){
	int count = 0;
	int r;	
	char c;
	char *apBuff = buff;	
	while (count<msgSize){
		r = recv(socket, &c, 1, 0);		
		if (r == -1){
			perror("error en receive");
			exit(-1);}
		apBuff = buff + count;
		*apBuff = c;
		count = count + r;
		}
	}

int ingresar () {		
	
	char nombre[32];
	char tipo[32];
	int edad;
	char raza[16];
	int estatura;
	float peso;
	char sexo;

	printf("\n Nombre : ");
	scanf("%s" , nombre);
	r = send(clientFd, nombre, sizeof(nombre), 0);
	if (r == -1){
		perror("error en send");
		exit(-1);}
	printf("\n Tipo : ");
	scanf("%s" , tipo);
	r = send(clientFd, tipo, sizeof(tipo), 0);
	if (r == -1){
		perror("error en send");
		exit(-1);}
	printf("\n Edad : ");
	scanf("%i" , &edad);
	r = send(clientFd, &edad, sizeof(edad), 0);
	if (r == -1){
		perror("error en send");
		exit(-1);}
	printf("\n Raza : ");
	scanf("%s" , raza);
	r = send(clientFd, raza, sizeof(raza), 0);
	if (r == -1){
		perror("error en send");
		exit(-1);}
	printf("\n Estatura cm : ");
	scanf("%i" , &estatura);
	r = send(clientFd,  &estatura, sizeof(estatura), 0);
	if (r == -1){
		perror("error en send");
		exit(-1);}
	printf("\n Sexo [H / M] : ");	
	scanf("%c" , &sexo);
	while (sexo != 'H' && sexo != 'M'){
		printf("\nIngrese un dato valido\n");
		scanf("%c" , &sexo);}
	r = send(clientFd,  &sexo, sizeof(sexo), 0);
	if (r == -1){
		perror("error en send");
		exit(-1);}
	printf("\n Peso  Kg: ");
	scanf("%f" , &peso);
	r = send(clientFd,  &peso, sizeof(peso), 0);
	if (r == -1){
		perror("error en send");
		exit(-1);}

	printf("\nEl registro se ha creado");
	
	int numeroAsignado;

	r = recv(clientFd, &numeroAsignado, sizeof(numeroAsignado), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}

	printf("\nNumero de registro asignado: %i", numeroAsignado);

	
	}

void ver(){
	int registroAconsultar = 0;
	int flag = 0;
	char nombre[32];
	char tipo[32];
	int edad;
	char raza[16];
	int estatura;
	float peso;
	char sexo;
	int noRegistro;
	int nRegistros;

	r = recv(clientFd, &nRegistros, sizeof(nRegistros), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}

	printf("\nNumero de registros presentes: %i", nRegistros);
	printf("\nIngrese el numero de registro a consultar\n");
	scanf("%i", &registroAconsultar);
	r = send(clientFd, &registroAconsultar, sizeof(registroAconsultar), 0);
	if (r == -1){
		perror("error en send");
		exit(-1);}
	
	r = recv(clientFd, &flag, sizeof(flag), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}

	if(flag==1){
		readString(clientFd, nombre, sizeof(nombre));
		printf("nombre: %s\n", nombre);
		readString(clientFd, tipo, sizeof(tipo));
		printf("tipo: %s\n", tipo);	
		r = recv(clientFd, &edad, sizeof(edad), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}
		printf("\nedad: %i\n", edad);
		readString(clientFd, raza, sizeof(raza));
		printf("raza: %s\n", raza);
		r = recv(clientFd, &estatura, sizeof(estatura), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}
		printf("estatura: %i\n", estatura);
		r = recv(clientFd, &sexo, sizeof(sexo), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}
		printf("genero: %c\n", sexo);
		r = recv(clientFd, &peso, sizeof(peso), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}
		printf("peso: %f\n", peso);
		r = recv(clientFd, &noRegistro, sizeof(noRegistro), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}
		printf("noRegistro: %i\n", noRegistro);			
			
		}

		if(flag == 0){
			printf("\nEl registro con dicho numero no existe");
			}
	}

void borrar(){
	int registroAborrar = 0;
	int nRegistros;	
	int flag;

	r = recv(clientFd, &nRegistros, sizeof(nRegistros), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}	
	printf("\nNumero de registros presentes: %i", nRegistros);
	printf("\nIngrese el numero de registro a borrar\n");
	scanf("%i", &registroAborrar);
	r = send(clientFd, &registroAborrar, sizeof(registroAborrar), 0);
	if (r == -1){
		perror("error en send");
		exit(-1);}
	r = recv(clientFd, &flag, sizeof(flag), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}
	if(flag == 0){
		printf("\nEl registro con dicho numero no existe");
		}
	else{
		printf("\nRegistro borrado");
		}
	}

void buscar(){
	
	char name [32];	
	int flag = 0;
	char nombre[32];
	char tipo[32];
	int edad;
	char raza[16];
	int estatura;
	float peso;
	char sexo;
	int noRegistro;
	int print = 0;
	
	printf("\nIngrese el nombre de la mascota a buscar\n");
	scanf("%s", name);
	r = send(clientFd, name, sizeof(name), 0);
	if (r == -1){
		perror("error en send");
		exit(-1);}
	
	while (1){
		r = recv(clientFd, &print, sizeof(print), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}
		if (print==0){
			break;}
		readString(clientFd, nombre, sizeof(nombre));
		printf("nombre: %s\n", nombre);
		readString(clientFd, tipo, sizeof(tipo));
		printf("tipo: %s\n", tipo);	
		r = recv(clientFd, &edad, sizeof(edad), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}
		printf("\nedad: %i\n", edad);
		readString(clientFd, raza, sizeof(raza));
		printf("raza: %s\n", raza);
		r = recv(clientFd, &estatura, sizeof(estatura), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}
		printf("estatura: %i\n", estatura);
		r = recv(clientFd, &sexo, sizeof(sexo), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}
		printf("genero: %c\n", sexo);
		r = recv(clientFd, &peso, sizeof(peso), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}
		printf("peso: %f\n", peso);
		r = recv(clientFd, &noRegistro, sizeof(noRegistro), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}
		printf("noRegistro: %i\n", noRegistro);			

		}

	r = recv(clientFd, &flag, sizeof(flag), 0);
	if (r == -1){
	perror("error en receive");
	exit(-1);}

	if (flag==0){
		printf("\nNombre no encontrado\n");}

	}


int menu(){
int a ;
printf("\n|----------------------------------|");
printf("\n|         TIENDA DE MASCOTAS       |");
printf("\n|----------------------------------|");
printf("\n| 1) Ingresar registro             |");
printf("\n| 2) Ver registro                  |");
printf("\n| 3) Borrar registro               |");
printf("\n| 4) Buscar registro               |");
printf("\n| 5) Salir                         |");
printf("\n|----------------------------------|");

scanf("%i" , &a);
r = send(clientFd, &a, sizeof(a), 0);
if (r == -1){
	perror("error en send");
	exit(-1);}
return a;
}





int main(){

struct sockaddr_in client;
clientFd = socket(AF_INET, SOCK_STREAM, 0);
if(clientFd < 0){
        perror("error en socket");
        exit(-1);}

client.sin_family = AF_INET;
client.sin_port = htons(PORT);
client.sin_addr.s_addr= inet_addr("127.0.0.1");
bzero(client.sin_zero, 8);
socklen_t tamano =  (socklen_t) (sizeof(struct sockaddr));
r = connect(clientFd, (struct sockaddr *) &client, tamano);
if (r == -1){
	perror("error en connect");
	exit(-1);}

	int a ;
	a = menu();
	while (a != 5){
	switch(a)	
	{
	case 1:
	printf("ingresar:");
	ingresar();
	a = menu();	
	break;
	case 2:
	printf("ver:");
	ver();
	a = menu();
	break;
	case 3:
	printf("borrar:");
	borrar();
	a = menu();
	break;
	case 4:
	printf("buscar:");
	buscar();
	a = menu();
	break;
	case 5:
	
	break;
	} 
	}

close(clientFd);
}
