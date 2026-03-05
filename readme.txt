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