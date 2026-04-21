VERSION 2 SIMULADOR VON NEUMAN

Autor : Lucas Álvarez Pérez 

Descripción : Segunda fase de las prácticas de sistema operativos - Se utiliza como base la práctica del Simulador V1


EJERCICIOS a realizar :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Ejercicio 0 ---------
Configuramos OperatingSystem_Initialize para limpiar la tabla de procesos poniendo casi todos
los valores a -1
Ajustamos OperatingSystem_PCBInitialization para los registros de trabajo a 0 


Ejercicio 1-----------
Modificamos Clock.c para que cada 5 ticks se envía una señal al procesador por la línea 9 

Creamos el manejador HandleClockInterrupt, cada vez que se activa el reloj
el sistema incrementa un contador global y nos avisa con un mensaje en color cyan 

Ejercicio 2 -----------
Definimos el bit 15 de la PSW como INTERRUPTED_MASK

Modificamos Processor.c para que al detectar interrupciones, se active automaticamente con un INTERRUPTED_MASK

Actualizamos la visualización para ver una M 


Ejercicio 3 y 4 ------------
Incorporamos llamadas a PrintStatus, por lo que comentamos los mensajes de estado antiguos 

Ejercicio 5 -----------------------------------
Introducir el estado BLOCKED y permitir que un proceso ceda el procesador por un tiempo 

    a) Añadir el campo whenToWakeUp al PCB y definir el marco SLEEPINGQUEUE en la cabecera de OperatingSysten

    b/c) Se añade el registro fisico registerD_CPU. La instrucción TRAP ahora guarda su segundo operando en este registro antes de saltar al SO 

    d/e) Creamos sleepingProcessQueue gestionada con un Heap e inicializarlo en OperatingSystem_Initialize 

    f) Se implementa el case SYSCALL_SLEEP calculando whenToWakeUp = delay + numberOfClockInterrupts +1. Creamos la función MoveToTheBlockedState() para gestionar la inserción en la cola


Ejercicio 6 ------------------------------------
Permitir que los procesos bloqueados vuelvan a ser candidatos a ejecutar y aplicar preempción si son prioritarios 

    a/b) En HandleClockInterrupt, el SO pasa a extraer de sleepingProcessesQueue a los procesos cuyo whenToWakeUp coincida con las interrupciones actuales y los mueve a READY

    c,d,e) Tras despertar procesos, se invoca al scheduler
            -Si tiene prioridad mayor >> Expulsa con el mensaje 58
            -Si no hay cambio, el candidato se devuelve a su cola de listos 
            -Se llama a PrintStatus si hubo algún despertar o cambio de proceso


