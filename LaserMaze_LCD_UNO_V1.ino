#include <Wire.h>
#include <BH1750FVI.h>
#include <LiquidCrystal.h>

BH1750FVI LightSensor(BH1750FVI::k_DevModeContHighRes2);  // Initialiseer de sensor in continu-modus met hoge resolutie

const int laserPin = A1;  // Pin waarop de laser is aangesloten
const int buzzerPin = A2;  // Pin waarop de laser is aangesloten
const int relaisPin = A3;  // Pin waarop de laser is aangesloten
const int meetInterval = 200;  // Interval in milliseconden tussen metingen
#define MAX_ALARM_DREMPEL 100
uint16_t alarmDrempel = 5;
unsigned long vorigeMillis = 0;
unsigned long tijdslot = 30 * 1000UL;
unsigned long maxOnderbreekTijd = 5 * 1000UL;
unsigned long toegestaneOnderbreekTijd = 0;
unsigned long onderbreekStartTijd;
bool isOnderbreekTijdRunning = false;
#define MAX_ONDERBREEK_TIJD (10 * 1000)

// lichtmeting
uint16_t lux;

// chronometer
unsigned long chronoStartTijd = 0;
bool isChronoRunning = false;
int chronoMin, chronoSec, chronoTiendenSec, vorigeChronoTiendenSec;
int chronoRefreshInterval = 100;  // Interval in milliseconden
#define MAX_CHRONO_TIJD (100 * 60 * 1000UL)

// operating mode
#define MAX_NUMBER_OPERATING_MODES 6

enum OperatingModeEnum {
  BEAM_ZOEKER = 0,
  BEAM_ONDERBREKER,
  BEAM_PUZZEL,
  TIJDSLOT,
  ONDERBREEKTIJD,
  DREMPELWAARDE
};

OperatingModeEnum operatingMode;

void startChronometer()
{
  chronoStartTijd = millis();
  isChronoRunning = true;
  vorigeChronoTiendenSec = 9;
}

void stopChronometer()
{
  isChronoRunning = false;
}

void runChronometer()
{
  if (isChronoRunning)
  {
    unsigned long huidigeTijd = millis();
    unsigned long chronoTijd = huidigeTijd - chronoStartTijd;
    unsigned long tijd;

    switch (operatingMode )
    {
      case BEAM_ZOEKER:
        tijd = chronoTijd;

        // stop chrono bij 99m59s9
        if (tijd >= MAX_CHRONO_TIJD) 
        {
          stopChronometer();
          tijd = MAX_CHRONO_TIJD - 100;
        }
        break;
     case BEAM_ONDERBREKER:
     case BEAM_PUZZEL:
        tijd = tijdslot - chronoTijd;
        
        // check tijdslimiet bereikt
        if (tijdslot == chronoTijd)
        { // tijdslimiet is bereikt
          stopChronometer();
          alarmAan();
        }
        break;
    }

    // Controleer of tienden van seconde gewijzigd is
    chronoTiendenSec = (tijd % 1000) / 100;
    if (vorigeChronoTiendenSec != chronoTiendenSec) 
    {
      // Bewaar de tienden van seconden
      vorigeChronoTiendenSec = chronoTiendenSec;
 
      displayChronometer(0, 1, tijd);
    }
  }
}

// titels
const String titel[16][5] = 
{
  "Laserbeam zoeker",
  "Laserbeam alarm ",
  "Laserbeam puzzel",
  "Tijdslot:       ",
  "Onderbreektijd: ",
  "Alarm:          "
};




// alarm
enum AlarmModeEnum {
  UIT = 0,
  ZOEKER,
  ONDERBREKER
};

AlarmModeEnum alarmMode;

// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
// define some values used by the panel and buttons
int vorigeLcdKey, lcdKey = 0;
int adcKeyIn = 0;
unsigned char message_count = 0;
unsigned long prev_trigger = 0;
#define btnRIGHT 0
#define btnUP 1
#define btnDOWN 2
#define btnLEFT 3
#define btnSELECT 4
#define btnNONE 5

// read the buttons
int read_LCD_buttons()
{
 adcKeyIn = analogRead(0); // read the value from the sensor
 if (adcKeyIn < 50) return btnRIGHT;
 if (adcKeyIn < 195) return btnUP;
 if (adcKeyIn < 380) return btnDOWN;
 if (adcKeyIn < 555) return btnLEFT;
 if (adcKeyIn < 790) return btnSELECT;
 return btnNONE; // when all others fail, return this...
} 

void toetsActies()
{
  lcdKey = read_LCD_buttons(); // read the buttons
  
  if (vorigeLcdKey != lcdKey)
  {
    switch (lcdKey) // depending on which button was pushed, we perform an action
    {
      case btnSELECT:
        selectToetsActie();
        break;
      case btnRIGHT:
        rightToetsActie();
        break;
      case btnLEFT:
        leftToetsActie();
        break;
      case btnUP:
        upToetsActie();
        break;
      case btnDOWN:
        downToetsActie();
        break;
    }
  }
  vorigeLcdKey = lcdKey;
   
}


void selectToetsActie()
{
  if (alarmMode == UIT)
  {
    switch (operatingMode )
    {
      case BEAM_ZOEKER:
        StartBeamZoeker();
        break;
      case BEAM_ONDERBREKER:
        StartBeamOnderbreker();
        break;
      case BEAM_PUZZEL:
        StartBeamPuzzel();
        break;
      case TIJDSLOT:
      case ONDERBREEKTIJD:
      case DREMPELWAARDE:
        SelectOperatingMode(BEAM_ZOEKER);
        break;
    }
  }
  else
  {
    if (isChronoRunning)
    {
      stopChronometer();
    }
    else
    {
      alarmModeUit();
      displayTitel();
    }
  }
}

void leftToetsActie()
{
  if (operatingMode > 0)
  {
    operatingMode = operatingMode - 1;
    SelectOperatingMode(operatingMode);
  }
}

void rightToetsActie()
{
  if (operatingMode < (MAX_NUMBER_OPERATING_MODES - 1))
  {
    operatingMode = operatingMode + 1;
    SelectOperatingMode(operatingMode);
  }
}

void upToetsActie()
{
  switch (operatingMode)  
  {
    case TIJDSLOT:
      incTijdslot();
      break;
    case ONDERBREEKTIJD:
      incOnderbreekTijd();
      break;
   case DREMPELWAARDE:
      incAlarmDrempel();
      break;
  }
}

void downToetsActie()
{
  switch (operatingMode)  
  {
    case TIJDSLOT:
      decTijdslot();
      break;
    case ONDERBREEKTIJD:
      decOnderbreekTijd();
      break;
   case DREMPELWAARDE:
      decAlarmDrempel();
      break;
  }
}


void incTijdslot()
{
  if (tijdslot < MAX_CHRONO_TIJD)
  {
    tijdslot += 1000;
    displayChronometer(9, 0, tijdslot);
  }
}

void decTijdslot()
{
  if (tijdslot > 0)
  {
    tijdslot -= 1000;
    displayChronometer(9, 0, tijdslot);
  }
}

void incOnderbreekTijd()
{
  if (maxOnderbreekTijd < MAX_ONDERBREEK_TIJD)
  {
    maxOnderbreekTijd += 1000;
    displayChronometer(0, 1, maxOnderbreekTijd);
  }
}

void decOnderbreekTijd()
{
  if (maxOnderbreekTijd > 0)
  {
    maxOnderbreekTijd -= 1000;
    displayChronometer(0, 1, maxOnderbreekTijd);
  }
}

void incAlarmDrempel()
{
  if (alarmDrempel < MAX_ALARM_DREMPEL)
  {
    alarmDrempel++;
    displayLux(8, 0, alarmDrempel);
  }
}

void decAlarmDrempel()
{
  if (alarmDrempel > 1)
  {
    alarmDrempel--;
    displayLux(8, 0, alarmDrempel);
  }
}


void lichtmeting()
{
  // Huidige tijd vastleggen
  unsigned long huidigeMillis = millis();

  // Controleer of het tijd is om een nieuwe meting uit te voeren
  if (huidigeMillis - vorigeMillis >= meetInterval)
  {
    // Bewaar de huidige tijd
    vorigeMillis = huidigeMillis;

     lux = LightSensor.GetLightIntensity();
     displayLux(8,1,lux);

     //Serial.println(lux);  // Stuur de lichtintensiteitsgegevens naar de Serial Monitor
  }
}

void laserUit(){
  digitalWrite(laserPin, LOW);  // Zet de laser uit
}

void laserAan(){
  digitalWrite(laserPin, HIGH);  // Zet de laser aan
}

void alarmUit(){
  digitalWrite(buzzerPin, LOW);  // Zet de buzzer uit
  digitalWrite(relaisPin, LOW);  // Zet de relais uit
}

void alarmAan(){
  digitalWrite(buzzerPin, HIGH);  // Zet de buzzer aan
  digitalWrite(relaisPin, HIGH);  // Zet de relais aan
}

void SelectOperatingMode(int mode)
{
  operatingMode = mode;
  alarmModeUit();
  displayTitel();
}

void StartBeamZoeker()
{
  alarmMode = ZOEKER;
  laserAan();
  startChronometer();
}

void StartBeamOnderbreker()
{
  alarmMode = ONDERBREKER;
  toegestaneOnderbreekTijd = 0;
  isOnderbreekTijdRunning = false;
  laserAan();
  startChronometer();
}

void StartBeamPuzzel()
{
  alarmMode = ONDERBREKER;
  toegestaneOnderbreekTijd = maxOnderbreekTijd;
  isOnderbreekTijdRunning = false;
  laserAan();
  startChronometer();
}

void alarmModeUit()
{
  alarmMode = UIT;
  alarmUit();
  laserUit();
  stopChronometer();
}


void startOnderbreekTijd()
{
  onderbreekStartTijd = millis();
  isOnderbreekTijdRunning = true;
}


void alarmUpdate(){
  // keuze van de  alarm mode
  switch (alarmMode)
  {
    case UIT:
    {
      alarmUit();  // Zet de buzzer en relais uit
      break;
    }
    case ZOEKER:
    {
      if (lux >= alarmDrempel)
      {
        alarmAan();  // Zet de buzzer en relais aan
        stopChronometer();
      }
      else
      {
        alarmUit();  // Zet de buzzer en relais uit
      }
      break;
    }
    case ONDERBREKER:
    {
      if (isChronoRunning)
      {
        if (lux >= alarmDrempel)
        {
          isOnderbreekTijdRunning = false;
          alarmUit();  // Zet de buzzer en relais uit
        }
        else
        {
          if (!isOnderbreekTijdRunning)
          {
            startOnderbreekTijd();
          }

          // Huidige tijd vastleggen
          unsigned long huidigeMillis = millis();

          if ((huidigeMillis - onderbreekStartTijd) >= toegestaneOnderbreekTijd)
          {
            alarmAan();  // Zet de buzzer en relais aan
            stopChronometer();
          }
        }
      }
      break;
    }
  }
}


void displayTitel()
{
  lcd.setCursor(0,0);
  lcd.print(titel[0][operatingMode]); // print a titel
  switch (operatingMode)
  {
    case BEAM_ZOEKER:
    case BEAM_ONDERBREKER:
    case BEAM_PUZZEL:
      lcd.setCursor(0,1);
      lcd.print("Start  ");
      break;
    case TIJDSLOT:
      displayChronometer(9, 0, tijdslot);
      lcd.setCursor(0,1);
      lcd.print("               ");
      break;
    case ONDERBREEKTIJD:
      lcd.setCursor(0,1);
      lcd.print("               ");
      displayChronometer(0, 1, maxOnderbreekTijd);
      break;

    case DREMPELWAARDE:
      displayLux(8, 0, alarmDrempel);
      lcd.setCursor(0,1);
      lcd.print("Huidig:        ");
      break;
  }
}


void displayLux(int xpos, int ypos, uint16_t value)
{
  // converteer lux waarde naar string 
  String luxStr = String(value);

  // max aantal karakters
  int fieldWidth = 5;

  // voeg spaties toe om rechts uit te lijen
  while (luxStr.length() < fieldWidth)
  {
    luxStr = ' ' + luxStr;
  }

  // print de lichtwaarde rechts gealligneerd op de tweede regel
  luxStr += " lx";
  lcd.setCursor(xpos,ypos);
  lcd.print(luxStr);
}


void displayChronometer(int xpos, int ypos, unsigned long tijd)
{
  String chronoStr;

  uint16_t chronoMin = tijd / 60000;
  uint16_t chronoSec = (tijd % 60000) / 1000;
  uint16_t chronoTiendenSec = (tijd % 1000) / 100;

  // minuten
  if (chronoMin == 0)
  {
    chronoStr = "   ";
  }
  else if (chronoMin < 10)
  {
    chronoStr = " " + String(chronoMin) +"m";
  }
  else
  {
    chronoStr = String(chronoMin) +"m";
  }

  // seconden
  if (chronoSec < 10)
  {
    chronoStr = chronoStr + " " + String(chronoSec) +"s";
  }
  else
  {
    chronoStr = chronoStr + String(chronoSec) +"s";
  }

  // tienden van seconden
  chronoStr = chronoStr + String(chronoTiendenSec);
  
  // display chronometertijd
  lcd.setCursor(xpos,ypos);
  lcd.print(chronoStr);
}

void setup() {
  Serial.begin(9600);
  LightSensor.begin();

  lcd.begin(16, 2); // start the library
  lcd.clear();
 
  pinMode(laserPin, OUTPUT);    // Stel de laserpin in als uitgang
  laserUit();                   // Zet de laser constant uit
  pinMode(buzzerPin, OUTPUT);   // Stel de buzzerpin in als uitgang
  pinMode(relaisPin, OUTPUT);   // Stel de relaypin in als uitgang

  alarmMode = UIT;
  alarmUpdate();

  operatingMode = BEAM_ZOEKER;
  displayTitel();
}


void loop()
{
  // lees de lichtsensor elke 'meetInterval' milliseconden uit
  lichtmeting();

  // verifiëer alarm
  alarmUpdate();

  // run chronometer
  runChronometer();

  // toestenbord acties
  toetsActies();
}
