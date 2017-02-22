// try pid controller
#include <Ethernet2.h>

//limits for the KP,TV,TN values
#define H_KP_MIN 0
#define H_KP_MAX 30
#define H_TN_MIN 0
#define H_TN_MAX 5000
#define H_TV_MIN 0
#define H_TV_MAX 100

#define C_KP_MIN 0
#define C_KP_MAX 2
#define C_TN_MIN 0
#define C_TN_MAX 50000
#define C_TV_MIN 0
#define C_TV_MAX 100 

//Declarations for the networking part
#define BUFFER_SIZE 60                              //Buffer the data for readout until it is delivered to the client,
String Buffer[BUFFER_SIZE];                         //if it is not read out until the Buffer is full it will be cleared.
int BufferCount;
//byte mac[] = { 0x2C, 0xF7, 0xF1, 0x08, 0x01, 0xD1}; 
byte mac[] = { 0x90, 0xA2, 0xDA, 0x10, 0x92, 0x50};
unsigned int localPort = 5000; 
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
String datReq; 
int packetSize; 
EthernetUDP Udp; 
//.

int eingang = A0; int heizer = DAC0; int cooler = DAC1;


const int REGELZEIT = 60;              // alle n sekunden regeln
const float MESSZEIT = 0.25;           // Abstand, in dem Wert gemessen werden soll [s]
/*
const int SOLL = 450;                  // set temperature sollwert
*/
const int SOLLCOOL = 39;               // set sollwert for cooler in accordance to heating power

const float A = 1.20;                // assume aperiodic pid controller
const float B = 2.00;
const float C = 0.42;
const float TOT = 3.0;               // Totzeit Ttot, system reacts to heizleistung nach Ttot
const float TG = 1.0;                // Ausgleichszeit Tg, endwert erreicht nach Ttot+Tg
const float KS = 1;                  // amplification factor

int SOLL;

// Regelparameter Heizer, assumed TOT=10min, TG=40min

/*
const int KP = 10;                      // proportional amplification factor,
const unsigned int TN = 1440;           //set Nachstellzeit [s]
const int TV = 10;                      // set Vorstellzeit [s]
const int T = REGELZEIT;                // Integrationszeit = Regelzeit??? [s]

const float KOEFF1 = KP * (1 + (float)TV / (float)T);   // caculation of regulation coefficients
const float KOEFF2 = KP * (1 - (float)T / (float)TN + 2 * (float)TV / (float)T);
const float KOEFF3 = KP * (float)TV / (float)T;
*/
int KP;
unsigned int TN;
int TV;
const int T = REGELZEIT;

float KOEFF1;
float KOEFF2;
float KOEFF3;

int ist; int x1minus; int x2minus; float y; float yMinus;        // declaration of variables heater regulation

//Regelparameter Cooler
/*
const float KP_C = 0.2;               // proportional amplification factor,
const unsigned int TN_C = 30000;             //set Nachstellzeit [s]
const int TV_C = 10;                  // set Vorstellzeit [s]
const unsigned int T_C = 6000;                     // Integrationszeit [s]

const float COOL1 = KP_C * (1 + (float)TV_C / (float)T_C);                    // caculation of regulation coefficients
const float COOL2 = KP_C * (1 - (float)T_C / (float)TN_C + 2 * (float)TV_C / (float)T_C);
const float COOL3 = KP_C * (float)TV_C / (float)T_C;
*/
float KP_C;
unsigned int TN_C;
int TV_C;
const unsigned int T_C = 6000;

float COOL1;
float COOL2;
float COOL3;


int xCoolMinus1; int xCoolMinus2; float yCool; float yCoolMinus; // declaration of variables cooler regulation

int zaehler; unsigned long regelungsZaehler; unsigned long tag; long summe;    // declaration counter for arithm mean; counter of regulation loops

void setup() {
  pinMode(heizer, OUTPUT);      // set pin heizer as output
  Serial.begin(9600);         // initialize serial communication
  while (!Serial) ;
  
  //start networking connection and initialize buffer
  Ethernet.begin(mac); 
  Serial.println(Ethernet.localIP());
  Udp.begin(localPort);
  delay(1500);
  for(int i = 0; i < BUFFER_SIZE; i++)
  {  Buffer[i] = "";}
  BufferCount = 0;
  //.
  
  Serial.print("\n");
  Serial.println("Regeln\tTemp\tx\tMittel\theater\tcooler\ttag");      // test output header

  // initializing variables 

  //initialize the heater and cooler PID values to the ones found by Vanessa Scheller

  SOLL = 450;
  
  KP = 10;
  TN = 1440;
  TV = 10;
  initKOEFF();

  KP_C = 0.2;
  TN_C = 30000;
  TV_C = 10;
  initCOOL();
  
  ist = 0;                      // ist reads first value from eingang starting into the loop
  x1minus = x2minus = 0;         // no preceding measurements, so assume 0 deviation from Soll value
  yMinus = 0;                   // no preceding regulation
  xCoolMinus1 = xCoolMinus2 = 0;
  yCoolMinus = 0;
  zaehler = 0;
  regelungsZaehler = 0;
  tag = 0;
  summe = 0;
}

void loop() {
  static float xMittel;           // arithmetic mean over measured differences to Soll value
  static int stellwert;           // stellwert sent to heater for regulation
  static float yCool;             // stellwert sent to cooler dependent on stellwert for heat regulation
  float messzeit = (float)1000 * MESSZEIT;      // [s]->  [ms]

  ist = analogRead(eingang);
  int x = SOLL - ist;                                        // evaluate difference to soll value (regeldifferenz)
  summe += (long)x;                  // sum over all measured x
  delay(messzeit);                    // wait measuring intervall [ms]
  zaehler++;                             // increase counter by 1
  int messdauer = (int)messzeit * zaehler;    // declaration of messdauer for value independent check of if condition [ms]

  if ( 0 == messdauer % (1000 * REGELZEIT))   // only run regulation loop after messdauer [ms]
  {
    xMittel = (float)summe / (float)zaehler;        // calculation of artithm mean inside if cond.


    float y = yMinus + KOEFF1 * xMittel - KOEFF2 * (float)x1minus + KOEFF3 * (float)x2minus; //id: left intervall, i: rectangular assumption
    if (y < 0)         {
      y = 0; // Beschränkung des regelwerts auf 0-1023
    }
    else if (y > 1023) {
      y = 1023;
    }
    stellwert = y * 255 / 1023;                           // Umrechnung des regelwertes und auf int setzen
    analogWrite(heizer, stellwert);                                  // send stellwert to heater for regulation broken down to 8bit
    // retrieve values for heizer regulation loop
    x2minus = x1minus;                                         // set last value as 2nd last
    x1minus = (int)xMittel;                                               // set used currendt value as last value
    yMinus = y;                                                // set this loops stellwert as last loops stellwert

    /*  pid controller for cooler dependent on stellwert to heizer
       go for greater time frame, so no positive feedback, massive oscillations
       controller, for faster reaction time, to controll minute scale influences
       dependent on heizer stellwert, so that coolers main operating range in heizers lower operation range of about 15pct
       according to that, set sollwert to 1.5V power sent to heizer, being 39bit at 255bit resolution
       in contrast to heizer controller this time w/o overpowering, meaning aperiodic approach
       calculations in 8 bit resolution: min 0, max 255
    */
    int xCool = SOLLCOOL - stellwert;                                        // evaluate difference to soll value of heizleistung
    yCool = yCoolMinus + COOL1 * xCool - COOL2 * xCoolMinus1 + COOL3 * xCoolMinus2; // id: left intervall, i: rectangular assumption
    if (yCool < 0)       {
      yCool = 0; // Limiting Stellwert Cooler to range 0-255
    }
    else if (yCool > 255) {
      yCool = 255;
    }
    analogWrite(cooler, (int)yCool);                                    //send stellwert to cooler
    // retrieve values for cooler regulation loop
    xCoolMinus2 = xCoolMinus1;                                         // set last value as 2nd last
    xCoolMinus1 = xCool;                                               // set used current value as last value
    yCoolMinus = yCool;                                                // set this loops stellwert as last loops stellwert



    regelungsZaehler++;
    zaehler = 0;                                                       // Zähler hochsetzen, zum unterbrechen der schleife
    summe = 0;
    if (1440 == regelungsZaehler) {
      tag += 1;
      regelungsZaehler = 0;
    }
    
    String dat = String(regelungsZaehler) + "\t" + String(ist) + "\t" + String(x) + "\t" + String(xMittel) + "\t" + String(stellwert) + "\t" + String(yCool) + "\t" + String(tag);
    Serial.print(dat);
    Serial.print("\n");

    //store the data in the buffer, receive and handle incoming messages from the client
    toBuffer(dat);
    packetSize = Udp.parsePacket();
    if(packetSize > 0)
    {
      Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
      String datReq(packetBuffer);
      if (!handleMessage(datReq))
      {
        Serial.println("Unrecognized command sent by client");
      }
    }
    memset(packetBuffer, 0, UDP_TX_PACKET_MAX_SIZE);
    
  }
}

// caculation of regulation coefficients
void initKOEFF()
{
  KOEFF1 = KP * (1 + (float)TV / (float)T);   
  KOEFF2 = KP * (1 - (float)T / (float)TN + 2 * (float)TV / (float)T);
  KOEFF3 = KP * (float)TV / (float)T;
}

void initCOOL()
{
  COOL1 = KP_C * (1 + (float)TV_C / (float)T_C);
  COOL2 = KP_C * (1 - (float)T_C / (float)TN_C + 2 * (float)TV_C / (float)T_C);
  COOL3 = KP_C * (float)TV_C / (float)T_C;
}

// from here on functions for networking

//write a new string to the buffer
int toBuffer(String data)
{
  if(BufferCount >= BUFFER_SIZE)
  {
    BufferCount = 0;
    Buffer[BufferCount++] = data;
    return 0;
  }
  Buffer[BufferCount++] = data;
  return 1;
}

//send message to client
void sendUdp(String message)
{
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());  //Initialize Packet send
  Udp.print(message);//Send string back to client
  Udp.endPacket(); //Packet has been sent
}

//send buffer to client
void sendBufferUdp()
{
  String message = Buffer[0];
  for(int i = 1; i < BufferCount; i++)
  {
    message += ("\n" + Buffer[i]);
  }
  sendUdp(message);
  for(int i = 0; i <= BufferCount; i++)
  {  Buffer[i] = "";}
  BufferCount = 0;
}

//handle incoming messages from client
int handleMessage(String message)
{
  if(message == "g")
  {
    sendBufferUdp();
    return 1;
  }else if(message == "gpid")
  {
    sendUdp("KP_H = " + String(KP) + ", " + "TN_H = " + String(TN) + ", " + "TV_H = " + String(TV) + "\n" + "KP_C = " + String(KP_C) + ", " + "TN_C = " + String(TN_C) + ", " + "TV_C = " + String(TV_C) + "\n" + "sollwert = " + String(SOLL));
  }
  else
  {
    String str1 = message.substring(0,1);
    String str2 = message.substring(1,2);
    String str3 = message.substring(2,message.length());

    int val = str3.toInt();
    if(str1 == "h")                                           //set the heater values
    {
      if(str2 == "p")                                      
      {
        if((val > H_KP_MIN)&&(val < H_KP_MAX))
        {
          KP = val;
          initKOEFF();
          sendUdp("KP for Heater set to: " + String(KP));
          return 1;  
        }else {sendUdp("Value not in bounds"); return 0;}
      }else 
      if(str2 == "i")
      {
        if((val > H_TN_MIN)&&(val < H_TN_MAX))
        {
          TN = val;
          initKOEFF();
          sendUdp("KP for Heater set to: " + String(TN));
          return 1;  
        }else {sendUdp("Value not in bounds"); return 0;}
      }else
      if(str2 == "d")
      {
        if((val > H_TV_MIN)&&(val < H_TV_MAX))
        {
          TV = val;
          initKOEFF();
          sendUdp("KP for Heater set to: " + String(TV));
          return 1;  
        }else {sendUdp("Value not in bounds"); return 0;}
      }else {sendUdp("Unrecognized command"); return 0;}
      
    }else if(str1 == "c")                                     //set the cooler values
    {
      if(str2 == "p")                                      
      {
        if((val > C_KP_MIN)&&(val < C_KP_MAX))
        {
          KP_C = val;
          initKOEFF();
          sendUdp("KP for Heater set to: " + String(KP_C));
          return 1;  
        }else {sendUdp("Value not in bounds"); return 0;}
      }else 
      if(str2 == "i")
      {
        if((val > C_TN_MIN)&&(val < C_TN_MAX))
        {
          TN_C = val;
          initKOEFF();
          sendUdp("KP for Heater set to: " + String(TN_C));
          return 1;  
        }else {sendUdp("Value not in bounds"); return 0;}
      }else
      if(str2 == "d")
      {
        if((val > C_TV_MIN)&&(val < C_TV_MAX))
        {
          TV_C = val;
          initKOEFF();
          sendUdp("KP for Heater set to: " + String(TV_C));
          return 1;  
        }else {sendUdp("Value not in bounds"); return 0;}
      }else{sendUdp("Unrecognized command"); return 0;}
    }else if(str1 == "s" && str2 == "o")                  //set sollwert
    {
      SOLL = val;
      sendUdp("Sollwert set to: " + String(SOLL));
      return 1;
    }
    else{sendUdp("Unrecognized command"); return 0;}    
  }
}
