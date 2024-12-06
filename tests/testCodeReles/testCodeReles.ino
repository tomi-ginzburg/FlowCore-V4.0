#include "../../lib/Unity/unity.h"

//-------------- MODULOS ---------------------
#include "../../modules/contador/contador.h"
#include "../../modules/contador/contador.cpp"

#include "../../modules/reles/reles.h"
#include "../../modules/reles/reles.cpp"
//--------------------------------------------

//-------------- TESTS ---------------------
#include "../testReles.h"
//--------------------------------------------


void setup(){
    Serial.begin(115200);
    inicializarContador();

    UNITY_BEGIN();
    RUN_TEST(test_inicializarReles);
    RUN_TEST(test_encendidoSimple);
    RUN_TEST(test_encendidoConTiempoLimite);
    RUN_TEST(test_encendidoConApagadoAutomatico);
    UNITY_END();
}

void loop(){}