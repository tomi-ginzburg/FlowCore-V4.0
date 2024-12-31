#include "../../lib/Unity/unity.h"

#define TESTING

//-------------- MODULOS ---------------------
#include "../../modules/contador/contador.h"
#include "../../modules/contador/contador.cpp"

#include "../../modules/reles/reles.h"
#include "../../modules/reles/reles.cpp"

#include "../../modules/entradasMecanicas/entradasMecanicas.h"
#include "../../modules/entradasMecanicas/entradasMecanicas.cpp"

#include "../../modules/sensores/sensores.h"
#include "../../modules/sensores/sensores.cpp"

#include "../../modules/buzzer/buzzer.h"
#include "../../modules/buzzer/buzzer.cpp"

#include "../../modules/NVS/NVS.h"
#include "../../modules/NVS/NVS.cpp"

#include "../../modules/controles/controles.h"
#include "../../modules/controles/controles.cpp"

// #include "../../modules/display/display.h"
// #include "../../modules/display/display.cpp"

//--------------------------------------------

void setUp(void) {
    // reiniciarEstadoControles();
}

void tearDown(void) {
    reiniciarEstadoControles();
}

void setup(){
    Serial.begin(921600);

    inicializarNVS();
    inicializarSensores();
    // inicializarDisplay();
    
    inicializarEntradasMecanicas();
    inicializarReles();

    // inicializarBuzzer();
    inicializarControles();

    inicializarContador();    

    UNITY_BEGIN();
    delay(1000);
    reiniciarEstadoControles();
    test_verificarFallaTermostatoSeguridad();
    test_verificarFallaTemperaturaAlta();
    test_verificarFallaTemperaturaBaja();
    test_verificarFallaApagadoBombas();
    test_verificarFallaSensorCaldera();
    test_verificarFallaSensorACS();
    test_verificarFallaEncendidoBombaCalefaccion();
    test_verificarFallaEncendidoBombaACS();
    test_verificarFallaPresionAlta();
    test_verificarFallaPresionBaja();
    UNITY_END();
}

void loop(){}