#ifndef BUZZER_H_
#define BUZZER_H_


// =====[Declaracion de funciones publicas]==========

/*
 * pre: -
 * post: configura el pin con PWM, e inicia el estado en apagado
 */
void inicializarBuzzer();

/*
 * pre: -
 * post: 
 */
void actualizarBuzzer();

/*
 * pre: -
 * post: setea la flag de activarAlarma
 */
void solicitarActivarAlarma();

/*
 * pre: -
 * post: setea la flag de desactivarAlarma
 */
void solicitarDesactivarAlarma();

/*
 * pre: -
 * post: setea la flag de activarBocina
 */
void solicitarActivarBocina();

#endif


