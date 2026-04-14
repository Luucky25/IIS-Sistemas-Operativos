VERSION 3 SIMULADOR VON NEUMAN

Autor : Lucas Álvarez Pérez 

Descripción : Segunda fase de las prácticas de sistema operativos - Se utiliza como base la práctica del Simulador V2


EJERCICIOS a realizar :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Ejercicio 1
Modificamos la condicion en el OperatingSystem.c >> LongTermScheduler para que en lugar de comrpobar si no es EMPTYPROCESS,
y pasamos a comprobar que la funcion IsThereANewProgram(). 


Ejercicio 2
En HandleClockInterrupt pasamos a comrpobar además de que haya algún proceso despierto, que el LongTermScheduler devuelva un PID mayor que 0


Ejercicio 3
    a) Añadimos a la comprobacion para hacer un shutdown que no existan nuevos procesos en llegada 

    b) Proponemos la parada del sistema si no hay procesos no terminados ni programas en llegada en HandleClockInterrupt

