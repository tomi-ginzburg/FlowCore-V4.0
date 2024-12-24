#include "reles.h"

#include "..\contador\contador.h"

// =====[Declaracion de defines privados]============

#define RELE_ETAPA_1 	  15
#define RELE_ETAPA_2    16
#define RELE_ETAPA_3    17
#define RELE_ETAPA_4    18
#define RELE_ETAPA_5    6
#define RELE_ETAPA_6    5
#define RELE_ETAPA_7    4

// =====[Declaracion de tipos de datos privados]=====

int pinesReles[CANT_ETAPAS] = {RELE_ETAPA_1, RELE_ETAPA_2, RELE_ETAPA_3, RELE_ETAPA_4, RELE_ETAPA_5, RELE_ETAPA_6, RELE_ETAPA_7};
bool estadoEtapas[CANT_ETAPAS];

uint64_t tiempoEncendidoEtapas[CANT_ETAPAS];
int64_t limiteEncendidoEtapas[CANT_ETAPAS];
uint64_t tiempoApagadoEtapas[CANT_ETAPAS];
int64_t limiteApagadoEtapas[CANT_ETAPAS];

bool flagsActivarRele[CANT_ETAPAS-1];
bool flagsDesactivarRele[CANT_ETAPAS-1];
bool flagsEtapasDesactivadas[CANT_ETAPAS-1];

// =====[Declaracion de funciones privadas]==========

// =====[Implementacion de funciones publicas]=======

void inicializarReles(){

	for (int i=0; i < CANT_ETAPAS - 1; i++){
		pinMode(pinesReles[i], OUTPUT);
		digitalWrite(pinesReles[i], HIGH);
		
    flagsActivarRele[i] = false;
		flagsDesactivarRele[i] = false;
    flagsEtapasDesactivadas[i] = false;

    tiempoEncendidoEtapas[i] = 0;
    limiteEncendidoEtapas[i] = 0;
    tiempoApagadoEtapas[i] = 100000;
    limiteApagadoEtapas[i] = 0;
		
    estadoEtapas[i] = false;
	}
    
	// El rele de emergencia que conecta Vcc a tierra lo dejo en estado de alta impedancia
	pinMode(pinesReles[CANT_ETAPAS-1], INPUT);
	estadoEtapas[CANT_ETAPAS-1] = false;
  tiempoEncendidoEtapas[CANT_ETAPAS-1] = 0;
  limiteEncendidoEtapas[CANT_ETAPAS-1] = 0;
  tiempoApagadoEtapas[CANT_ETAPAS-1] = 0;
  limiteApagadoEtapas[CANT_ETAPAS-1] = 0;
}

void actualizarReles(){
	for (int i=0; i < CANT_ETAPAS - 1; i++){
		
    if (flagsActivarRele[i] == true){
			// Prendo la etapa y cambio los flags
      digitalWrite(pinesReles[i], LOW);
			estadoEtapas[i] = true;
			flagsActivarRele[i] = false;

      // Empiezo a contar el tiempo que esta prendida la etapa si se solicito (limiteEncendido >=0) y ( o no estaba encendida de antes o fue por apagado automatico)
      if (limiteEncendidoEtapas[i] >= 0 && (tiempoEncendidoEtapas[i] == 0 || flagsDesactivarRele[i] == true)){
        contarMs(&tiempoEncendidoEtapas[i], limiteEncendidoEtapas[i], HIGH);
      }

      // Freno el contador de tiempo apagado y lo pongo en 0
      frenarContarMs(&tiempoApagadoEtapas[i]);
      tiempoApagadoEtapas[i] = 0;
		}
    
		if (flagsDesactivarRele[i] == true && tiempoEncendidoEtapas[i] >= limiteEncendidoEtapas[i]){
			// Apago la etapa y cambio los flags
			digitalWrite(pinesReles[i], HIGH);
			estadoEtapas[i] = false;
			flagsDesactivarRele[i] = false;

      // Empiezo a contar el tiempo que esta apagada la etapa si se solicito (limiteApagado >=0) y no estaba apagada de antes
      if (limiteApagadoEtapas[i] >= 0 && tiempoApagadoEtapas[i] == 0){
        contarMs(&tiempoApagadoEtapas[i], limiteApagadoEtapas[i], HIGH);
      }
      // freno el contador de tiempo encendido y lo pongo en 0
      frenarContarMs(&tiempoEncendidoEtapas[i]);
      tiempoEncendidoEtapas[i] = 0;
		}
	}
}

void solicitarActivarRele(int numeroRele, bool contarTiempo, int64_t tiempoLimite, bool apagadoAutomatico){
  if (numeroRele < 0 || numeroRele >= CANT_ETAPAS || tiempoLimite < 0 || flagsEtapasDesactivadas[numeroRele] == true){
    return;
  }

  flagsActivarRele[numeroRele] = true;
  
  if (contarTiempo) {
    limiteEncendidoEtapas[numeroRele] = tiempoLimite;
  } else{
    limiteEncendidoEtapas[numeroRele] = -1;
  }
  
  if (apagadoAutomatico){
    flagsDesactivarRele[numeroRele] = true;
    limiteApagadoEtapas[numeroRele] = -1;
  }
}

void solicitarDesactivarRele(int numeroRele, bool contarTiempo, int64_t tiempoLimite){
  if (numeroRele < 0 || numeroRele >= CANT_ETAPAS || flagsEtapasDesactivadas[numeroRele] == true){
    return;
  }
  flagsDesactivarRele[numeroRele] = true;

  // Igualo los timepos para asegurarme que se apague en la proxima actualizacion
  if (limiteEncendidoEtapas[numeroRele] != 0){
    limiteEncendidoEtapas[numeroRele] = tiempoEncendidoEtapas[numeroRele];
  }

  if (contarTiempo) {
    limiteApagadoEtapas[numeroRele] = tiempoLimite;
  } else {
    limiteApagadoEtapas[numeroRele] = -1;
  }

}

void desactivarEtapa(int numeroRele, bool contarTiempo, int64_t tiempoLimite){
  solicitarDesactivarRele(numeroRele, contarTiempo, tiempoLimite);
  flagsEtapasDesactivadas[numeroRele] = true;
}

void forzarFalla(){
  pinMode(pinesReles[CANT_ETAPAS-1], OUTPUT);
  digitalWrite(pinesReles[CANT_ETAPAS-1], LOW);
  estadoEtapas[CANT_ETAPAS-1] = true;
}

void cortarFalla(){
  pinMode(pinesReles[CANT_ETAPAS-1], INPUT);
	estadoEtapas[CANT_ETAPAS-1] = false;
}

const bool* obtenerEstadoEtapas(){
	return estadoEtapas;
}

const uint64_t* obtenerTiempoEncendidoEtapas(){
  return tiempoEncendidoEtapas;
}

const uint64_t* obtenerTiempoApagadoEtapas(){
  return tiempoApagadoEtapas;
}

// =====[Implementacion de funciones privadas]=======