# Ejercicio: Invocación del LTS por Excepción

Este documento explica los cambios requeridos para añadir una tercera situación bajo la cual se invoca al planificador a largo plazo (LTS). Esta situación ocurre cuando un proceso finaliza por una excepción, siempre y cuando la ejecución previa del LTS no haya creado procesos de usuario y hayan pasado al menos 2 tics de reloj.

## 1. Archivos modificados

* **`OperatingSystem.c`**: Contiene la lógica del sistema operativo (planificadores, gestión de excepciones y terminación de procesos). Es el archivo principal donde recae la lógica del ejercicio.
* **`messagesSTD.txt`**: Archivo de configuración donde se incluye el mensaje de debug número 115 para la sección de examen.

## 2. Modificaciones realizadas

Para cumplir con todos los requisitos del ejercicio, se han realizado los siguientes ajustes en el código de `OperatingSystem.c`:

* **Inclusión de librerías necesarias:** Se ha incluido `#include "Clock.h"` en la cabecera para poder utilizar la función `Clock_GetTime()` y medir el paso real del tiempo en el sistema, en lugar del número de interrupciones.
* **Registro preciso del estado del LTS:** 
  * En la función `OperatingSystem_LongTermScheduler()`, se ha inicializado una variable `userProcessesCreated = 0` y se incrementa en `1` únicamente cuando el tipo de programa cargado es `USERPROGRAM`.
  * Al finalizar la función, se guardan los datos de la ejecución de forma global: `lastLTSTic = Clock_GetTime();` y `lastLTSCreatedUserProcesses = userProcessesCreated;`.
* **Control y disparo por excepción:** 
  * En `OperatingSystem_HandleException()`, tras terminar el proceso infractor, se calcula el tiempo transcurrido: `int ticksSinceLastLTS = Clock_GetTime() - lastLTSTic;`.
  * Se evalúa si `lastLTSCreatedUserProcesses == 0 && ticksSinceLastLTS >= 2`. Si se cumple, se imprime el mensaje personalizado `115` (`ComputerSystem_DebugMessage(TIMED_MESSAGE, 115, EXAM, ticksSinceLastLTS);`) y se fuerza una invocación manual a `OperatingSystem_LongTermScheduler()`.
* **Prevención de doble selección del STS:** 
  * Se ha extraído la invocación a `OperatingSystem_ShortTermScheduler()` y `OperatingSystem_Dispatch()` fuera de `OperatingSystem_TerminateExecutingProcess()`. 
  * Ahora, las rutinas superiores como `OperatingSystem_HandleSystemCall()` o `OperatingSystem_HandleException()` son responsables de llamar al Short-Term Scheduler en el orden correcto (después de que el LTS haya tenido oportunidad de ejecutarse).

## 3. Archivos de prueba y líneas de ejecución

Para poner a prueba esta implementación, se requiere el uso del siguiente comando en la terminal utilizando el simulador compilado:

```bash
./Simulator --debugSections=Xd --numProcesses=8 --daemonsProgramsFile=daemonsFile1 --initialPID=5 ex1 7 ex2 8 ex3 9 ex4 20 iDonExist 37
```

### Requisitos para la ejecución:
1. Archivo **`daemonsFile1`** existente en el mismo directorio (incluido en el sistema base).
2. Ficheros que lancen excepciones cargados a lo largo del tiempo (como `ex1`, `ex2`, `ex3`, `ex4`).
3. Un programa inexistente o que falle deliberadamente en su carga (`iDonExist`), para comprobar que no produce una falsa creación de procesos de usuario en el STS.

### Resultado Esperado:
Se deberá comprobar en la consola que salta el siguiente mensaje en color magenta:

> `[41] The Long-Term Scheduler is executed because of an exception [4] ticks after its last execution`

De acuerdo a los datos de la simulación, este mensaje específico debería activarse correctamente en los **tics 21, 31 y 41** si la medición del tiempo (utilizando `Clock_GetTime()`) y el conteo de procesos (`userProcessesCreated`) funcionan de manera adecuada.
