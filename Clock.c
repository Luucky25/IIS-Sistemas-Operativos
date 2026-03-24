#include "Clock.h"
#include "Processor.h"
#include "ComputerSystemBase.h"

int tics=0;

void Clock_Update() {
	tics++;
	//Interval Between Interrupts es una variable global del simulador 
	if(tics % intervalBetweenInterrupts == 0){
		Processor_RaiseInterrupt(CLOCKINT_BIT);
	}
}


int Clock_GetTime() {

	return tics;
}

