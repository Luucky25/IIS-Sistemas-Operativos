# VERSION 4 SIMULADOR VON NEUMAN

Autor : Lucas Álvarez Pérez 
 
Descripción : Cuarta fase de las prácticas de sistemas operativos. En esta versión se ha implementado un nuevo sistema de gestión de excepciones en el procesador y un sistema de administración de memoria mediante particiones dinámicas y huecos, utilizando la política de Mejor Ajuste (Best Fit).
 

## Nueva Funcionalidad: Política de Asignación de Memoria Diferenciada (Daemons vs Usuarios)

### 1. Explanation of the problem and the solution
**Problem:**
El sistema usaba por defecto la misma política de asignación para todos los procesos independientemente de si eran de sistema o de usuario. De igual manera, se detectó un pequeño error semántico en el código base donde, en un intento por hacer "Mejor Ajuste", se estaba aplicando inadvertidamente una lógica de "Peor Ajuste" (`holeSize > worstSize`).
**Solution:**
Se han diferenciado las políticas en función del tipo de proceso que solicita la memoria:
* **Procesos de usuario:** Siguen ocupando memoria con el método de *Mejor Ajuste* (corrigiéndose su lógica a `holeSize < bestSize`). Ocupan el inicio del hueco.
* **Daemons:** Empiezan a utilizar la política de *Último Ajuste* (Last Fit). Para los daemons la partición se adosa al *final del hueco*, dejando el espacio remanente como un nuevo hueco inmediatamente antes.
Por requerimientos de legibilidad de las trazas de memoria, se ha invertido el orden de los mensajes de debug, notificando la creación del nuevo hueco (si lo hay) antes de informar sobre la asignación.

### 2. Files that are modified during the process
- `OperatingSystem.c`
- `readme.txt` (Migrado íntegramente a `readme.md`)

### 3. Modifications made during the process
En **`OperatingSystem.c`**:
- **`OperatingSystem_ObtainMainMemory`**: Se recuperó la funcionalidad del argumento `PID` (retirando el atributo unused) para consultar `programList[processTable[PID].programListIndex]->type`. Se implementó la lógica diferenciada dentro del bucle de particiones (`Last Fit` para Daemons y un `Best Fit` debidamente implementado para Usuarios).
- **`OperatingSystem_CreateProcess`**: Se calcularon diferentes direcciones base `loadingPhysicalAddress` según si era Daemon o Usuario. Si se generaba un hueco sobrante y era Daemon, el hueco se insertaba desplazando los índices para que ocupase la misma posición lógica que la partición original (`partitionIndex`), colocando a la partición final tras dicho hueco (`partitionIndex + 1`).
- **Orden de impresión**: Se reordenó la salida de mensajes `43` y `44` para que el nuevo hueco sobrante se notifique antes que la partición asignada.

### 4. Trial part. Guide to know how to check if the modifications are OK
Puedes comprobar la corrección de las asignaciones de memoria ejecutando el simulador con un filtro sobre los mensajes `has been`. Utiliza el siguiente comando:

```bash
./Simulator --debugSections=M --userProgramsFile=usersFile2 --daemonsProgramsFile=daemonsFile2 --numProcesses=8 iDontExist 130 | grep "has been"
```

La salida esperada, la cual demuestra que los Daemons se alojan al final del bloque generando los huecos al principio y los procesos de usuario lo hacen de forma habitual, debe coincidir con la siguiente traza:

```
 [0] New hole [0: 0 -> 236] has been created after assigning memory to process [7 - SystemIdleProcess]
 [0] Partition [1: 236 -> 4] has been assigned to process [7 - SystemIdleProcess]
 [12] New hole [0: 0 -> 206] has been created after assigning memory to process [0 - daemon2]
 [12] Partition [1: 206 -> 30] has been assigned to process [0 - daemon2]
 [23] New hole [1: 30 -> 176] has been created after assigning memory to process [1 - ex6]
 [23] Partition [0: 0 -> 30] has been assigned to process [1 - ex6]
 [23] New hole [2: 60 -> 146] has been created after assigning memory to process [2 - ex5]
 [23] Partition [1: 30 -> 30] has been assigned to process [2 - ex5]
 [27] Partition [0: 0 -> 30] used by process [1 - ex6] has been released
 [30] New hole [2: 60 -> 116] has been created after assigning memory to process [3 - daemon2]
 [30] Partition [3: 176 -> 30] has been assigned to process [3 - daemon2]
 [30] Partition [0: 0 -> 30] has been assigned to process [4 - ex6]
 [30] New hole [3: 90 -> 86] has been created after assigning memory to process [5 - ex5]
 [30] Partition [2: 60 -> 30] has been assigned to process [5 - ex5]
 [37] Partition [0: 0 -> 30] used by process [4 - ex6] has been released
 [67] Partition [2: 60 -> 30] used by process [5 - ex5] has been released
 [67] Two or more holes has been coalesced
 [77] Partition [1: 30 -> 30] used by process [2 - ex5] has been released
 [77] Two or more holes has been coalesced
 [92] Partition [2: 206 -> 30] used by process [0 - daemon2] has been released
 [102] New hole [0: 0 -> 26] has been created after assigning memory to process [6 - daemon3]
 [102] Partition [1: 26 -> 150] has been assigned to process [6 - daemon3]
 [106] Partition [1: 26 -> 150] used by process [6 - daemon3] has been released
 [106] Two or more holes has been coalesced
 [116] Partition [2: 206 -> 30] used by process [0 - daemon2] has been released
 [131] Partition [1: 176 -> 30] used by process [3 - daemon2] has been released
 [131] Two or more holes has been coalesced
 [141] Partition [1: 236 -> 4] used by process [7 - SystemIdleProcess] has been released
 [141] Two or more holes has been coalesced
```
