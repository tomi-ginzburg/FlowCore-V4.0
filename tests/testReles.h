#ifndef TEST_RELES
#define TEST_RELES

#include "../lib/Unity/unity.h"
#include "../modules/reles/reles.h"

// Testeo si todas las etapas estan apagadas
void test_etapasApagadas(){
    for (int i = 0; i < CANT_ETAPAS - 1; i++) {
        TEST_ASSERT_EQUAL(HIGH, digitalRead(pinesReles[i]));
    }
}

/*
 * Testeo la correcta inicializacion de reles
 * - Salidas apagadas
 * - Estado de salidas apagado (false)
 * - Flags en false
*/ 
void test_inicializarReles(){
    
    inicializarReles();

    for (int i = 0; i < CANT_ETAPAS - 1; i++) {
        TEST_ASSERT_EQUAL(HIGH, digitalRead(pinesReles[i]));
        TEST_ASSERT_EQUAL(false, flagsActivarRele[i]);
        TEST_ASSERT_EQUAL(false, flagsDesactivarRele[i]);
        TEST_ASSERT_EQUAL(false, flagsEtapasDesactivadas[i]);
        TEST_ASSERT_EQUAL(false, estadoEtapas[i]);
    }
}

void test_encendidoSimple(){
    
    // Solicito encenderla
    solicitarActivarRele(ETAPA_2);
    actualizarReles();
    // Chequeo que se haya encendido
    TEST_ASSERT_EQUAL(LOW, digitalRead(pinesReles[ETAPA_2]));

    // Solicito apagarla
    solicitarDesactivarRele(ETAPA_2);
    actualizarReles();
    // Chequeo que se haya apagado
    TEST_ASSERT_EQUAL(HIGH, digitalRead(pinesReles[ETAPA_2]));
}

void test_encendidoConTiempoLimite(){
    
    int tiempoLimite = 10;
    int tiempoInicio = millis();
    solicitarActivarRele(ETAPA_2, true, tiempoLimite);

    while (millis()- tiempoInicio < tiempoLimite*2){
        actualizarReles();
    }

    // VEO QUE SE HAYA PRENDIDO; Y QUE NO CONTO MAS QUE EL TIEMPO LIMITE
    TEST_ASSERT_EQUAL(LOW, digitalRead(pinesReles[ETAPA_2]));
    TEST_ASSERT_EQUAL(tiempoLimite, tiempoEncendidoEtapas[ETAPA_2]);

}

void test_encendidoConApagadoAutomatico(){
    
    int tiempoLimite = 10;
    int tiempoInicio = millis();
    solicitarActivarRele(ETAPA_2, true, tiempoLimite, true);

    while (millis()- tiempoInicio < tiempoLimite*0.9){
        actualizarReles();
    }

    // VEO QUE ESTE PRENDIDA
    TEST_ASSERT_EQUAL(LOW, digitalRead(pinesReles[ETAPA_2]));

    while (millis()- tiempoInicio < tiempoLimite*2){
        actualizarReles();
    }

    // VEO QUE SE HAYA APAGADO
    TEST_ASSERT_EQUAL(HIGH, digitalRead(pinesReles[ETAPA_2]));

}


#endif