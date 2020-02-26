#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <time.h>
#include <ctype.h>

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

int hashCode(char a[32]){
	int i=0;
	int hash = 0, c = 0;
	while (a[i] != '\0'){
 		c = toascii(a[i]);
		hash = hash + c;
		i++;}
	hash = hash % 1265;
	return (hash);
}


int main() 
{
	struct dogtype *animal;	 	
	
	int tails[1265]={};//ubicacion del ultimo elemento de cada lista enlazada
	char names[1717][32];//arreglo de apuntadores a char para guardar todos los nombres que se pueden asignar
	

	FILE *file;
	file = fopen("nombresMascotas.txt", "r");
	
	FILE *fp;
	fp = fopen("dataDogs.dat", "w+" );
	FILE *registroNameFile;
	registroNameFile = fopen("registroName.bin", "w+");	
	FILE *tailsFile;
	tailsFile = fopen("tails.bin", "w+");
	
    	for(int i=0; i<1717; i++) {		
		fscanf(file,"%s", names[i]); 		
		}	 

	fclose(file); 

	//se crean los siguientes arreglos para generar aleatoreamente los datos de cada estructura
	const char types[5][32]={"perro", "gato", "hamster", "torutga", "pez"};
	const char races[5][16]={"dalmata", "chiguagua", "pitbull", "beagle", "pastor aleman"};
	const char genders[2]={'H','M'};	
	float a = 100.0;	

	srand(time(NULL)); 
	int rint; 	
	float rfloat;

	animal=malloc(sizeof(struct dogtype));
	
	
	for(int i=0; i<1717; i++) {
		
		tails[hashCode(names[i])]=108*sizeof(char)*hashCode(names[i]);//al comienzo el ultimo elemento de cada lista es el 			primero			

		strcpy(animal->nombre, names[i]);		
		rint = (rand() % 5);
		strcpy(animal->tipo, types[rint]);		
		rint = (rand() % 51);		
		animal->edad =	rint;
		rint = (rand() % 5);
		strcpy(animal->raza, races[rint]);		
		rint = (rand() % 61);
		animal->estatura = rint;
		rfloat = ((float)rand() / (float)(RAND_MAX)) * a;
		animal->peso = rfloat;	
		rint = (rand() % 2);
		animal->sexo =	genders[rint];
		animal->noRegistro = i+1;			
		fseek(registroNameFile, 32*sizeof(char)*animal->noRegistro , SEEK_SET);
		fwrite (animal->nombre,32*sizeof(char),1,registroNameFile);		
		animal->next=-1;
		animal->borrado='F';

		fseek(fp, 108*sizeof(char)*hashCode(names[i]) , SEEK_SET);
		fwrite (animal,sizeof(struct dogtype),1,fp);
			
		}

	struct dogtype *currentAnimal;
	struct dogtype *animalAux;
	animalAux=malloc(sizeof(struct dogtype));
	currentAnimal=malloc(sizeof(struct dogtype));	

	
	for(int i=1717; i<10000000; i++) { 		

		rint = (rand() % 1717);
		int indexNameRamdom = rint;	
		
		fseek(fp, 0, SEEK_END);//todos los nuevos registros se agregan al final del archivo
		int dirNext = ftell(fp);//la direccion del final del archivo es la ubicacion de la nueva cola de la lista correspondiente
		
		strcpy(animal->nombre, names[indexNameRamdom]);		
		rint = (rand() % 5);
		strcpy(animal->tipo, types[rint]);		
		rint = (rand() % 51);		
		animal->edad =	rint;
		rint = (rand() % 5);
		strcpy(animal->raza, races[rint]);		
		rint = (rand() % 61);
		animal->estatura = rint;
		rfloat = ((float)rand() / (float)(RAND_MAX)) * a;
		animal->peso = rfloat;	
		rint = (rand() % 2);
		animal->sexo =	genders[rint];
		animal->noRegistro = i+1;		
		fseek(registroNameFile, 32*sizeof(char)*animal->noRegistro , SEEK_SET);
		fwrite (animal->nombre,32*sizeof(char),1,registroNameFile);		
		animal->next=-1;
		animal->borrado='F';
		
		fwrite (animal,sizeof(struct dogtype),1,fp);		
		fseek(fp, tails [hashCode(names[indexNameRamdom])], SEEK_SET);//ir a la posicion de la actual cola para cambiar el atributo next		
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
	
		fseek(fp, tails [hashCode(names[indexNameRamdom])] , SEEK_SET);	
		fwrite (animalAux,sizeof(struct dogtype),1,fp);
		tails [hashCode(names[indexNameRamdom])]=dirNext;//actualizar la ubicacion de la cola			
		
	}
	
	fseek(tailsFile, 0, SEEK_SET);
	fwrite (tails,sizeof(tails),1,tailsFile);
	fseek(tailsFile, sizeof(int)*1266, SEEK_SET);
	int registrosBorrados=0;
	fwrite (&registrosBorrados,sizeof(int),1,tailsFile);
	fseek(tailsFile, sizeof(int)*1267, SEEK_SET);
	int nRegistros=10000000;
	fwrite (&nRegistros,sizeof(int),1,tailsFile);
	fseek(tailsFile, sizeof(int)*1268, SEEK_SET);
	int noRegistroAsignar=10000000;
	fwrite (&noRegistroAsignar,sizeof(int),1,tailsFile);		

	free(animal);		
	free(animalAux);

	//prueba
/*
char nameAux [32];
for(int i=0; i<17; i++) {	
		
	fseek(fp, 108*sizeof(char)*hashCode(names[i]), SEEK_SET);
	printf("tail: %i\n", tails[hashCode(names[i])]);
	printf("ftell: %lu\n", ftell(fp));	
	fread(currentAnimal,sizeof(struct dogtype),1,fp);
		printf("edad: %i\n", currentAnimal->edad);
		printf("nombre: %s\n", currentAnimal->nombre);
		printf("hashCode: %i\n", hashCode(currentAnimal->nombre));
		printf("raza: %s\n", currentAnimal->raza);
		printf("tipo: %s\n", currentAnimal->tipo);
		printf("genero: %c\n", currentAnimal->sexo);
		printf("estatura: %i\n", currentAnimal->estatura);
		printf("peso: %f\n", currentAnimal->peso);
		printf("noRegistro: %i\n", currentAnimal->noRegistro);	
		printf("next: %i\n", currentAnimal->next);
		printf("borrado: %c\n", currentAnimal->borrado);	
		
		fseek(registroNameFile, 32*sizeof(char)*currentAnimal->noRegistro , SEEK_SET);
		fread (&nameAux,32*sizeof(char),1,registroNameFile);
		printf("nombreReg: %s\n", nameAux);

	
		
	while(currentAnimal->next != -1){
		//// currentAnimal->next					
		fseek(fp, currentAnimal->next, SEEK_SET);
		fread(currentAnimal,sizeof(struct dogtype),1,fp);
		printf("edad: %i\n", currentAnimal->edad);
		printf("nombre: %s\n", currentAnimal->nombre);
		printf("hashCode: %i\n", hashCode(currentAnimal->nombre));
		printf("raza: %s\n", currentAnimal->raza);
		printf("tipo: %s\n", currentAnimal->tipo);
		printf("genero: %c\n", currentAnimal->sexo);
		printf("estatura: %i\n", currentAnimal->estatura);
		printf("peso: %f\n", currentAnimal->peso);
		printf("noRegistro: %i\n", currentAnimal->noRegistro);	
		printf("next: %i\n", currentAnimal->next);
		printf("borrado: %c\n", currentAnimal->borrado);	
		
		fseek(registroNameFile, 32*sizeof(char)*currentAnimal->noRegistro , SEEK_SET);
		fread (&nameAux,32*sizeof(char),1,registroNameFile);
		printf("nombreReg: %s\n", nameAux);
	}


}
*/
	free(currentAnimal);		
	fclose(fp);
	fclose(registroNameFile);
	fclose(tailsFile);	
}
