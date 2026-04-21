VERSION 4 SIMULADOR VON NEUMAN

Autor : Lucas Álvarez Pérez 
 
Descripción : Cuarta fase de las prácticas de sistemas operativos. En esta versión se ha implementado un nuevo sistema de gestión de excepciones en el procesador, así como el control de llamadas a sistema inválidas.
 
 
EJERCICIOS REALIZADOS :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 
Ejercicio 1: Nueva gestión de excepciones
a. [Processor.h] Se añade el enumerado `EXCEPTIONS` con los diferentes tipos de error soportados (DIVISIONBYZERO, INVALIDPROCESSORMODE, INVALIDADDRESS, INVALIDINSTRUCTION).
b. [Processor.c] Se adaptan las diferentes operaciones del procesador (`DIV`, `HALT`, `OS`, `IRET`) para que utilicen la nueva rutina `Processor_RaiseException()`, registrando el motivo en el registro D antes de interrumpir.
c. [MMU.c | MMU_SetCTRL] Se mejora la verificación de direcciones. Se comprueba si la dirección lógica es válida teniendo en cuenta tanto el modo ejecución protegido (intervalo [0, MAINMEMORYSIZE-1]) como el esquema de particiones en modo normal (intervalo [0, registerLimit_MMU-1]). Si falla, eleva `INVALIDADDRESS`.
 
Ejercicio 2: Mensajes de error personalizados para excepciones
a. [OperatingSystem.c | OperatingSystem_HandleException] Se crea un array global `typeOfExceptions` que contiene el texto descriptivo de cada excepción. 
b. [OperatingSystem.c | OperatingSystem_HandleException] Se extrae del registro D de la CPU el código de la excepción. Se imprime el mensaje de depuración 32 indicando el error específico y acto seguido se elimina el proceso en ejecución.
 
Ejercicio 3: Tratamiento de instrucciones no válidas
a. [Processor.c | Processor_DecodeAndExecuteInstruction] En el bloque `switch`, se modifica el comportamiento `default` (instrucción desconocida). En lugar de avanzar el PC silenciosamente, se eleva la excepción de tipo `INVALIDINSTRUCTION` y se detiene el ciclo para esa instrucción.
b. [OperatingSystem.c] Gracias al Ejercicio 2, el Sistema Operativo ya es capaz de capturar este nuevo evento e imprimir "invalid instruction" antes de expulsar al programa.
 
Ejercicio 4: Llamadas a sistema (TRAP) inexistentes
a. [OperatingSystem.c | OperatingSystem_HandleSystemCall] Se añade un caso `default` dentro de la gestión de llamadas al sistema. 
b. [OperatingSystem.c | OperatingSystem_HandleSystemCall] Si se invoca una TRAP desconocida, se emite el mensaje 33, se termina el proceso obligatoriamente invocando a `OperatingSystem_TerminateExecutingProcess()`.
c. [OperatingSystem.c | OperatingSystem_HandleSystemCall] Al finalizar, se muestra el estado del sistema mediante `OperatingSystem_PrintStatus()` ya que las colas sufren cambios.