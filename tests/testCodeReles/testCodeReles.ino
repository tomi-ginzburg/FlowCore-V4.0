#include "../../lib/Unity/unity.h"

//-------------- MODULOS ---------------------
#include "../../modules/contador/contador.h"
#include "../../modules/contador/contador.cpp"

#include "../../modules/reles/reles.h"
#include "../../modules/reles/reles.cpp"
// //--------------------------------------------

// //-------------- TESTS ---------------------
#include "../testReles.h"
//--------------------------------------------


void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void setup(){
    Serial.begin(921600);

    inicializarContador();

    UNITY_BEGIN();
    delay(1000);
    RUN_TEST(test_inicializarReles);
    delay(1000);
    RUN_TEST(test_encendidoSimple);
    delay(1000);
    RUN_TEST(test_encendidoConTiempoLimite);
    delay(1000);
    RUN_TEST(test_encendidoConApagadoAutomatico);
    delay(1000);
    UNITY_END();
}

void loop(){}