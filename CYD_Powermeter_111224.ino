//    RF Wattmeter using a CYD Cheap TFT_YELLOW Display ESP32-2432S028 R  
//
//********************* VERY IMPORTANT ***********************************************
// The library TFT_eSPI
// Can be installed from the library manager (Search for "TFT_eSPI")
//https://github.com/Bodmer/TFT_eSPI
//Make sure to copy the UserSetup.h file into the library as
//per the Github Instructions.

// La librarie TFT_eSPI peut être be installée à partie du Gestionnaire de bibliothèque (Recherche pour "TFT_eSPI")
// https://github.com/Bodmer/TFT_eSPI 
// Récupérer le fichier UserSetup.h et le mettre dans la librairie TFT-eSPI (Documents/Arduino/librairies)
//***********************************************************************************

/* 11 décembre 2024
 * CYD Wattmetre RF utilisant une sonde Hewlett Packard HP33330B
 * Cheap Tellow Display RF Wattmeter using an HP33330B RF negative detector
 * By Ham radio F1CJN. The diode detector HP33330B from F1GE or any négative voltage RF diode detector f1ge.mg at gmail.com
 * alain.fort.f1cjn at gmail.com
 * 
 * Version V 1.0 pour CYD Cheap TFT_YELLOW Display
 * choix d'atténuateur d'entrée  0 à 40 dB par pas de 10 dB
 * choix du type de diode par le bouton central (3 types)
 * interpolation logarithmique entre -10 et -30 dBm 
 * "Alerte niveau" sur écran  si Pin>20dBm 
 * Inversion écran en touchant l'écran
 * Voltage = voltage read by the ADS1115 from the output of the OP192 == (HP33330B output voltage x -2)
*/

#include <TFT_eSPI.h>
#include <ADS1115_WE.h>
#include <EEPROM.h>
#include <XPT2046_Bitbang.h> //Effacer la librairie XPT2046_BitBang si elle est installée avant d'installer la librairie XPT2046_BitBang_Slim

ADS1115_WE adc = ADS1115_WE(0x48);

#define M_SIZE 1.3333  // Define meter size at 1.3333

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

XPT2046_Bitbang ts(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK, XPT2046_CS);
TFT_eSPI tft = TFT_eSPI();
TFT_eSPI_Button key[3];

uint16_t osx = M_SIZE * 120, osy = M_SIZE * 120;  // Saved x & y coords
uint32_t updateTime = 0;                          // time for next update
float ltx = 0;                                    // Saved x coord of bottom of needle
float voltage = 0;
float puissance_uW = 0;
float puissance_dBm = 0;
float v_m;

int interval = 10;      // Update interval 10 milliseconds
int old_analog = 9999;  // Value last displayed
int value = 2, att = 0;
int d = 0;
int flag = 0;
int flagNC=0;
int diode = 0;
int gamme = 1, oldgamme = 3;
int moins = true;
int tsize = 1 ; // Taille texte Volt/dBm

bool toggle = true; bool ecran=true;
int MeterLabel[11] = { -20, -19, -18, -17, -16, -15, -14, -13, -12, -11, -10 };                               //dBm
String MeterLabeldB10[11] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };                       //dBm
String MeterLabeldB00[11] = { "-10", "-9", "-8", "-7", "-6", "-5", "-4", "-3", "-2", "-1", "0" };             //dBm
String MeterLabeldB01[11] = { "-20", "-19", "-18", "-17", "-16", "-15", "-14", "-13", "-12", "-11", "-10" };  //dBm


//**********    Variables pour la mesure des sondes détectrices 
//**********( Entrez les valeurs mesurées en fin de programme à partir des lignes 359)  ****
//**********                   Ne pas modifier les 9 lignes suivantes              *********
float v_m30 = 0;  // Tension mesurée en mV par le CYD avec -30dBm au géné
float v_m20 = 0;  // Tension mesurée en mV par le CYD avec -20dBm au géné
float v_m10 = 0;  // Tension mesurée en mV par le CYD avec -10dBm au géné
float v_m5 = 0;   // Tension mesurée en mV par le CYD avec -5dBm au géné
float v_0 = 0;    // Tension mesurée en mV par le CYD avec  0dBm au géné
float v_5 = 0;    // Tension mesurée en mV par le CYD avec +5dBm au géné
float v_10 = 0;   // Tension mesurée en mV par le CYD avec +10dBm au géné
float v_15 = 0;   // Tension mesurée en mV par le CYD avec +15dBm au géné
float v_20 = 0;   // Tension mesurée en mV par le CYD avec +20dBm au géné
//******************************************************************************************


float readChannel(ADS1115_MUX channel) {
  adc.setCompareChannels(channel);
  adc.startSingleMeasurement();
  while (adc.isBusy()) {}
  voltage = adc.getResult_mV();  // alternative: getResult_mV for Millivolt
  return voltage;
}

// #########################################################################
//               Draw the analogue meter on the screen
// #########################################################################

void analogMeter() {
  tft.fillRect(0, 0, M_SIZE * 239, M_SIZE * 126, TFT_LIGHTGREY);
  tft.fillRect(5, 3, M_SIZE * 230, M_SIZE * 119, TFT_WHITE);

  tft.setTextColor(TFT_BLACK);  // Text colour

  // Draw ticks every 10 degrees from -50 to +50 degrees (100 deg. FSD swing)
  for (int i = -50; i < 51; i += 10)  // i=10  10 graduations
  {
    // Long scale tick length
    int tl = 15;
    // Coordinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (M_SIZE * 100 + tl) + M_SIZE * 120;
    uint16_t y0 = sy * (M_SIZE * 100 + tl) + M_SIZE * 140;
    uint16_t x1 = sx * M_SIZE * 100 + M_SIZE * 120;
    uint16_t y1 = sy * M_SIZE * 100 + M_SIZE * 140;

    // Short scale tick length
    if (i % 25 != 0) tl = 8;

    // Recalculate coords incase tick lenght changed
    x0 = sx * (M_SIZE * 100 + tl) + M_SIZE * 120;
    y0 = sy * (M_SIZE * 100 + tl) + M_SIZE * 140;
    x1 = sx * M_SIZE * 100 + M_SIZE * 120;
    y1 = sy * M_SIZE * 100 + M_SIZE * 140;

    // Draw tick
    tft.drawLine(x0, y0, x1, y1, TFT_BLACK);

    // Check if labels should be drawn, with position tweaks
    if (i % 10 == 0) {
      // Calculate label positions
      x0 = sx * (M_SIZE * 100 + tl + 10) + M_SIZE * 120;
      y0 = sy * (M_SIZE * 100 + tl + 10) + M_SIZE * 140;

      // gamme=att // for test
      switch (i / 10) {  // affichage valeurs incluant changement de gamme
        case -5: tft.drawCentreString(String(MeterLabel[0] + gamme), x0, y0 - 12, 2); break;
        case -4: tft.drawCentreString(String(MeterLabel[1] + gamme), x0, y0 - 9, 2); break;
        case -3: tft.drawCentreString(String(MeterLabel[2] + gamme), x0, y0 - 7, 2); break;
        case -2: tft.drawCentreString(String(MeterLabel[3] + gamme), x0, y0 - 9, 2); break;
        case -1: tft.drawCentreString(String(MeterLabel[4] + gamme), x0, y0 - 12, 2); break;
        case 0: tft.drawCentreString(String(MeterLabel[5] + gamme), x0, y0 - 12, 2); break;
        case 1: tft.drawCentreString(String(MeterLabel[6] + gamme), x0, y0 - 9, 2); break;
        case 2: tft.drawCentreString(String(MeterLabel[7] + gamme), x0, y0 - 7, 2); break;
        case 3: tft.drawCentreString(String(MeterLabel[8] + gamme), x0, y0 - 9, 2); break;
        case 4: tft.drawCentreString(String(MeterLabel[9] + gamme), x0, y0 - 12, 2); break;
        case 5: tft.drawCentreString(String(MeterLabel[10] + gamme), x0, y0 - 12, 2); break;
      }
    }

    // Now draw the arc of the scale
    sx = cos((i + 10 - 90) * 0.0174532925);
    sy = sin((i + 10 - 90) * 0.0174532925);
    x0 = sx * M_SIZE * 100 + M_SIZE * 120;
    y0 = sy * M_SIZE * 100 + M_SIZE * 140;
    // Draw scale arc, don't draw the last part
    if (i < 50) { tft.drawLine(x0, y0, x1, y1, TFT_BLACK); }
  }
  tft.drawCentreString("dBm", M_SIZE * 120, M_SIZE * 70, 4);
  tft.drawRect(5, 3, M_SIZE * 230, M_SIZE * 119, TFT_BLACK);  // Draw bezel line
  plotNeedle(0, 0);
  if (moins == true) {
    plotNeedle(0, 0);
  } else {
    plotNeedle(99, 0);
  }  // Reduit le temps de ralliement
}

// #########################################################################
// Update needle position
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################

void plotNeedle(int value, byte ms_delay) {
  //Serial.print("aiguille1=");
  //Serial.println(value);
  if (value < -10) value = -10;  // Limit value to emulate needle end stops
  if (value > 110) value = 110;

  // Move the needle until new value reached
  while (!(value == old_analog)) {
    if (old_analog < value) old_analog++;
    else old_analog--;

    if (ms_delay == 0) old_analog = value;  // Update immediately if delay is 0

    float sdeg = map(old_analog, -10, 110, -150, -30);  // Map value to angle
    // Calculate tip of needle coords
    float sx = cos(sdeg * 0.0174532925);
    float sy = sin(sdeg * 0.0174532925);

    // Calculate x delta of needle start (does not start at pivot point)
    float tx = tan((sdeg + 90) * 0.0174532925);

    // Erase old needle image
    tft.drawLine(M_SIZE * (120 + 20 * ltx - 1), M_SIZE * (140 - 20), osx - 1, osy, TFT_WHITE);
    tft.drawLine(M_SIZE * (120 + 20 * ltx), M_SIZE * (140 - 20), osx, osy, TFT_WHITE);
    tft.drawLine(M_SIZE * (120 + 20 * ltx + 1), M_SIZE * (140 - 20), osx + 1, osy, TFT_WHITE);

    // Re-plot text under needle
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString("dBm", M_SIZE * 120, M_SIZE * 70, 4);
    tft.fillRect(120, M_SIZE * 90, 160, 20, TFT_WHITE);  //efface ancienne ligne suivante
    tft.drawCentreString(("Att=" + String(att)), M_SIZE * 120, M_SIZE * 90, 4);
    // Enregistre les coordonnées du bout de aiguille pour le prochain effacement
    ltx = tx;
    osx = M_SIZE * (sx * 98 + 120);
    osy = M_SIZE * (sy * 98 + 140);

    // Draw the needle in the new position, magenta makes needle a bit bolder
    // dessine 3 lignes pour épaissir l'aiguille
    tft.drawLine(M_SIZE * (120 + 20 * ltx - 1), M_SIZE * (140 - 20), osx - 1, osy, TFT_RED);
    tft.drawLine(M_SIZE * (120 + 20 * ltx), M_SIZE * (140 - 20), osx, osy, TFT_MAGENTA);
    tft.drawLine(M_SIZE * (120 + 20 * ltx + 1), M_SIZE * (140 - 20), osx + 1, osy, TFT_RED);

    // Slow needle down slightly as it approaches new position
    if (abs(old_analog - value) < 10) ms_delay += ms_delay / 5;
    //Serial.print("ms_delay=");Serial.println(ms_delay);
    delay(10);
  }
}

void VoltdBm () {
    //Serial.print("Toggle=");Serial.println(toggle);
    tsize=4;
   if (toggle == false) {   //Affichage tension detecteur
      //tft.setTextColor(TFT_BLACK,TFT_BLACK); 
      //tft.drawString("                                                            ", 1, 180, tsize);  //Efface
      //tft.fillRect(0,170,320,30,TFT_BLACK);
      tft.setTextColor(TFT_WHITE,TFT_BLACK);
      tft.drawString("           Voltage = " + String(voltage) + " mV            ",1, 180, tsize);
      delay(100);
    }

    else if (toggle == true) { // affichage puissance
      
      tft.fillRect(170,170,160,30,TFT_BLACK); // efface 1/2 ligne de texte droite

      tft.setTextColor(TFT_WHITE,TFT_BLACK);
      tft.drawString("Pwr  " + String(puissance_dBm + att) + "dBm     ", 5, 180, tsize); //ok

      int xtxt = 300; int ytxt=207;
      tft.fillRect(180,170,140,30,TFT_BLACK); // efface 1/2 ligne de texte droite

      tft.setTextDatum(8);  // Test justifié droite bas
      if ((puissance_uW <= 1000) && (att == 0))  // affichage puissance avec attenuateur = 0 dB
      { tft.drawString(String(puissance_uW) + "uW",xtxt, ytxt, tsize); }
    
      if ((puissance_uW > 1000) && (att == 0)) { tft.drawString((String(puissance_uW / 1000)) + "mW", xtxt, ytxt, tsize); }
      if (att == 10)  // affichage puissance avec attenuateur = 10 dB
      { tft.drawString(String(puissance_uW / 100) + "mW", xtxt, ytxt, tsize); }

      if ((puissance_uW <= 1000) && (att == 20))  // affichage puissance avec attenuateur = 20 dB
      { tft.drawString(String(puissance_uW / 10) + "mW",xtxt, ytxt,tsize); }
      if ((puissance_uW > 1000) && (att == 20)) { tft.drawString(String(puissance_uW / 10000) + " W", xtxt, ytxt, tsize); }

      if (att == 30)  // affichage puissance avec attenuateur = 30 dB
      { tft.drawString(String(puissance_uW) + "uW", xtxt, ytxt, tsize); }

      if (att == 40)  // affichage puissance avec attenuateur = 40 dB
      { tft.drawString(String(puissance_uW / 100) + " W", xtxt, ytxt, tsize); }

    tft.setTextDatum(0); // Texte justifié haut gauche
    }

}

void drawButtons() {
  uint16_t bWidth = 106;
  uint16_t bHeight = 30;
  // Generate buttons with different size X deltas
  for (int i = 0; i < 3; i++) {
    key[i].initButton(&tft,
                      bWidth * (i%3) + bWidth/2, // position x
                      225,//position y
                      bWidth,
                      bHeight,
                      TFT_BLACK, // Outline
                      TFT_BLUE, // Fill
                      TFT_WHITE, // Text
                      "",
                      1);

    key[i].drawButton(false);
  }
  tft.drawCentreString("ATT="+String(att), 54, 217, 2);
  tft.drawCentreString("Select diode", 160, 217, 2);
  tft.drawCentreString("Volt/dBm", 262, 217, 2);
}

void setup() {
  Wire.begin(27,22);// I2C Utilisation des pins 27 (SDA) et 22 (SCLK)  avec le CYD module
  Serial.begin(115200);
  delay(1000);
  ts.begin() ; //demarrage touch screen
  tft.init();
  tft.setRotation(1);    //   Display en paysage
  tft.fillScreen(TFT_BLACK);
  Serial.println("Start");

  EEPROM.begin(16);
  diode = EEPROM.read(0);
  if (diode == 1 || diode == 2 || diode == 3) { choix_diode(); }  //
  else {diode = 1;choix_diode();att=0;}
  ecran = EEPROM.read(1);
  tft.invertDisplay(ecran); // Inversion ou non selon contenu de EEPROM (1)
  
  adc.setVoltageRange_mV(ADS1115_RANGE_2048);  //ADC range max= 2,047V  avec 32768 valeurs
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
  tft.drawCentreString("HP33330B", 160, 60, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("RF POWER METER", 160, 100, 4);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString("       F1CJN  F1GE  F1BHY       ", 160, 140, 4);

  adc.setCompareChannels(ADS1115_COMP_1_GND);  // mesure channel 1 par rapport à la masse
 
  delay(2000);
  tft.fillScreen(TFT_BLACK);
  updateTime = millis();  // Save update time
  drawButtons();
  choix_diode();
}

void choix_diode() {
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  if (diode == 1) {
   tft.drawCentreString("  HP33330B  ", 160, 217, 2);
    //************         Pour un étalonnage précis de votre sonde HP33330B           ************
    //********** Entrez ci-dessous les valeurs mesurées avec un générateur de précision ************
    //***************    valeurs  par défaut avec ma sonde  HP33330B                 ************
    v_m30 = 0.13;  // Tension mesurée en mV par le CYD avec -30dBm au géné
    v_m20 = 2.90;  // Tension mesurée en mV par le CYD avec -20dBm au géné
    v_m10 = 28.0;  // Tension mesurée en mV par le CYD avec -10dBm au géné
    v_m5 = 73;     // Tension mesurée en mV par le CYD avec -5dBm au géné
    v_0 = 167;     // Tension mesurée en mV par le CYD avec  0dBm au géné
    v_5 = 366;     // Tension mesurée en mV par le CYD avec +5dBm au géné
    v_10 = 738;    // Tension mesurée en mV par le CYD avec +10dBm au géné
    v_15 = 1713;   // Tension mesurée en mV par le CYD avec +15dBm au géné
    v_20 = 2052;   // Tension mesurée en mV par le CYD avec +20dBm au géné
    //**********************************************************************************************
  }

  if (diode == 2) {
   tft.drawCentreString("    DIODE2   ", 160, 217, 2);
    //************         Pour un étalonnage précis de votre sonde n°2                ************
    //********** Entrez ci-dessous les valeurs mesurées avec un générateur de précision ************
    //***************               valeurs  par défaut pour Diode n°2                  ************
    v_m30 = 0.13;  // Tension mesurée en mV par le CYD avec -30dBm au géné
    v_m20 = 2.9;   // Tension mesurée en mV par le CYD avec -20dBm au géné
    v_m10 = 28;    // Tension mesurée en mV par le CYD avec -10dBm au géné
    v_m5 = 73;     // Tension mesurée en mV par le CYD avec -5dBm au géné
    v_0 = 167;     // Tension mesurée en mV par le CYD avec  0dBm au géné
    v_5 = 366;     // Tension mesurée en mV par le CYD avec +5dBm au géné
    v_10 = 738;    // Tension mesurée en mV par le CYD avec +10dBm au géné
    v_15 = 1713;   // Tension mesurée en mV par le CYD avec +15dBm au géné
    v_20 = 2052;   // Tension mesurée en mV par le CYD avec +20dBm au géné
    //*********************************************************************************************
  }

  if (diode == 3) {
   tft.drawCentreString("    DIODE3   ", 160, 217, 2);
    //************         Pour un étalonnage précis de votre Diode n°3                *************
    //********** Entrez ci-dessous les valeurs mesurées avec un générateur de précision *************
    //***************               valeurs  par défaut pour Diode n°3                  *************
    v_m30 = 0.13;  // Tension mesurée en mV par le CYD avec -30dBm au géné
    v_m20 = 2.9;   // Tension mesurée en mV par le CYD avec -20dBm au géné
    v_m10 = 28;    // Tension mesurée en mV par le CYD avec -10dBm au géné
    v_m5 = 73;     // Tension mesurée en mV par le CYD avec -5dBm au géné
    v_0 = 167;     // Tension mesurée en mV par le CYD avec  0dBm au géné
    v_5 = 366;     // Tension mesurée en mV par le CYD avec +5dBm au géné
    v_10 = 738;    // Tension mesurée en mV par le CYD avec +10dBm au géné
    v_15 = 1713;   // Tension mesurée en mV par le CYD avec +15dBm au géné
    v_20 = 2052;   // Tension mesurée en mV par le CYD avec +20dBm au géné
    //**********************************************************************************************
  }
  Serial.print("Diode n°: ");
  Serial.println(diode);
  EEPROM.write(0, diode);  //memorisation du numero de diode en EEPROM
  EEPROM.commit();
  delay(10);
}

void loop() {
  Serial.println("Loop");
  
  if (updateTime <= millis()) {
    updateTime = millis() + interval;  // Update interval
    v_m = 0;
    voltage = 0;

    if (!adc.init()) {
    tft.setTextColor(TFT_RED, TFT_BLUE);
    tft.drawString("  ASD1115 not connected", 5, 215, 4);
    plotNeedle(2, 0);
    flagNC=1;
  }

 if (adc.init()) {
    for (int i = 1; i <= 10; i++) {
      voltage = readChannel(ADS1115_COMP_0_GND);  // mesure ADC
      if (flagNC==1) {flagNC=0;drawButtons(); }
      delay(1);
      v_m = v_m + voltage;
      //Serial.print("v_m="); Serial.println(v_m);
    }  // tension en mv
  }

    voltage = v_m / 10;  // moyenne de 10 mesures  // Vmax = 4V avec ampli G=2
    
    if (voltage < 0) { (voltage = v_m30); }  // si offset negatif ) la mise sous tension
    Serial.print("Voltage mV: ");Serial.println(voltage);

    // Square law de -30 à -10dBm
    //{puissance_dBm= -pow(10,(log10(Y0)+log10(Y1/Y0) * log10(X/X0) /log10(X1/X0)));// formule de base pour interpolation log }

    if ((voltage > v_m30) && (voltage <= v_m20))  // -30 à -20 dBm
    {
    //  Serial.print("OK30");
      puissance_dBm = -pow(10, (log10(20) + log10(1.5) * log10(voltage / v_m20) / (log10(v_m30 / v_m20))));
    }  // interpolation log

    if ((voltage > v_m20) && (voltage <= v_m10))  // -20 à -10 dBm
    {
      Serial.print("OK20");
      puissance_dBm = -pow(10, (log10(10) + log10(2) * log10(voltage / v_m10) / log10(v_m20 / v_m10)));
    }  // interpolation quadratique

    if ((voltage > v_m10) && (voltage <= v_m5)) { puissance_dBm = ((5 / (v_m5 - v_m10)) * (voltage - v_m10)) - 10; }  // -10 à -5dBm
    if ((voltage > v_m5) && (voltage <= v_0)) { puissance_dBm = ((5 / (v_0 - v_m5)) * (voltage - v_m5)) - 5; }        // -5 à 0dBm
    if ((voltage > v_0) && (voltage <= v_5)) { puissance_dBm = ((5 / (v_5 - v_0)) * (voltage - v_0)); }               // 0 à +5dBm
    if ((voltage > v_5) && (voltage <= v_10)) { puissance_dBm = ((5 / (v_10 - v_5)) * (voltage - v_5)) + 5; }         // +5 à +10dBm
    if ((voltage > v_10) && (voltage <= v_15)) { puissance_dBm = ((5 / (v_15 - v_10)) * (voltage - v_10)) + 10; }     // +10 à +15dBm
    if ((voltage > v_15) && (voltage <= v_20)) { puissance_dBm = ((5 / (v_20 - v_15)) * (voltage - v_15)) + 15; }     // +15 à +20dBm
    if (voltage > v_20) { puissance_dBm = 20; }                                                                   // limite 20dBm
    if (voltage <= v_m30) { puissance_dBm = -29.9; }                                                                    // si puissance <= -30dBm
    
    //  Butée si P >+20dBm en entrée du détecteur
    if (voltage >= (v_20 + 10)) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      Serial.print("Voltage butée=");Serial.println(voltage);
      tft.drawCentreString(String(" Danger Niveau "), 160, 180, 4);
      delay(1000);
    }
    //Serial.print("Puissance dBm: "); Serial.println(puissance_dBm,DEC);

  }

  float z = pow(10, ((puissance_dBm + 30) / 10));
  //Serial.print("z="); Serial.println(z);  // power with microwatt
  puissance_uW = z;

  if ((puissance_dBm > 10) && (puissance_dBm <= 20))  // gamme 10 à 20dBm
  {value = (puissance_dBm - 10) * 10;gamme = att + 30;}
  
  // gamme 0 à 10dBm
   else if ((puissance_dBm >= 0) && (puissance_dBm <= 10))  // gamme 0 à 10dBm
  { value = (puissance_dBm + 0 )* 10; gamme = att + 20;}

   // gamme -10 à 0dBm 
   else if ( (puissance_dBm >= -10) && (puissance_dBm < 0))  // gamme -10 à 0dBm
  { value = (puissance_dBm + 10) * 10; gamme = att + 10;}

   //gamme -20 à -10dBm
   else if ((puissance_dBm >= -20) && (puissance_dBm < -10))
  { value = (puissance_dBm + 20) * 10; gamme = att;}

   //gamme -30 à -20dBm
   else if ((puissance_dBm >= -30) && (puissance_dBm < -20)) 
  { value = (puissance_dBm + 30) * 10; gamme = att - 10;}

  else if (puissance_dBm <= -30) {
    value = 1;  // valeur pour aiguille
    gamme = att - 10;
  }  //gamme -30 à -20dBm {value=-2;}
  if (flag == 0) {
    gamme = 0;
    //Serial.print("value=");Serial.println(value);
    analogMeter();
    plotNeedle(value, 10);
    flag = 1;
  }  // mise sous tension
  if (gamme <= oldgamme) {
    moins = true;
  } else {
    moins = false;
  }
  if ((gamme != oldgamme) || (flag == 0)) {
    oldgamme = gamme;
    analogMeter();
    //plotNeedle(1, 10);
    flag = 1;
  }  // changement de gamme
  //Serial.print("gamme=");Serial.println(gamme);
  //moins = true;
  //old_analog=value;
  plotNeedle(value, 0);
  delay(10);

  
    TouchPoint p = ts.getTouch();  // utiliser la librairie XPT2046_BitBang_Slim (Et effacer la librairie XPT2046_BitBang )
    if (p.zRaw != 0 && p.y<160) {
    Serial.print (p.y);
    ecran = !ecran;
    tft.invertDisplay(ecran);  // inversion des couleurs
    EEPROM.write(1,ecran);  //mémorisation EEPROM de l'inversion de l'écran
    EEPROM.commit(); 
    }
  

  // Adjust press state of each key appropriately
  for (uint8_t b = 0; b < 3; b++) {
    if ((p.zRaw > 0) && key[b].contains(p.x, p.y)) {
      key[b].press(true);  // tell the button it is pressed
    } 
    else {
      key[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // Check if any key has changed state
  for (uint8_t b = 0; b < 3; b++) {
    // If button was just pressed, TFT_REDraw inverted button
    if (key[b].justPressed()) {
    // Serial.printf("Button %d pressed\n", b);
      key[b].drawButton(true, "");
      if (b==0 ) att = att + 10; if (att == 50) att = 0; // choix attenuation
      if (b==1) {diode = diode + 1; if (diode == 4) diode = 1; choix_diode();} // Choix diode
      if (b==2) {toggle = !toggle; } // VoltdBm
      delay(100);
    }

    // If button was just released, TFT_REDraw normal color button
    if (key[b].justReleased()) {
    //  Serial.printf("Button %d released\n", b); // pour le test
    //  Serial.println("Button " + (String)b + " released"); // pour le test
      key[b].drawButton(false, "");
      if(b==0)tft.drawCentreString("ATT="+String(att),54,217,2);
      if(b==2)tft.drawCentreString("Volt/dBm",257,217,2);
      if(diode==1)tft.drawCentreString(" HP33330B ",160, 217, 2);
      if(diode==2)tft.drawCentreString("  DIODE2  ", 160, 217, 2);
      if(diode==3)tft.drawCentreString("  DIODE3  ", 160, 217, 2);
    }
  }
  VoltdBm() ;
  plotNeedle(value, 0);
  delay(50);

}

 