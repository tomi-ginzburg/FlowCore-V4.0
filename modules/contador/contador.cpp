#include "contador.h"

#include <unordered_map>
#include <list>
#include <tuple>

// =====[Declaracion de defines privados]============

#define TIEMPO_CONTADOR_MS  1
#define FRECUENCIA          1000000
#define TIMER_CONTADOR      0

// =====[Declaracion de tipos de datos privados]=====

hw_timer_t * timerContador = NULL;
volatile  uint64_t valorContador = 0; // Tiempo de ejecucion del programa
std::list<uint64_t*> listaContadoresDescendentes;
std::list<std::tuple<uint64_t*, uint64_t>> listaContadoresAscendentes;

// =====[Declaracion de funciones privadas]==========

static void IRAM_ATTR handlerTimerContador();

// =====[Implementacion de funciones publicas]=======

void inicializarContador(){
    timerContador = timerBegin(FRECUENCIA);
    timerAttachInterrupt(timerContador, &handlerTimerContador);
    timerAlarm(timerContador, TIEMPO_CONTADOR_MS*1000, true, 0);
    
}

bool loopMs(uint64_t intervalo){
    static std::unordered_map<uint64_t, uint64_t> last_times;  // Diccionario de intervalos y ultimos tiempos
    
    // Si el intervalo no se uso antes, inicializa el último tiempo en 0
    if (last_times.find(intervalo) == last_times.end()) {
        last_times[intervalo] = 0;
    }
    
    if (valorContador - last_times[intervalo] >= intervalo) {
        last_times[intervalo] += intervalo;  // Actualiza para la próxima ejecución
        return true;
    }
    return false;
}

void contarMs(uint64_t* variable, uint64_t intervalo, bool tipoConteo) {
    if (variable == nullptr || intervalo < 0) {
        return;
    }

    if (tipoConteo == HIGH) {
        // Busco el puntero en la lista
        auto it = std::find_if(listaContadoresAscendentes.begin(), listaContadoresAscendentes.end(),
            [variable](const std::tuple<uint64_t*, uint64_t>& elem) { return std::get<0>(elem) == variable; }); 

        // Si ya estaba, actualizo el límite, sino lo agrego
        if (it != listaContadoresAscendentes.end()) {
            std::get<1>(*it) = intervalo;  // Actualizo el límite
            *std::get<0>(*it) = 0;  // Reinicio el contador
        } else {
            *variable = 0;  // Inicializo el contador a 0
            listaContadoresAscendentes.push_back(std::make_tuple(variable, intervalo));
        }
    } else if (tipoConteo == LOW) {
        // En el conteo descendente no tiene sentido tener intervalo de 0
        if (intervalo == 0){
            return;
        }

        // Contador descendente
        auto it = std::find(listaContadoresDescendentes.begin(), listaContadoresDescendentes.end(), variable);

        if (it != listaContadoresDescendentes.end()) {
            *(*it) = intervalo;
        } else {
            *variable = intervalo;
            listaContadoresDescendentes.push_back(variable);
        }
    }
}

void frenarContarMs(uint64_t* variable) {
    if (variable == nullptr) {
        return;
    }

    // Busca en la lista de contadores ascendentes y elimínalo si lo encuentra
    auto itAscendente = std::find_if(listaContadoresAscendentes.begin(), listaContadoresAscendentes.end(),
        [variable](const std::tuple<uint64_t*, uint64_t>& elem) { return std::get<0>(elem) == variable; });
    
    if (itAscendente != listaContadoresAscendentes.end()) {
        listaContadoresAscendentes.erase(itAscendente);
        return;
    }

    // Busca en la lista de contadores descendentes y elimínalo si lo encuentra
    auto itDescendente = std::find(listaContadoresDescendentes.begin(), listaContadoresDescendentes.end(), variable);
    if (itDescendente != listaContadoresDescendentes.end()) {
        listaContadoresDescendentes.erase(itDescendente);
    }
}

// =====[Implementacion de funciones privadas]=======

static void IRAM_ATTR handlerTimerContador(){
    
    valorContador++;

    // Actualiza los contadores ascendentes
    for (auto it = listaContadoresAscendentes.begin(); it != listaContadoresAscendentes.end(); ) {
        auto& [variable, limite] = *it;

        (*variable)++;  // Incrementa el valor del contador

        if (*variable >= limite && limite != 0) {
            it = listaContadoresAscendentes.erase(it);  // Eliminar si alcanza el límite
        } else {
            ++it;
        }
    }

    // Actualiza los contadores descendentes
    for (auto it = listaContadoresDescendentes.begin(); it != listaContadoresDescendentes.end(); it++) {
        if (*(*it) > 0) {
            (*(*it))--;  // Decrementa el valor al que apunta el puntero
        } else {
            it = listaContadoresDescendentes.erase(it);  // Elimina el puntero de la lista si llega a 0
        } 
    }
}