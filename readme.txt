VERSION 2 SIMULADOR VON NEUMAN

Autor : Lucas Álvarez Pérez 

Descripción : Segunda fase de las prácticas de sistema operativos - Se utiliza como base la práctica del Simulador V1


EJERCICIOS :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

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


