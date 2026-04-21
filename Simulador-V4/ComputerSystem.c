#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ComputerSystem.h"
#include "OperatingSystem.h"
#include "ComputerSystemBase.h"
#include "Processor.h"
#include "Messages.h"
#include "Asserts.h"
#include "Wrappers.h"

heapItem *arrivalTimeQueue;
int numberOfProgramsInArrivalTimeQueue=0;
// Functions prototypes

// Powers on of the Computer System.
void ComputerSystem_PowerOn(int argc, char *argv[], int paramIndex) {
	
	ComputerSystem_DebugMessage(TIMED_MESSAGE, 99, POWERON, "STARTING simulation\n");
	
	// Obtain a list of programs in the command line
	int programsFromFilesBaseIndex = ComputerSystem_ObtainProgramList(argc, argv, paramIndex);
	
	//Llamada a PrintProgramList antes del arranque del SO 
	ComputerSystem_PrintProgramList();

	// Request the OS to do the initial set of tasks. The last one will be
	// the processor allocation to the process with the highest priority
	OperatingSystem_Initialize(programsFromFilesBaseIndex);
	
	// Tell the processor to begin its instruction cycle 
	Processor_InstructionCycleLoop();
	
}

// Powers off the CS (the C program ends)
void ComputerSystem_PowerOff() {
	// Show message in red colour: "END of the simulation\n" 
	ComputerSystem_DebugMessage(TIMED_MESSAGE,99,SHUTDOWN,"END of the simulation\n"); 
	exit(0);
}


/////////////////////////////////////////////////////////
//  New functions below this line  //////////////////////

//Print the contents at the ProgramLists
//
//	Format : Program_[Example1]_with_arrival_time_[0]
void ComputerSystem_PrintProgramList(){
	//IMprimir mensaje cabecera x101
	ComputerSystem_DebugMessage(NO_TIMED_MESSAGE, 101, INIT);
	//Recorrer ProgramList e imprimir mensaje x102
	for(int i=1; i<PROGRAMSMAXNUMBER; i++){
		if(programList[i] != NULL){
			ComputerSystem_DebugMessage(NO_TIMED_MESSAGE, 102, INIT,programList[i]->executableName,  programList[i]->arrivalTime);
		}
	}
}