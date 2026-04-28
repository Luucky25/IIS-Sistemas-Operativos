VERSION 4 SIMULADOR VON NEUMAN

Autor : Lucas Álvarez Pérez 
 
Descripción : Cuarta fase de las prácticas de sistemas operativos. En esta versión se ha implementado un nuevo sistema de gestión de excepciones en el procesador y un sistema de administración de memoria mediante particiones dinámicas y huecos, utilizando la política de Mejor Ajuste (Best Fit).
 
 
EJERCICIOS REALIZADOS :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 
Ejercicio 1: Nueva gestión de excepciones
    -Centralizamos y estandarizamos la forma en que el hardware (CPU) notifica al Sistema Operativo sobre errores en tiempo de ejecución. En lugar de tener comportamientos independientes por instrucción, el hardware ahora detiene la ejecución, guarda la causa del problema en un registro específico (D) y eleva una interrupción estándar. Esto permite al SO gestionar las anomalías de forma centralizada y segura.
a. [Processor.h] Se añade el enumerado `EXCEPTIONS` con los diferentes tipos de error soportados (DIVISIONBYZERO, INVALIDPROCESSORMODE, INVALIDADDRESS, INVALIDINSTRUCTION).
b. [Processor.c] Se adaptan las diferentes operaciones del procesador (`DIV`, `HALT`, `OS`, `IRET`) para que utilicen la nueva rutina `Processor_RaiseException()`, registrando el motivo en el registro D antes de interrumpir.
c. [MMU.c | MMU_SetCTRL] Se mejora la verificación de direcciones. Se comprueba si la dirección lógica es válida teniendo en cuenta tanto el modo ejecución protegido (intervalo [0, MAINMEMORYSIZE-1]) como el esquema de particiones en modo normal (intervalo [0, registerLimit_MMU-1]). Si falla, eleva `INVALIDADDRESS`.
 
Ejercicio 2: Mensajes de error personalizados para excepciones
    -Mejorar la observabilidad y auditabilidad del sistema. Al dotar al Sistema Operativo de la capacidad para traducir códigos de error del hardware a descripciones legibles (ej. "invalid address" o "division by zero"), facilitamos enormemente la depuración tanto para los desarrolladores del sistema como para los usuarios finales, aislando y terminando el programa culpable.
a. [OperatingSystem.c | OperatingSystem_HandleException] Se crea un array global `typeOfExceptions` que contiene el texto descriptivo de cada excepción. 
b. [OperatingSystem.c | OperatingSystem_HandleException] Se extrae del registro D de la CPU el código de la excepción. Se imprime el mensaje de depuración 32 indicando el error específico y acto seguido se elimina el proceso en ejecución.
 
Ejercicio 3: Tratamiento de instrucciones no válidas
    -Incremento de la robustez del procesador. Anteriormente, si un programa intentaba ejecutar código corrupto o basura (opcodes no reconocidos), el procesador lo ignoraba (comportamiento tipo NOP). Ahora, detecta operaciones ilegales y levanta una excepción, impidiendo comportamientos impredecibles y forzando al SO a terminar el proceso defectuoso.
a. [Processor.c | Processor_DecodeAndExecuteInstruction] En el bloque `switch`, se modifica el comportamiento `default` (instrucción desconocida). En lugar de avanzar el PC silenciosamente, se eleva la excepción de tipo `INVALIDINSTRUCTION` y se detiene el ciclo para esa instrucción.
b. [OperatingSystem.c] Gracias al Ejercicio 2, el Sistema Operativo ya es capaz de capturar este nuevo evento e imprimir "invalid instruction" antes de expulsar al programa.
 
Ejercicio 4: Llamadas a sistema (TRAP) inexistentes
    -Protección de la interfaz de servicios del Sistema Operativo. Si un programa de usuario solicita un servicio al SO mediante una llamada al sistema (syscall) que no está definida, el SO debe rechazarla para preservar la seguridad y estabilidad, castigando al proceso intruso o defectuoso con su finalización inmediata.
a. [OperatingSystem.c | OperatingSystem_HandleSystemCall] Se añade un caso `default` dentro de la gestión de llamadas al sistema. 
b. [OperatingSystem.c | OperatingSystem_HandleSystemCall] Si se invoca una TRAP desconocida, se emite el mensaje 33, se termina el proceso obligatoriamente invocando a `OperatingSystem_TerminateExecutingProcess()`.
c. [OperatingSystem.c | OperatingSystem_HandleSystemCall] Al finalizar, se muestra el estado del sistema mediante `OperatingSystem_PrintStatus()` ya que las colas sufren cambios.

Ejercicio 5: Tabla de Particiones y Huecos Inicial
    -Transición de una gestión de memoria estática a una gestión dinámica mediante particiones variables. Para administrar dinámicamente dónde y cómo se alojan los procesos en la memoria, el Sistema Operativo necesita unas estructuras de datos de control (Tablas). Inicialmente, todo el espacio de memoria para procesos de usuario se concibe como un único "gran hueco" libre.
a. [OperatingSystem.h] Se descomenta/define la macro de configuración `#define MEMCONFIG` para activar el soporte de código relacionado con particiones de memoria.
b. [OperatingSystem.c | OperatingSystem_Initialize] Antes de permitir la creación de cualquier proceso, se inicializa la tabla llamando a `OperatingSystem_InitializePartitionsAndHolesTable(OS_address_base)`. Esto genera un gran hueco único con la memoria no ocupada por el Sistema Operativo.

Ejercicio 6: Asignación de memoria mediante Mejor Ajuste (Best-Fit)
    -Implementación de la política de asignación de memoria Best-Fit. Esta estrategia busca en toda la tabla el hueco más pequeño que sea lo suficientemente grande para albergar al nuevo proceso. Con ello se reduce la cantidad de memoria sobrante (minimización de la fragmentación interna remanente), dejando huecos sobrantes diminutos. Si hay empate de tamaño, se prioriza la dirección física más baja para agrupar espacios.
a. [OperatingSystem.c | OperatingSystem_ObtainMainMemory] Se cambia por completo la asignación de memoria. Se implementa un algoritmo de Mejor Ajuste que recorre la tabla y escoge el hueco libre (HOLE) con el tamaño más ajustado (pero suficiente) para el proceso. En caso de empate, la iteración escoge automáticamente la partición situada en la dirección física más baja.
b. [OperatingSystem.c | OperatingSystem_ObtainMainMemory] Al iniciar la búsqueda de memoria para un proceso, se emite el mensaje 42.
c. [OperatingSystem.c | OperatingSystem_ObtainMainMemory] Si se tiene éxito, se registra el PID en la partición y se emite el mensaje 43. Si la partición era más grande de lo necesario, el sobrante se fragmenta creando un nuevo hueco que se reinserta en la tabla, emitiendo además el mensaje 44. Se ha adaptado también `OperatingSystem_CreateProcess` a la nueva naturaleza del valor retornado (el índice de la tabla en lugar de la dirección).
d. [OperatingSystem.c | OperatingSystem_ObtainMainMemory] Se utilizan las funciones base `OperatingSystem_InsertIntopartitionsAndHolesTable` y `OperatingSystem_RemovePartitionOrHole` de forma intensiva.
e. [OperatingSystem.h / OperatingSystem.c] Si falla la asignación de memoria, se devuelven códigos de error precisos. Si es por tamaño se emite el mensaje `TOOBIGPROCESS`, y si es por falta de espacio conjunto se devuelve el nuevo error `#define MEMORYFULL -5` y se muestra el mensaje correspondiente 31.

Ejercicio 7: Trazas visuales de la asignación
    -Auditoría visual de la memoria. Herramienta vital para verificar cómo afecta la política Best-Fit a la estructura de la memoria con el paso del tiempo. Imprimir la lista de huecos y particiones ocupadas ayuda a comprobar si los procesos rellenan correctamente la memoria y cómo el espacio se va "astillando" (fragmentando).
a. [OperatingSystem.c | OperatingSystem_ObtainMainMemory] Justo después de garantizar que existe un hueco válido y antes de operar sobre él, se llama a la función `OperatingSystem_ShowPartitionsAndHolesTable` pasándole el texto "before allocating memory".
b. [OperatingSystem.c | OperatingSystem_ObtainMainMemory] Tras aplicar las modificaciones sobre la partición seleccionada y/o la generación de posibles huecos remanentes, se vuelve a invocar a la misma función pasándole el texto "after allocating memory".

Ejercicio 8: Liberación y Coalescencia de huecos de memoria
    -Combate contra la fragmentación externa (defragmentación). Si solo se liberase la memoria sin ordenarla, el sistema terminaría con infinidad de minihuecos aislados, impidiendo cargar programas grandes (MEMORYFULL) a pesar de tener memoria total suficiente. La coalescencia resuelve esto fusionando huecos contiguos en bloques continuos e indivisos cada vez que un proceso acaba.
a. [OperatingSystem.c] Se crea la función `OperatingSystem_ReleaseMainMemory()`. Ésta se llama siempre desde la función de finalización de procesos. Busca qué partición de la memoria estaba asignada al proceso, emite el mensaje 45, y la libera marcándola con `NOPROCESS` (convirtiéndola en hueco). 
b. [OperatingSystem.c] Se implementa la función de condensación `OperatingSystem_CoalesceHoles()`. Funciona como un bucle iterativo que analiza toda la tabla de particiones combinando pares de huecos adyacentes en uno único más grande. Al acabar de barrer la tabla, si ha realizado al menos una fusión, imprime el mensaje 114 de éxito.
c. [OperatingSystem.c] Se incluyen las trazas de estado "before releasing memory" y "after releasing memory" en el proceso de liberación.
d. [messagesSTD.txt] Se añade el mensaje de estudiante número 114 coloreado en verde: "[72] Two or more holes have been coalesced".
