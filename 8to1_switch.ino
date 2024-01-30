
/* ------------------------------------------------------------------------- *
 * Name   : 8:1 Switch Antenna
 * Author : Stéphane HELAIEM - F4IRX
 * Date   : August 14, 2023
 * Purpose: Switch Antenna / Radio ( 8 antenna / 8 Radio )
 * Versions:
 *    0.1  : Initial code base, test AQV212 with relay
 *    0.2  : Add Pre-Code and Versioning
 *    0.3  : Add EEPROM and save/get config
 *    0.4  : Add buttun Radio+Antenna (GUI)
 *    0.5  : Add memory GUI and Setting
 *    0.6  : Add debug trace
 *    0.7  : Add Relay
 *    0.8a : Add BIP
 *    0.9a : Add PTT and Stick
 *    0.9b : 
 * ------------------------------------------------------------------------- */
#define progVersion "0.9b"                  
/* ------------------------------------------------------------------------- *
 *             GNU LICENSE CONDITIONS
 * ------------------------------------------------------------------------- *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * ------------------------------------------------------------------------- *
 *       Copyright (C) August 2023 Stéphane HELAIEM - F4IRX
 * ------------------------------------------------------------------------- */


/*

TFT 4.0 ST7796 SPI 
******************
TFT_eSPI ver = 2.5.31
Processor    = RP2040
Transactions = Yes
Interface    = SPI
Display driver = 7796
Display width  = 320
Display height = 480

MOSI    = GPIO 19
MISO    = GPIO 16
SCK     = GPIO 18
TFT_CS   = GPIO 17
TFT_DC   = GPIO 21
TFT_RST  = GPIO 20
TOUCH_CS = GPIO 13

Font GLCD   loaded
Font 2      loaded
Font 4      loaded
Font 6      loaded
Font 7      loaded
Font 8      loaded
Smooth font enabled

Display SPI frequency = 40.00
Touch SPI frequency   = 2.50

*/






/* ------------------------------------------------------------------------- *
 *       8/1 Switch debugging on / off 
 * ------------------------------------------------------------------------- */
#define DEBUG 1

#if DEBUG == 1
  #define debugstart(x) Serial.begin(x)
  #define debug(x) Serial.print(x)
  #define debugln(x) Serial.println(x)
#else
  #define debugstart(x)
  #define debug(x)
  #define debugln(x)
#endif
/* ------------------------------------------------------------------------- *
 *       Include libraries 
 * ------------------------------------------------------------------------- */


#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>      // Hardware-specific library
#include <EEPROM.h>

/* ------------------------------------------------------------------------- *
 *       Pin definitions 
 * ------------------------------------------------------------------------- */
#define Relai_A 10  // GPIO10
#define Relai_B 11
#define Relai_C 12
#define Relai_Antenna_A 9  // GPIO10
#define Relai_Antenna_B 8
#define Relai_Antenna_C 7
#define TFT_DC 21
#define TFT_CS 17
#define TFT_MOSI 19
#define TFT_CLK 18
#define TFT_RST 20
#define TFT_MISO 16
#define TCS_PIN  13
#define TIRQ_PIN 14
#define control_eclair_affich 5
#define Vibreur 22
#define PTT 2

/* ------------------------------------------------------------------------- *
 *       Other definitions
 * ------------------------------------------------------------------------- */
unsigned int antenne_Tx = 0; 
int i;

struct Settings {
  int Radio;  
  int Antenna;
  bool Bip;
};
Settings mySettings;                        // Create the object


unsigned long int debut_rafraich_pause_millis = 0; 

// This is the file name used to store the calibration data
// You can change this to create new calibration files.
// The SPIFFS file name must start with "/".
#define CALIBRATION_FILE "/TouchCalData2"
#define CONFIG_FILE "/config"
// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false

// Using two fonts since numbers are nice when bold
#define LABEL1_FONT &FreeSansOblique12pt7b // Key label font 1
#define LABEL2_FONT &FreeSansBold12pt7b    // Key label font 2
// use for positionning Radio button
uint16_t Radio_Button_X[8]={60,180,300,420,60,180,300,420};
uint16_t Radio_Button_Y[8]={60,60,60,60,120,120,120,120};
#define Radio_W 100//62 // Width and height
#define Radio_H 40//30
// use for positionning antenna button
uint16_t Antenna_Button_X[8]={60,180,300,420,60,180,300,420};
uint16_t Antenna_Button_Y[8]={220,220,220,220,280,280,280,280};
#define Antenna_W 100//62 // Width and height
#define Antenna_H 40//30
// value active and deselected
uint8_t Radio_Active=1;
uint8_t Old_Radio_Active=1;
uint8_t Antenna_Active=1;
uint8_t Old_Antenna_Active=1;
bool BIP;
bool ptt_actif = false; 
bool ptt_Stick = false;
// Create 15 keys for the keypad

char Radio_Label[8][6] = {"FT857", "FT102", "210X", "SB301","SB100", "HW101", "QCX", "SDR"};
char Antenna_Label[8][7] = {"Dummy", "G5RV", "YAGI", "n/a","Sloper", "LoG", "n/a", "n/a"};
char Radio_Stick[8][15]={"Yaesu FT857D","Yaesu FT102","ATLAS 210X","Heathkit SB301","Heathkit SB100","Heatkit HW101","QCX Mini","SDR RSPlay"};
char Antenna_Stick[8][15]={"Dummy LOAD 50W","G5RV","YAGI FB33","n/a","1/2 Sloper 40m","Loop on Ground","n/a","n/a"};
// uint16_t keyColor[15] = {TFT_RED, TFT_BLUE, TFT_BLUE,
//                          TFT_BLUE, TFT_BLUE, TFT_BLUE,
//                          TFT_BLUE, TFT_BLUE, TFT_BLUE,
//                          TFT_BLUE, TFT_BLUE, TFT_BLUE,
//                          TFT_BLUE, TFT_BLUE, TFT_BLUE
//                         };
// Keypad start position, key sizes and spacing
// #define KEY_X 60 //40 // Centre of key
// #define KEY_Y 60 //96
// #define KEY_W 100//62 // Width and height
// #define KEY_H 40//30

// #define KEY_X_Antenna 60 //40 // Centre of key
// #define KEY_Y_Antenna 220
// #define KEY_W_Antenna 100//62 // Width and height
// #define KEY_H_Antenna 40//30

// #define KEY_SPACING_X 15 // X and Y gap
// #define KEY_SPACING_Y 20
#define KEY_TEXTSIZE 1   // Font size multiplier

int incomingByte = 0; // for incoming serial data

TFT_eSPI_Button Radio_button[8];
TFT_eSPI_Button Antenna_button[8];



/* ------------------------------------------------------------------------- *
 *       Create objects 
 * ------------------------------------------------------------------------- */

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library



// Fonction produisant un court bip sonore sur le vibreur magnétique
void vibreur_bip()
{
  tone(Vibreur, 2400);                                            // 2400 Hz, la fréquence suggérée du vibreur magnétique
  debut_rafraich_pause_millis = millis();
  while (millis() <= (debut_rafraich_pause_millis + 90));         // Une durée de 30 millisecondes
  noTone(Vibreur);
}




/* ------------------------------------------------------------------------- *
 *       Store settings to EEPROM                            storeSettings()
 * ------------------------------------------------------------------------- */
void storeSettings() {

  EEPROM.write(0, Radio_Active);
  EEPROM.write(1, Antenna_Active);
  EEPROM.write(2, BIP);
  EEPROM.commit();
//  getSettings();
}
/* ------------------------------------------------------------------------- *
 *       Retrieve settings from EEPROM                         getSettings()
 * ------------------------------------------------------------------------- */
void getSettings() {
   Radio_Active    = EEPROM.read(0);
   Antenna_Active    = EEPROM.read(1);
   BIP  = EEPROM.read(2);
   debug("Radio:");debugln(Radio_Active);
   debug("Antenne:");debugln(Antenna_Active);
   debug("Bip:");debugln(BIP);
    // init radio and antenna
  // Print_Radio_Stick(Radio_Stick[Radio_Active],TFT_YELLOW);
  // Print_Antenna_Stick(Antenna_Stick[Antenna_Active],TFT_YELLOW);  
}



void setup() {
  debugstart(9600);
  #if DEBUG == 1
   while (!Serial && (millis() <= 5000));                  // Une pause d'au moins 5 seconde
  #endif
  debug("8/1 Switch Version :");
  debug(progVersion);
  debugln(" - debugging start");
  debug("Filename:");
  debugln(__FILE__);
  debug("Board:");debugln(BOARD_NAME);
  debug("TFT Verion : ");debugln(TFT_ESPI_VERSION);
  EEPROM.begin(256);  
  getSettings();
  // init relay
  digitalWrite(Relai_A, LOW);                            
  pinMode(Relai_A, OUTPUT);                              
  digitalWrite(Relai_B, LOW);                            
  pinMode(Relai_B, OUTPUT);                              
  digitalWrite(Relai_C, LOW);                            
  pinMode(Relai_C, OUTPUT);                              
  digitalWrite(Relai_Antenna_A, LOW);                            
  pinMode(Relai_Antenna_A, OUTPUT);                              
  digitalWrite(Relai_Antenna_B, LOW);                            
  pinMode(Relai_Antenna_B, OUTPUT);                              
  digitalWrite(Relai_Antenna_C, LOW);                            
  pinMode(Relai_Antenna_C, OUTPUT);                              

 pinMode(PTT, INPUT);                                 // La broche PTT_In initalisée en entrée

  // init ecran
  pinMode(TCS_PIN, OUTPUT);                               // S'assurer que les broches CS sont au niveau haut pour commencer
  digitalWrite(TCS_PIN, HIGH);                            //    "
  pinMode(TFT_CS, OUTPUT);                                //    "
  digitalWrite(TFT_CS, HIGH);                             //    "
  pinMode(control_eclair_affich, OUTPUT);                 // Initialiser la broche de contrôle d'éclairage de l'affichage comme sortie et allumer l'éclairage
  digitalWrite(control_eclair_affich, HIGH);              //    "

// Initialise the TFT screen
  tft.init();
  // Set the rotation before we calibrate
  tft.setRotation(1);
  touch_calibrate();
  // Clear the screen
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 320, 480, TFT_BLACK);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(20, 20);
  tft.setFreeFont(LABEL2_FONT);
  tft.println("F4IRX");
  tft.setCursor(100, 20);
  tft.println("version ");
  tft.setCursor(200, 20);
  tft.println(progVersion);
  drawKeypad();
  // bouton memoire
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextDatum(TR_DATUM);
  tft.setTextSize(1);
  tft.drawString("M", 475, 3);
UpdateRadioInfo();
UpdateAntennaInfo();

  // Print_Radio_Stick(Radio_Stick[Radio_Active],TFT_YELLOW);
  // Print_Antenna_Stick(Antenna_Stick[Antenna_Active],TFT_YELLOW);  
  // Radio_button[Radio_Active].initButton(&tft,Radio_Button_X[Radio_Active],Radio_Button_Y[Radio_Active],Radio_W,Radio_H,TFT_WHITE, TFT_RED, TFT_WHITE,Radio_Label[Radio_Active],KEY_TEXTSIZE); // x, y, w, h, outline, fill, text
  // Radio_button[Radio_Active].drawButton(); 
  // Antenna_button[Antenna_Active].initButton(&tft,Antenna_Button_X[Antenna_Active],Antenna_Button_Y[Antenna_Active],Radio_W,Radio_H,TFT_WHITE, TFT_RED, TFT_WHITE,Antenna_Label[Antenna_Active],KEY_TEXTSIZE); // x, y, w, h, outline, fill, text
  // Antenna_button[Antenna_Active].drawButton(); 
}

void setup1()
{

  //  Serial.begin(9600);                                   // Port debug USB. Activer seulement pour déboguer
  Serial1.begin(4800);                                    // Port série connecté au transceiver (protocole CI-V)

  // Envoi d'une requête initiale de fréquence. Si pas de réponse obtenue, les antennes en EEPROM seront applicables.
  const byte tx_data[] = {0xFE, 0xFE, 0x94, 0xE0, 0x03, 0xFD}; // La syntaxe du message de requête de fréquence
  Serial1.write(tx_data, sizeof(tx_data));                // Envoi du message

}

void loop() {
  uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
  if (ptt_actif != !digitalRead(PTT))                            // Niveau bas actif sur la broche PTT_In?
    {
      ptt_actif = !digitalRead(PTT);                               // Oui, changer l'état de la variable PTT
    }
  if (ptt_actif)                                                 
    {
      if (ptt_Stick!=ptt_actif) {
        tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
        tft.setTextDatum(TC_DATUM);
        tft.setTextSize(1);
        tft.drawString(" - PTT ON - ", 340, 3);
        ptt_Stick=ptt_actif;
      }
    }
    else                                                           
    {
      // on efface 
      if (ptt_Stick!=ptt_actif) {
        tft.fillRect(250, 1, 200, 30, TFT_BLACK);
        ptt_Stick=ptt_actif;
      }

  // Pressed will be set true is there is a valid touch on the screen
  bool pressed = tft.getTouch(&t_x, &t_y);
  if (pressed && t_x>460 && t_y<20) {
    vibreur_bip();
    debugln("Store MEMORY");
    Print_Radio_Stick("Store in Memory",TFT_RED);
    Print_Antenna_Stick("OK",TFT_RED);
    storeSettings();
    delay(3000);
    Print_Radio_Stick(Radio_Stick[Radio_Active],TFT_YELLOW);
    Print_Antenna_Stick(Antenna_Stick[Antenna_Active],TFT_YELLOW);}
  // / Check if any key coordinate boxes contain the touch coordinates
  for (uint8_t b = 0; b < 8; b++) {
    // check for radio button
     if (pressed && Radio_button[b].contains(t_x, t_y)) {
       Radio_button[b].press(true);  // tell the button it is pressed
       Old_Radio_Active=Radio_Active;
       vibreur_bip();
     } else {
       Radio_button[b].press(false);  // tell the button it is NOT pressed
     }
    // check for antenna button
     if (pressed && Antenna_button[b].contains(t_x, t_y)) {
       Antenna_button[b].press(true);  // tell the button it is pressed
       Old_Antenna_Active=Antenna_Active;
       vibreur_bip();
     } else {
       Antenna_button[b].press(false);  // tell the button it is NOT pressed
     }
  }
  for (uint8_t b = 0; b < 8; b++) {
    if (Radio_button[b].justPressed()) {
        debug("Touch Radio button:");debugln(b);
        Radio_Active=b;
        UpdateRadioInfo();
        }
    if (Antenna_button[b].justPressed()) {
        debug("Touch Antenna button:");debugln(b);
        Antenna_Active=b;
        UpdateAntennaInfo();
        }
    }

    }



} // end of loop

void UpdateRadioInfo(){
        Radio_button[Old_Radio_Active].initButton(&tft,Radio_Button_X[Old_Radio_Active],Radio_Button_Y[Old_Radio_Active],Radio_W,Radio_H,TFT_WHITE, TFT_BLUE, TFT_WHITE,Radio_Label[Old_Radio_Active],KEY_TEXTSIZE); // x, y, w, h, outline, fill, text
        Radio_button[Old_Radio_Active].drawButton(); 
        Radio_button[Radio_Active].initButton(&tft,Radio_Button_X[Radio_Active],Radio_Button_Y[Radio_Active],Radio_W,Radio_H,TFT_WHITE, TFT_RED, TFT_WHITE,Radio_Label[Radio_Active],KEY_TEXTSIZE); // x, y, w, h, outline, fill, text
        Radio_button[Radio_Active].drawButton(); 
        Print_Radio_Stick(Radio_Stick[Radio_Active],TFT_YELLOW);
        digitalWrite(Relai_A, bitRead(Radio_Active, 0));                 // Sélectionne les bons relais en Tx en fonction du choix de l'usager
        digitalWrite(Relai_B, bitRead(Radio_Active, 1));
        digitalWrite(Relai_C, bitRead(Radio_Active, 2));
        debug("Radio -> ");debug(Radio_Active);debug("  -  ");debug( bitRead(Radio_Active, 2));debug( bitRead(Radio_Active, 1));debugln( bitRead(Radio_Active, 0));
  
}

void UpdateAntennaInfo(){
        Antenna_button[Old_Antenna_Active].initButton(&tft,Antenna_Button_X[Old_Antenna_Active],Antenna_Button_Y[Old_Antenna_Active],Radio_W,Radio_H,TFT_WHITE, TFT_BLUE, TFT_WHITE,Antenna_Label[Old_Antenna_Active],KEY_TEXTSIZE); // x, y, w, h, outline, fill, text
        Antenna_button[Old_Antenna_Active].drawButton(); 
        Antenna_button[Antenna_Active].initButton(&tft,Antenna_Button_X[Antenna_Active],Antenna_Button_Y[Antenna_Active],Radio_W,Radio_H,TFT_WHITE, TFT_RED, TFT_WHITE,Antenna_Label[Antenna_Active],KEY_TEXTSIZE); // x, y, w, h, outline, fill, text
        Antenna_button[Antenna_Active].drawButton(); 
        Print_Antenna_Stick(Antenna_Stick[Antenna_Active],TFT_YELLOW);
        digitalWrite(Relai_Antenna_A, bitRead(Antenna_Active, 0));                 // Sélectionne les bons relais en Tx en fonction du choix de l'usager
        digitalWrite(Relai_Antenna_B, bitRead(Antenna_Active, 1));
        digitalWrite(Relai_Antenna_C, bitRead(Antenna_Active, 2));
        debug("Antenna -> ");debug(Antenna_Active);debug("  -  ");debug( bitRead(Antenna_Active, 2));debug( bitRead(Antenna_Active, 1));debugln( bitRead(Antenna_Active, 0));
}


void loop1() {
// Gestion des messages CI-V de fréquence courante du transceiver. Signale lorsque les MHz de la fréquence changent
  if (Serial1.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial1.read();

    // say what you got:
    Serial.print("I received: ");
    Serial.println(incomingByte, DEC);
  }

}

void Print_Radio_Stick(const char *string,uint32_t Color)
{
    tft.fillRect(10, 140, 300, 45, TFT_BLACK);
    tft.setTextColor(Color, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.drawString(string, 5, 160);
    debugln("Update radio stick");
}

void Print_Antenna_Stick(const char *string,uint32_t Color)
{
    tft.fillRect(250, 140, 300, 45, TFT_BLACK);
    tft.setTextColor(Color, TFT_BLACK);
    tft.setTextDatum(TR_DATUM);
    tft.setTextSize(1);
    tft.drawString(string, 475, 160);
    debugln("Update Antenna Stick");
}


void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;
  debugln("Calibration start");
  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("Formating file system");
    SPIFFS.format();
    SPIFFS.begin();
  }
  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      debugln("Remove File");
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      debugln("Open calibration file");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    debugln("User can start calibration");
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.println("Touch corners as indicated");
    tft.setTextFont(1);
    tft.println();
    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }
    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");
    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
      debugln("Calibration file UPDATE");
    }
  }
}
/* ------------------------------------------------------------------------- *
 *       Draw Buttons
 * ------------------------------------------------------------------------- */
void drawKeypad()
{
  tft.setFreeFont(LABEL2_FONT);
  for (uint8_t col = 0; col < 8; col++) {
  Radio_button[col].initButton(&tft,Radio_Button_X[col],Radio_Button_Y[col],Radio_W,Radio_H,TFT_WHITE, TFT_BLUE, TFT_WHITE,Radio_Label[col],KEY_TEXTSIZE); // x, y, w, h, outline, fill, text
  Radio_button[col].drawButton();
  Antenna_button[col].initButton(&tft,Antenna_Button_X[col],Antenna_Button_Y[col],Radio_W,Radio_H,TFT_WHITE, TFT_BLUE, TFT_WHITE,Antenna_Label[col],KEY_TEXTSIZE); // x, y, w, h, outline, fill, text
  Antenna_button[col].drawButton();  }
}
  
  


