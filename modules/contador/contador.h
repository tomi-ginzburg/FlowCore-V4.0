#ifndef CONTADOR_H_
#define CONTADOR_H_

#include <Arduino.h>

// =====[Declaracion de defines publicos]============


// =====[Declaracion de tipos de datos publicos]=====


// =====[Declaracion de funciones publicas]==========

/*
 * pre: -
 * post: inicia el timer 0 generar una interrupcion cada 1ms
 */
void inicializarContador();

/*
 * pre: intervalo debe ser positivo y entero
 * post: devuelve true cada vez que pasan intervalo ms
 */
bool loopMs(uint64_t intervalo);

/*
 * pre: el puntero a variable no debe nulo, y el intervalo debe ser mayor a 0
 * post: si la variable ya se estaba usando para contar se actualiza el valor, sino se agrega en la lista de contadores 
 * la cuenta es ascendente si tipoConteo==HIGH o descendente si tipoConteo==LOW
 */
void contarMs(uint64_t* variable, uint64_t intervalo, bool tipoConteo);

/*
 * pre: el puntero a variable no sea nulo
 * post: elimina el puntero de listaContadorAscendente o listacontadorDescendente si se encuentra dentro de ellas
 */
void frenarContarMs(uint64_t* variable);

#endif