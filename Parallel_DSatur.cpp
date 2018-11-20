#include<stdio.h>
#include<stdlib.h>
#include <time.h>
#include "mpi.h"
//Creamos la matriz que representa el grafo
int **graph=NULL;
int *colored=NULL;
int *recvArray=NULL;
int colors=1;
int num=0;
int	taskid,    // ID del procesador */
	numtasks,    // Numero de procesadores a usar
	start,
	size,
	actNode;
void splitWork();
int getNodeGrade(int index);
int getMaxGradeNode();
bool isColored(int index);
void setColor(int index);
int getSaturation(int index);
void getMaxSaturation();
void loadFile(char[]);

int main(int argc, char *argv[]){
	/* Inicializa MPI */
	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
	MPI_Comm_size(MPI_COMM_WORLD,&numtasks);

	char c[]="graphs/tam.txt";
	loadFile(c);
	//Asina la porcion del arreglo a las tareas
	splitWork();

	if(taskid==0){
		//Imprimimos el grafo
	 	for(int i = 0; i < num; i++){
	 		for (int j = 0; j < num; ++j)
	 			printf("%d ",graph[i][j]);
	    printf("\n");
	 	}

	  //Algoritmo DSatur
	  //1. Seleccionar el nodo inicial (nodo con mallor grado)
	  actNode=getMaxGradeNode();
	  //2. Asignar el color de menor grado al nodo
	  setColor(actNode);
	  printf("Nodo Actual: %d Color: %d\n",actNode,colored[actNode]);
	}

	int visited=1;
	//Broadcast actual Node
	MPI_Bcast(&actNode,1,MPI_INT,0,MPI_COMM_WORLD);
	//Broadcast arregl de coloreo
	MPI_Bcast(colored,num,MPI_INT,0,MPI_COMM_WORLD);

  //3. Iterar hasta colorearlos todos
  while (visited<num) {
  /*
  4. Seleccionar el nodo no coloreado con
     el mayor indice de saturacion, en caso
     de un empate, tomar el de mayor grado
  */
    getMaxSaturation();
		if(taskid==0){
	    //5. Asignar color
	    setColor(actNode);
	    printf("Nodo Actual: %d Color: %d\n",actNode,colored[actNode]);
		}
		//Broadcast actual Node
		MPI_Bcast(&actNode,1,MPI_INT,0,MPI_COMM_WORLD);
		//Broadcast arregl de coloreo
		MPI_Bcast(colored,num,MPI_INT,0,MPI_COMM_WORLD);
		//Broadcast colors
		MPI_Bcast(&colors,1,MPI_INT,0,MPI_COMM_WORLD);
    //6. Marcarlo como visitado
    visited++;
  }
	if(taskid==0)
  	printf("Numero minimo de colores: %d\n",colors);

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
	free(graph);
	free(colored);
	return 0;
}
//Carga el archivo
void loadFile(char c[]){
	//Creamos un puntero de tipo FILE
	FILE *fp;
	//Abrimos el archivo a leer
 	if((fp = fopen (c, "r" ))==NULL){
 		printf("No se pudo leer el archivo\n");
 		//return 0;
 	}
	//leemos los datos
 	fscanf(fp, "%d" ,&num);
  colored= (int*)malloc(num*sizeof(int*));
 	graph = (int**)malloc(num*sizeof(int*));
	// Columnas
	for(int i=0;i<num;i++){
		graph[i] = (int*)malloc(num*sizeof(int));
		// Inicializar la matriz en ceros
		for(int j=0;j<num;j++){
			graph[i][j]=0;
		}
    colored[i]=0;
	}
	int a,b;
 	//Leemos las aristas de cada vertice
 	while(feof(fp)==0){
 		fscanf(fp,"%d %d",&a,&b);
 		graph[a-1][b-1]=1;
    graph[b-1][a-1]=1;
 	}
}
//obtiene el grado de un nodo
int getNodeGrade(int index){
  int grade=0;
  for(int i=0;i<num;i++){
    if(graph[index][i]!=0)
      grade++;
  }
  return grade;
}
void splitWork(){
	int nmin, nleft, nnum;
	//Define el tamaño de particion
	nmin=num/numtasks;
	nleft=num%numtasks;
	int k=0;
	for (int i = 0; i < numtasks; i++) {
		 nnum = (i < nleft) ? nmin + 1 : nmin;
		 if(i==taskid){
			 start=k;
			 size=nnum;
			 printf ("tarea=%2d  Inicio=%2d  tamaño=%2d \n", taskid,start, size);
		 }
	k+=nnum;
	}
}
//Verfica si un nodo esta coloreado
bool isColored(int index){
  if(colored[index]!=0)
    return true;
  else
    return false;
}
//obtiene el nodo no coloreado con mayor grado
int getMaxGradeNode(){
  int max=0; int index=0;
  for(int i=0;i<num;i++){
    int aux=getNodeGrade(i);
    if(!isColored(i)&&aux>max){
        max=aux;
        index=i;
    }
  }
  return index;
}
//Asiga el color a un nodo
void setColor(int index){
  bool flag=true;
  int colorIndex=1;
  while(flag){
    int count=0;
    for(int i=0;i<num;i++){
      if(graph[index][i]==1 && colored[i]==colorIndex)
        count++;
    }
    if(count==0){
      flag=false;
    }else{
      colorIndex++;
    }
  }
  colored[index]=colorIndex;
  if(colorIndex>colors)
    colors=colorIndex;
}
//Obtiene el grado de saturacion de un nodo
int getSaturation(int index){
  int count=0;
  for(int i=1;i<=colors;i++){
    for(int j=0;j<num;j++){
      if(graph[index][j]==1 && colored[j]==i){
        count++;
        j=num;
      }
    }
  }
  return count;
}
void getMaxSaturation(){
	//Cada proceo calcula el maxmo de saturacion local
	if(taskid==0){
		recvArray=(int*)malloc(numtasks*3*sizeof(int));
	}
  int maxSat=0; int index=0; int grade=0;
  for(int i=start;i<start+size;i++){
    if(colored[i]==0){
      int aux=getSaturation(i);
      if(aux > maxSat){
        maxSat=aux;
        index=i;
				grade=getNodeGrade(index);
      }else if(aux==maxSat){
        if(getNodeGrade(i)>grade)
          index=i;
					grade=getNodeGrade(index);
      }
    }
  }
	int result[3];
		result[0]=maxSat;
		result[1]=grade;
		result[2]=index;
		//Los resultados de cada proceso se colectan por el proceso maestro
	MPI_Gather(
    result,	//Arreglo a enviar
    3,		//Numero de elem a enviar
    MPI_INT,		//Tipo de dato
    recvArray,	//Arreglo donde se recibe
    3,		//Numero de elementos a recibir por cada proceso
    MPI_INT, //Tipo de dato
    0,	//Tarea que va a recibir
    MPI_COMM_WORLD);	//Comunicador
		//El proceso maestro optiene la saturacion maxima global
	if(taskid==0){
		maxSat=0; index=-1; grade=0;
		for(int i=0;i<numtasks*3;i+=3){
			if(recvArray[i]>maxSat){
				maxSat=recvArray[i];
				grade=recvArray[i+1];
				index=recvArray[i+2];
			}else if(recvArray[i]==maxSat){
				if(recvArray[i+1]>grade){
					maxSat=recvArray[i];
					grade=recvArray[i+1];
					index=recvArray[i+2];
				}
			}
		} //Se escoge el siguiente nodo actual
		actNode=index;
	}
}
