#include "display.h"

#include "..\reles\reles.h"
#include "..\sensores\sensores.h"
#include "..\NVS\NVS.h"
#include "..\controles\controles.h"

#include <lvgl.h>
#include <TFT_eSPI.h>
// #include "..\ui\ui.h"
// // #include "..\ui\ui.c"

#include <ui.h>

// =====[Declaracion de defines privados]============

// =====[Declaracion de tipos de datos privados]=====

static const uint16_t screenWidth  = 480;
static const uint16_t screenHeight = 320;

enum { SCREENBUFFER_SIZE_PIXELS = screenWidth * screenHeight / 10 };
static lv_color_t buf [SCREENBUFFER_SIZE_PIXELS];

TFT_eSPI tft = TFT_eSPI( screenWidth, screenHeight ); 
// TFT_eSprite sprite = TFT_eSprite(&tft);

// const int *estadoEntradasUI;
const bool *etapasUI;
const estadoControl_t *estadoControlUI;
const causaAlarma_t *causaAlarmaUI;
const float *valorSensoresUI;
const Configs *configuracionesUI;
const int32_t *alarmasUI;

SPIClass& spix = SPI;

// =====[Declaracion de funciones privadas]==========

void my_disp_flush (lv_display_t *disp, const lv_area_t *area, uint8_t *pixelmap);
void my_touchpad_read (lv_indev_t * indev_driver, lv_indev_data_t * data);
void touch_calibrate();
static uint32_t my_tick_get_cb ();

// Funciones para la inicializacion de los widgets
void inicializarValores();
void inicializarValoresPantallaPrincipal();
void actualizarValoresPantallaApagado();
void inicializarValoresConfiguracionesUsuario();
void inicializarValoresConfiguracionesAvanzadas();
void inicializarValoresAlarmas();
void inicializarValoresDiagnostico();

// Funciones para la actualizacion de los widgets de cada pantalla
void actualizarValores();
void actualizarValoresPantallaPrincipal();
void actualizarValoresConfiguracionesUsuario();
void actualizarValoresConfiguracionesAvanzadas();
void actualizarValoresPantallaAlarmas();
void actualizarValoresPantallaAlertas();

const char* obtenerLabelAlarma(causaAlarma_t causa);
const char* obtenerLabelAclaracionAlarma(causaAlarma_t causa);

// =====[Implementacion de funciones publicas]=======

void inicializarDisplay(){
    
    // Obtengo los punteros a variables de otros modulos
    // estadoEntradasUI = obtenerEstadoEntradasMecanicas();
    etapasUI = obtenerEstadoEtapas();
    estadoControlUI = obtenerEstadoControl();
    causaAlarmaUI = obtenerCausaAlarma();
    valorSensoresUI = obtenerValorSensores();
    configuracionesUI = obtenerConfiguracionesNVS();
    alarmasUI = obtenerAlarmasNVS();

    lv_init();

    tft.begin();
    // spix = tft.getSPIinstance();
    tft.setRotation(3);

    // Si nunca se calibro, se realiza la calibracion, sino se cargan los valores en memoria
    if (configuracionesUI->calibracion == false){
      touch_calibrate();
    } else {
      uint16_t data[5];
      memcpy(data, configuracionesUI->calData, sizeof(data));
      tft.setTouch(data);
    }

    static lv_disp_t* disp;
    disp = lv_display_create( screenWidth, screenHeight );
    lv_display_set_buffers( disp, buf, NULL, SCREENBUFFER_SIZE_PIXELS * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL );
    lv_display_set_flush_cb( disp, my_disp_flush );

    static lv_indev_t* indev;
    indev = lv_indev_create();
    lv_indev_set_type( indev, LV_INDEV_TYPE_POINTER );
    lv_indev_set_read_cb( indev, my_touchpad_read );

    lv_tick_set_cb( my_tick_get_cb );

    ui_init();
    inicializarValores();
    lv_timer_handler();
}

void actualizarDisplay(int tiempoRefresco_ms){
  static int contador = 1;
  
  if (contador < 100/tiempoRefresco_ms){
    contador++;
  } else {
    actualizarValores();
    contador = 1;
  }
  lv_timer_handler_run_in_period(tiempoRefresco_ms);
}

SPIClass& obtenerInstanciaSPI(){
  return spix;
}

// =====[Implementacion de funciones privadas]=======

// FUCNIONES PARA TFT_ESPI

void my_disp_flush (lv_display_t *disp, const lv_area_t *area, uint8_t *pixelmap){
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    if (LV_COLOR_16_SWAP) {
        size_t len = lv_area_get_size( area );
        lv_draw_sw_rgb565_swap( pixelmap, len );
    }
    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( (uint16_t*) pixelmap, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp );
}

void my_touchpad_read (lv_indev_t * indev_driver, lv_indev_data_t * data){
    uint16_t touchX = 0, touchY = 0;

    bool touched = tft.getTouch( &touchX, &touchY, 20);

    if (!touched)
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;

    }
}

void touch_calibrate(){

  uint16_t datosCalibracion[5];
  bool calibracionOk;

  // Calibrate
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 140);
  // tft.setTextFont(1);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println("Tocar las esquinas segun las flechas");

  tft.calibrateTouch(datosCalibracion, TFT_MAGENTA, TFT_BLACK, 15);
  guardarConfigsNVS(CAL_DATA, datosCalibracion, sizeof(datosCalibracion));
  calibracionOk = true;
  guardarConfigsNVS(CALIBRACION, &calibracionOk, sizeof(calibracionOk));
}

static uint32_t my_tick_get_cb () {
  return millis(); 
}

// INICIALIZACION DE VALORES DE WIDGETS

void inicializarValores(){

  inicializarValoresPantallaPrincipal();
  inicializarValoresConfiguracionesUsuario();
  inicializarValoresConfiguracionesAvanzadas();
  inicializarValoresAlarmas();
  inicializarValoresDiagnostico();
  
}

void inicializarValoresPantallaPrincipal(){
  lv_obj_add_flag(ui_ButtonAlarma, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_ButtonAlarma2, LV_OBJ_FLAG_HIDDEN);
}

void inicializarValoresConfiguracionesUsuario(){
  if (configuracionesUI->calefaccionOn){
    lv_obj_add_state(ui_ButtonCalefaccion, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(ui_ButtonCalefaccion, LV_STATE_CHECKED);
  }
}

void inicializarValoresConfiguracionesAvanzadas(){

}

void inicializarValoresAlarmas(){
  if (alarmasUI[0] == 0){ 
    lv_obj_add_flag(ui_PanelAlarmas, LV_OBJ_FLAG_HIDDEN);
  }
    
}

void inicializarValoresDiagnostico(){
  if (configuracionesUI->diagnosticoOn){
    lv_obj_add_state(ui_CheckboxDiagnostico, LV_STATE_CHECKED);
		lv_obj_clear_flag(ui_LabelDiagnosticoPrincipal, LV_OBJ_FLAG_HIDDEN);
    
		lv_obj_add_flag(ui_BotonEtapa1, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(ui_BotonEtapa2, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(ui_BotonEtapa3, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(ui_BotonEtapa4, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(ui_BotonEtapa5, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(ui_BotonEtapa6, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui_ButtonPurga, LV_OBJ_FLAG_CLICKABLE);
  } else {
    lv_obj_add_flag(ui_LabelDiagnosticoPrincipal, LV_OBJ_FLAG_HIDDEN);
  }
}

// ACTUALIZACION DE WIDGETS DE CADA PANTALLA

void actualizarValores(){
  lv_obj_t * pantalla_activa = lv_scr_act();

    static causaAlarma_t alarmaAnterior = NO_FALLA;

    // SI EL CONTROL DETECTA UNA ALARMA, CAMBIO LA PANTALLA
    if ( *causaAlarmaUI != alarmaAnterior ){

      alarmaAnterior = *causaAlarmaUI;
      lv_label_set_text(ui_LabelError, obtenerLabelAlarma(*causaAlarmaUI));
      lv_label_set_text(ui_LabelAclaracion, obtenerLabelAclaracionAlarma(*causaAlarmaUI));
      if (*causaAlarmaUI != NO_FALLA){
        // Si es por el apagado de las bombas doy la opcion de reset
        if (*causaAlarmaUI == BOMBAS_OFF || *causaAlarmaUI == BOMBAS_ON){
              lv_label_set_text(ui_Label12, "Reset");
        }
        _ui_screen_change(&ui_Alertas, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_Alertas_screen_init);
      }
    }

    // VEO EN QUE PANTALLA ESTOY Y LA ACTUALIZO

    if (pantalla_activa == ui_Principal){
        actualizarValoresPantallaPrincipal();
    }
    else if (pantalla_activa == ui_Apagado){
      actualizarValoresPantallaApagado();
    }
    else if (pantalla_activa == ui_ConfiguracionesUsuario){
      actualizarValoresConfiguracionesUsuario();
    }
    else if (pantalla_activa == ui_ConfiguracionesAvanzadas){
      actualizarValoresConfiguracionesAvanzadas();
    }
    else if (pantalla_activa == ui_Alertas){
      actualizarValoresPantallaAlertas();
      if (*causaAlarmaUI == NO_FALLA){
        if (configuracionesUI->encendidoOn){
          lv_disp_load_scr(ui_Principal);
        } else {
          lv_disp_load_scr(ui_Apagado);
        }
      }
    }  
    else if (pantalla_activa == ui_Alarmas){
      actualizarValoresPantallaAlarmas();
    }  
   else if (pantalla_activa == ui_Inicio){
      if (configuracionesUI->encendidoOn){
        lv_disp_load_scr(ui_Principal);
      } else {
        lv_disp_load_scr(ui_Apagado);
      }
    }  


}

void actualizarValoresPantallaPrincipal(){
  char buffer[10];
  bool checked;

  // VALOR DE TEMPERATURA
  if (*causaAlarmaUI != FALLA_SENSOR_1 && *estadoControlUI != ENCENDIDO_ACS && *estadoControlUI != APAGADO && valorSensoresUI[PT100_1] != VALOR_ERROR_SENSOR){
    snprintf(buffer, sizeof(buffer), "%.1f °C", valorSensoresUI[PT100_1]);
    lv_label_set_text(ui_LabelTemperatura, buffer);
    lv_arc_set_value(ui_ArcTemperatura, valorSensoresUI[PT100_1]);
  } else if (*estadoControlUI == ENCENDIDO_ACS){
    snprintf(buffer, sizeof(buffer), "%.1f °C", valorSensoresUI[PT100_2]);
    lv_label_set_text(ui_LabelTemperatura, buffer);
    lv_arc_set_value(ui_ArcTemperatura, valorSensoresUI[PT100_2]);
  } else {
    snprintf(buffer, sizeof(buffer), "--- °C");
    lv_label_set_text(ui_LabelTemperatura, buffer);
    lv_arc_set_value(ui_ArcTemperatura, 0);
  }

  // VALOR DE PRESION
  if (*estadoControlUI != APAGADO){
    snprintf(buffer, sizeof(buffer), "%.1f psi", valorSensoresUI[LOOP_CORRIENTE]);
    lv_label_set_text(ui_LabelPresion, buffer);
  } else {
    snprintf(buffer, sizeof(buffer), "--- psi", valorSensoresUI[LOOP_CORRIENTE]);
    lv_label_set_text(ui_LabelPresion, buffer);
  }

  // INDICADORES CALEFACCION Y ACS
  if (etapasUI[ETAPA_5] == HIGH && lv_image_get_src(ui_ImageCal) != &ui_img_icono_calefaccion_encendida_png){
    lv_image_set_src(ui_ImageCal, &ui_img_icono_calefaccion_encendida_png);
  }

  else if (etapasUI[ETAPA_5] == LOW && lv_image_get_src(ui_ImageCal) != &ui_img_icono_calefaccion_apagada_png){
    lv_image_set_src(ui_ImageCal, &ui_img_icono_calefaccion_apagada_png);
  }

  if (etapasUI[ETAPA_6] == HIGH && lv_image_get_src(ui_ImageAcs) != &ui_img_icono_acs_encendida_png){
    lv_image_set_src(ui_ImageAcs, &ui_img_icono_acs_encendida_png);
  }

  else if (etapasUI[ETAPA_6] == LOW && lv_image_get_src(ui_ImageAcs) != &ui_img_icono_acs_apagada_png){
    lv_image_set_src(ui_ImageAcs, &ui_img_icono_acs_apagada_png);
  }
  // BOTON PARA PANTALLA DE ALARMA
  if (*causaAlarmaUI != NO_FALLA && lv_obj_has_flag(ui_ButtonAlarma, LV_OBJ_FLAG_HIDDEN)){
    lv_obj_clear_flag(ui_ButtonAlarma, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ui_LabelError, obtenerLabelAlarma(*causaAlarmaUI));
    lv_label_set_text(ui_LabelAclaracion, obtenerLabelAclaracionAlarma(*causaAlarmaUI));
  }

  if (*causaAlarmaUI == NO_FALLA && !lv_obj_has_flag(ui_ButtonAlarma, LV_OBJ_FLAG_HIDDEN)){
    lv_obj_add_flag(ui_ButtonAlarma, LV_OBJ_FLAG_HIDDEN);
  }

  if (configuracionesUI->diagnosticoOn == false && !lv_obj_has_flag(ui_LabelDiagnosticoPrincipal, LV_OBJ_FLAG_HIDDEN)){
    lv_obj_add_flag(ui_LabelDiagnosticoPrincipal, LV_OBJ_FLAG_HIDDEN);
  }

  // BOTON PARA HISTORIAL DE ALARMAS
  if (alarmasUI[0] != 0  && lv_obj_has_flag(ui_PanelAlarmas, LV_OBJ_FLAG_HIDDEN)){
    lv_obj_clear_flag(ui_PanelAlarmas, LV_OBJ_FLAG_HIDDEN);
  } else if (alarmasUI[0] == 0  && !lv_obj_has_flag(ui_PanelAlarmas, LV_OBJ_FLAG_HIDDEN)){
    lv_obj_add_flag(ui_PanelAlarmas, LV_OBJ_FLAG_HIDDEN);
  }

}

void actualizarValoresPantallaApagado(){
  
  // BOTON PARA PANTALLA DE ALARMA
  if (*causaAlarmaUI != NO_FALLA && lv_obj_has_flag(ui_ButtonAlarma2, LV_OBJ_FLAG_HIDDEN)){
    lv_obj_clear_flag(ui_ButtonAlarma2, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ui_LabelError, obtenerLabelAlarma(*causaAlarmaUI));
    lv_label_set_text(ui_LabelAclaracion, obtenerLabelAclaracionAlarma(*causaAlarmaUI));
  }

  if (*causaAlarmaUI == NO_FALLA && !lv_obj_has_flag(ui_ButtonAlarma2, LV_OBJ_FLAG_HIDDEN)){
    lv_obj_add_flag(ui_ButtonAlarma2, LV_OBJ_FLAG_HIDDEN);
  }
}

void actualizarValoresConfiguracionesUsuario(){
  // CALEFACCION ON/OFF
  // Activo o desactivo el switch segun si la calefaccion esta encendida o no
  if (lv_obj_has_flag(ui_ButtonCalefaccion, LV_OBJ_FLAG_CLICKABLE) && (*estadoControlUI == APAGADO || *estadoControlUI == ALARMA)){
    lv_obj_remove_flag(ui_ButtonCalefaccion, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_state(ui_ButtonCalefaccion, LV_STATE_CHECKED);
  }
  else if (!lv_obj_has_flag(ui_ButtonCalefaccion, LV_OBJ_FLAG_CLICKABLE) && *estadoControlUI != APAGADO && *estadoControlUI != ALARMA){
    if (*causaAlarmaUI != BOMBA_CALEFACCION_ON && *causaAlarmaUI != FALLA_SENSOR_1){
      lv_obj_add_flag(ui_ButtonCalefaccion, LV_OBJ_FLAG_CLICKABLE);
    }
    
    if (configuracionesUI->calefaccionOn){
      lv_obj_add_state(ui_ButtonCalefaccion, LV_STATE_CHECKED);
    }
  }

  // Si el switch esta en ON pero la calefaccion estaba apagada, la cambio
  if (lv_obj_has_state(ui_ButtonCalefaccion, LV_STATE_CHECKED) && configuracionesUI->calefaccionOn == false){
    lv_obj_clear_state(ui_ButtonCalefaccion, LV_STATE_CHECKED);
    // Si se apago por la causa de alarma
    if (*causaAlarmaUI == BOMBA_CALEFACCION_ON || *causaAlarmaUI == BOMBAS_OFF || *causaAlarmaUI == BOMBAS_ON || *causaAlarmaUI == FALLA_SENSOR_1){
      lv_obj_remove_flag(ui_ButtonCalefaccion, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_set_style_bg_color(ui_ButtonCalefaccion, lv_color_hex(0xC52222), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  }
}

void actualizarValoresPantallaAlertas(){
  static causaAlarma_t alarmaEnPantalla = NO_FALLA;
  
  if (*causaAlarmaUI != alarmaEnPantalla){
    lv_label_set_text(ui_LabelError, obtenerLabelAlarma(*causaAlarmaUI));
    lv_label_set_text(ui_LabelAclaracion, obtenerLabelAclaracionAlarma(*causaAlarmaUI));
    alarmaEnPantalla = *causaAlarmaUI;
  }
  
}

void actualizarValoresPantallaAlarmas(){

}

void actualizarValoresConfiguracionesAvanzadas(){
}

const char* obtenerLabelAlarma(causaAlarma_t causa){
  switch (causa){
    case BOMBA_CALEFACCION_ON:  
      return "ENCENDIDO BOMBA CALEFACCION";
      break;
    case BOMBA_AGUA_CALIENTE_ON:    
      return "ENCENDIDO BOMBA ACS";
      break; 
    case BOMBAS_OFF:
      return "APAGADO DE BOMBAS";
      break;
    case BOMBAS_ON:
      return "ENCENDIDO DE BOMBAS";
      break;
    case PRESION_ALTA:
      return "PRESION ALTA";
      break;        
    case PRESION_BAJA:
      return "PRESION BAJA";
      break;
    case TEMPERATURA_ALTA:
      return "TEMPERATURA ALTA";
      break;
    case TEMPERATURA_BAJA:
      return "TEMPERATURA BAJA";
      break;
    case TERMOSTATO_SEGURIDAD:
      return "TERMOSTATO SEGURIDAD";
      break;
    case FALLA_SENSOR_1:
      return "FALLA SENSOR CALDERA";
      break;
    case FALLA_SENSOR_2:
      return "FALLA SENSOR AGUA CALIENTE";
      break;
    case FALLA_SENSOR_I:
      return "FALLA SENSOR DE PRESION";
      break;
    default: "";
  }
}

const char* obtenerLabelAclaracionAlarma(causaAlarma_t causa){
  switch (causa){
    case BOMBA_CALEFACCION_ON:  
    case BOMBA_AGUA_CALIENTE_ON: 
    case BOMBAS_ON: 
      return "Fallo al encender la bomba. Resetear y si vuevlve a ocurrir llamar al tecnico";
      break;
    case BOMBAS_OFF:
      return "Fallo al apagar las bombas. Resetear y si vuevlve a ocurrir llamar al tecnico";
      break;
    case PRESION_ALTA:
      return "Se detecto presion alta, se recomienda no usar el equipo";
      break;        
    case PRESION_BAJA:
      return "Se detecto presion baja, se recomienda no usar el equipo";
      break;
    case TEMPERATURA_ALTA:
    case TERMOSTATO_SEGURIDAD:
      return "Desconectar el equipo y llamar al tecnico antes de volver a utilizar";
      break;
    case TEMPERATURA_BAJA:
      return "Riesgo de congelamiento";
      break;
    case FALLA_SENSOR_1:
    case FALLA_SENSOR_2:
    case FALLA_SENSOR_I:
      return "Llamar al tecnico para cambiar el sensor";
      break;
    default: "";
  }
}
