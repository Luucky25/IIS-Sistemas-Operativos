VERSION 4 DE LA VERSION 4 VON NEUMAN

Autor : Lucas Álvarez Pérez 
 
Descripción : Ejercicio 1 simulacro examen de la versión V4
 
EJERCICIOS REALIZADOS :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
EJERCICIO 1 (Examen): Invocación condicional del Planificador a Largo Plazo por Excepción

PROCEDIMIENTO Y JUSTIFICACIÓN DE LOS CAMBIOS:
El objetivo era invocar al Planificador a Largo Plazo (LTS) cuando un proceso finaliza por una excepción, respetando condiciones de tiempo (>= 2 tics desde su última ejecución), procesos de usuario previos creados (deben ser 0), y evitar un doble cambio de contexto consecutivo.

1. Control del estado del LTS (OperatingSystem.c):
   - Justificación: Para saber la información histórica del LTS, necesitamos guardarla en variables globales cada vez que este finaliza su ejecución para su futura consulta.
   - Implementación: Se añadieron las variables globales `last_LTS_tick` y `last_LTS_user_processes`. En `OperatingSystem_LongTermScheduler()`, se usa una variable local `userProcessesCreated` que se incrementa únicamente si el proceso creado y alojado es de tipo `USERPROGRAM`. Al salir del método, este contador junto con `Clock_GetTime()` actualizan las globales correspondientes.

2. Detección del origen de terminación del proceso (OperatingSystem.c):
   - Justificación: La función `OperatingSystem_TerminateExecutingProcess` gestiona la muerte de procesos de forma genérica (ya sea por una finalización normal o por error). Necesitamos informarle de cuándo la terminación procede específicamente de una excepción.
   - Implementación: Se añadió la variable global bandera `terminatingFromException`. En `OperatingSystem_HandleException`, este flag se enciende a 1 justo antes de mandar a terminar el proceso, y se restablece a 0 a su regreso.

3. Invocación al LTS sin doble planificación (OperatingSystem.c):
   - Justificación: Si el LTS se invocaba de forma plana al volver del `TerminateExecutingProcess`, tendríamos un problema: el sistema ya habría llamado al Planificador a Corto Plazo (STS) y habría invocado a `Dispatch()`. Ejecutar el LTS después de esto podría introducir un nuevo proceso a la cola de mayor prioridad, forzando a replanificar e invocar `Dispatch` nuevamente (rompiendo el requisito de seleccionar el próximo proceso en una sola ocasión). Además, es fundamental que la partición de memoria del proceso con errores esté liberada para que el LTS pueda reciclarla.
   - Implementación: Por estos motivos, la comprobación se insertó de manera estratégica DENTRO del flujo de `OperatingSystem_TerminateExecutingProcess`, justo *después* de liberar la memoria del proceso deficiente (`OperatingSystem_ReleaseMainMemory`) y *antes* del `OperatingSystem_ShortTermScheduler`. Si la bandera `terminatingFromException == 1` y se cumplen el resto de validaciones LTS, se invoca al mismo. Al terminar este LTS anidado, el ciclo sigue su curso y el ShortTermScheduler se ejecuta de forma natural una única vez seleccionando al mejor candidato definitivo.

4. Formato visual y de Mensajes (messagesSTD.txt):
   - Implementación: Se añadió la definición del mensaje solicitado con su formato visual en magenta `@M...@@` (mensaje número 41/115) para ser impreso justo antes de que el LTS interceda por la excepción.
