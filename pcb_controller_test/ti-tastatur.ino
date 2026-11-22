//Projekt:  TI-32 – TI-84 als PC-Controller
//Author:   Daniel
//Date:     11.26
//
//Ablauf: TI-84 (getKey -> Send(K)) -> ESP32 -> Serial "KEY:<code>" -> Python-Skript -> Tastendruck


//Bibliotheken für TI-Protokoll von https://github.com/KermMartian/ArTICL
#include <TICL.h>
#include <CBL2.h>
#include <TIVar.h>

//Konstanten (constexpr: Compile-Zeit-Konstante mit Typprüfung, keine reine Textersetzung wie #define)
constexpr auto RING_PIN      = D10;
constexpr auto TIP_PIN       = D1;
constexpr auto MAX_DATALEN   = 4096;      //Maximale Variablengröße für cbl-Callbacks in Bytes
constexpr auto MAX_HEADERLEN = 16;        //Maximale Headergröße für cbl-Callbacks in Bytes
constexpr bool DEBUG         = false;     //true: ausführliche Ausgabe jeder Variable (zum Fehlersuchen)

//Prototypen
int onReceived(uint8_t type, enum Endpoint model, int datalen);
int onRequest(uint8_t type, enum Endpoint model, int* headerlen, int* datalen, data_callback* cb);
void debugAusgabe(uint8_t type, enum Endpoint model, int datalen);

//Globale Programmvariablen: setup(), loop() und die Callbacks greifen darauf zu
CBL2 cbl;                                         //cbl-Objekt, simuliert ein CBL2-Messgerät für den TI-84
uint8_t header_puffer[MAX_HEADERLEN] = {0};       //Header der zuletzt empfangenen Variable
uint8_t data_puffer[MAX_DATALEN]     = {0};       //Nutzdaten der zuletzt empfangenen Variable


void setup() {
  Serial.begin(9600);                     //Serial zum PC (Serial Monitor bzw. Python-Skript)
  delay(2000);                            //Zeit, damit sich der PC nach dem Reset wieder verbindet

  cbl.setLines(TIP_PIN, RING_PIN);        //Pins für Tip und Ring festlegen
  cbl.resetLines();                       //beide Leitungen freigeben (Ruhezustand, Open-Drain)
  cbl.setupCallbacks(header_puffer, data_puffer, MAX_DATALEN, onReceived, onRequest);

  Serial.println("[ready] TI-Link gestartet");
}

void loop() {
  cbl.eventLoopTick();                    //Link-Verkehr abarbeiten, ruft ggf. die Callbacks auf
}


//Wird von cbl aufgerufen, sobald der TI-84 eine Variable per Send( geschickt hat
int onReceived(uint8_t type, enum Endpoint model, int datalen) {
  if (DEBUG) {
    debugAusgabe(type, model, datalen);
  }

  char name = header_puffer[3];           //Variablenname, z. B. 'K'

  //Tastendruck: reelle Zahl in Variable K
  if (type == VarTypes82::VarReal && name == 'K') {
    int taste = (int)TIVar::realToLong8x(data_puffer, model);
    Serial.printf("KEY:%d\n", taste);     //genau dieses Format erwartet das Python-Skript
    return 0;
  }

  //alles andere wird ignoriert (nur Info fürs Debugging)
  Serial.printf("Ignoriert: Typ 0x%02X, Name 0x%02X\n", type, (uint8_t)name);
  return 0;
}

//Wird von cbl aufgerufen, wenn der TI-84 per Get( etwas abholen will – hier noch nicht benutzt
int onRequest(uint8_t type, enum Endpoint model, int* headerlen, int* datalen, data_callback* cb) {
  return 0;                               //Achtung: kein Get( auf dem Rechner verwenden!
}


//Ausführliche Ausgabe einer empfangenen Variable (nur wenn DEBUG = true)
void debugAusgabe(uint8_t type, enum Endpoint model, int datalen) {
  Serial.println("--- Variable empfangen ---");
  Serial.printf("Typ:    0x%02X\n", type);
  Serial.printf("Model:  0x%02X\n", model);
  Serial.printf("Länge:  %d Bytes\n", datalen);

  Serial.printf("Name:   0x%02X", header_puffer[3]);
  if (header_puffer[3] >= 'A' && header_puffer[3] <= 'Z') {
    Serial.printf(" ('%c')", header_puffer[3]);
  }
  Serial.println();

  Serial.print("Daten:  ");
  for (int j = 0; j < datalen && j < 32; j++) {
    Serial.printf("%02X ", data_puffer[j]);
  }
  Serial.println();

  if (type == VarTypes82::VarReal) {
    Serial.printf("Wert:   %f\n", TIVar::realToFloat8x(data_puffer, model));
  } else if (type == VarTypes82::VarString) {
    Serial.println("Text:   " + TIVar::strVarToString8x(data_puffer, model));
  }
}