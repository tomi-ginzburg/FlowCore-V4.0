#ifndef ENTRADAS_MECANICAS_H_
#define ENTRADAS_MECANICAS_H_

#include <Arduino.h>
// =====[Declaracion de defines publicos]============

#define CANT_ENTRADAS       4

// Orden en el array de estadoEntradas
#define TERMOSTATO_SEG      0
#define TERMOSTATO_AMB      1
#define FLOW_SW_BOMBAS      2
#define FLOW_SW_ACS         3

// =====[Declaracion de tipos de datos publicos]=====

// =====[Declaracion de funciones publicas]==========

/*
 * pre: -
 * post: inicializa los pines distintos de 0 como entradas pulldown. Guarda las primeras lecturas
 *      en el array estadoEntradas, y comienza a contar los tiempo de cada entrada. 
 */
void inicializarEntradasMecanicas();

/*
 * pre: -
 * post: lee los nuevos estados de las entradas y actualiza el valor de estadoEntradas y cambioEstadoEntradas.
 */
void actualizarEntradasMecanicas();

/*
 * pre: -
 * post: devuelve un puntero constante a estadoEntradas
 */
const int* obtenerEstadoEntradasMecanicas(); 

/*
 * pre: -
 * post: devuelve un puntero constante a tiempoCambioEstadoEntradas
 */
const uint64_t* obtenerTiempoCambioEntradasMecanicas(); 


#endif