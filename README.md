# FLOWCORE V4

<p align="justify">
El sistema será alimentado directamente de la linea eléctrica y luego adaptará las tensiones necesarias para el resto del circuito. El microcontrolador tendrá a cargo la gestión del sistema. Por un lado, recibirá los datos a través de los adaptadores que serán los encargados de acondicionar las mediciones para una correcta lectura en el microcontrolador. Luego de procesarlos, hará uso de los actuadores para la ejecución de las acciones de control. Finalmente, intercambiará datos a través de la interfaz de usuario tanto para configuraciones, como para exhibir el estado del control. 
</p>

<img src="img/Diagrama de bloques.png" alt="Diagrama de bloques" width="100%">

## Tabla de Contenidos

1. [Requisitos Previos](#requisitos-previos)  
2. [Configuración](#configuración)  
3. [Instalación](#instalación)  
4. [Uso](#uso)  
5. [Estructura del Proyecto](#estructura-del-proyecto)  

---

## Requisitos Previos
Es necesario usar el IDE de Arduino e instalar las siguientes librerias:

- TFT_espi by Bodmer v2.5.43
- XPT2046_Touchscreen by Paul Stoffregen v1.4
- LVGL by kisvegabor v9.1.0
- PubSubClient by Nick O'Leary v2.8
- WiFiManager by tzapu v2.0.
- ArduinoJson by Benoit Blanchon v7.3.0

---

## Configuracion
1. Abrir File->Preferences, y ver Sketchbook location. 
2. Ir a esa dirección en el explorador de archivos y entrar a la carpeta "libraries".
3. Sobreescribir el archivo "lv_conf.h" con el que esta disponible en el respositorio dentro de la carpeta "setup".
4. Entrar a la carpeta TFT_eSPI y reemplazar los archivos "User_Setup_Select.h" y "User_Setup.h" con los disponibles en la carpeta "setup" del repositorio.
5. Volver a la carpeta "libraries" y copiar en ella la carpeta "ui" que se encuentra dentro de "modules" en el repo.
6. Entrar a "ui" y abrir el archuivo "ui.h". Luego reemplazar las lineas indicadas con la ubicacion de los archivos. Hay que buscar en que ubicacion de la computadora se guardo la carpeta "modules" y cambiar "G:\Mi unidad\FLOWING\FLOWCORE V4\FlowCore-V4.0\" por esa ubicación.

---

## Instalación
Para poder cargar el programa sobre la ESP32-S3, se deben seguir los pasos a continuación:

1. Desde el board manager instalar esp32 by Espressif Systems
2. En el menu ir a tools->Board->esp32 y seleccionar ESP32S3 Dev Module
3. En el menu ir a tools y seleccionar:
    1. USB CDC On Boot: "Enable"
    2. Flash Size: 16MB (128Mb)"
    3. Partition Scheme: "16M Flash (3MB APP/9.9MB FATFS)"

Finalmente se puede hacer el upload del archivo "src/flowcoreV4.ino". Es importante que en caso que sea la primera carga sobre el microcontrolador, antes de inicar la programacion, se presione el boton de BOOT, y se mantenga apretado mientras se presiona el boton RESET y mientras se carga el programa. Una vez la programacion fue exitosa es posible soltar el boton de BOOT.

---

## Uso

---
## Estructura del proyecto
```
/
├── img/                # Imagenes para el README
├── lib/                # Librerías adicionales
├── mediciones/         # Codigo para caracterización de los sensores
├── modules/            # Modulos en los que se basa el programa
├── setup/              # Archivos de configuración 
├── src/flowcoreV4/     
│   ├── flowcoreV4.ino  # Código fuente
├── test/               # Archivos para pruebas unitarias
├── README.md           # Este archivo
```