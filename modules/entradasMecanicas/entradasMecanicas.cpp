#include "entradasMecanicas.h"

#include "..\contador\contador.h"

// =====[Declaracion de defines privados]============

#define ENTRADA_TERMOSTATO_SEG  36
#define ENTRADA_TERMOSTATO_AMB  35
#define ENTRADA_FLOW_SW_BOMBAS  33
#define ENTRADA_FLOW_SW_ACS     34

// =====[Declaracion de tipos de datos privados]=====

int estadoEntradas[CANT_ENTRADAS];
uint64_t tiempoDesdeCambioEntradas[CANT_ENTRADAS];
int entradas[CANT_ENTRADAS] = {ENTRADA_TERMOSTATO_SEG, ENTRADA_TERMOSTATO_AMB, ENTRADA_FLOW_SW_BOMBAS, ENTRADA_FLOW_SW_ACS};

// =====[Declaracion de funciones privadas]==========

// =====[Implementacion de funciones publicas]=======

void inicializarEntradasMecanicas(){
    for (int i=0; i < CANT_ENTRADAS; i++){
        if (entradas[i] != 0){
            pinMode(entradas[i], INPUT_PULLDOWN);
            estadoEntradas[i] = digitalRead(entradas[i]);
            tiempoDesdeCambioEntradas[i] = 0;
            contarMs(&tiempoDesdeCambioEntradas[i],0,HIGH);
        }
    }
}

void actualizarEntradasMecanicas(){
    int nuevoEstado;

    for (int i=0; i < CANT_ENTRADAS; i++){
        
      if (entradas[i] != 0){
        nuevoEstado = digitalRead(entradas[i]);
        if (nuevoEstado != estadoEntradas[i]){
            estadoEntradas[i] = nuevoEstado;
            contarMs(&tiempoDesdeCambioEntradas[i],0,HIGH);
        }
      }
    }
}

const int* obtenerEstadoEntradasMecanicas(){
    return estadoEntradas;
}

const uint64_t* obtenerTiempoCambioEntradasMecanicas(){
    return tiempoDesdeCambioEntradas;
}

// =====[Implementacion de funciones privadas]=======