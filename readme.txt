=========================================
EJERCICIOS 1-5 || V1-1-UO308701
=========================================

Ejercicio 1 ===============================
Modificamos el método Processor_DecodeAndExecuteInstruction() y añadimos un case para MEMADD_INST, definido en Instructions.def
El concepto de este programa es el siguiente: 
    1. Almacenamos el operando1 (posicion de memoria) 
    2. Leemos el valor de la posición y lo guardamos en registerMBR_CPU
    3. Ahora dependiendo del registro que modifiquemos: 
        - Pasamos al registroAcumulador la suma del registro seleccionado y la posición leida
    4. Aumentamos en 1 el register_CPU

Ejercicio 2 ================================
Creamos la función PrintProgramList que imprime aquellos programas que van a ejecutarse, 
asi como su tiempo de llegada (arrivalTime)
Definimos también dos nuevos mensajes con los códigos 101 y 102 propios y con el formato 
adecuado para el correcto funcionamiento del método

Ejercicio 3 =================================
Ahora implementamos ese método PrintProgramList para que lo ejecute la función PowerOn durante el arranque 
del sistema operativo, con la finalidad de mostrar todos los programas y su tiempo de llegada

Ejercicio 4 ==================================
EL programVerySimple no funciona dos veces debido a que este termina con un HALT, lo que finaliza la ejecución
de la simulación. En su lguar, creamos un programa prog-V1-E4 que utiliza TRAP 3 para volver de un proceso. 

Ejercicio 5 ==================================
Al ejecutar un programa inexistente, se lanza una interrupción, que finaliza y continua normalmente con la ejecución del SO. 
Si ejecutaramos un programa tras de un programa inexistente, la ejecución continuaría normalmente. 


OBSERVACIÓN: El código del programMEMADD solciitado para ejecutar por el estudiante resulta en un bucle infinito. 
Esto debido a que sumamos la posición 1, que contiene muy probablemente algo residual, y utilizamos el registro 33, 
al no existir tal registro, utilizamos el default (acumulador) y entonces la operación que realizamos con cada iteracion es: 
    acumulador = 2*acumulador -7
Haciendo esto imposible el llegar a 0 para acabar el programa en el ZJUMP 2 que va al HALT

La linea de terminal que se nos pide escribir debería ejecutar lo siguiente: 
    -progNotExist : Simplemente no carga el proceso y sigue ejecutando 
    - progTooBig : Debería saltar una interrupción al declarar un tamaño mayor del que permite de manera predeterminada
    - progInvalid : Intentamos no declarar el espacio o declarar menos del que debería emplear el programa, pero no funciona y se queda perpetuamente ejecutando fetch
    - progMEMADD : En la observación ya definimos el principal problema del programa, aunque en este caso ni siquiera llegó a ejecutarse


Los programas modificados para realizar estos 5 ejercicios son: 
+ progInvalid 
+ programMEMADD
+ prog-V1-E4
+ progTooBig
- Processor
- ComputerSystem
- Instructions.def
- messagesSTD.txt
Estos archivos van compilados en la entrega 

================================================================
        SESION 7 : Ejercicios 6-10 
=================================================================
Ejercicio 6 ============================
Ejecutamos el simulador con 4 versiones del programa, valor acorde a la variable PROCESSTABLEMAXSIZE declarada a 4. 
Al ejecutarla salta un error "SegmentationFault"

Hacemos un if comprobando comprobando NOFREENTRY y retornando (evitando el error). 

Añadimos los mensajes de error oportunos a NOFREEENTRY con el mensaje asignado 50 y sus parámetros. 
Modificamos el método OperatingSystem_LongTermScheduler

Ejercicio 7 ============================

Probamos los 3 casos: 
- Size negativo --> Error 
- Prioridad negativa --> Eror
- No hay elementos presentes --> Bucle infinito

Hacemos un if comprobando PROGRAMNOTVALID y retornando (evitando el error)

Añadimos los mensajes de error oportunos a NOFREEENTRY con el mensaje asignado 51 y sus parametros. 
MOdificamos el meótodo OperatingSystem_LongTermScheduler

Ejercicio 8 ==============================
Creamos el programa prog-V1-E8 que supera el max de proceso posible 
Al ejecutarlo genera una ejecución infinita en bucle 

De nuevo, repetimos la dinámica de los dos anteriores casos, deteniendo la creación del proceso y 
gestionando la interrupción con un mensaje concreto.


Ejercicio 9 ===============================
Creamos el programa prog-V1-E9 que supera el numero de instrucciones indicados por el programa 
Al ejecutarlo entramos en un bucle infinito de ejecución 

Repetimos la dinámica de los anteriores casos, deteniendo la ejecución y 
gestionando la interrupcion con un mensaje concreto. 


Ejercicio 10 ================================
Ejecutamos programas con las opciones indicadas por el enunciado. 
El resultado de la ejecución es: El SystemIdleProcess se carga directamente en la posición [3] de la lista de procesos

El PID inicial en caso de no especificarse es -1. Indicado en la linea 45 (en la versión actual). 

Trabajmos para que el PID sea algo aislado de la ejecución y funcionamiento correcto de los programas. 
    - Modificamos el valor predeterminado de initialPID (line 77) para que se ajuste a: 
            PROCESSTABLEMAXSIZE -1, siendo este valor el último valor con el que los 
            programas no generan errores


========================================================================
============= SESION 8 - Ejercicios 11-14 =============================
========================================================================

Ejercicio 11 =====================================================
Escribimos el código necesario: 
    - Creamos la nueva plantilla de mensajes 
    - Indicamos la llamada a ese método para cuando se mueva a Ready un proceso 

Ejercicio 12 =====================================================
Escribimos el código en cada situación donde se asigna un nuevo estado ( ".state=")
- Tras copiar y pegar el código necesario, tenemos las referencias necesarias para utilizar los mensajes 53 y 54 para pasar los parámetros correctos a estos mensajes. 

Ejercicio 13 =====================================================
Preparamos OperatingSystem.h para que podamos poseer tres colas, al igual que preparar el array de char que contiene las colas de prioridad 

    1. Asignamos a la cola oportuna cada proceso dependiendo de su tamaño 
    2. Enviamos el mensaje oportuno dependiendo de la cola en la que se encuentre cada proceso
    
Ejercicio 14 ======================================================
Creamos el nuevo SYSCALL_YIELD en OperatingSystem.h >> añadiendolo a 'SystemCallIdentifiers', 
Ahora modificamos 'OperatingSystem_HandleSystemCall', añadiendo un nuevo caso en el que se llama al SYSCALL_YIELD (4) que hemos declarado 
    Valoramos tres casos: 
        1. No hay procesos con la misma prioridad >> Escribimos el mensaje 55 pasando parámetros el PID y el nombre del programa ejecutandose 
        2. No hay procesos en la lista de programas >> Mismo que en el apartado 1 
        3. Si que existe un proceso con la misma prioridad >> Pasamos el control a ese proceso (liberando de la CPU el anterior proceso ejecutandose)