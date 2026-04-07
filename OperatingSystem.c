#include "Simulator.h"
#include "OperatingSystem.h"
#include "OperatingSystemBase.h"
#include "MMU.h"
#include "Processor.h"
#include "Buses.h"
#include "Heap.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

// Functions prototypes
void OperatingSystem_PCBInitialization(int, int, int, int, int);
void OperatingSystem_PrintReadyToRunQueue();
void OperatingSystem_MoveToTheREADYState(int);
void OperatingSystem_Dispatch(int);
void OperatingSystem_RestoreContext(int);
void OperatingSystem_SaveContext(int);
void OperatingSystem_TerminateExecutingProcess();
int OperatingSystem_LongTermScheduler();
void OperatingSystem_PreemptRunningProcess();
int OperatingSystem_CreateProcess(int);
int OperatingSystem_ObtainMainMemory(int, int);
int OperatingSystem_ShortTermScheduler();
int OperatingSystem_ExtractFromReadyToRunQueue(int queueID);
void OperatingSystem_HandleException();
void OperatingSystem_HandleSystemCall();
void OperatingSystem_HandleClockInterrupt();
void OperatingSystem_MoveToTheBLOCKEDState();

// Variables V2  ::::::::::::::::::::::::::::::::::
int numberOfClockInterrupts = 0;

heapItem *sleepingProcessesQueue;
int numberOfSleepingProcesses = 0;

// The process table
PCB * processTable;

// Size of the memory occupied for the OS
int OS_MEMORY_SIZE=32;

// Address base for OS code in this version
int OS_address_base; 

// Identifier of the current executing process
int executingProcessID=NOPROCESS;

// Identifier of the System Idle Process
int sipID;

// Initial PID for assignation (Not assigned)
int initialPID=-1;

// Begin indes for daemons in programList
// int baseDaemonsInProgramList; 

// Array that contains the identifiers of the READY processes
heapItem *readyToRunQueue[NUMBEROFQUEUES];
int numberOfReadyToRunProcesses[NUMBEROFQUEUES];

// Variable containing the number of not terminated user processes
int numberOfNotTerminatedUserProcesses=0;

int MAINMEMORYSECTIONSIZE = 60;

extern int MAINMEMORYSIZE;

int PROCESSTABLEMAXSIZE = 4;

//Variables del ejercicio 11-14
char * statesNames [5]={"NEW","READY","EXECUTING","BLOCKED","EXIT"};
char * queueNames [NUMBEROFQUEUES] = {"HIGHPRIOUSER", "LOWPRIOUSER", "DAEMONS"};

// Initial set of tasks of the OS
void OperatingSystem_Initialize(int programsFromFileIndex) {
	
	int i, selectedProcess;
	
// In this version, with original configuration of memory size (300) and number of processes (4)
// every process occupies a 60 positions main memory chunk 
// and OS code and the system stack occupies 60 positions 

	OS_address_base = MAINMEMORYSIZE - OS_MEMORY_SIZE;

	MAINMEMORYSECTIONSIZE = OS_address_base / PROCESSTABLEMAXSIZE;

	if (initialPID<0) // if not assigned in command-line options...
		initialPID=PROCESSTABLEMAXSIZE -1 ; 
	
	// Space for the processTable
	processTable = (PCB *) malloc(PROCESSTABLEMAXSIZE*sizeof(PCB));

	for(i = 0 ; i< NUMBEROFQUEUES; i++){
		readyToRunQueue[i] = Heap_create(PROCESSTABLEMAXSIZE);
		numberOfReadyToRunProcesses[i] = 0;
	}

	//V2 - Ejercicio 5 >> Inicializar cola de dormidos 
	sleepingProcessesQueue = Heap_create(PROCESSTABLEMAXSIZE);

	// Load Operating System Code
	OperatingSystem_LoadOperatingSystemCode(OPERATING_SYSTEM_CODE_FILE, OS_address_base);
	
	// Process table initialization (all entries are free)
	for (i=0; i<PROCESSTABLEMAXSIZE;i++){
		processTable[i].busy=0;
		processTable[i].programListIndex=-1;
		processTable[i].initialPhysicalAddress=-1;
		processTable[i].processSize=-1;
		processTable[i].copyOfSPRegister=-1;
		processTable[i].state=-1;
		processTable[i].priority=-1;
		processTable[i].copyOfPCRegister=-1;
		processTable[i].copyOfPSWRegister=-1;
		processTable[i].programListIndex=-1;
	}
	// Initialization of the interrupt vector table of the processor
	Processor_InitializeInterruptVectorTable(OS_address_base+2);
		
	// Include in program list all user or system daemon processes
	OperatingSystem_PrepareDaemons(programsFromFileIndex);

	// Create and fill arrivalTimeQueue heap with user programs and daemons
	arrivalTimeQueue = Heap_create(PROGRAMSMAXNUMBER);
	ComputerSystem_FillInArrivalTimeQueue();

	ComputerSystem_PrintArrivalTimeQueue();
	
	// Create all user processes from the information given in the command line
	OperatingSystem_LongTermScheduler();
	
	if (strcmp(programList[processTable[sipID].programListIndex]->executableName,SYSTEM_IDLE_PROCESS)!=0
		&& processTable[sipID].state==READY) {
		// Show red message "FATAL ERROR: Missing SIP program!\n"
		ComputerSystem_DebugMessage(NO_TIMED_MESSAGE,99,SHUTDOWN,"FATAL ERROR: Missing SIP program!\n");
		exit(1);		
	}

	// Check if at least one user process has been created
	if (numberOfNotTerminatedUserProcesses == 0) {
		// Simulation must finish 
		OperatingSystem_ReadyToShutdown();
	}

	// At least, one process has been created
	// Select the first process that is going to use the processor
	selectedProcess=OperatingSystem_ShortTermScheduler();

	Processor_SetSSP(MAINMEMORYSIZE-1);

	// Assign the processor to the selected process
	OperatingSystem_Dispatch(selectedProcess);

	// Initial operation for Operating System
	Processor_SetPSW(Processor_GetPSW() | (1 << EXECUTION_MODE_BIT)); 
	Processor_SetPC(OS_address_base);

	//Ejercicio 3-a Ultima sentencia 
	OperatingSystem_PrintStatus();

}

// The LTS is responsible of the admission of new processes in the system.
// Initially, it creates a process from each program specified in the 
// 			command line and daemons programs
int OperatingSystem_LongTermScheduler() {
  
	int createdProcessPID, i,
		numberOfSuccessfullyCreatedProcesses=0;
	
	while (OperatingSystem_IsThereANewProgram()!=EMPTYQUEUE) {
		i=Heap_poll(arrivalTimeQueue,QUEUE_ARRIVAL,&numberOfProgramsInArrivalTimeQueue);
		createdProcessPID=OperatingSystem_CreateProcess(i);
		switch (createdProcessPID) {
			case PROGRAMDOESNOTEXIST:
				ComputerSystem_DebugMessage(TIMED_MESSAGE,51, ERROR, programList[i]->executableName, "it does not exist");
				break;
			case NOFREEENTRY: 
				ComputerSystem_DebugMessage(TIMED_MESSAGE,50,ERROR, programList[i] -> executableName);
				break;
			case PROGRAMNOTVALID: 
				ComputerSystem_DebugMessage(TIMED_MESSAGE,51, ERROR, programList[i] -> executableName, "invalid priority or size");
				break;
			case TOOBIGPROCESS: 
				ComputerSystem_DebugMessage(TIMED_MESSAGE, 52, ERROR, programList[i] -> executableName);
				break;
			default:
				// Process creation has succeeded: additional actions
				// Show message "Process [createdProcessPID] created from program [executableName]\n"
				ComputerSystem_DebugMessage(TIMED_MESSAGE, 54, SYSPROC, createdProcessPID, statesNames[NEW], programList[i]->executableName);
				//ComputerSystem_DebugMessage(TIMED_MESSAGE,70,SYSPROC,createdProcessPID,programList[i]->executableName);
				numberOfSuccessfullyCreatedProcesses++;
				if (programList[i]->type==USERPROGRAM) 
					numberOfNotTerminatedUserProcesses++;
				// Move process to the ready state
				OperatingSystem_MoveToTheREADYState(createdProcessPID);
		}
	}
	//Ejercicio V2 - 3e
	/*if(numberOfSuccessfullyCreatedProcesses > 0 ){
		OperatingSystem_PrintStatus();
	}*/

	// Return the number of succesfully created processes
	return numberOfSuccessfullyCreatedProcesses;
}

// This function creates a process from an executable program
int OperatingSystem_CreateProcess(int indexOfExecutableProgram) {
  
	int assignedPID;
	int processSize;
	int loadingPhysicalAddress;
	int priority;
	FILE *programFile;
	PROGRAMS_DATA *executableProgram=programList[indexOfExecutableProgram];

	// Obtain a process ID
	assignedPID=OperatingSystem_ObtainAnEntryInTheProcessTable();
	if(assignedPID == NOFREEENTRY){
		return assignedPID;
	}

	// Check if programFile exists
	programFile=fopen(executableProgram->executableName, "r");
	if (programFile==NULL){
		return PROGRAMDOESNOTEXIST;
	}
	// Obtain the memory requirements of the program
	processSize=OperatingSystem_ObtainProgramSize(programFile);	
	if(processSize == PROGRAMNOTVALID){
		return processSize;
	}
	

	// Obtain the priority for the process
	priority=OperatingSystem_ObtainPriority(programFile);
	if(priority == PROGRAMNOTVALID){
		return priority;
	}
	
	// Obtain enough memory space
 	loadingPhysicalAddress=OperatingSystem_ObtainMainMemory(processSize, assignedPID);
	if(loadingPhysicalAddress == TOOBIGPROCESS){
		return loadingPhysicalAddress;
	}

	// Load program in the allocated memory
	if(OperatingSystem_LoadProgram(programFile, loadingPhysicalAddress, processSize) == TOOBIGPROCESS){
		return TOOBIGPROCESS;
	}
	
	// PCB initialization
	OperatingSystem_PCBInitialization(assignedPID, loadingPhysicalAddress, processSize, priority, indexOfExecutableProgram);

	return assignedPID;
}


// Main memory is assigned in chunks. All chunks are the same size. A process
// always obtains the chunk whose position in memory is equal to the processor identifier
int OperatingSystem_ObtainMainMemory(int processSize, int PID) {

 	if (processSize>MAINMEMORYSECTIONSIZE)
		return TOOBIGPROCESS;
	
 	return PID*MAINMEMORYSECTIONSIZE;
}


// Assign initial values to all fields inside the PCB
void OperatingSystem_PCBInitialization(int PID, int initialPhysicalAddress, int processSize, int priority, int processPLIndex) {

	//V2 - Ejercicio 5 >> Para cumplir con el ejercicio 0
	processTable[PID].whenToWakeUp = -1;

	processTable[PID].busy=1;
	processTable[PID].initialPhysicalAddress=initialPhysicalAddress;
	processTable[PID].processSize=processSize;
	processTable[PID].copyOfSPRegister=initialPhysicalAddress+processSize;
	processTable[PID].state=NEW;
	processTable[PID].priority=priority;
	processTable[PID].programListIndex=processPLIndex;
	//Asignar correctamente el proceso 
	if(programList[processPLIndex] -> type == DAEMONPROGRAM){
		processTable[PID].queueID = DEAMONSQUEUE;
	}else{
		if(processSize < 30 ) processTable[PID].queueID = HIGHPRIOUSERPROCQUEUE;
		else processTable[PID].queueID = LOWPRIOUSERPROCQUEUE;
	}

	//Los Daemons corren en modo protegido y la MMU usa direcciones físicas 
	if(programList[processPLIndex] -> type == DAEMONPROGRAM){
		processTable[PID].copyOfPCRegister = initialPhysicalAddress;
		processTable[PID].copyOfPSWRegister = ((unsigned int) 1) << EXECUTION_MODE_BIT;
	}else{
		processTable[PID].copyOfPCRegister = 0; 
		processTable[PID].copyOfPSWRegister = 0;
	}
	
	//Inicializar variables de Restaurado 
	processTable[PID].copyOfAccumulator = 0; 
	processTable[PID].copyOfRegisterA = 0; 
	processTable[PID].copyOfRegisterB = 0; 
}


// Move a process to the READY state: it will be inserted, depending on its priority, in
// a queue of identifiers of READY processes
void OperatingSystem_MoveToTheREADYState(int PID) {
	int previousState = processTable[PID].state;
	int processQueueID = processTable[PID].queueID;

	if (Heap_add(PID, readyToRunQueue[processQueueID],QUEUE_PRIORITY ,&(numberOfReadyToRunProcesses[processQueueID]))>=0) {
		processTable[PID].state=READY;
	} 

	//Funcion creada PrintReadyToRunQueue
	//Ejercicio V2 - 4.a - Comentar la llamada para evitar redundancia 
	//OperatingSystem_PrintReadyToRunQueue();

	//Imprimir el mensaje de cambio de estado - message 53 - 
	ComputerSystem_DebugMessage(TIMED_MESSAGE, 53, SYSPROC, PID, programList[processTable[PID].programListIndex]-> executableName, statesNames[previousState], statesNames[READY]);
}


// The STS is responsible of deciding which process to execute when specific events occur.
// It uses processes priorities to make the decission. Given that the READY queue is ordered
// depending on processes priority, the STS just selects the process in front of the READY queue
int OperatingSystem_ShortTermScheduler() {
	
	int selectedProcess=NOPROCESS;
	
	for(int i= 0; i<NUMBEROFQUEUES; i++){
		selectedProcess = OperatingSystem_ExtractFromReadyToRunQueue(i);
		if(selectedProcess != NOPROCESS){
			return selectedProcess; 
		}
	}
	return selectedProcess;

	// selectedProcess=OperatingSystem_ExtractFromReadyToRunQueue(ALLPROCESSESQUEUE);
	
	return selectedProcess;
}


// Return PID of process with the highest priority in the READY queue
int OperatingSystem_ExtractFromReadyToRunQueue(int queueID) {
  
	int selectedProcess=NOPROCESS;

	selectedProcess=Heap_poll(readyToRunQueue[queueID],QUEUE_PRIORITY ,&(numberOfReadyToRunProcesses[queueID]));

	// Return highest priority process or NOPROCESS if empty queue
	return selectedProcess; 
}


// Function that assigns the processor to a process
void OperatingSystem_Dispatch(int PID) {
	int previousState = processTable[PID].state;
	// The process identified by PID becomes the current executing process
	executingProcessID=PID;
	// Change the process' state
	processTable[PID].state=EXECUTING;
	// Modify hardware registers with appropriate values for the process identified by PID
	OperatingSystem_RestoreContext(PID);

	//Print state change message - message 53-
	ComputerSystem_DebugMessage(TIMED_MESSAGE, 53, SYSPROC, PID, programList[processTable[PID].programListIndex] -> executableName, statesNames[previousState], statesNames[EXECUTING]);
	
	//Ejercicio V2 - 4. Comentar la llamada y evitar redundancia 
	//OperatingSystem_PrintReadyToRunQueue();
}


// Modify hardware registers with appropriate values for the process identified by PID
void OperatingSystem_RestoreContext(int PID) {
  
	// New values for the CPU registers are obtained from the PCB
	Processor_PushInSystemStack(processTable[PID].copyOfPCRegister);
	Processor_PushInSystemStack(processTable[PID].copyOfPSWRegister);
	Processor_SetRegisterSP(processTable[PID].copyOfSPRegister);

	//Restore register values for the process identified by PID
	Processor_SetAccumulator(processTable[PID].copyOfAccumulator);
	Processor_SetRegisterA(processTable[PID].copyOfRegisterA);
	Processor_SetRegisterB(processTable[PID].copyOfRegisterB);

	// Same thing for the MMU registers
	MMU_SetBase(processTable[PID].initialPhysicalAddress);
	MMU_SetLimit(processTable[PID].processSize);
}


// Function invoked when the executing process leaves the CPU 
void OperatingSystem_PreemptRunningProcess() {

	// Save in the process' PCB essential values stored in hardware registers and the system stack
	OperatingSystem_SaveContext(executingProcessID);
	// Change the process' state
	OperatingSystem_MoveToTheREADYState(executingProcessID);
	// The processor is not assigned until the OS selects another process
	executingProcessID=NOPROCESS;
}


// Save in the process' PCB essential values stored in hardware registers and the system stack
void OperatingSystem_SaveContext(int PID) {
	
	// Load PSW saved for interrupt manager
	processTable[PID].copyOfPSWRegister=Processor_PopFromSystemStack();
	
	// Load PC saved for interrupt manager
	processTable[PID].copyOfPCRegister=Processor_PopFromSystemStack();
	
	// Save RegisterSP 
	processTable[PID].copyOfSPRegister=Processor_GetRegisterSP();

	//Save register values for the process identified by PID 
	processTable[PID].copyOfAccumulator = Processor_GetAccumulator();
	processTable[PID].copyOfRegisterA = Processor_GetRegisterA();
	processTable[PID].copyOfRegisterB = Processor_GetRegisterB();
}


// Exception management routine
void OperatingSystem_HandleException() {
  
	// Show message "Process [executingProcessID] has generated an exception and is terminating\n"
	ComputerSystem_DebugMessage(TIMED_MESSAGE,71,INTERRUPT,executingProcessID,programList[processTable[executingProcessID].programListIndex]->executableName);
	
	OperatingSystem_TerminateExecutingProcess();

	//Ejercicio V2 - 3b 
	OperatingSystem_PrintStatus();
}

// All tasks regarding the removal of the executing process
void OperatingSystem_TerminateExecutingProcess() {
	int previousState = processTable[executingProcessID].state;
	processTable[executingProcessID].state=EXIT;

	//Imprimir el mensaje de cambio de estado - message 53 - 
	ComputerSystem_DebugMessage(TIMED_MESSAGE, 53, SYSPROC, executingProcessID, programList[processTable[executingProcessID].programListIndex] -> executableName, statesNames[previousState], statesNames[EXIT]);
	
	if (executingProcessID==sipID) {
		// finishing sipID, change PC to address of OS HALT instruction
		Processor_SetSSP(MAINMEMORYSIZE-1);
		Processor_PushInSystemStack(OS_address_base+1);
		Processor_PushInSystemStack(Processor_GetPSW());
		executingProcessID=NOPROCESS;
		ComputerSystem_DebugMessage(TIMED_MESSAGE,99,SHUTDOWN,"The system will shut down now...\n");
		return; // Don't dispatch any process
	}

	Processor_SetSSP(Processor_GetSSP()+2); // unstack PC and PSW stacked

	if (programList[processTable[executingProcessID].programListIndex]->type==USERPROGRAM) 
		// One more user process that has terminated
		numberOfNotTerminatedUserProcesses--;
	
	if (numberOfNotTerminatedUserProcesses==0) {
		// Simulation must finish, telling sipID to finish
		OperatingSystem_ReadyToShutdown();
	}
	// Select the next process to execute (sipID if no more user processes)
	int selectedProcess=OperatingSystem_ShortTermScheduler();

	// Assign the processor to that process
	OperatingSystem_Dispatch(selectedProcess);
}

// System call management routine
void OperatingSystem_HandleSystemCall() {
  
	int systemCallID;

	// Register C contains the identifier of the issued system call
	systemCallID=Processor_GetRegisterC();
	
	switch (systemCallID) {
		case SYSCALL_PRINTEXECINFO:
			// Show message: "Process [executingProcessID] is using the CPU ...\n"
			ComputerSystem_DebugMessage(TIMED_MESSAGE,72,SYSPROC,executingProcessID,programList[processTable[executingProcessID].programListIndex]->executableName,Processor_GetRegisterA(),Processor_GetRegisterB(),processTable[executingProcessID].copyOfPCRegister);
			break;

		case SYSCALL_END:
			// Show message: "Process [executingProcessID] has requested to terminate\n"
			ComputerSystem_DebugMessage(TIMED_MESSAGE,73,SYSPROC,executingProcessID,programList[processTable[executingProcessID].programListIndex]->executableName);
			OperatingSystem_TerminateExecutingProcess();
			break;
		//Ejercicio 14, incluir llamada SYSCAL_YIELD - Give control to 
		//		READY process with same prio - Make it the highest prio process in the READY queue
		//		Call function DebugMessage with custom message 55, using SHORTERMSCHEDULER 
		//		if there's not same prio process in the READY queue or not anymore process in the READY queue {
		//			do nothing, the executing process don't leave the CPU 
		//			show custom message 56, using SHORTERMSCHEDULER
		//			}
		case SYSCALL_YIELD: 
		{
			int miQUEUEID = processTable[executingProcessID].queueID;
			int miPriority = processTable[executingProcessID].priority;
			
			//Consultamos el siguiente proceso en cola
			int siguiente = Heap_getFirst(readyToRunQueue[miQUEUEID], numberOfReadyToRunProcesses[miQUEUEID]);

			//Comprobar si hay procesos del mismo tipo y prioridad -> Porducir cambio de contexto en caso correcto 
			if(siguiente != NOPROCESS && processTable[siguiente].priority == miPriority){
				//Hay proceso con la misma proridad -> Ceder el control 
				ComputerSystem_DebugMessage(TIMED_MESSAGE, 55, SHORTTERMSCHEDULE, 
					executingProcessID, programList[processTable[executingProcessID].programListIndex]-> executableName, 
					siguiente, programList[processTable[siguiente].programListIndex]->executableName);
				
					//Sacamos el proceso actual de la CPU y lo devolvemos a la cola 
				OperatingSystem_PreemptRunningProcess();

				int selectedProcess = OperatingSystem_ShortTermScheduler();

				OperatingSystem_Dispatch(selectedProcess);

				//Ejercicio V2 - 3b
				OperatingSystem_PrintStatus();
			}else{
				ComputerSystem_DebugMessage(TIMED_MESSAGE, 56, SHORTTERMSCHEDULE, executingProcessID, programList[processTable[executingProcessID].programListIndex]-> executableName);
			}
			break;
		}
		case SYSCALL_SLEEP:
		{
			int delay; 
			//Segundo operando > 0 >> delay : abs del accum
			if(Processor_GetRegisterD() >0)
				delay = Processor_GetRegisterD();
			else
				delay = abs(Processor_GetAccumulator());
			//5-f >> Calcular el despertar
			processTable[executingProcessID].whenToWakeUp = delay + numberOfClockInterrupts +1;

			//Bloquear el proceso 
			OperatingSystem_SaveContext(executingProcessID);
			OperatingSystem_MoveToTheBLOCKEDState(executingProcessID);

			//Liberar CPU y despachar sigueinte
			executingProcessID = NOPROCESS; 
			OperatingSystem_Dispatch(OperatingSystem_ShortTermScheduler());

			//5-g Mostrar estado actualizado del sistema 
			OperatingSystem_PrintStatus();
			break;
		}
	}
}
	
//	Implement interrupt logic calling appropriate interrupt handle
void OperatingSystem_InterruptLogic(int entryPoint){
	switch (entryPoint){
		case SYSCALL_BIT: // SYSCALL_BIT=2
			OperatingSystem_HandleSystemCall();
			break;
		case EXCEPTION_BIT: // EXCEPTION_BIT=6
			OperatingSystem_HandleException();
			break;
		case CLOCKINT_BIT: 
			OperatingSystem_HandleClockInterrupt();
			break;
	}

}

// ================== SESION PRACTICA 11- 14 ==================

void OperatingSystem_PrintReadyToRunQueue(){
	//Imprimir el mensaje cabecera 
	ComputerSystem_DebugMessage(TIMED_MESSAGE, 103, SHORTTERMSCHEDULE);

	//Recorremos las tres colas de programas 
	for(int i = 0; i < NUMBEROFQUEUES; i++){
		ComputerSystem_DebugMessage(TIMED_MESSAGE, 104, SHORTTERMSCHEDULE, queueNames[i]);
		if(numberOfReadyToRunProcesses[i] > 0){
			Heap_print(readyToRunQueue[i], QUEUE_PRIORITY, numberOfReadyToRunProcesses[i]);
		}else{
			printf("\n");
		}
	}
}


// Adiciones del V2 ::::::::::::::::::::::::::::::::
void OperatingSystem_HandleClockInterrupt() { 
	numberOfClockInterrupts ++;
	ComputerSystem_DebugMessage(TIMED_MESSAGE, 57, INTERRUPT, numberOfClockInterrupts);

	int awakened = 0; 
	//6a 6b >> Despertar procesos cuyo tiempo hay llegado 
	//Si hay procesos y el tiempo de despertar del primero sea el actual 
	while(numberOfSleepingProcesses > 0 && processTable[Heap_getFirst(sleepingProcessesQueue, numberOfSleepingProcesses)].whenToWakeUp == numberOfClockInterrupts){
		int pid = Heap_poll(sleepingProcessesQueue, QUEUE_WAKEUP, &numberOfSleepingProcesses);
		OperatingSystem_MoveToTheREADYState(pid);
		awakened++;
	}

	//6c >> Al despertar procesos, ver si hay que cambiar el proceso en ejecución
	int selectedProcess = OperatingSystem_ShortTermScheduler();

	if(selectedProcess != NOPROCESS){
		//Expulsamos si el candidato tiene menor prioridad
		if(processTable[selectedProcess].priority <= processTable[executingProcessID].priority){
			//6d >> Mensaje 58 de preempcion
			ComputerSystem_DebugMessage(TIMED_MESSAGE, 58, SHORTTERMSCHEDULE,
								executingProcessID, programList[processTable[executingProcessID].programListIndex] -> executableName,
								selectedProcess, programList[processTable[selectedProcess].programListIndex] -> executableName);
							
			OperatingSystem_PreemptRunningProcess();
			OperatingSystem_Dispatch(selectedProcess);

			//6e >> Mostrar estado actualizado 
			OperatingSystem_PrintStatus();
		}else{
			//No es mejor, lo devolvemos a su cola de listos 
			OperatingSystem_MoveToTheREADYState(selectedProcess);
			//6e >> Si se despertó, alguien pero no cambió, mostrarmos el estado actualizado 
			if(awakened > 0){
				OperatingSystem_PrintStatus();
			}
		}
	}else if(awakened > 0){
		//Si hay candidatos nuevos pero se despertó alguien, mostrarmos estado
		OperatingSystem_PrintStatus();
	}
} 

void OperatingSystem_MoveToTheBLOCKEDState(int PID){
	if(Heap_add(PID, sleepingProcessesQueue, QUEUE_WAKEUP, &numberOfSleepingProcesses)>= 0){
		processTable[PID].state = BLOCKED;
	}
 }



