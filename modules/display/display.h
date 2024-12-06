#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <SPI.h>

// =====[Declaracion de defines publicos]============

// =====[Declaracion de tipos de datos publicos]=====

// =====[Declaracion de funciones publicas]==========

void inicializarDisplay();

void actualizarDisplay(int tiempoRefresco_ms);

SPIClass& obtenerInstanciaSPI();

#endif