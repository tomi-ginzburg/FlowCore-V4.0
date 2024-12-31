#include "sensores.h"

#include <Arduino.h>
#include <SPI.h>

#define UNITARIO (0)
#define CONJUNTO (1)
#define TESTEO (UNITARIO) // Uso buses SPI separados para sensores y display

#if TESTEO == (CONJUNTO)
    #include "..\..\Integracion\integrationCode\display.h"
#endif

// =====[Declaracion de defines privados]============

#define SINCRONICO (0)
#define ASINCRONICO (1)
#define TEMPORIZACION (ASINCRONICO) // ACA ELIJO CON CUAL COMPILAR

// Pines usados para la comunicacion

#define SENSORES_MISO   39 //12 
#define SENSORES_MOSI   40 //13
#define SENSORES_SCLK   41 //14

#define SENSORES_CS     42   // Chip Select SPI
#define SENSORES_DRDY   2   // ~ DRDY
#define SENSORES_I_SW   1   // IN multiplexor

#define RESOLUCION      16

#define CANT_MUESTRAS   6
#define CANT_MEDICIONES_ERRADAS 5

#define RREF_RTD        470
#define RREF_4_20       100
#define RTD0            100

#define MAX_PRESION_PSI 200
#define MIN_PRESION_PSI 0
#define MAX_CURRENT     20
#define MIN_CURRENT     4

// Comandos y registros del ADS1148

#define RESET       0x06
#define WRITE_REG   0x40    // Write to register rrrr
#define READ_REG    0x20    // Read de register rrrr
#define RDATA       0x12    // Read data once
#define RDATAC      0x14    // Read data on continuous mode
#define SDATAC      0x16    // Stop read data on continuous mode
#define SELF_CAL    0x62    // Self offset calibration

#define ONE_BYTE    0x00    // bytes a transimitir
#define TWO_BYTE    0x01    

#define REG_MUX0    0x00    // Multiplexer Control Register 0
#define MUX_SP_AIN0 0x00    // bits 5:3 de MUX0, 000: AIN0
#define MUX_SP_AIN1 0x08    // bits 5:3 de MUX0, 001: AIN1
#define MUX_SP_AIN2 0x10    // bits 5:3 de MUX0, 010: AIN2
#define MUX_SP_AIN3 0x28    // bits 5:3 de MUX0, 011: AIN3
#define MUX_SP_AIN4 0x20    // bits 5:3 de MUX0, 100: AIN4
#define MUX_SP_AIN5 0x28    // bits 5:3 de MUX0, 101: AIN5
#define MUX_SP_AIN6 0x30    // bits 5:3 de MUX0, 110: AIN6
#define MUX_SP_AIN7 0x38    // bits 5:3 de MUX0, 111: AIN7
#define MUX_SN_AIN0 0x00    // bits 2:0 de MUX0, 000: AIN0
#define MUX_SN_AIN1 0x01    // bits 2:0 de MUX0, 001: AIN1
#define MUX_SN_AIN2 0x02    // bits 2:0 de MUX0, 010: AIN2
#define MUX_SN_AIN3 0x03    // bits 2:0 de MUX0, 011: AIN3
#define MUX_SN_AIN4 0x04    // bits 2:0 de MUX0, 100: AIN4
#define MUX_SN_AIN5 0x05    // bits 2:0 de MUX0, 101: AIN5
#define MUX_SN_AIN6 0x06    // bits 2:0 de MUX0, 110: AIN6
#define MUX_SN_AIN7 0x07    // bits 2:0 de MUX0, 111: AIN7

#define REG_MUX1    0x02    // Multiplexer Control Register 1
#define VREFCON_ON  0x20    // bits 6:5 de MUX1, 01: Internal reference is always on
#define REFSELT_E0  0x00    // bits 4:3 de MUX1, 00: REFP0 y REFN0 selected
#define REFSELT_INT 0x10    // bits 4:3 de MUX1, 10: Internal reference selected

#define REG_SYS0    0x03    // System Control Register 0
#define PGA_GAIN_1  0x00    // bits 6:4 de SYS0, 000: PGA = 1 (default) 
#define PGA_GAIN_4  0x20    // bits 6:4 de SYS0, 010: PGA = 4 
#define DR_20_SPS   0x02    // bits 3:0 de SYS0, 0010: DR = 20SPS  

#define REG_IDAC0   0x0A    // IDAC Control Register 0
#define IMAG_1_mA   0x06    // bits 2:0 de IDAC0, 110: 1000uA

#define REG_IDAC1   0x0B    // IDAC Control Register 1
#define I1DIR_AIN0  0x00    // bits 7:4 de IDAC1, 0000: AIN0
#define I1DIR_AIN6  0x60    // bits 7:4 de IDAC1, 0110: AIN6
#define I2DIR_AIN1  0x01    // bits 3:0 de IDAC1, 0001: AIN1
#define I2DIR_AIN7  0x07    // bits 3:0 de IDAC1, 0111: AIN7


// =====[Declaracion de tipos de datos privados]=====

typedef enum{
    SENSOR_1,   // PT100_1
    SENSOR_2,   // PT100_2
    SENSOR_3    // LOOP 4-20
} estadoLecturaSensores_t;

typedef enum{
    RTD,
    LOOP_4_20
} tecnologiaSensor_t;

float valorSensores[CANT_SENSORES];
float mediciones[CANT_SENSORES][CANT_MUESTRAS];
estadoLecturaSensores_t estadoLecturaSensores;
tecnologiaSensor_t tecnologiaSensor[CANT_SENSORES] = {RTD, RTD, LOOP_4_20};

int contadorCiclos = 0;

#if TESTEO == (CONJUNTO)
    SPIClass& spiSensores = SPI;
#endif
#if TESTEO == (UNITARIO)
    SPIClass spiSensores(FSPI);
#endif


// =====[Declaracion de funciones privadas]==========

void cambiarConfiguracionesADC();
void actualizarEstadoSensores();
float conversionResistencia(float valorADC, tecnologiaSensor_t tecnologiaSensor);
float conversionTemperatura(float valorResistencia, tecnologiaSensor_t tecnologiaSensor);
float conversionCorriente(float valorADC);
float conversionPresion(float valorCorriente);

// =====[Implementacion de funciones publicas]=======

void inicializarSensores(){
    
    #if TESTEO == (CONJUNTO)
        spiSensores = obtenerInstanciaSPI();
    #endif
    #if TESTEO == (UNITARIO)
        spiSensores.begin(SENSORES_SCLK, SENSORES_MISO, SENSORES_MOSI, -1);
    #endif

    pinMode(SENSORES_DRDY, INPUT);
    pinMode(SENSORES_CS, OUTPUT);
    pinMode(SENSORES_I_SW, OUTPUT);

    digitalWrite(SENSORES_I_SW, LOW);
    digitalWrite(SENSORES_CS, HIGH);

    
    /* CONFIGURACIONES INICIALES QUE LUEGO NO MODIFICO
    *  - Configuro el valor de las IDACs en 1mA
    *  - Configuro la IDAC1 en AIN0
    *  - Configuro la IDAC2 en AIN1
    *  - Autocalibracion del offset
    */

    spiSensores.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
    digitalWrite(SENSORES_CS, LOW);

    delayMicroseconds(1);
    spiSensores.transfer(WRITE_REG | REG_IDAC0);
    delayMicroseconds(1);
    spiSensores.transfer(ONE_BYTE);
    delayMicroseconds(1);
    spiSensores.transfer(IMAG_1_mA);
    
    delayMicroseconds(1);
    spiSensores.transfer(WRITE_REG | REG_IDAC1);
    delayMicroseconds(1);
    spiSensores.transfer(ONE_BYTE);
    delayMicroseconds(1);
    spiSensores.transfer(I1DIR_AIN6 | I2DIR_AIN7);

    delayMicroseconds(1);
    spiSensores.transfer(SELF_CAL);
    delay(1000); // tiempo de espera hasta que finalize la auto calibracion del offset

    digitalWrite(SENSORES_CS, HIGH);
    spiSensores.endTransaction();

    // Inicio las lectruas en 0
    for (int i=0;i<CANT_SENSORES;i++){
        valorSensores[i] = 0;
    }

    // Inicio con el sensor 1
    estadoLecturaSensores = SENSOR_1;

    cambiarConfiguracionesADC();
}

void actualizarSensores(){
    static int msb, lsb, valorADC;
    static float nuevaLectura;
    static int muestrasPromediadas = 0;
    
    // LECTURA DE DATOS
    spiSensores.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
    digitalWrite(SENSORES_CS, LOW);
    
    delayMicroseconds(1);
    msb = spiSensores.transfer(0xff);
    delayMicroseconds(1);
    lsb = spiSensores.transfer(0xff);

    digitalWrite(SENSORES_CS, HIGH);
    spiSensores.endTransaction();
    
    valorADC = (msb << 8) | lsb;

    if (contadorCiclos == 0){
        muestrasPromediadas = 0;
    } 
 
    // INTERPRETACION Y GUARDADO DE DATOS
    static int contadorMedicionesErradas[CANT_SENSORES] = {0, 0, 0};

    // Si tengo un error de medicion, no lo mando
    // Cuento a tener varios errores antes de mandar una medicion de error
    // Mientras tanto mantengo el valor anterior
    if (valorADC > 32750 || valorADC < 50){
        contadorMedicionesErradas[estadoLecturaSensores]++;
        if (contadorMedicionesErradas[estadoLecturaSensores]++ > CANT_MEDICIONES_ERRADAS){
            valorSensores[estadoLecturaSensores] = VALOR_ERROR_SENSOR;
        }
    } 
    
    else {
        contadorMedicionesErradas[estadoLecturaSensores] = 0;
        static float aux = 0;

        switch(estadoLecturaSensores){
            case SENSOR_1:
                nuevaLectura = conversionTemperatura(conversionResistencia(valorADC, tecnologiaSensor[PT100_1]), tecnologiaSensor[PT100_1]);

                // MEDICIONES ASINCRONICAS
                #if (TEMPORIZACION == ASINCRONICO)
                    mediciones[PT100_1][muestrasPromediadas] = nuevaLectura;
                    if (muestrasPromediadas == CICLOS_PT100_1 - 1){
                        aux = 0;
                        for (int i=0; i < CICLOS_PT100_1; i++){
                            aux += mediciones[PT100_1][i];
                        }
                        valorSensores[PT100_1] = aux / CICLOS_PT100_1;
                    }
                #endif
                // MEDICIONES SINCRONICAS
                #if (TEMPORIZACION == SINCRONICO)
                    valorSensores[PT100_1] = (valorSensores[PT100_1]*(muestrasPromediadas) + nuevaLectura) / (muestrasPromediadas+1);
                #endif

                break;
            case SENSOR_2:
                nuevaLectura = conversionTemperatura(conversionResistencia(valorADC, tecnologiaSensor[PT100_2]), tecnologiaSensor[PT100_2]);
                
                // MEDICIONES ASINCRONICAS
                #if (TEMPORIZACION == ASINCRONICO)
                    mediciones[PT100_2][muestrasPromediadas] = nuevaLectura;
                    if (muestrasPromediadas == CICLOS_PT100_2 - 1){
                        aux = 0;
                        for (int i=0; i < CICLOS_PT100_2; i++){
                            aux += mediciones[PT100_2][i];
                        }
                        
                        valorSensores[PT100_2] = aux / CICLOS_PT100_2;
                    }
                #endif
                // MEDICIONES SINCRONICAS
                #if (TEMPORIZACION == SINCRONICO)
                    valorSensores[PT100_2] = (valorSensores[PT100_2]*(muestrasPromediadas) + nuevaLectura) / (muestrasPromediadas+1);
                #endif

                break;

            case SENSOR_3:
                nuevaLectura = conversionPresion(conversionCorriente(valorADC));

                // MEDICIONES ASINCRONICAS
                #if (TEMPORIZACION == ASINCRONICO)
                    mediciones[LOOP_CORRIENTE][muestrasPromediadas] = nuevaLectura;
                    if (muestrasPromediadas == CICLOS_LOOP_I - 1){
                        aux = 0;
                        for (int i=0; i < CICLOS_LOOP_I; i++){
                            aux += mediciones[LOOP_CORRIENTE][i];
                        }
                        valorSensores[LOOP_CORRIENTE] = aux / (CICLOS_LOOP_I);
                    }
                #endif
                // MEDICIONES SINCRONICAS
                #if (TEMPORIZACION == SINCRONICO)
                    valorSensores[LOOP_CORRIENTE] = (valorSensores[LOOP_CORRIENTE]*(muestrasPromediadas) + nuevaLectura) / (muestrasPromediadas+1);
                #endif
                
                break;
            
        }
    }
    
    
    
    muestrasPromediadas++;      
    contadorCiclos++; 

    actualizarEstadoSensores();
}

#ifdef TESTING
float* obtenerValorSensores(){
#else 
const float* obtenerValorSensores(){
#endif
    return valorSensores;
}

// =====[Implementacion de funciones privadas]=======

void cambiarConfiguracionesADC(){

    spiSensores.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
    digitalWrite(SENSORES_CS, LOW);
    
    switch(estadoLecturaSensores){
        case SENSOR_1:  

            digitalWrite(SENSORES_I_SW, LOW);

            // PGA = 4
            delayMicroseconds(1);
            spiSensores.transfer(WRITE_REG | REG_SYS0);
            delayMicroseconds(1);
            spiSensores.transfer(ONE_BYTE);
            delayMicroseconds(1);
            spiSensores.transfer(DR_20_SPS | PGA_GAIN_4);

            // REFERENCIA EXTERNA
            delayMicroseconds(1);
            spiSensores.transfer(WRITE_REG | REG_MUX1);
            delayMicroseconds(1);
            spiSensores.transfer(ONE_BYTE);
            delayMicroseconds(1);
            spiSensores.transfer(VREFCON_ON | REFSELT_E0);

            // AIN0 Y AIN1 
            delayMicroseconds(1);
            spiSensores.transfer(WRITE_REG | REG_MUX0);
            delayMicroseconds(1);
            spiSensores.transfer(ONE_BYTE);
            delayMicroseconds(1);
            spiSensores.transfer(MUX_SP_AIN0 | MUX_SN_AIN1);

            break;

        case SENSOR_2:  

            digitalWrite(SENSORES_I_SW, HIGH);

            // AINP4 Y AIN5 
            delayMicroseconds(1);
            spiSensores.transfer(WRITE_REG | REG_MUX0);
            delayMicroseconds(1);
            spiSensores.transfer(ONE_BYTE);
            delayMicroseconds(1);
            spiSensores.transfer(MUX_SP_AIN4 | MUX_SN_AIN5);

            break;

        case SENSOR_3: 

            // PGA = 1
            delayMicroseconds(1);
            spiSensores.transfer(WRITE_REG | REG_SYS0);
            delayMicroseconds(1);
            spiSensores.transfer(ONE_BYTE);
            delayMicroseconds(1);
            spiSensores.transfer(DR_20_SPS | PGA_GAIN_1);
            
            // REFERENCIA INTERNA
            delayMicroseconds(1);
            spiSensores.transfer(WRITE_REG | REG_MUX1);
            delayMicroseconds(1);
            spiSensores.transfer(ONE_BYTE);
            delayMicroseconds(1);
            spiSensores.transfer(VREFCON_ON | REFSELT_INT);
            
            // AIN2 Y AIN3
            delayMicroseconds(1);
            spiSensores.transfer(WRITE_REG | REG_MUX0);
            delayMicroseconds(1);
            spiSensores.transfer(ONE_BYTE);
            delayMicroseconds(1);
            spiSensores.transfer(MUX_SP_AIN2 | MUX_SN_AIN3);

            break;
    }

    delayMicroseconds(1);
    digitalWrite(SENSORES_CS, HIGH);
    spiSensores.endTransaction();
}

void actualizarEstadoSensores(){
    switch (estadoLecturaSensores){
        case SENSOR_1:
            if (contadorCiclos == CICLOS_PT100_1){
                estadoLecturaSensores = SENSOR_2;
                contadorCiclos = 0;
                cambiarConfiguracionesADC();
            }
            break;
        case SENSOR_2:
            if (contadorCiclos == CICLOS_PT100_2){
                estadoLecturaSensores = SENSOR_3;
                contadorCiclos = 0;
                cambiarConfiguracionesADC();
            }
            break;
        case SENSOR_3:  
            if (contadorCiclos == CICLOS_LOOP_I){
                estadoLecturaSensores = SENSOR_1;
                contadorCiclos = 0;
                cambiarConfiguracionesADC();
            }
            break;
    }
}

float conversionResistencia(float valorADC, tecnologiaSensor_t tecnologiaSensor){
    static int N = pow(2,RESOLUCION) - 1;
    float resistencia; 

    switch (tecnologiaSensor){
        case RTD: 
            resistencia = (valorADC * RREF_RTD)/ N;
            break;
    }

    return resistencia;
}

float conversionTemperatura(float valorResistencia, tecnologiaSensor_t tecnologiaSensor){
    float ALPHA = 0.00385;
    float temperatura;

    switch (tecnologiaSensor){
        case RTD: 
            temperatura = (valorResistencia - RTD0) / (ALPHA * RTD0);
            break;
    }
    
    return temperatura;
}

float conversionCorriente(float valorADC){
    return (valorADC*2048.0*2)/(pow(2,16)*RREF_4_20);
}

float conversionPresion(float valorCorriente){
    return (valorCorriente - MIN_CURRENT)*(MAX_PRESION_PSI - MIN_PRESION_PSI)/(MAX_CURRENT - MIN_CURRENT);
}