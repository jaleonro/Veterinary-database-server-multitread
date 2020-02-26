#include <semaphore.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/stat.h> 
#include <fcntl.h>     
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define NUMTHREADS 100
#define BACKLOG 32//cantidad maxima de procesos en cola
#define SEMINIT 1//numero de procesos que pueden ejecutarse a la vez - semaforos


struct dogtype{

	int noRegistro;	
	char borrado;
	char nombre[32];
	char tipo[32];
	int edad;
	char raza[16];
	int estatura;
	float peso;
	char sexo;
	int next;
       
};

struct log{
	
	char dateTime[26];
	char ip[16];	
	char command[10];
	char name[32];       
};

//estructura para agrupar en uno solo el argumento que recibe la rutina de cada hilo, descriptor del socket del cliente y la estructura
//que almacena su direccion
struct arg_struct {
    int sock;
    struct in_addr dir;
};

//DECLARACION METODOS DE SINCRONIZACION:
//Las protecciones se hacen necesarias en las regiones en las que se usan recursos compartidos por todos los hilos

//sem_t *semaforo;
//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int pipefd [2];
char caracter = 'x';
char buffer;

int servFd; //descriptor del socket del servidor
struct sockaddr_in server, cliente;
int threadNumber = 0;//para llevar la cuenta del numero de hilos creados para asignar el identificador de cada hilo en su respectiva
//posicion en el arreglo hilos
pthread_t hilos[NUMTHREADS];

FILE *fplogs;//descriptor del archivo en donde se almacenan los logs

int registrosBorrados;
int nRegistros;//numero de registros actual
int noRegistroAsignar;//guarda el codigo de registro asignar cuando se crea uno nuevo
int tails[1265]={};//ubicaciones del ultimo elemento de cada lista de la tabla hash
	

FILE *fp;//apuntador al archivo en donde se encuentran las estructuras
FILE *registroNameFile;//apunatdor al archivo que asocia numero de registro con nombre correspondiente
FILE *tailsFile;//apuntador al archivo que guarda la posicion de cada una de las colas de cada lista de la tabla hash

//recibe el apuntador al lugar en memoria en donde se va almacenar el dato, el socket desde el que se recibe y su tamaño
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


int hashCode(char a[32]){
	int i=0;
	int hash = 0, c = 0;
	while (a[i] != '\0'){
 		c = toascii(a[i]);
		hash = hash + c;
		i++;}
	hash = hash % 1265;//1265 es la cantidad de listas de la tabla hash
	return (hash);
}


int ingresar (int socket){
	int r;
	int clientFd = socket;
	
	struct dogtype *dato = malloc(sizeof(struct dogtype));	
	struct dogtype *currentAnimal = malloc(sizeof(struct dogtype));
	struct dogtype *animalAux = malloc(sizeof(struct dogtype));
	
	
	readString(clientFd, dato->nombre, sizeof(dato->nombre));
	readString(clientFd, dato->tipo, sizeof(dato->tipo));	
	r = recv(clientFd, &dato->edad, sizeof(dato->edad), 0);
		if (r == -1){
			perror("error en receive");
			exit(-1);}
	readString(clientFd, dato->raza, sizeof(dato->raza));
	r = recv(clientFd, &dato->estatura, sizeof(dato->estatura), 0);
		if (r == -1){
			perror("error en receive");
			exit(-1);}
	r = recv(clientFd, &dato->sexo, sizeof(dato->sexo), 0);
		if (r == -1){
			perror("error en receive");
			exit(-1);}
	r = recv(clientFd, &dato->peso, sizeof(dato->peso), 0);
		if (r == -1){
		perror("error en receive");
		exit(-1);}

//INICIO SECCION CRITICA
//JUSTIFICACION: Si mas de un hilo accede al valor de la variable global 'noRegistroAsignar' para hacer la asignacion al mismo tiempo, 
//todos leeran el mismo valor y lo aumentaran en uno por lo que guardaran en la variable exactamente el mismo valor. Asignando
//el mismo numero a mas de un registro, razon por la cual se hace necesaria la proteccion 	
 	
	int x = read(pipefd[0],&buffer,1);	
	if (x == -1){
		perror("error en read pipe");
		exit(-1);}

	//sem_wait (semaforo);	
	//pthread_mutex_lock(&mutex);

	noRegistroAsignar++;//variable global-no se puede asignar un mismo numero a dos registros
	dato->noRegistro = noRegistroAsignar;

	//sem_post (semaforo);
	//pthread_mutex_unlock(&mutex);
	
	x = write(pipefd [1], &caracter, 1);
	if (x == -1){
		perror("error en write pipe");
		exit(-1);}	

//FIN SECCION CRITICA

	fseek(registroNameFile, 32*sizeof(char)*dato->noRegistro, SEEK_SET);
	fwrite (dato->nombre,32*sizeof(char),1,registroNameFile);//en el archivo que asocia numeros de registro con nombres se guarda en la posicion del numero de registro creado el nombre correspondiente
	
	dato->next=-1;
	dato->borrado='F';


//INICIO SECCION CRITICA
//JUSTIFICACION: Dado que todas las nuevas estructuras se agregan al final del archivo, es necesario conocer la direccion del final del archivo para agregar un nuevo registro, la cual se guarda en la variable 'dirNext'. Puede pasar que mas de un hilo ponga el puntero del archivo en la misma posicion para escribir diferentes estructuras, ya que, la direccion direccion del final de archivo no cambiara hasta que se agregue una nueva estuctura. Por lo que se pone la proteccion antes de leer la posicion final del archivo y despues de escribir la nueva estructura para que lo que devuelve ftell cambie
	
	x = read(pipefd[0],&buffer,1);	
	if (x == -1){
		perror("error en read pipe");
		exit(-1);}	

	//sem_wait (semaforo);
	//pthread_mutex_lock(&mutex);	
	fseek(fp, 0, SEEK_END);//todos los nuevos registros se agregan al final del archivo
	int dirNext = ftell(fp);//la direccion del final del archivo es la ubicacion de la nueva cola de la lista correspondiente
	fwrite (dato,sizeof(struct dogtype),1,fp);	

	//sem_post (semaforo);
	//pthread_mutex_unlock(&mutex);

	x = write(pipefd [1], &caracter, 1);
	if (x == -1){
		perror("error en write pipe");
		exit(-1);}

//FIN SECCION CRITICA	
	
	fseek(fp, tails [hashCode(dato->nombre)] , SEEK_SET);//ir a la posicion de la actual cola para cambiar el atributo next
	fread(currentAnimal,sizeof(struct dogtype),1,fp);	

	strcpy(animalAux->nombre, currentAnimal->nombre);		
	strcpy(animalAux->tipo, currentAnimal->tipo);
	animalAux->edad = currentAnimal->edad;	
	strcpy(animalAux->raza, currentAnimal->raza);
	animalAux->estatura = currentAnimal->estatura;		
	animalAux->peso = currentAnimal->peso;
	animalAux->sexo = currentAnimal->sexo;
	animalAux->noRegistro = currentAnimal->noRegistro;
	animalAux->next = dirNext;
	animalAux->borrado = currentAnimal->borrado;

//INICIO SECCION CRITICA
//JUSTIFICACION: Si un hilo cambia la posicion del puntero del archivo al ejecutar la funcion 'fseek' luego de que otro hilo lo haya hecho, con una posicion diferente antes de que se escriba en el archivo, el dato se escribira en la posicion equivocada. Ademas, se pueden preesentar conflictos si mas de un hilo intenta cambiar la posicion del puntero del archivo al mismo tiempo. Adicionalmente, se requiere sincronizacion para evitar conflictos con las variables compartidas presentes en esta seccion
	
	x = read(pipefd[0],&buffer,1);	
	if (x == -1){
		perror("error en read pipe");
		exit(-1);}		

	//sem_wait (semaforo);
	//pthread_mutex_lock(&mutex);

	fseek(fp, tails [hashCode(animalAux->nombre)] , SEEK_SET);	
	fwrite (animalAux,sizeof(struct dogtype),1,fp);		
	tails [hashCode(animalAux->nombre)]=dirNext;//actualizar la ubicacion de la cola	

	nRegistros++;

	//sem_post (semaforo);
	//pthread_mutex_unlock(&mutex);

	x = write(pipefd [1], &caracter, 1);
	if (x == -1){
		perror("error en write pipe");
		exit(-1);}

//FIN SECCION CRITICA


	r = send(clientFd, &noRegistroAsignar, sizeof(noRegistroAsignar), 0);
	if (r == -1){
		perror("error en send");
		exit(-1);}	

	//printf("\nEl registro se ha creado");		
	
	free(currentAnimal);		
	free(animalAux);
	free(dato);
	
}



void ver(int socket){
	
	int r;
	int clientFd = socket;
	int registroAconsultar = 0;
	char name [32];
	struct dogtype *currentAnimal = malloc(sizeof(struct dogtype));
	int flag = 0;
	
	r = send(clientFd, &nRegistros, sizeof(nRegistros), 0);
	if (r == -1){
		perror("error en send");
		exit(-1);}

	r = recv(clientFd, &registroAconsultar, sizeof(registroAconsultar), 0);
		if (r == -1){
			perror("error en receive");
			exit(-1);}

//INICIO SECCION CRITICA
//JUSTIFICACION: Si se cambia la posicion del archivo antes de leer se leera el dato equivocado, lo que puede pasar si un hilo cambia la posicion en el archivo despues de un 'fseek' ejecutado por un primer hilo antes de que este ejecute el 'fread'	
 	
	int x = read(pipefd[0],&buffer,1);	
	if (x == -1){
		perror("error en read pipe");
		exit(-1);}

	//sem_wait (semaforo);	
	//pthread_mutex_lock(&mutex);

	fseek(registroNameFile, 32*sizeof(char)*registroAconsultar, SEEK_SET);
	fread(&name, sizeof(name), 1, registroNameFile);//se busca el nombre que le corresponde al numero de registro a consultar

	//sem_post (semaforo);
	//pthread_mutex_unlock(&mutex);
	
	x = write(pipefd [1], &caracter, 1);
	if (x == -1){
		perror("error en write pipe");
		exit(-1);}	

//FIN SECCION CRITICA

	if (strcmp( name,"###") == 0){//se verifica si ya fue borrado o no se ha creado		
		//printf("\nEl registro con dicho numero no existe");	

	}else {
//se recorre la lista hasta encontrar el registro buscado
//cuando se encuentra el registro se envian al cliente uno a uno los datos de la estructura

//INICIO SECCION CRITICA
//JUSTIFICACION: Si se cambia la posicion del archivo antes de leer se leera el dato equivocado, lo que puede pasar si un hilo cambia la posicion en el archivo despues de un 'fseek' ejecutado por un primer hilo antes de que este ejecute el 'fread'	
 	
		x = read(pipefd[0],&buffer,1);	
		if (x == -1){
			perror("error en read pipe");
			exit(-1);}

		//sem_wait (semaforo);	
		//pthread_mutex_lock(&mutex);
				
		fseek(fp, 108*sizeof(char)*hashCode(name) , SEEK_SET);
		fread(currentAnimal,sizeof(struct dogtype),1,fp);

		//sem_post (semaforo);
		//pthread_mutex_unlock(&mutex);
	
		x = write(pipefd [1], &caracter, 1);
		if (x == -1){
			perror("error en write pipe");
			exit(-1);}	

//FIN SECCION CRITICA


		if(currentAnimal->noRegistro == registroAconsultar){
			flag = 1;
			r = send(clientFd, &flag, sizeof(flag), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, currentAnimal->nombre, sizeof(currentAnimal->nombre), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, currentAnimal->tipo, sizeof(currentAnimal->tipo), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, &currentAnimal->edad, sizeof(currentAnimal->edad), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, currentAnimal->raza, sizeof(currentAnimal->raza), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, &currentAnimal->estatura, sizeof(currentAnimal->estatura), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, &currentAnimal->sexo, sizeof(currentAnimal->sexo), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, &currentAnimal->peso, sizeof(currentAnimal->peso), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, &currentAnimal->noRegistro, sizeof(currentAnimal->noRegistro), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}			
			}

		if(flag != 1){

			while(currentAnimal->next != -1){

//INICIO SECCION CRITICA
//JUSTIFICACION: Si se cambia la posicion del archivo antes de leer se leera el dato equivocado, lo que puede pasar si un hilo cambia la posicion en el archivo despues de un 'fseek' ejecutado por un primer hilo antes de que este ejecute el 'fread'	
 	
				x = read(pipefd[0],&buffer,1);	
				if (x == -1){
					perror("error en read pipe");
					exit(-1);}

				//sem_wait (semaforo);	
				//pthread_mutex_lock(&mutex);

						
				fseek(fp, currentAnimal->next, SEEK_SET);
				fread(currentAnimal,sizeof(struct dogtype),1,fp);

				//sem_post (semaforo);
				//pthread_mutex_unlock(&mutex);
	
				x = write(pipefd [1], &caracter, 1);
				if (x == -1){
					perror("error en write pipe");
					exit(-1);}	

//FIN SECCION CRITICA

				if(currentAnimal->noRegistro == registroAconsultar){
					flag = 1;
					r = send(clientFd, &flag, sizeof(flag), 0);
					if (r == -1){
						perror("error en send");
						exit(-1);}
					r = send(clientFd, currentAnimal->nombre, sizeof(currentAnimal->nombre), 0);
					if (r == -1){
						perror("error en send");
						exit(-1);}
					r = send(clientFd, currentAnimal->tipo, sizeof(currentAnimal->tipo), 0);
					if (r == -1){
						perror("error en send");
						exit(-1);}
					r = send(clientFd, &currentAnimal->edad, sizeof(currentAnimal->edad), 0);
					if (r == -1){
						perror("error en send");
						exit(-1);}
					r = send(clientFd, currentAnimal->raza, sizeof(currentAnimal->raza), 0);
					if (r == -1){
						perror("error en send");
						exit(-1);}
					r = send(clientFd, &currentAnimal->estatura, sizeof(currentAnimal->estatura), 0);
					if (r == -1){
						perror("error en send");
						exit(-1);}
					r = send(clientFd, &currentAnimal->sexo, sizeof(currentAnimal->sexo), 0);
					if (r == -1){
						perror("error en send");
						exit(-1);}
					r = send(clientFd, &currentAnimal->peso, sizeof(currentAnimal->peso), 0);
					if (r == -1){
						perror("error en send");
						exit(-1);}
					r = send(clientFd, &currentAnimal->noRegistro, sizeof(currentAnimal->noRegistro), 0);
					if (r == -1){
						perror("error en send");
						exit(-1);}					

					break;
					}	
		
				}
			}		
		}

	if(flag == 0){
			//printf("\nEl registro con dicho numero no existe");
			r = send(clientFd, &flag, sizeof(flag), 0);//se envia una señal para indicar al cliente si el registro existe
			if (r == -1){
				perror("error en send");
				exit(-1);}
			}

	free(currentAnimal);	
}


//Borra el resgistro de un paciente 
void borrar(int socket){
	
	int r;
	int clientFd = socket;
	int flag = 0;
	int registroAborrar = 0;	
	char name [32];
	char marcaBorrado [32] = "###";
	struct dogtype *currentAnimal = malloc(sizeof(struct dogtype));
	struct dogtype *animalAux = malloc(sizeof(struct dogtype));

	r = send(clientFd, &nRegistros, sizeof(nRegistros), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}

	r = recv(clientFd, &registroAborrar, sizeof(registroAborrar), 0);
		if (r == -1){
			perror("error en receive");
			exit(-1);}

//INICIO SECCION CRITICA
//JUSTIFICACION: Si se cambia la posicion del archivo antes de leer se leera el dato equivocado, lo que puede pasar si un hilo cambia la posicion en el archivo despues de un 'fseek' ejecutado por un primer hilo antes de que este ejecute el 'fread'	
 	
	int x = read(pipefd[0],&buffer,1);	
	if (x == -1){
		perror("error en read pipe");
		exit(-1);}

	//sem_wait (semaforo);	
	//pthread_mutex_lock(&mutex);

	fseek(registroNameFile, 32*sizeof(char)*registroAborrar, SEEK_SET);//se busca el nombre que le corresponde al numero de registro a borrar
	fread(&name, sizeof(name), 1, registroNameFile);

	//sem_post (semaforo);
	//pthread_mutex_unlock(&mutex);
	
	x = write(pipefd [1], &caracter, 1);
	if (x == -1){
		perror("error en write pipe");
		exit(-1);}	

//FIN SECCION CRITICA

//si el registro no ha sido ya eliminado, se procede a borrarlo

	if (strcmp( name, marcaBorrado) == 0){
		flag = 0;
		//printf("\nEl registro con dicho numero no existe");	
	}else {
		
//INICIO SECCION CRITICA
//JUSTIFICACION: Si se cambia la posicion del archivo antes de leer se leera el dato equivocado, lo que puede pasar si un hilo cambia la posicion en el archivo despues de un 'fseek' ejecutado por un primer hilo antes de que este ejecute el 'fread'	
 	
		x = read(pipefd[0],&buffer,1);	
		if (x == -1){
			perror("error en read pipe");
			exit(-1);}

		//sem_wait (semaforo);	
		//pthread_mutex_lock(&mutex);

		fseek(fp, 108*sizeof(char)*hashCode(name) , SEEK_SET);//se accede  a la lista en donde se encuentra el nombre
		fread(currentAnimal,sizeof(struct dogtype),1,fp);//se lee el primer nombre de la lista

		//sem_post (semaforo);
		//pthread_mutex_unlock(&mutex);
	
		x = write(pipefd [1], &caracter, 1);
		if (x == -1){
			perror("error en write pipe");
			exit(-1);}	

//FIN SECCION CRITICA

//se recorre la lista hasta encontrar el numero de registro que se desea borrar y se marca como borrado

		if(currentAnimal->noRegistro == registroAborrar){

			strcpy(animalAux->nombre, currentAnimal->nombre);		
			strcpy(animalAux->tipo, currentAnimal->tipo);
			animalAux->edad = currentAnimal->edad;	
			strcpy(animalAux->raza, currentAnimal->raza);
			animalAux->estatura = currentAnimal->estatura;		
			animalAux->peso = currentAnimal->peso;
			animalAux->sexo = currentAnimal->sexo;
			animalAux->noRegistro = currentAnimal->noRegistro;
			animalAux->next = currentAnimal->next;
			animalAux->borrado ='T';//borrado = True

			flag = 1;//el elemento ya se ha encontrado

//INICIO SECCION CRITICA
//JUSTIFICACION: Si se cambia la posicion del archivo antes de escribir se se escribira el dato en la posicion equivocada, lo que puede pasar si un hilo cambia la posicion en el archivo despues de un 'fseek' ejecutado por un primer hilo antes de que este ejecute el 'fwrite'	
 	
			x = read(pipefd[0],&buffer,1);	
			if (x == -1){
				perror("error en read pipe");
				exit(-1);}

			//sem_wait (semaforo);	
			//pthread_mutex_lock(&mutex);

		
			fseek(fp, 108*sizeof(char)*hashCode(name) , SEEK_SET);
			fwrite (animalAux,sizeof(struct dogtype),1,fp);//se reescribe la estrucutra ya marcada
		
			//sem_post (semaforo);
			//pthread_mutex_unlock(&mutex);
	
			x = write(pipefd [1], &caracter, 1);
			if (x == -1){
				perror("error en write pipe");
				exit(-1);}	

//FIN SECCION CRITICA
			}

		if(flag != 1){

			while(currentAnimal->next != -1){

//INICIO SECCION CRITICA
//JUSTIFICACION: Se lee y se escribe en el archivo compartido por todos los hilos	
	
				x = read(pipefd[0],&buffer,1);	
				if (x == -1){
					perror("error en read pipe");
					exit(-1);}			

				//pthread_mutex_lock(&mutex);
				//sem_wait (semaforo);
						
				fseek(fp, currentAnimal->next, SEEK_SET);
				int posicionCurrentPet=ftell(fp);
				fread(currentAnimal,sizeof(struct dogtype),1,fp);

				//sem_post (semaforo);
				//pthread_mutex_unlock(&mutex);
	
				x = write(pipefd [1], &caracter, 1);
				if (x == -1){
					perror("error en write pipe");
					exit(-1);}	

//FIN SECCION CRITICA			

				if(currentAnimal->noRegistro == registroAborrar){
				
					strcpy(animalAux->nombre, currentAnimal->nombre);		
					strcpy(animalAux->tipo, currentAnimal->tipo);
					animalAux->edad = currentAnimal->edad;	
					strcpy(animalAux->raza, currentAnimal->raza);
					animalAux->estatura = currentAnimal->estatura;		
					animalAux->peso = currentAnimal->peso;
					animalAux->sexo = currentAnimal->sexo;
					animalAux->noRegistro = currentAnimal->noRegistro;
					animalAux->next = currentAnimal->next;
					animalAux->borrado ='T';
		
//INICIO SECCION CRITICA
//JUSTIFICACION: Se escribe en el archivo compartido por todos los hilos	
	
					x = read(pipefd[0],&buffer,1);	
					if (x == -1){
						perror("error en read pipe");
						exit(-1);}			

					//pthread_mutex_lock(&mutex);
					//sem_wait (semaforo);
						

					fseek(fp, posicionCurrentPet, SEEK_SET);
					fwrite (animalAux,sizeof(struct dogtype),1,fp);

					//sem_post (semaforo);
					//pthread_mutex_unlock(&mutex);
	
					x = write(pipefd [1], &caracter, 1);
					if (x == -1){
						perror("error en write pipe");
						exit(-1);}	

//FIN SECCION CRITICA


					flag=1;
					break;//apenas se encuentra y se borra el registro solicitado se rompe el ciclo

					}				
	
				}

			}
		}

	if(flag == 0){
			//printf("\nEl registro con dicho numero no existe");
			}
		else{
//INICIO SECCION CRITICA
//JUSTIFICACION:varibles globales y se escribe en el archivo	
	
			int x = read(pipefd[0],&buffer,1);	
			if (x == -1){
				perror("error en read pipe");
				exit(-1);}	

			//sem_wait (semaforo);
			//pthread_mutex_lock(&mutex);
			nRegistros--;
			registrosBorrados++;
			
			fseek(registroNameFile, 32*sizeof(char)*registroAborrar , SEEK_SET);		
			strcpy(name, marcaBorrado);
			fwrite (&name, sizeof(name), 1, registroNameFile);// el numero de registro borrado se marca en el archivo que asocia numeros de registro con nombres		
			//printf("\nRegistro borrado");
			
			//pthread_mutex_unlock(&mutex);
			//sem_post (semaforo);

			x = write(pipefd [1], &caracter, 1);
			if (x == -1){
				perror("error en write pipe");
				exit(-1);}	

//FIN SECCION CRITICA

			}

//se envia una señal par indicar al cliente si el registro se borro o si no existia
	r = send(clientFd, &flag, sizeof(flag), 0); 
	if (r == -1){
		perror("error en send");
		exit(-1);}

//si el numero de registros borrados supera los 1000 se copia el archivo
	if(registrosBorrados>1000){
		//copiar-reconstruir dataDogs

//INICIO SECCION CRITICA
//JUSTIFICACION:variable global	
	
		x = read(pipefd[0],&buffer,1);	
		if (x == -1){
			perror("error en read pipe");
			exit(-1);}			

		//pthread_mutex_lock(&mutex);
		//sem_wait (semaforo);
		registrosBorrados=0;
		//pthread_mutex_unlock(&mutex);
		//sem_post (semaforo);

		x = write(pipefd [1], &caracter, 1);
		if (x == -1){
			perror("error en write pipe");
			exit(-1);}	

//FIN SECCION CRITICA
		}
	
	free(currentAnimal);		
	free(animalAux);
}



void buscar(int socket){
	
	int r;
	int clientFd = socket;
	char name [32];	
	struct dogtype *currentAnimal = malloc(sizeof(struct dogtype));
	int flag = 0;
	int print = 0;

	readString(clientFd, name, sizeof(name));

//INICIO SECCION CRITICA
//JUSTIFICACION: Si se cambia la posicion del archivo antes de leer, se leera el dato equivocado. Lo que puede pasar si un hilo cambia la posicion en el archivo despues de un 'fseek' ejecutado por un primer hilo antes de que este ejecute el 'fread'	
 	
	int x = read(pipefd[0],&buffer,1);	
	if (x == -1){
		perror("error en read pipe");
		exit(-1);}

	//sem_wait (semaforo);	
	//pthread_mutex_lock(&mutex);	
	
	fseek(fp, 108*sizeof(char)*hashCode(name), SEEK_SET);//se va a la posicion del archivo dataDogs en la que se encuentra la lista de estructuras con el codigo hash calculado a partir del nombre

	fread(currentAnimal,sizeof(struct dogtype),1,fp);

	//pthread_mutex_unlock(&mutex);
	//sem_post (semaforo);

	x = write(pipefd [1], &caracter, 1);
	if (x == -1){
		perror("error en write pipe");
		exit(-1);}	

//FIN SECCION CRITICA	

//se recorre la lista y se imprimen solo las estructuras que tienen el nombre buscado
//cada vez que se encuentra una estructura con el nombre especificado se envian uno a uno los datos de cada animal
	if(strcmp(name,currentAnimal->nombre) == 0 && currentAnimal->borrado != 'T'){
		//se envia una señal al cliente para anunciarle que se van a enviar los datos de un animal
		print = 1;
		r = send(clientFd, &print, sizeof(print), 0);
		if (r == -1){
			perror("error en send");
			exit(-1);}

		r = send(clientFd, currentAnimal->nombre, sizeof(currentAnimal->nombre), 0);
		if (r == -1){
			perror("error en send");
			exit(-1);}
		r = send(clientFd, currentAnimal->tipo, sizeof(currentAnimal->tipo), 0);
		if (r == -1){
			perror("error en send");
			exit(-1);}
		r = send(clientFd, &currentAnimal->edad, sizeof(currentAnimal->edad), 0);
		if (r == -1){
			perror("error en send");
			exit(-1);}
		r = send(clientFd, currentAnimal->raza, sizeof(currentAnimal->raza), 0);
		if (r == -1){
			perror("error en send");
			exit(-1);}
		r = send(clientFd, &currentAnimal->estatura, sizeof(currentAnimal->estatura), 0);
		if (r == -1){
			perror("error en send");
			exit(-1);}
		r = send(clientFd, &currentAnimal->sexo, sizeof(currentAnimal->sexo), 0);
		if (r == -1){
			perror("error en send");
			exit(-1);}
		r = send(clientFd, &currentAnimal->peso, sizeof(currentAnimal->peso), 0);
		if (r == -1){
			perror("error en send");
			exit(-1);}
		r = send(clientFd, &currentAnimal->noRegistro, sizeof(currentAnimal->noRegistro), 0);
		if (r == -1){
			perror("error en send");
			exit(-1);}					

		
		flag = 1;	
		}

	while(currentAnimal->next != -1){

//INICIO SECCION CRITICA
//JUSTIFICACION: Si se cambia la posicion del archivo antes de leer, se leera el dato equivocado. Lo que puede pasar si un hilo cambia la posicion en el archivo despues de un 'fseek' ejecutado por un primer hilo antes de que este ejecute el 'fread'	
 	
		x = read(pipefd[0],&buffer,1);	
		if (x == -1){
			perror("error en read pipe");
			exit(-1);}

		//sem_wait (semaforo);	
		//pthread_mutex_lock(&mutex);			
						
		fseek(fp, currentAnimal->next, SEEK_SET);
		fread(currentAnimal,sizeof(struct dogtype),1,fp);

		//pthread_mutex_unlock(&mutex);
		//sem_post (semaforo);

		x = write(pipefd [1], &caracter, 1);
		if (x == -1){
			perror("error en write pipe");
			exit(-1);}	

//FIN SECCION CRITICA	

		
		if(strcmp(name,currentAnimal->nombre) == 0 && currentAnimal->borrado != 'T'){
			print = 1;
			r = send(clientFd, &print, sizeof(print), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}

			r = send(clientFd, currentAnimal->nombre, sizeof(currentAnimal->nombre), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, currentAnimal->tipo, sizeof(currentAnimal->tipo), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, &currentAnimal->edad, sizeof(currentAnimal->edad), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, currentAnimal->raza, sizeof(currentAnimal->raza), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, &currentAnimal->estatura, sizeof(currentAnimal->estatura), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, &currentAnimal->sexo, sizeof(currentAnimal->sexo), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, &currentAnimal->peso, sizeof(currentAnimal->peso), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}
			r = send(clientFd, &currentAnimal->noRegistro, sizeof(currentAnimal->noRegistro), 0);
			if (r == -1){
				perror("error en send");
				exit(-1);}					
		

			flag = 1;
			}	
		
	}
//si print es igual a 0 no se enviaran mas datos
	print = 0;
	r = send(clientFd, &print, sizeof(print), 0);
	if (r == -1){
	perror("error en send");
	exit(-1);}
//se informa al cliente si el nombre no fue encontrado
	r = send(clientFd, &flag, sizeof(flag), 0);
	if (r == -1){
	perror("error en send");
	exit(-1);}
/*
	if (flag==0){
		printf("\nNombre no encontrado\n");}
*/
	free(currentAnimal);
	
}


void * atenderCliente(void *arg){

	struct arg_struct *argumentos = (struct arg_struct *)arg;
	int socket = argumentos->sock;//descriptor del socket del cliente 
	struct in_addr dir = argumentos->dir; //direccion del cliente
	char strIp[INET_ADDRSTRLEN];
	inet_ntop( AF_INET, &dir, strIp, INET_ADDRSTRLEN );//se convierte la direccion a string

	struct log *log;	
	time_t current_time;
	char c_time_string[26];	

	log = malloc(sizeof(struct log));	
	
	char name[32] = "xxx";	

	int a;	
	a = recibirInstruccion(socket);
	while (a != 5){
	switch(a)	
	{
	case 1:
	/* Obtain current time. */
	current_time = time(NULL);	
	strcpy(c_time_string, ctime(&current_time));	
	strcpy(log->dateTime, c_time_string);
	strcpy(log->ip, strIp);	
	strcpy(log->command, "Insercion");
	strcpy(log->name, name);

	fwrite (log,sizeof(struct log),1,fplogs);//se regsitra el log de la operacion

	ingresar(socket);
	a = recibirInstruccion(socket);	
	break;
	case 2:
	current_time = time(NULL);	
	strcpy(c_time_string, ctime(&current_time));	
	strcpy(log->dateTime, c_time_string);
	strcpy(log->ip, strIp);	
	strcpy(log->command, "Lectura");
	strcpy(log->name, name);

	fwrite (log,sizeof(struct log),1,fplogs);	

	ver(socket);
	a = recibirInstruccion(socket);
	break;
	case 3:
	current_time = time(NULL);	
	strcpy(c_time_string, ctime(&current_time));	
	strcpy(log->dateTime, c_time_string);
	strcpy(log->ip, strIp);	
	strcpy(log->command, "Borrado");
	strcpy(log->name, name);

	fwrite (log,sizeof(struct log),1,fplogs);

	borrar(socket);
	a = recibirInstruccion(socket);
	break;
	case 4:
	current_time = time(NULL);	
	strcpy(c_time_string, ctime(&current_time));	
	strcpy(log->dateTime, c_time_string);
	strcpy(log->ip, strIp);	
	strcpy(log->command, "Busqueda");
	strcpy(log->name, name);

	fwrite (log,sizeof(struct log),1,fplogs);

	buscar(socket);
	a = recibirInstruccion(socket);
	break;
	case 5:	
	close(socket);//cuando el usuario decide salir se cierra el descriptor del socket del cliente
	break;
	} 
	}
	}

int recibirInstruccion(int socket){
	int a;
	int r;
	r = recv(socket, &a, sizeof(a), 0);//recibe las instrucciones de cada cliente
		if (r == -1){
		perror("error en receive");
		exit(-1);}
	return a;
	}
//ejecuta las tareas necesarias para cerrar el programa servidor correctamente cuando se produce la interrupcion
void shutdownHandler(int sig) {
	//guardar datos
	fseek(tailsFile, 0, SEEK_SET);
	fwrite (tails,sizeof(tails),1,tailsFile);
	fseek(tailsFile, sizeof(int)*1266, SEEK_SET);
	fwrite (&registrosBorrados,sizeof(int),1,tailsFile);
	fseek(tailsFile, sizeof(int)*1267, SEEK_SET);
	fwrite (&nRegistros,sizeof(int),1,tailsFile);
	fseek(tailsFile, sizeof(int)*1268, SEEK_SET);
	fwrite (&noRegistroAsignar,sizeof(int),1,tailsFile);
	fclose(fp);
	fclose(registroNameFile);
	fclose(tailsFile);

	int r;

	for (int i=0; i<threadNumber; i++){
		r= pthread_join(hilos[i], NULL);
		//validar		
		}
	
	fclose(fplogs);
	close(servFd);

	//sem_close (semaforo);
	//sem_unlink("semaforo1");

	//pthread_mutex_destroy(&mutex);

	close (pipefd [0]);
	close (pipefd [1]);

	exit(0);
}

int main(){

	signal(SIGINT, shutdownHandler);//funcion que esta pendiente de cuando se oprime Ctrl+C
	//cargar datos:
	tailsFile = fopen("tails.bin", "r+");//archivo que guarda la posicion de cada una de las colas de cada lista de la tabla hash
	fread(tails, sizeof(tails), 1, tailsFile);//se lee y se guarda en un arreglo para trabajar desde el 
	fseek(tailsFile, sizeof(int)*1266, SEEK_SET);
	fread(&registrosBorrados, sizeof(int), 1, tailsFile);
	fseek(tailsFile, sizeof(int)*1267, SEEK_SET);
	fread(&nRegistros, sizeof(int), 1, tailsFile);
	fseek(tailsFile, sizeof(int)*1268, SEEK_SET);
	fread(&noRegistroAsignar, sizeof(int), 1, tailsFile);

	fplogs = fopen("serverDogs.log", "w+");
	
	//108 = cantidad de bytes ocupada por una estructura
	fp = fopen("dataDogs.dat", "r+" );//archivo en donde se encuentran las estructuras
	registroNameFile = fopen("registroName.bin", "r+");//archivo que asocia numero de registro con nombre correspondiente
	
	//semaforo = sem_open("semaforo1", O_CREAT, 0700, SEMINIT);	

	int x = pipe(pipefd);
	if (x == -1){
		perror("error en pipe");
		exit(-1);}
	
	x = write(pipefd [1], &caracter, 1);
	if (x == -1){
		perror("error en wirte en pipe");
		exit(-1);}	
	
	int r;
	int opt=1;//opcion para activar reuso del socket
	char buff[32];
	socklen_t tamano;	
	socklen_t tamano2;	
	servFd= socket(AF_INET, SOCK_STREAM, 0);
	if(servFd < 0){
        perror("error en socket");
        exit(-1);}
//se activa el reuso del socket del servidor, aunque no es necesario ya que se tiene shutdownHandler
	if (setsockopt(servFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		perror("setsockopt");
        	exit(-1);
		}
	server.sin_family = AF_INET;
	server.sin_port = htons(3535);
	server.sin_addr.s_addr = INADDR_ANY;
	bzero(server.sin_zero,8);
	tamano = sizeof(struct sockaddr_in);
	r=bind(servFd, (struct sockaddr *) &server, tamano);
	if (r == -1){
	perror("error en bind");
	exit(-1);}

	r=listen(servFd, BACKLOG);
	if (r == -1){
	perror("error en listen");
	exit(-1);}
	tamano2 = 0;

	int clientFd;	
	
	while(clientFd = accept(servFd, (struct sockaddr *) &cliente, &tamano2)){
          
	struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&cliente;
	struct in_addr ipAddr = pV4Addr->sin_addr;	
	struct arg_struct *args = malloc(sizeof(struct arg_struct)); 
	args->sock = clientFd;
	args->dir = ipAddr;//direccion del cliente         	      
         //cada vez que se acepta un cliente se crea un nuevo hilo que se encarga de atender a cada cliente
        if(pthread_create( &hilos[threadNumber] , NULL ,  atenderCliente , (void*) args) < 0){
        perror("error en pthread_create");
	exit(-1);
        }	
        threadNumber++;
        
        }	
	
	if(clientFd < 0){
        perror("error en accept");
        exit(-1);}
	

}
