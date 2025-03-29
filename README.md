# FLOWCORE V4

El sistema será alimentado directamente de la linea eléctrica y luego adaptará las tensiones necesarias para el resto del circuito. El microcontrolador tendrá a cargo la gestión del sistema. Por un lado, recibirá los datos a través de los adaptadores que serán los encargados de acondicionar las mediciones para una correcta lectura en el microcontrolador. Luego de procesarlos, hará uso de los actuadores para la ejecución de las acciones de control. Finalmente, intercambiará datos a través de la interfaz de usuario tanto para configuraciones, como para exhibir el estado del control. 

<img src="img/Diagrama de bloques.png" alt="Texto alternativo" width="300">

## Tabla de Contenidos

1. [Requisitos Previos](#requisitos-previos)  
2. [Instalación](#instalación)  
3. [Configuración](#configuración)  
4. [Uso](#uso)  
5. [Estructura del Proyecto](#estructura-del-proyecto)  

---

## Requisitos Previos

---

## Instalación

---

## Configuracion

---
## Uso

---
## Estructura del proyecto
```
/
├── config/             # Archivos de configuración 
├── lib/                # Librerías adicionales
├── src/flowcoreV4/     
│   ├── flowcoreV4.ino  # Código fuente
├── test/               # Archivos para pruebas unitarias
├── mediciones/         # Archivos para mediciones
├── README.md           # Este archivo
└── LICENSE             # Información de licencia
```