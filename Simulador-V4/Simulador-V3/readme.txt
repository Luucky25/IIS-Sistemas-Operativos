VERSION 3 SIMULADOR VON NEUMAN

Autor : Lucas Álvarez Pérez 
 
Descripción : Tercera fase de las prácticas de sistema operativos - Se utiliza como base la práctica del Simulador V2.
 
 
EJERCICIOS REALIZADOS :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 
Ejercicio 1: Creación dinámica de procesos
- [OperatingSystem.c | OperatingSystem_LongTermScheduler] Se modifica el bucle `while` para que su condición sea `OperatingSystem_IsThereANewProgram() == YES`. Esto asegura que el planificador a largo plazo solo intente crear procesos cuyos programas ya hayan llegado al sistema según el reloj actual.
 
Ejercicio 2: Planificación en interrupciones de reloj
- [OperatingSystem.c | OperatingSystem_HandleClockInterrupt] Se añade una llamada a `OperatingSystem_LongTermScheduler()` en la condición del `if` principal. De esta forma, en cada interrupción de reloj se comprueba si han llegado nuevos programas.
- [OperatingSystem.c | OperatingSystem_HandleClockInterrupt] Si se ha despertado algún proceso dormido o se ha creado algún proceso nuevo, se busca el proceso más prioritario en las colas de listos.
- [OperatingSystem.c | OperatingSystem_HandleClockInterrupt] Si el proceso más prioritario de las colas de listos tiene mayor prioridad que el que está en ejecución, se produce una expulsión (preemption), se le cede la CPU al nuevo proceso y se muestra el estado del sistema.
 
Ejercicio 3: Condiciones de parada del sistema
- [OperatingSystem.c | OperatingSystem_Initialize] Se corrige la condición de apagado inicial. Ahora el sistema solo se apaga si no se han creado procesos en el instante 0 Y no quedan programas por llegar en la cola de llegadas (`numberOfProgramsInArrivalTimeQueue == 0`).
- [OperatingSystem.c | OperatingSystem_HandleClockInterrupt] Se añade una condición de apagado que se comprueba si no se han despertado ni creado procesos durante la interrupción.
- [OperatingSystem.c | OperatingSystem_TerminateExecutingProcess] Se actualiza la condición de apagado para que también verifique que no quedan programas por llegar, además de no haber procesos de usuario activos.
 
Ejercicio 4: Estadísticas de carga del sistema
- [OperatingSystem.c | OperatingSystem_HandleClockInterrupt] Al inicio de la rutina, se calcula el número total de procesos en todas las colas de listos y se inserta este valor en la estructura de estadísticas `stats` mediante `OperatingSystem_InsertStatistics()`.
- [OperatingSystem.c | OperatingSystem_HandleSystemCall] Se implementa la nueva llamada al sistema `SYSCALL_LOAD`.
- [OperatingSystem.c | OperatingSystem_HandleSystemCall] Dentro del `case SYSCALL_LOAD`, se calcula y muestra el último valor de carga, la media de los últimos 5 (si hay 5 o más) y la media de todos los valores (si hay 6 o más), mostrando 0 en caso contrario.
- [OperatingSystem.c | calcular_media_movil] Se crea una función auxiliar para calcular las medias móviles de los valores de carga almacenados.