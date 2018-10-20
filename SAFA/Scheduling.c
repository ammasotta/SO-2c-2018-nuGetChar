#include "incs/Scheduling.h"

////////////////////////////////////////////////////////////

void InitQueuesAndLists()
{

	NEWqueue = queue_create();
	READYqueue = queue_create();
	BLOCKEDqueue = list_create();
	EXECqueue = list_create();
	EXITqueue = list_create();

}

void InitSemaphores()
{

	pthread_mutex_init(mutexPLPtask, NULL);
	pthread_mutex_init(mutexPCPtask, NULL);
	sem_init(&workPLP, 0, 0);

}

void CreateDummy()
{

	dummyDTB = (DTB*) malloc(sizeof(DTB));

	dummyDTB->id = 0;
	dummyDTB->initialized = 0;
	dummyDTB->programCounter = 0;
	dummyDTB->status = -1;								//Valor basura, todavia no esta en ninguna cola

	return;

}

void InitGlobalVariables()
{

	nextID = 1;										//Arrancan en 1, la 0 es reservada para el Dummy

	InitSemaphores();
	InitQueuesAndLists();

	//Al ser extern en main, deberian inicializarse ahi mismo
	toBeCreated = (CreatableGDT*) malloc(sizeof(CreatableGDT));
	toBeCreated->script = malloc(1);				//Medio paragua, para poder hacer reallocs

	CreateDummy();									//Malloceo y creo el DummyDTB

	return;

}

void SetPLPTask(int taskCode)
{

	pthread_mutex_lock(&mutexPLPtask);
	PLPtask = taskCode;
	pthread_mutex_unlock(&mutexPLPtask);
	sem_post(&workPLP);

}

void SetPCPTask(int taskCode)
{

	pthread_mutex_lock(&mutexPCPtask);
	PCPtask = taskCode;
	pthread_mutex_unlock(&mutexPCPtask);

}

////////////////////////////////////////////////////////////

int showOptions()
{

	int chosen;
	do
	{
		printf("------Consola del Gestor de Programas G.DT!!------\n");
		printf("Seleccione la operacion a realizar\n");
		printf("1. Ejecutar\n");
		printf("2. Status\n");
		printf("3. Finalizar\n");
		printf("4. Metricas\n");
		scanf("%d", &chosen);
	} while(chosen != 1);		//Por ahora, solo probamos con ejecutar
	return chosen;

}

void GestorDeProgramas()
{

	int chosenOption;
	while(1)
	{

		chosenOption = ShowOptions();
		char* path;

		if(chosenOption == 1)
		{
			printf("Ahora, ingrese el path del Escriptorio a ejecutar\n");
			scanf("%s", path);
			int pathLength = strlen(path) + 1;

			//Realloco el toBeCreated, porque su char* cambio; y tambien realloco este
			toBeCreated = realloc(toBeCreated, sizeof(int) + pathLength);
			toBeCreated->script = realloc(toBeCreated->script, pathLength);
			strcpy(toBeCreated->script, path);

			//Actualizo la tarea a realizar del PLP, y activo el semaforo binario
			SetPLPTask(PLP_TASK_CREATE_DTB);
		}

	}

}

////////////////////////////////////////////////////////////

DTB* CreateDTB(char* script)
{

	int dtbSize = (sizeof(int) * 5) + strlen(script) + 1;		//Medio cabeza, mejorar; 5 por los 5 enteros fijos
	DTB* newDTB = (DTB*) malloc(dtbSize);

	newDTB->id = nextID++;
	//Le pongo 0 para saber que no lo inicialice; nunca llegara a PCP con 0, solo el Dummy lo haria
	newDTB->initialized = 0;
	newDTB->pathEscriptorio = malloc(strlen(script) + 1);
	strcpy(newDTB->pathEscriptorio, script);
	newDTB->programCounter = 0;
	//Le pongo un valor basura, para su primera ejecucion (por si fuera con VRR)
	newDTB->quantumRemainder = -1;
	newDTB->status = -1;										//Valor basura, todavia no esta en NEW

	return newDTB;

}

void AddToNew(DTB* myDTB)
{

	myDTB->status = DTB_STATUS_NEW;
	//Todavia no prendo el flag initialized, falta la operacion Dummy
	list_add(NEWqueue, myDTB);

}

void AddToReady(DTB* myDTB)
{

	myDTB->status = DTB_STATUS_READY;
	queue_push(READYqueue, myDTB);

}

void AddToBlocked(DTB* myDTB)
{

	myDTB->status = DTB_STATUS_BLOCKED;
	list_add(BLOCKEDqueue, myDTB);

}

void AddToExec(DTB* myDTB)
{

	myDTB->status = DTB_STATUS_EXEC;
	list_add(EXECqueue, myDTB);

}

bool IsDTBtoBeInitialized(DTB* myDTB)
{

	if(myDTB->id == toBeCreated->dtbID)
	{
		return true;
	}
	else
	{
		return false;
	}

}

bool IsInitialized(DTB* myDTB)
{

	if(myDTB->initialized == 1)
	{
		return true;
	}
	else
	{
		return false;
	}

}

bool IsDummy(DTB* myDTB)
{

	if(myDTB->id == 0)
	{
		return true;
	}
	else
	{
		return false;
	}

}

bool IsToBeMoved(DTB* myDTB)
{

	if(myDTB->id == toBeMoved.dtbID)
	{
		return true;
	}
	else
	{
		return false;
	}

}

DTB* GetNextDTB()
{

	DTB* nextToExecute;
	nextToExecute = queue_pop(READYqueue);
	return nextToExecute;

}

////////////////////////////////////////////////////////////

void PlanificadorLargoPlazo(void* gradoMultiprogramacion)
{
	int multiprogrammingDegree = (int) gradoMultiprogramacion;

	while(1)
	{

		sem_wait(&workPLP);

		if(PLPtask == PLP_TASK_NORMAL_SCHEDULE)
		{

			int inMemoryAmount = queue_size(READYqueue) + list_size(BLOCKEDqueue) + list_size(EXECqueue);
			//Descuento el dummy de la cantidad de procesos en memoria (no afecta para el grado de multiprogramacion)
			//No lo descuento si su estado es basura (esto solo pasaria si aun no se lo utilizo)
			if(dummyDTB->status > 0)
			{
				inMemoryAmount--;
			}

			//Si el grado de multiprogramacion no lo permite, o no hay procesos en NEW, voy a la proxima iteracion del while
			if(inMemoryAmount >= multiprogrammingDegree || list_is_empty(NEWqueue))
			{
				sleep(3);				//Retardo ficticio, para debuggear; puede servir, para esperar
				continue;
			}

			//Si el Dummy esta en estado BLOCKED (no en la cola, sino esperando a ser asignado), agarro el primero de la cola
			if(dummyDTB->status == DTB_STATUS_BLOCKED)
			{
				DTB* queuesFirst = list_remove(NEWqueue, 1);

				//Si el primero no esta inicializado, es porque debo avisar al PCP que haga la operacion Dummy
				if(queuesFirst->initialized == 0)
				{

					//Devuelvo el que saque a NEW
					AddToNew(queuesFirst);

					//Realloco el dummyDTB, porque su char* cambio; cargo el path del DTB a inicializar (no lo hace el PCP)
					int pathLength = strlen(queuesFirst->pathEscriptorio) + 1;
					int newSize = (sizeof(int) * 5) + pathLength;		//Medio cabeza, mejorar; 5 por los 5 enteros fijos
					dummyDTB = realloc(dummyDTB, newSize);
					dummyDTB->pathEscriptorio = realloc(dummyDTB->pathEscriptorio, pathLength);
					strcpy(dummyDTB->pathEscriptorio, queuesFirst->pathEscriptorio);

					//Le aviso al PCP que debe hacer la tarea de LOAD_DUMMY (pasarlo a READY, nada mas)
					SetPCPTask(PCP_TASK_LOAD_DUMMY);

				}
				//Si el primero esta inicializado, lo agrego a READY; no aviso al PCP, ya deberia estar planificando (?)
				else if(queuesFirst->initialized == 1)
				{
					AddToReady(queuesFirst);
				}
			}

			//Si el Dummy esta en READY o en EXEC, es porque no puede usarse para ningun DTB a inicializar
			//Por eso, agarro el primero que ya este inicializado (que posea el flag en 1)
			else if(dummyDTB->status == DTB_STATUS_READY || dummyDTB->status == DTB_STATUS_EXEC)
			{

				//Si no hay ninguno inicializado, no hay nada que hacer, voy a la siguiente iteracion (bloqueo en el semaforo)
				if(!list_any_satisfy(NEWqueue, IsInitialized))
				{
					sleep(3);
					continue;
				}
				DTB* firstInitialized = list_remove_by_condition(NEWqueue, IsInitialized);
				AddToReady(firstInitialized);

			}

		}

		else if(PLPtask == PLP_TASK_CREATE_DTB)
		{
			DTB* newDTB;
			newDTB = CreateDTB(toBeCreated->script);
			AddToNew(newDTB);
			//Vuelvo a poner la tarea del PLP en Planificacion Normal (1)
			SetPLPTask(PLP_TASK_NORMAL_SCHEDULE);
		}

		else if(PLPtask == PLP_TASK_INITIALIZE_DTB)
		{
			//Busco el DTB que tiene el mismo ID que indicaba el Dummy; le pongo el flag en 1
			//Para ello, debo sacarlo de la lista, modificarlo, y ponerlo al final de nuevo (es una cola)
			DTB* toBeInitialized = list_remove_by_condition(NEWqueue, IsDTBtoBeInitialized);
			toBeInitialized->initialized = 1;
			AddToNew(toBeInitialized);
			//Vuelvo a poner la tarea del PLP en Planificacion Normal (1)
			SetPCPTask(PCP_TASK_NORMAL_SCHEDULE);
			SetPLPTask(PLP_TASK_NORMAL_SCHEDULE);
		}

	}

}

////////////////////////////////////////////////////////////

void PlanificadorCortoPlazo(void* algoritmo)
{

	while(1)
	{

		if(PCPtask == PCP_TASK_NORMAL_SCHEDULE)
		{

			//Si no hay ningun CPU libre, espero dos segundos y salgo del ciclo
			//O deberia quedarme a esperar que haya uno libre? Puede entrar un pedido por consola...
			if(!ExistsIdleCPU())
			{
				sleep(2);
				continue;
			}

			//Agarro el primer CPU libre que haya, lo saco de la lista y lo pongo aca
			CPU* chosenCPU = list_remove_by_condition(cpus, IsIdle);

			//Elegir el DTB adecuado, y obtener el mensaje a enviarle al CPU; ponerlo en EXEC
			//if(algoritmo == "RR") => scheduleRR(quantum)
			//if(algoritmo == "VRR") => scheduleVRR(quantum)
			//if(algoritmo == "PROPIO") => scheduleSelf()

			//Enviarle el mensaje al CPU, y actualizarle el estado en la lista
			//Deberia activar algun semaforo para que sepa que debe mandarselo por el socket?

		}

		//Esto es solo pasar el Dummy a READY para poder planificarlo desde ahi
		else if(PCPtask == PCP_TASK_LOAD_DUMMY)
		{
			dummyDTB->status = DTB_STATUS_READY;
			AddToReady(dummyDTB);
			SetPCPTask(PCP_TASK_NORMAL_SCHEDULE);
		}

		else if(PCPtask == PCP_TASK_BLOCK_DUMMY)
		{

			//Saco el Dummy de la cola de EXEC, y lo paso a la de BLOCKED (modifico su estado)
			list_remove_by_condition(EXECqueue, IsDummy);
			AddToBlocked(dummyDTB);
			//Aca habria que liberar el CPU, con FreeCPU(toBeMoved.cpuSocket)
			SetPCPTask(PCP_TASK_NORMAL_SCHEDULE);

		}

		//Tras aviso desde CPU
		else if(PCPtask == PCP_TASK_BLOCK_DTB)
		{

			DTB* target = list_remove_by_condition(EXECqueue, IsToBeMoved);
			AddToBlocked(target);
			//Aca habria que liberar el CPU, con FreeCPU(toBeMoved.cpuSocket)
			SetPCPTask(PCP_TASK_NORMAL_SCHEDULE);

		}

		//Tras aviso desde DAM
		else if(PCPtask == PCP_TASK_UNLOCK_DTB)
		{

			DTB* target = list_remove_by_condition(BLOCKEDqueue, IsToBeMoved);
			AddToReady(target);
			SetPCPTask(PCP_TASK_NORMAL_SCHEDULE);

		}

	}

}

void* scheduleRR(int quantum)
{

	DTB* chosenDTB = GetNextDTB();
	chosenDTB->quantumRemainder = quantum;

	int* idToSend;
	*idToSend = chosenDTB->id;
	int* pcToSend;
	*pcToSend = chosenDTB->programCounter;
	int* quantumToSend;
	*quantumToSend = quantum;

	//Estructuras con los datos a serializar y mandar como cadena
	SerializedPart idSP, pathSP, pcSP, quantumSP;
	idSP.size = sizeof(idToSend);
	idSP.data = idToSend;
	pathSP.size = strlen(chosenDTB->pathEscriptorio) + 1;
	strcpy(pathSP.data, chosenDTB->pathEscriptorio);
	pcSP.size = sizeof(pcToSend);
	pcSP.data = pcToSend;
	quantumSP.size = sizeof(quantumToSend);
	quantumSP.data = quantumToSend;

	//La idea es armar un paquete serializado que va a tener la estructura:
	// |IDdelDTB|PathEscriptorioAsociado|ProgramCounterDelDTB|QuantumAEjecutar|
	//(cada cual con su respectivo tamanio antes del dato en si)
	void* packet = Serialization_Serialize(4, idSP, pathSP, pcSP, quantumSP);

	AddToExec(chosenDTB);
	//OJO: Alguien deberia hacer free de ese packet despues
	return packet;

}

void* scheduleVRR(int maxQuantum)
{

	DTB* chosenDTB = GetNextDTB();
	//Si le quedara 0 de quantum (se quedo sin) o tuviera mas del maximo (por haber sido
	//planificado con otro algoritmo antes), le actualizo el maximo quantum a ejecutar
	if(chosenDTB->quantumRemainder == 0 || (chosenDTB->quantumRemainder == 0 > maxQuantum))
	{
		chosenDTB->quantumRemainder = maxQuantum;
	}

	int* idToSend;
	*idToSend = chosenDTB->id;
	int* pcToSend;
	*pcToSend = chosenDTB->programCounter;
	int* quantumToSend;
	*quantumToSend = chosenDTB->quantumRemainder;

	//Estructuras con los datos a serializar y mandar como cadena
	SerializedPart idSP, pathSP, pcSP, quantumSP;
	idSP.size = sizeof(idToSend);
	idSP.data = idToSend;
	pathSP.size = strlen(chosenDTB->pathEscriptorio) + 1;
	strcpy(pathSP.data, chosenDTB->pathEscriptorio);
	pcSP.size = sizeof(pcToSend);
	pcSP.data = pcToSend;
	quantumSP.size = sizeof(quantumToSend);
	quantumSP.data = quantumToSend;

	//La idea es armar un paquete serializado que va a tener la estructura:
	// |IDdelDTB|PathEscriptorioAsociado|ProgramCounterDelDTB|QuantumAEjecutar|
	//(cada cual con su respectivo tamanio antes del dato en si)
	void* packet = Serialization_Serialize(4, idSP, pathSP, pcSP, quantumSP);

	AddToExec(chosenDTB);
	//OJO: Alguien deberia hacer free de ese packet despues
	return packet;

}
