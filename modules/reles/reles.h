#ifndef RELES_H_
#define RELES_H_

#include <Arduino.h>

// =====[Declaracion de defines publicos]============

#define CANT_ETAPAS 7

#define ETAPA_1     0   
#define ETAPA_2     1
#define ETAPA_3     2
#define ETAPA_4     3   // De la 1 a la 4 son las resistencias
#define ETAPA_5     4   // Bomba de calefaccion
#define ETAPA_6     5   // Bomba de agua caliente
#define ETAPA_7     6   // Etapa de emergencia

// =====[Declaracion de tipos de datos publicos]=====

// =====[Declaracion de funciones publicas]==========

/*
 * pre: -
 * post: realiza las configuraciones sobre los pines, con las etapas apagadas,
 *      e inicia los valores de los flags y las variables contadoras de tiempos
 */
void inicializarReles();

/*
 * pre: -
 * post: cambia los estados de las salidas
 */
void actualizarReles();

/*
 * pre: numeroRele >= 0 y numeroRele < CANT_ETAPAS, y el flagEtadaDesactivada en false
 * post: setea el flag de activacion, el valor limite de conteo si contarTiempo es true, y
 *      si el apagadoAutomatico es true, setea el flag de desactivacion 
 */
void solicitarActivarRele(int numeroRele, bool contarTiempo = false, int64_t tiempoLimite = 0, bool apagadoAutomatico = false);

/*
 * pre: numeroRele >= 0 y numeroRele < CANT_ETAPAS, y el flagEtadaDesactivada en false
 * post: setea el flag de desactivacion, e iguala el valor del limite encendido al tiempo de encendido
 *      para asegurarse que se apague la etapa. Si contar tiempo es true, se setea el valor limite de conteo.
 */
void solicitarDesactivarRele(int numeroRele, bool contarTiempo = false, int64_t tiempoLimite = 0);

/*
 * pre: -
 * post: desctiva la etapa, dejando sin efecto la funcion solicitarActivarRele
*/
void desactivarEtapa(int numeroRele, bool contarTiempo = false, int64_t tiempoLimite = 0);

/*
 * pre: -
 * post: Setea el pin correspondiente al último relé como salida,
 *      y lo pone en estado bajo para cerrar el relé y forzar la falla del disyuntor.
 */
void forzarFalla();

/*
 * pre: -
 * post: setea el pin que corresponde al ultimo rele como entrada,
 *       para abrir el cortocircuito a entre fase y tierra
 */
void cortarFalla();

/*
 * pre: -
 * post: devuelve un puntero constante a estadoEtapas
 */
const bool* obtenerEstadoEtapas();

/*
 * pre: -
 * post: devuelve un puntero constante a tiempoEncendidoEtapas
 */
 #ifdef TESTING
uint64_t* obtenerTiempoEncendidoEtapas(); 
#else
const uint64_t* obtenerTiempoEncendidoEtapas();
#endif

/*
 * pre: -
 * post: devuelve un puntero constante a tiempoApagadoEtapas
 */
 #ifdef TESTING
uint64_t* obtenerTiempoApagadoEtapas(); 
#else
const uint64_t* obtenerTiempoApagadoEtapas();
#endif

#endif