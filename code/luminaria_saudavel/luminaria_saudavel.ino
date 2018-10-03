/*
   Luminaria Saudavel
   por MauMaker
   www.maumaker.com

   para We Fab
   www.wefab.cc

   Uma luminaria com fita de led endereçável ws2811/2812 
   e sensor de distância HC-SR04
   Hands-On / Luminária Saudável

    Os participantes irão construir uma Luminária de
    mesa que possui iluminação em RGB (todas as cores).
    Ela tem a capacidade de detectar a sua presença e
    iniciar uma contagem para depois de um tempo
    determinado te lembrar de se levantar, caminhar um
    pouco e refrescar a cabeça.
    
    Problemática: Muitas vezes, sem perceber, passamos o
    dia todo sentados, sem quase nenhuma pausa para
    esticar um pouco as pernas. Isso causa uma série de
    problemas graves para nossa saúde. O que podia me
    ajudar?

   
   Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0)
   https://creativecommons.org/licenses/by-nc-sa/4.0/deed.pt_BR
*/

#include "FastLED.h"
#define DATA_PIN A0
#define NUM_LEDS (6)

#include "Ultrasonic.h"
// https://github.com/ErickSimoes/Ultrasonic

#define TRIG_PIN A1
#define ECHO_PIN A2

Ultrasonic us(TRIG_PIN, ECHO_PIN);

#define BRIGHTNESS  200
#define MILLIS_PER_FRAME 100
#define MILLIS_PER_DEBUG 2


CRGB leds[NUM_LEDS];

/////////////////////////////////////////////////////////////
// cada "frame" vale um décimo de segundo, os tempos são contados em frames

const long t_acende = 6L; //
const long t_liga = 33000L; // 55 minutos contado em milisegundos
const long t_alerta = 3000L; //  5 minutos, contado em milisegundos
const long t_pulsar = 12L;
const long t_afastado =  3000L; // 5 minutos, tempo minimo afastado

const long t_muda = 6000L; // 10 minutos
const long t_apaga = 6L; //
/////////////////////////////////////////////////////////////

const float d_presente = 120.0; // cm
const float d_gesto = 10.0; // cm
const long t_gesto = 1000L; // tempo em ms = 1 segundos
const long t_longe = 3000L; // tantos microsegundos sem detectar significa ausencia

/////////////////////////////////////////////////////////////

byte estado;
byte estado_antes;
long frame;
long frame_antes;

long t_ciclo;
long ultima_vez;
long t_medida;
float distancia;

/////////////////////////////////////////////////////////////
// MAQUINA DE ESTADOS FINITOS
/////////////////////////////////////////////////////////////
const byte APAGADO = 0;
const byte ACESO = 1;
const byte ALERTA = 2;
const byte PULSAR = 3;
const byte AUSENTE = 4;

/////////////////////////////////////////////////////////////

boolean gesto = false;
boolean ativado = false;
boolean detect = false;
boolean detect_antes = false;
long t_detect;

boolean ausente = true;
boolean presente = false;
long t_presente;
long t_ausente;

/////////////////////////////////////////////////////////////
// MODO DE DESENVOLVIMENTO
/////////////////////////////////////////////////////////////
const int dev_pin = 13;
/////////////////////////////////////////////////////////////

void setup() {
  delay(100);

  Serial.begin(115200);

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  pinMode (dev_pin, INPUT_PULLUP);

  estado = APAGADO;
  frame = 0;

  for (int j = 0; j < NUM_LEDS; j++) {
    leds[j] = CRGB::Black;
  }
  prox_estado(APAGADO);
  ultima_vez = millis();
  t_medida = -t_gesto;
}

void loop() {
  // modo desenvolvimento
  if (digitalRead(dev_pin) == LOW) {
    t_ciclo = MILLIS_PER_DEBUG;
  }
  else {
    t_ciclo = MILLIS_PER_FRAME;
  }

  if (millis() - t_medida > t_gesto / 10) {
    le_dist();
    t_medida = millis();
  }

  // maquina de estados finitos
  //mudar();
  switch (estado) {
    case APAGADO:
      estado_apagado();
      break;

    case ACESO:
      estado_aceso();
      break;

    case ALERTA:
      estado_alerta();
      break;

    case PULSAR:
      estado_pulsar();
      break;

    case AUSENTE:
      estado_ausente();
      break;

  }

  Serial.print(estado);
  Serial.print(",");
  Serial.print(estado_antes);
  Serial.print(",");
  Serial.print(t_ciclo);
  Serial.print(",");
  Serial.print((float(frame) / 1000.0), 3);
  Serial.print(",");
  Serial.print(distancia);
  Serial.print(",");
  Serial.print(d_presente);
  Serial.print(",");
  Serial.print(presente);
  Serial.print(",");
  Serial.print(ausente);
  Serial.println();

  // delay(t_ciclo);
  while (millis() - ultima_vez < t_ciclo) {
    // faz nada
  }
  ultima_vez = millis();

  frame++;
}

void prox_estado (int e) {
  gesto = false;
  estado_antes = estado;
  frame_antes = frame;
  estado = e;
  frame = 0;
}

void undo_estado() {
  estado = estado_antes;
  frame = frame_antes + frame;
}

void estado_apagado() {
  float qt = (NUM_LEDS - 1) * float(frame) / float(t_apaga);
  for ( long j = 0; j < NUM_LEDS; j++) {
    if (qt >= j) {
      leds[j] = CRGB::Black;
    }
    FastLED.show();
  }
  if (gesto) prox_estado(ACESO);
}

void estado_aceso() {
  float qt = (NUM_LEDS - 1) * float(frame) / float(t_acende);
  for ( long j = 0; j < NUM_LEDS; j++) {
    if (qt >= j) {
      leds[(NUM_LEDS - 1) - j] = CRGB::White;
    }
    FastLED.show();
  }
  if (gesto) prox_estado(APAGADO);
  if (frame >= t_liga) prox_estado(ALERTA);
}


void estado_alerta() {
  float qt = (NUM_LEDS ) * float(frame) * 4.0 / float(t_alerta);
  for ( long j = 0; j < NUM_LEDS; j++) {
    if (qt >= j) {
      leds[(NUM_LEDS - 1) - j] = CRGB(255, 120, 0);
    }
    if (qt >= j + NUM_LEDS) {
      leds[(NUM_LEDS - 1) - j] = CRGB(255, 80, 0);
    }
    if (qt >= j + 2 * NUM_LEDS) {
      leds[(NUM_LEDS - 1) - j] = CRGB(255, 40, 0);
    }
    if (qt >= j + 3 * NUM_LEDS) {
      leds[(NUM_LEDS - 1) - j] = CRGB(255, 0, 0);
    }
    FastLED.show();
  }
  if (gesto) prox_estado(APAGADO);
  if (frame >= t_alerta) prox_estado(PULSAR);
}

void estado_pulsar() {
  float qt = (NUM_LEDS) * float(frame % t_pulsar) * 2.0 / float(t_pulsar);
  for ( long j = 0; j < NUM_LEDS; j++) {
    if (qt >= j) {
      leds[(NUM_LEDS - 1) - j] = CRGB(10, 0, 0);
    }
    if (qt >= j + NUM_LEDS) {
      leds[(NUM_LEDS - 1) - j] = CRGB::Red;
    }
    FastLED.show();
  }
//  if (gesto) prox_estado(APAGADO);
}

void estado_ausente() {
  float qt = (NUM_LEDS) * float(frame) / float(t_afastado);
  for ( long j = 0; j < NUM_LEDS; j++) {
    if (qt >= j) {
      leds[j] = CRGB(0, 10, 0);
    }

    FastLED.show();
  }

  if (presente) undo_estado();
  if (frame > t_afastado) prox_estado(APAGADO);
  //  if (gesto) prox_estado(APAGADO);
}

void le_dist() {
  distancia = us.read();
  gesto = false;
  boolean detect = (distancia <= d_gesto);
  if  (!detect_antes && detect) {
    t_detect = millis();
    Serial.println("_____________________________________");
  }
  if  (detect_antes && !detect) {
    ativado = false;
  }
  if (!ativado && detect && detect_antes && millis() - t_detect >= t_gesto) {
    gesto = true;
    ativado = true;
    Serial.println("*************************************");
  }
  detect_antes = detect;

  if (distancia <= d_presente && presente == false) {
    presente = true;
    ausente = false;
    t_presente = millis();
    Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  }
  if (distancia > d_presente  && ausente == false) {
    ausente = true;
    t_ausente = millis();
    Serial.println("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
  }
  if (ausente == true && presente == true && millis() - t_ausente >= t_longe) {
    presente = false;
    if (estado == ACESO || estado == ALERTA || estado == PULSAR) {
      prox_estado(AUSENTE);
    }
    Serial.println("/////////////////////////////////////");
  }
}
