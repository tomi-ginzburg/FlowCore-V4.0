#ifndef SENSORES_H_
#define SENSORES_H_

// =====[Declaracion de defines publicos]============

#define CANT_SENSORES   3
#define PT100_1         0
#define PT100_2         1
#define LOOP_CORRIENTE  2

#define CICLOS_PT100_1  6
#define CICLOS_PT100_2  6
#define CICLOS_LOOP_I   6

#define VALOR_ERROR_SENSOR  1000

// =====[Declaracion de tipos de datos publicos]=====

// =====[Declaracion de funciones publicas]==========

/*
 * pre: -
 * post: Inicia la configuracion con el ADS1148, y le da las configuraciones iniciales
 */
void inicializarSensores();

/*
 * pre: haber llamado a inicializarSensores()
 * post: maneja la configuracion y la toma de datos de los 3 sensores 
 */
void actualizarSensores();

/*
 * pre: -
 * post: devuelve un puntero constante a los valores de los sensores
 */
const float* obtenerValorSensores();

#endif