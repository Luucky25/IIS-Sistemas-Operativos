# 💻 Simulador Von Neumann 
Repositorio dedicado a la evolución de un sistema operativo y hardware simulado.

## 🚀 Evolución del Sistema
V0 ➡️ Introducción al lenguaje y simulador base.

V1 ➡️ Gestión de procesos, cambio de contexto y excepciones.

V2 ➡️ Tiempo real (reloj), estados de bloqueo y preempción.

V3 ➡️ Creación dinámica de procesos, nuevas condiciones de parada y estadísticas de carga del sistema.

V4 ➡️ Gestión avanzada de excepciones, control de instrucciones/llamadas inválidas y administración de memoria (particiones dinámicas mediante Mejor Ajuste).

***

## 📂 Estructura del Repositorio
- **Carpetas `V*`**: Contienen el código fuente y los ejercicios correspondientes a cada una de las versiones evolutivas del simulador.
- **Carpetas `Entrega*`**: Contienen las versiones finales y empaquetadas para las entregas oficiales de las prácticas.
- **Carpetas de `Simulacros*`**: Existen varios directorios (ej. *Simulacros_Examen*) que contienen ejercicios de repaso y pruebas diseñadas para la preparación de exámenes.

***

## 🛠️ Tecnologías
Lenguaje: C.

Automatización: GNU Make.

Entorno: Recomendado Unix/Linux o WSL.

***

## 📖 Guía de Uso

Es necesario saber, que el gitIgnore actualmente incluye los archivos tras compilación .o . Es decir, estos archivos son omitidos y por tanto, cualquier clone que se realice obvia estos archivos. 

Por tanto, esta guía debe usarse bajo esta situación y bajo cualquier modificación realizada en el proyecto, de manera previa a su ejecución. 

Compilar:

```Bash
make
```
Ejecutar:

```Bash
./Simulator
```
Limpiar:

```Bash
make clean
```
Autor: Lucas Álvarez Pérez
