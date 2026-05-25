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
void OperatingSystem_MoveToTheSLEEPINGState();
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
int OperatingSystem_ExtractFromSleepingProcessesQueue();
void OperatingSystem_HandleException();
void OperatingSystem_HandleSystemCall();
void OperatingSystem_HandleClockInterrupt();
void OperatingSystem_ReleaseMainMemory();
void OperatingSystem_CoalesceHoles();
//V3 - 4 >> Función para calcular la media 
float calcular_media_movil(int *load, int used, int n);

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
char * typeOfExceptions[] = {"division by zero", "invalid processor mode", "invalid address", "invalid instruction"};

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
		processTable[i].queueID = -1; 
		processTable[i].copyOfAccumulator = 0; 
		processTable[i].copyOfRegisterA = 0; 
		processTable[i].copyOfRegisterB = 0; 
		processTable[i].whenToWakeUp = -1; 
	}
	// Initialization of the interrupt vector table of the processor
	Processor_InitializeInterruptVectorTable(OS_address_base+2);
		
	// Include in program list all user or system daemon processes
	OperatingSystem_PrepareDaemons(programsFromFileIndex);

	// Create and fill arrivalTimeQueue heap with user programs and daemons
	arrivalTimeQueue = Heap_create(PROGRAMSMAXNUMBER);
	ComputerSystem_FillInArrivalTimeQueue();

	ComputerSystem_PrintArrivalTimeQueue();
	
	//V3 - 4 >> Inicializar Statistics de OperatingSystebase
	OperatingSystem_InitializeStatistics(&stats, 10);

	// Inicializar la tabla de particiones y huecos del Sistema Operativo
	OperatingSystem_InitializePartitionsAndHolesTable(OS_address_base);

	// Create all user processes from the information given in the command line
	OperatingSystem_LongTermScheduler();
	
	if (strcmp(programList[processTable[sipID].programListIndex]->executableName,SYSTEM_IDLE_PROCESS)!=0
		&& processTable[sipID].state==READY) {
		// Show red message "FATAL ERROR: Missing SIP program!\n"
		ComputerSystem_DebugMessage(NO_TIMED_MESSAGE,99,SHUTDOWN,"FATAL ERROR: Missing SIP program!\n");
		exit(1);		
	}

	// Check if at least one user process has been created
	if (numberOfNotTerminatedUserProcesses == 0 && numberOfProgramsInArrivalTimeQueue == 0) {
		// Simulation must finish if there are no user processes and no more programs to arrive
		OperatingSystem_ReadyToShutdown();
	}

	// At least, one process has been created
	// Select the first process that is going to use the processor
	selectedProcess=OperatingSystem_ShortTermScheduler();

	Processor_SetSSP(MAINMEMORYSIZE-1);

	// Assign the processor to the selected process
	OperatingSystem_Dispatch(selectedProcess);

	// Initial operation for Operating System 
	Processor_SetPC(OS_address_base);

	OperatingSystem_PrintStatus();
}

// The LTS is responsible of the admission of new processes in the system.
// Initially, it creates a process from each program specified in the 
// 			command line and daemons programs
int OperatingSystem_LongTermScheduler() {
  
	int createdProcessPID, i,
		numberOfSuccessfullyCreatedProcesses=0;
	
	while (OperatingSystem_IsThereANewProgram() == YES) {
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
				//Ejercicio V3 - 1 >> Llamadas a IsThereANewProgram 
				ComputerSystem_DebugMessage(TIMED_MESSAGE, 52, ERROR, programList[i] -> executableName);
				break;
			case MEMORYFULL:
				ComputerSystem_DebugMessage(TIMED_MESSAGE, 31, ERROR, programList[i]->executableName);
				break;
			default:
				// Process creation has succeeded: additional actions
				// Show message "Process [createdProcessPID] created from program [executableName]\n"
				ComputerSystem_DebugMessage(TIMED_MESSAGE, 54, SYSPROC, createdProcessPID, statesNames[NEW], programList[i]->executableName);
				
				numberOfSuccessfullyCreatedProcesses++;
				if (programList[i]->type==USERPROGRAM) 
					numberOfNotTerminatedUserProcesses++;
				// Move process to the ready state
				OperatingSystem_MoveToTheREADYState(createdProcessPID);
		}
	}
	if(numberOfSuccessfullyCreatedProcesses > 0)
		OperatingSystem_PrintStatus();

	// Return the number of succesfully created processed 
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
		return NOFREEENTRY;
	}

	// Set programLisIndex early so the partition table display can look up the name
	processTable[assignedPID].programListIndex = indexOfExecutableProgram;

	// Check if programFile exists
	programFile=fopen(executableProgram->executableName, "r");
	if (programFile==NULL){
		return PROGRAMDOESNOTEXIST;
	}
	// Obtain the memory requirements of the program
	processSize=OperatingSystem_ObtainProgramSize(programFile);	
	if(processSize < 0){
		fclose(programFile); 
		return PROGRAMNOTVALID;
	}

	// Obtain the priority for the process
	priority=OperatingSystem_ObtainPriority(programFile);
	if(priority < 0){
		fclose(programFile);
		return PROGRAMNOTVALID;
	}

	// Always show memory request message before attempting allocation
	ComputerSystem_DebugMessage(TIMED_MESSAGE, 42, SYSMEM, 
		assignedPID, executableProgram-> executableName, processSize);
	
	// Obtain enough memory space (returns partition table index or error)
	int partitionIndex = OperatingSystem_ObtainMainMemory(processSize, assignedPID);
	if(partitionIndex == TOOBIGPROCESS || partitionIndex == MEMORYFULL){
		fclose(programFile);
		return partitionIndex;
	}
	
	// Show partition table before allocation
	OperatingSystem_ShowPartitionsAndHolesTable("before allocating memory");

	// Get hole info and convert hole to partition 
	int holeStartAddr = partitionsAndHolesTable[partitionIndex].initAddress;
	int holeSize = partitionsAndHolesTable[partitionIndex].size;
	partitionsAndHolesTable[partitionIndex].PID = assignedPID; 
	partitionsAndHolesTable[partitionIndex].size = processSize;
	loadingPhysicalAddress = holeStartAddr;

	// Create leftover hole if needed 
	if(holeSize > processSize){
		OperatingSystem_InsertIntopartitionsAndHolesTable(
			partitionIndex + 1, HOLE, holeStartAddr + processSize, holeSize - processSize);
	}

	// Show assignment messages
	ComputerSystem_DebugMessage(TIMED_MESSAGE, 43, SYSMEM, 
		partitionIndex, holeStartAddr, processSize, assignedPID, executableProgram -> executableName);
	if(holeSize > processSize){
		ComputerSystem_DebugMessage(TIMED_MESSAGE, 44, SYSMEM, 
			partitionIndex +1, holeStartAddr + processSize, holeSize - processSize,
			assignedPID, executableProgram -> executableName);
	}

	// Show partition table after allocation 
	OperatingSystem_ShowPartitionsAndHolesTable("after allocating memory");

	// Load program in the allocated memory
	if(OperatingSystem_LoadProgram(programFile, loadingPhysicalAddress, processSize) == TOOBIGPROCESS){
		processTable[assignedPID].busy = 0;
		return TOOBIGPROCESS;
	}
	
	// PCB initialization
	OperatingSystem_PCBInitialization(assignedPID, loadingPhysicalAddress, processSize, priority, indexOfExecutableProgram);

	return assignedPID;
}


// Main memory is assigned in chunks. All chunks are the same size. A process
// always obtains the chunk whose position in memory is equal to the processor identifier
int OperatingSystem_ObtainMainMemory(int processSize, int PID __attribute__((unused))) {
	int worstIndex = -1; 
	int worstSize = -1; 
	int maxHoleSize = 0;

	for (int i = 0; i < numberOfPartitionsAndHoles; i++) {
		if (partitionsAndHolesTable[i].PID == HOLE) { 
			int holeSize = partitionsAndHolesTable[i].size;
			if (holeSize >= maxHoleSize) 
				maxHoleSize = holeSize;
			if(holeSize >= processSize){
				if(worstIndex == -1 || holeSize > worstSize || 
					(holeSize == worstSize && partitionsAndHolesTable[i].initAddress < partitionsAndHolesTable[worstIndex].initAddress)){
					worstIndex = i; 
					worstSize = holeSize;
				}
			}
		}
	}

	if(worstIndex == -1){
		if(processSize > OS_address_base)
			return TOOBIGPROCESS;
		else
			return MEMORYFULL;
	}

	return worstIndex;
}


// Assign initial values to all fields inside the PCB
void OperatingSystem_PCBInitialization(int PID, int initialPhysicalAddress, int processSize, int priority, int processPLIndex) {
	
	processTable[PID].busy=1;
	processTable[PID].initialPhysicalAddress=initialPhysicalAddress;
	processTable[PID].processSize=processSize;
	processTable[PID].copyOfSPRegister=initialPhysicalAddress+processSize;
	processTable[PID].state=NEW;
	
	//Ejercicio 15 
	processTable[PID].copyOfAccumulator = 0; 
	processTable[PID].copyOfRegisterA = 0; 
	processTable[PID].copyOfRegisterB = 0; 
	processTable[PID].whenToWakeUp = -1;
	
	processTable[PID].priority=priority;
	processTable[PID].programListIndex=processPLIndex;
	
	if(programList[processPLIndex] -> type == DAEMONPROGRAM){
		processTable[PID].queueID = DEAMONSQUEUE;
	}else{
		if(processSize < 30){
			processTable[PID].queueID = HIGHPRIOUSERPROCQUEUE;
		}else{
			processTable[PID].queueID = LOWPRIOUSERPROCQUEUE;
		}
	}

	//Los Daemons corren en modo protegido y la MMU usa direcciones físicas 
	if(programList[processPLIndex] -> type == DAEMONPROGRAM){
		processTable[PID].copyOfPCRegister = initialPhysicalAddress;
		processTable[PID].copyOfPSWRegister = ((unsigned int) 1) << EXECUTION_MODE_BIT;
	}else{
		processTable[PID].copyOfPCRegister = 0; 
		processTable[PID].copyOfPSWRegister = 0;
	}
	
}


// Move a process to the READY state: it will be inserted, depending on its priority, in
// a queue of identifiers of READY processes
void OperatingSystem_MoveToTheREADYState(int PID) {
	int previousState = processTable[PID].state;
	int processQueueID = processTable[PID].queueID;

	//Imprimir el mensaje de cambio de estado - message 53 - 
	ComputerSystem_DebugMessage(TIMED_MESSAGE, 53, SYSPROC, PID, programList[processTable[PID].programListIndex]-> executableName, statesNames[previousState], statesNames[READY]);
	
	if (Heap_add(PID, readyToRunQueue[processQueueID],QUEUE_PRIORITY ,&(numberOfReadyToRunProcesses[processQueueID]))>=0) {
		processTable[PID].state=READY;
	} 

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

}


// Return PID of process with the highest priority in the READY queue
int OperatingSystem_ExtractFromReadyToRunQueue(int queueID) {
  
	int selectedProcess=NOPROCESS;

	selectedProcess=Heap_poll(readyToRunQueue[queueID],QUEUE_PRIORITY ,&(numberOfReadyToRunProcesses[queueID]));

	// Return highest priority process or NOPROCESS if empty queue
	return selectedProcess; 
}

int OperatingSystem_ExtractFromSleepingProcessesQueue(){
	int selectedProcess = NOPROCESS;

	selectedProcess = Heap_poll(sleepingProcessesQueue, QUEUE_WAKEUP, &numberOfSleepingProcesses);

	return selectedProcess;
}


// Function that assigns the processor to a process
void OperatingSystem_Dispatch(int PID) {
	int previousState = processTable[PID].state;
	// The process identified by PID becomes the current executing process
	executingProcessID=PID;
	// Change the process' state
	processTable[PID].state=EXECUTING;
	
	//Print state change message - message 53-
	ComputerSystem_DebugMessage(TIMED_MESSAGE, 53, SYSPROC, PID, programList[processTable[PID].programListIndex] -> executableName, statesNames[previousState], statesNames[EXECUTING]);
	
	// Modify hardware registers with appropriate values for the process identified by PID
	OperatingSystem_RestoreContext(PID);
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
  
	// Obtenemos el tipo de excepción generada leyendo el registro D
	int exceptionType = Processor_GetRegisterD();
	char *exceptionNames[] = {"division by zero", "invalid processor mode", "invalid address", "invalid instruction"};
	char *exceptionDescription = (exceptionType >= 0 && exceptionType <= 3) ? exceptionNames[exceptionType] : "unknown";
	
	ComputerSystem_DebugMessage(TIMED_MESSAGE, 32, INTERRUPT, 
		executingProcessID,
		programList[processTable[executingProcessID].programListIndex]->executableName, 
		exceptionDescription);
	
	OperatingSystem_TerminateExecutingProcess();
	OperatingSystem_PrintStatus();
}

// All tasks regarding the removal of the executing process
void OperatingSystem_TerminateExecutingProcess() {
	int previousState = processTable[executingProcessID].state;
	processTable[executingProcessID].state=EXIT;

	//Imprimir el mensaje de cambio de estado - message 53 - 
	ComputerSystem_DebugMessage(TIMED_MESSAGE, 53, SYSPROC, executingProcessID, programList[processTable[executingProcessID].programListIndex] -> executableName, statesNames[previousState], statesNames[EXIT]);
	
	OperatingSystem_ReleaseMainMemory();
	
	if (executingProcessID==sipID) {
		// finishing sipID, change PC to address of OS HALT instruction
		Processor_SetSSP(MAINMEMORYSIZE-1);
		Processor_PushInSystemStack(OS_address_base+1);
		Processor_PushInSystemStack(Processor_GetPSW());
		executingProcessID=NOPROCESS;
		ComputerSystem_DebugMessage(TIMED_MESSAGE,99,SHUTDOWN,"The system will shut down now...\n");
		OperatingSystem_PrintStatus();
		return; // Don't dispatch any process
	}


	Processor_SetSSP(Processor_GetSSP()+2); // unstack PC and PSW stacked

	if (programList[processTable[executingProcessID].programListIndex]->type==USERPROGRAM) 
		// One more user process that has terminated
		numberOfNotTerminatedUserProcesses--;
	
	
	if (numberOfNotTerminatedUserProcesses==0 && numberOfProgramsInArrivalTimeQueue == 0) {
		// Simulation must finish, telling sipID to finish
		OperatingSystem_ReadyToShutdown();
	}
	
	// Select the next process to execute (sipID if no more user processes)
	int selectedProcess=OperatingSystem_ShortTermScheduler();

	// Assign the processor to that process
	OperatingSystem_Dispatch(selectedProcess);

	OperatingSystem_PrintStatus();
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
				OperatingSystem_PrintStatus();
			}else{
				ComputerSystem_DebugMessage(TIMED_MESSAGE, 56, SHORTTERMSCHEDULE,
					 executingProcessID,
					 programList[processTable[executingProcessID].programListIndex]-> executableName);
			}
			break;
		}
		case SYSCALL_SLEEP:
		{
			int delay; 
			if(Processor_GetRegisterD() > 0) {
				delay = Processor_GetRegisterD(); 
			}else{
				delay = abs(Processor_GetAccumulator());
			}
			processTable[executingProcessID].whenToWakeUp = delay + numberOfClockInterrupts +1;

			//Bloquear el proceso 
			OperatingSystem_MoveToTheSLEEPINGState(executingProcessID);

			int selectedProcess = OperatingSystem_ShortTermScheduler();
			OperatingSystem_Dispatch(selectedProcess);
			OperatingSystem_PrintStatus();	// V2 - 5g
			break;
		}
		case SYSCALL_LOAD:
		{
			float last_charged_value = 0;
			float media_last_five_charged_values = 0;
			float media_all_charged_values = 0;

			if (stats.used > 0) {
				last_charged_value = stats.load[stats.used-1];
			}
			if (stats.used >= 5) {
				media_last_five_charged_values = calcular_media_movil(stats.load, stats.used, 5);
			}
			if (stats.used >= 6) {
				media_all_charged_values = calcular_media_movil(stats.load, stats.used, stats.used);
			}

			ComputerSystem_DebugMessage(TIMED_MESSAGE, 30, SHORTTERMSCHEDULE, last_charged_value, media_last_five_charged_values, media_all_charged_values);
			break;
		}
		default:
			ComputerSystem_DebugMessage(TIMED_MESSAGE, 33, INTERRUPT, executingProcessID, programList[processTable[executingProcessID].programListIndex]->executableName, systemCallID);
			OperatingSystem_TerminateExecutingProcess();
			OperatingSystem_PrintStatus();
			break;
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

	//V3 - 4 >> Insertar estadistica con el numero de procesos listoos 
	int totalReadyProcesses = 0;
	for (int i = 0; i < NUMBEROFQUEUES; i++) {
		totalReadyProcesses += numberOfReadyToRunProcesses[i];
	}
	OperatingSystem_InsertStatistics(&stats, totalReadyProcesses);


	//Candidato_PID es el PID del proceso con menor tiempo para levanttarse
	int candidato_PID = Heap_getFirst(sleepingProcessesQueue, numberOfSleepingProcesses);
	int awakened = 0; 

	//6a 6b >> Despertar procesos cuyo tiempo hay llegado 
	//Si hay procesos y el tiempo de despertar del primero sea el actual 
	while(candidato_PID != NOPROCESS 
			&& processTable[candidato_PID].whenToWakeUp == numberOfClockInterrupts){
		int pid = OperatingSystem_ExtractFromSleepingProcessesQueue();
		OperatingSystem_MoveToTheREADYState(pid);
		awakened++;
		candidato_PID = Heap_getFirst(sleepingProcessesQueue, numberOfSleepingProcesses);
	} 

	if(awakened > 0 ){
		OperatingSystem_PrintStatus();
	}

	// V3 - 2a >> Llamar al LTS en cada interrupción de reloj
	int nuevosProcesos = OperatingSystem_LongTermScheduler();

	// V3 - b >> Proponemos la parada del sistema si no hay procesos no terminados ni programas en llegada
	if(numberOfProgramsInArrivalTimeQueue == 0 && numberOfNotTerminatedUserProcesses == 0){
		OperatingSystem_ReadyToShutdown();
	}

	//V3-c >> Comprobar si hay procesos despertados o nuevos procesos en llegada para decidir si hacer un cambio de contexto
	if(awakened > 0 || nuevosProcesos > 0){
		int nuevoCandidato_PID = NOPROCESS; 
		int nuevoCandidato_QUEUE = -1;
		for(size_t i = 0; i < NUMBEROFQUEUES; i++){
			nuevoCandidato_PID = Heap_getFirst(readyToRunQueue[i], numberOfReadyToRunProcesses[i]);
			if(nuevoCandidato_PID != NOPROCESS){
				nuevoCandidato_QUEUE = i;
				break;
			}
		}
	
		if(nuevoCandidato_PID != NOPROCESS){
			int actualQueue = processTable[executingProcessID].queueID;
			int mustPreempt = 0; 
	
			if(nuevoCandidato_QUEUE < actualQueue){
				mustPreempt = 1; 
			}else if (nuevoCandidato_QUEUE == actualQueue 
					&& processTable[nuevoCandidato_PID].priority < processTable[executingProcessID].priority){
				mustPreempt = 1; 
			}
	
			if(mustPreempt){
				ComputerSystem_DebugMessage(TIMED_MESSAGE, 58, SHORTTERMSCHEDULE, 
					executingProcessID, 
					programList[processTable[executingProcessID].programListIndex] -> executableName, 
					nuevoCandidato_PID, 
					programList[processTable[nuevoCandidato_PID].programListIndex]-> executableName);
				OperatingSystem_PreemptRunningProcess();
				int selectedProcess = OperatingSystem_ShortTermScheduler();
				OperatingSystem_Dispatch(selectedProcess);
				OperatingSystem_PrintStatus();
			}
		}
	}


	return;
} 

void OperatingSystem_MoveToTheSLEEPINGState(int PID){
	OperatingSystem_SaveContext(PID);
	int previous = processTable[PID].state;
	processTable[PID].state = BLOCKED; 
	ComputerSystem_DebugMessage(TIMED_MESSAGE, 53, SYSPROC, PID, 
		programList[processTable[PID].programListIndex]-> executableName, 
		statesNames[previous], statesNames[BLOCKED]);
	Heap_add(PID, sleepingProcessesQueue, QUEUE_WAKEUP, &numberOfSleepingProcesses);
	executingProcessID = NOPROCESS;
 }


 // V3 -4 >> Calcular la media de las ultimas n cargas
 float calcular_media_movil(int *load, int used, int n){
	float media = 0;
	int start = used - n;
	if(start < 0){			
		start = 0;
	}
	for(int i = start; i < used; i++){
		media += load[i];
	}
	media = media / n;
	return media;
}

// Libera la memoria de la partición del proceso en ejecución
void OperatingSystem_ReleaseMainMemory() {
	int i;
	
	// Buscar la partición que pertenece al proceso
	for (i = 0; i < numberOfPartitionsAndHoles; i++) {
		if (partitionsAndHolesTable[i].PID == executingProcessID) {
			break; 
		}
	}

	if(i == numberOfPartitionsAndHoles)
		return;
	
	OperatingSystem_ShowPartitionsAndHolesTable("before releasing memory");
		
	ComputerSystem_DebugMessage(TIMED_MESSAGE, 45, SYSMEM, 
		i, partitionsAndHolesTable[i].initAddress, 
		partitionsAndHolesTable[i].size, 
		executingProcessID, 
		programList[processTable[executingProcessID].programListIndex]->executableName);
		
	partitionsAndHolesTable[i].PID = HOLE; // HOLE representa un hueco
		
	OperatingSystem_CoalesceHoles();
		
	OperatingSystem_ShowPartitionsAndHolesTable("after releasing memory");
}

// Condensa los huecos adyacentes en uno solo
void OperatingSystem_CoalesceHoles() {
	int coalesced = 0;
	int i = 0;
	
	while (i < numberOfPartitionsAndHoles - 1) {
		if (partitionsAndHolesTable[i].PID == HOLE && partitionsAndHolesTable[i+1].PID == HOLE) {
			partitionsAndHolesTable[i].size += partitionsAndHolesTable[i+1].size;
			OperatingSystem_RemovePartitionOrHole(i+1);
			coalesced = 1;
		} else {
			i++;
		}
	}

	if(coalesced){
		ComputerSystem_DebugMessage(TIMED_MESSAGE, 114, SYSMEM);
	}
}