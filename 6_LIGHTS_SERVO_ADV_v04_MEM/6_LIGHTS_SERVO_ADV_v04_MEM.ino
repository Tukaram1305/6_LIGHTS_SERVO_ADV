#include "Kronos.h"
#include "JSONcreator.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "WiFi.h"
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <AsyncElegantOTA.h>
AsyncWebServer server(80);

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#include <Preferences.h>
Preferences preferences;

const char* ssid = "RagNet";
const char* password =  "azgard666";
// spobuje ustawic statyczne IP
IPAddress local_IP(192, 168, 1, 65);
IPAddress gateway(192, 168, 1 ,1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8); 
IPAddress secondaryDNS(8, 8, 4, 4);

#define SERVO_FREQ 90

int SER[14] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int SERP[14] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int SLED[6] = {0,0,0,0,0,0};
Kronos SDEL, SDELL, RGBFXDEL;
unsigned int DELUS = 7000;
unsigned int DELmSL = 2;

int INSTANT = 0;
int ON_RAINBOW = 0;
int SPEED_RAINBOW = 50; // ms
int RGB_H1 = 128 /*zmienne*/, RGB_H2 = 0, RGB_S = 255 /*stale*/, RGB_V = 255; /*zmienne*/

void hsv2rgb(int h, int s, int v, int*r, int*g, int*b) // same 255
{
    double  H, S, V, P, Q, T, RC, GC, BC, fract;
    H = 1.4117647058823529411764705882353*h; // mapuj 0-360 -> 0-255
    S = 0.0039215686274509803921568627451*s; //       0-1   -> 0-255
    V = 0.0039215686274509803921568627451*v; //       0-1   -> 0-255
    (H == 360.)?(H = 0.):(H /= 60.);
    fract = H - floor(H);
    P = V*(1. - S);
    Q = V*(1. - S*fract);
    T = V*(1. - S*(1. - fract));
    if (0. <= H && H < 1.) { RC = V; GC = T; BC = P; }
    else if (1. <= H && H < 2.) { RC = Q; GC = V; BC = P; }
    else if (2. <= H && H < 3.) { RC = P; GC = V; BC = T; }
    else if (3. <= H && H < 4.) { RC = P; GC = Q; BC = V; }
    else if (4. <= H && H < 5.) { RC = T; GC = P; BC = V; }
    else if (5. <= H && H < 6.) { RC = V; GC = P; BC = Q; }
    else { RC = 0; GC = 0; BC = 0; }
    (*r) = int(RC*255);
    (*g) = int(GC*255);
    (*b) = int(BC*255);
}
word rgb2hex( byte R, byte G, byte B)
{
  return ( ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3) );
}

void RGBrainbow()
{
  if (RGB_H1 > 255 || RGB_H1 < 0) RGB_H1 = 0;
  if (RGB_H2 > 255 || RGB_H2 < 0) RGB_H2 = 0;
  hsv2rgb(RGB_H1, RGB_S, RGB_V, &SLED[0], &SLED[1], &SLED[2]);
  hsv2rgb(RGB_H2, RGB_S, RGB_V, &SLED[3], &SLED[4], &SLED[5]);
  RGB_H1++; RGB_H2++;
}

void startWIFI()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Lacze z WiFi..");
    }
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {Serial.println("Nie udalo sie skonfigurowac WiFi STA");}

  String IP = WiFi.localIP().toString().c_str();
  int RSI = WiFi.RSSI();
  Serial.println("IP ESP: "+IP+" , sygnal: "+String(RSI));
}

// our servo # counter
uint8_t servonum = 0;

int SNUM = 0;
int SVAL = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Modular Ligting");
  pwm.begin();
  startWIFI();
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA

    ledcAttachPin(32, 1);
    ledcAttachPin(33, 2);
    ledcAttachPin(25, 3);
    ledcAttachPin(26, 4);
    ledcAttachPin(27, 5);
    ledcAttachPin(14, 6);
    for (int i=1; i<7; i++)
    {
       ledcSetup(i, 10000, 8);
       ledcWrite(i, 0);
    }
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String mess = "Modular lights OK!";
    request->send(200, "text/plain", mess.c_str());
  });

    server.on("/prefs", HTTP_GET, [](AsyncWebServerRequest *request){
      String SLOT = "";
      String NAME = "";

      // ZAPIS
      if (request->hasParam("slot") && request->hasParam("name") && request->hasParam("save") ) 
      {
        String inputMessage = request->getParam("slot")->value();
        SLOT = "slot"+inputMessage;
        inputMessage = request->getParam("name")->value();
        NAME = inputMessage;
        
        preferences.begin(SLOT.c_str(), false);
        // ZAPIS NAZWY DLA PRESETU
        preferences.putString("Name", NAME.c_str());
        // ZAPIS 8 SERV i 6 SPOT LED
        for (int i=0; i<14; i++)
        {
          String FIELD = "S"+String(i);
          preferences.putInt(FIELD.c_str(), SER[i]);
        }
        // ZAPIS RGB 2x LED (6 kanalow)
        for (int i=0; i<6; i++)
        {
          String FIELD = "L"+String(i);
          preferences.putInt(FIELD.c_str(), SLED[i]);
        }
        // ZAPIS RANBOW FX
        preferences.putInt("FX1", ON_RAINBOW);
        preferences.putInt("FX2", SPEED_RAINBOW);
        preferences.putInt("FX3", RGB_H1);
        preferences.putInt("FX4", RGB_V);


        preferences.end();
        String re = "Zapis prefs na: "+SLOT+", o nazwie: "+NAME;
        request->send(200, "text/plain", re.c_str());
      } // KONIEC ZAPISU

      // LOAD
      if (request->hasParam("slot") && request->hasParam("load") )
      {
        String inputMessage = request->getParam("slot")->value();
        SLOT = "slot"+inputMessage;
        
        preferences.begin(SLOT.c_str(), false);
        for (int i=0; i<14; i++)
        {
          String FIELD = "S"+String(i);
          SER[i] = preferences.getInt(FIELD.c_str(), 0);
        }
        for (int i=0; i<6; i++)
        {
          String FIELD = "L"+String(i);
          SLED[i] = preferences.getInt(FIELD.c_str(), 0);
        }
        ON_RAINBOW = preferences.getInt("FX1", 0);
        SPEED_RAINBOW = preferences.getInt("FX2", 50);
        RGB_H1 = preferences.getInt("FX3", 0);
        RGB_V = preferences.getInt("FX4", 128);
        
        preferences.end();
        String re = "Load prefs z: "+SLOT;
        request->send(200, "text/plain", re.c_str());
      }

      // DAJ NAZWY
      if (request->hasParam("give") ) 
        {
          String nArr[8];
          for (int i=0; i<8; i++)
          {
            String SLOT = "slot"+String(i);
            preferences.begin(SLOT.c_str(), false);
            nArr[i] = preferences.getString("Name", "BRAK");
            preferences.end();
          }
          /*
            String form = 
              "{\"0\":\""+nArr[0]+
              "\",\"1\":\""+nArr[1]+
              "\",\"2\":\""+nArr[2]+
              "\",\"3\":\""+nArr[3]+
              "\",\"4\":\""+nArr[4]+"\"}\n";
          */
            // Nowe podejscie z moja klasa
            JSONcreator js1;
            js1.clearJson();
            // duze serva
            js1.strVal("0",nArr[0]);
            js1.strVal("1",nArr[1]);
            js1.strVal("2",nArr[2]);
            js1.strVal("3",nArr[3]);
            js1.strVal("4",nArr[4]);
            js1.strVal("5",nArr[5]);
            js1.strVal("6",nArr[6]);
            js1.strVal("7",nArr[7]);
            js1.endJson();
            
            request->send(200, "application/json",js1.giveCstr());
        }
       
    request->send(200, "text/plain", String("A nic u prefs!").c_str());
  });
  
  server.on("/set", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam("n") && request->hasParam("v")) 
    {
      String inputMessage = request->getParam("n")->value();
      int val = inputMessage.toInt();
      SNUM = val;
      inputMessage = request->getParam("v")->value();
      val = inputMessage.toInt();
      SVAL = val;
      SER[SNUM] = SVAL;
      if (INSTANT==1) { pwm.setPWM(SNUM, 0, SVAL); }
      String txt = "OK SER "+String(SNUM)+" na "+String(SVAL);
      request->send(200, "text/plain", txt.c_str());
    }
    
    else request->send(200, "text/plain", String("A NIC!").c_str());
    });    

  server.on("/setAll", HTTP_GET, [] (AsyncWebServerRequest *request) { // Ustaw wszystkie 14 PWM na raz (moze byc mniej)
    int ALLNUMS = 0;
    int VAL = 0;
    String serNum = "";
    for (int i=0; i<14; i++)
    {
      serNum = "n"+String(i);
      if (request->hasParam(serNum)) 
      {
        String inputMessage = request->getParam(serNum)->value();
        VAL = inputMessage.toInt();
        SER[i] = VAL;
        ALLNUMS++;
        if (INSTANT==1) { pwm.setPWM(i, 0, VAL); }
      } 
    }

    String txt = "All SER OK, setting: "+String(ALLNUMS)+" PWMs";
    request->send(200, "text/plain", txt.c_str());
    }); 
    
  server.on("/setus", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam("us")) 
    {
      String inputMessage = request->getParam("us")->value();
      int val1 = inputMessage.toInt();
      DELUS = static_cast<unsigned int>(val1);
    }

    if (request->hasParam("usl")) 
    {
      String inputMessage = request->getParam("usl")->value();
      int val2 = inputMessage.toInt();
      DELmSL = static_cast<unsigned int>(val2);
    }
    
    String form = "{\"s\":"+String(DELUS)+
      ",\"l\":"+String(DELmSL)+"}\n";  
    request->send(200, "application/json",form.c_str());
    });  

  server.on("/hardReset", HTTP_GET, [] (AsyncWebServerRequest *request) {
    ESP.restart();
    request->send(200, "application/json",String("RESET").c_str());
    }); 

  server.on("/instant", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam("val")) 
    {
      String inputMessage = request->getParam("val")->value();
      int val = inputMessage.toInt();
      INSTANT = val;
    }
    request->send(200, "application/json",String("Zmiana zachowania SERV").c_str());
    });  

  server.on("/rainbow", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam("onoff")) 
    {
      String inputMessage = request->getParam("onoff")->value();
      int val = inputMessage.toInt();
      ON_RAINBOW = val;
        if (ON_RAINBOW == 0) { for (int i=0; i<6; i++) {SLED[i] = 0;} }
      
      if (request->hasParam("offset") && request->hasParam("speed") && request->hasParam("light")) 
      {
        inputMessage = request->getParam("offset")->value();
        val = inputMessage.toInt();
        RGB_H1 = (val < 0 || val > 255) ? 0 : val;
        RGB_H2 = 0;
        
        inputMessage = request->getParam("speed")->value();
        val = inputMessage.toInt();
        SPEED_RAINBOW = (val < 0) ? 0 : val;
        
        inputMessage = request->getParam("light")->value();
        val = inputMessage.toInt();
        RGB_V = (val < 0 || val > 255) ? 0 : val;;
        
        request->send(200, "application/json",(String("Ustawiam RAINBOW, offset: ")+String(RGB_H1)).c_str());
      }
    request->send(200, "application/json","Przelaczam RAINBOW"); 
    }
    request->send(200, "application/json","RGB FX nic nie robi");
    }); 
    
  server.on("/givesers", HTTP_GET, [] (AsyncWebServerRequest *request) {
    /*
    String form = 
    "{\"0\":"+String(SER[0])+
    ",\"1\":"+String(SER[1])+
    ",\"2\":"+String(SER[2])+
    ",\"3\":"+String(SER[3])+
    ",\"4\":"+String(SER[4])+
    ",\"5\":"+String(SER[5])+
    ",\"6\":"+String(SER[6])+
    ",\"7\":"+String(SER[7])+
    ",\"8\":"+String(SER[8])+
    ",\"9\":"+String(SER[9])+
    ",\"10\":"+String(SER[10])+
    ",\"11\":"+String(SER[11])+
    ",\"12\":"+String(SER[12])+
    ",\"13\":"+String(SER[13])+
    ",\"14\":"+String(SLED[0])+ // LEDY 2x 6ch RGB
    ",\"15\":"+String(SLED[1])+
    ",\"16\":"+String(SLED[2])+
    ",\"17\":"+String(SLED[3])+
    ",\"18\":"+String(SLED[4])+
    ",\"19\":"+String(SLED[5])+"}\n";
    */
    // Nowe podejscie z moja klasa
    JSONcreator js1;
    js1.clearJson();
    // duze serva
    js1("0",SER[0]);  js1("1",SER[1]);
    // male serva
    js1("2",SER[2]);  js1("3",SER[3]);  js1("4",SER[4]);   js1("5",SER[5]);   js1("6",SER[6]);   js1("7",SER[7]);
    // spot LED
    js1("8",SER[8]);  js1("9",SER[9]);  js1("10",SER[10]); js1("11",SER[11]); js1("12",SER[12]); js1("13",SER[13]);
    // RGB Led
    js1("14",SLED[0]); js1("15",SLED[1]); js1("16",SLED[2]);  js1("17",SLED[3]);  js1("18",SLED[4]);  js1("19",SLED[5]);
    js1.endJson();
        
    request->send(200, "application/json",js1.giveCstr());
    
    }); 
    
  server.on("/setStrip", HTTP_GET, [] (AsyncWebServerRequest *request) {
    int num=0, r=0, g=0, b=0;
    if (request->hasParam("n") && request->hasParam("r") && request->hasParam("g") && request->hasParam("b")) 
    {
      ON_RAINBOW = 0; // WYLACZ RGBFX
      String inputMessage = request->getParam("r")->value();
      r = inputMessage.toInt();
      
      inputMessage = request->getParam("g")->value();
      g = inputMessage.toInt();

      inputMessage = request->getParam("b")->value();
      b = inputMessage.toInt();

      inputMessage = request->getParam("n")->value();
      num = inputMessage.toInt();

      if (num == 0) { SLED[0] = r; SLED[1] = g; SLED[2] = b; }
      else if (num == 1) { SLED[3] = r; SLED[4] = g; SLED[5] = b; }
      
      request->send(200, "text/plain", String("OK LEDY").c_str());
    }
    
    else request->send(200, "text/plain", String("A NIC!").c_str());
    });  
    
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    server.begin();
  
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  delay(10);
}

String rec = "";
int cSer = 0;
void loop() {

// RGB FX (RAINBOW)
if (ON_RAINBOW == 1 && RGBFXDEL.del(SPEED_RAINBOW))
{
  RGBrainbow();
}

// REGULACJA RGB LED x6
  // pierwsza listwa - dolna
  ledcWrite(1, SLED[2]);  // L1 R
  ledcWrite(2, SLED[0]);  // L1 G
  ledcWrite(3, SLED[1]);  // L1 B
  // druga listwa - gorna
  ledcWrite(4, SLED[4]);  // L2 R
  ledcWrite(5, SLED[3]);  // L2 G
  ledcWrite(6, SLED[5]);  // L2 B
  
// REGULACJA SERV 0-7
if (INSTANT==0){
if (SDEL.delMicro(DELUS))
{
  for (int i=0; i<8; i++)
  {
    if (SERP[i] != SER[i])
    {
      if (SER[i] > SERP[i])
      {
        SERP[i]++;
        pwm.setPWM(i, 0, SERP[i]);
      }
      else if (SER[i] < SERP[i])
      {
        SERP[i]--;
        pwm.setPWM(i, 0, SERP[i]);
      }
    }
  }
}
}

// REGULACJA SPOT LED 8-13
if (SDELL.del(DELmSL))
{
  for (int i=8; i<14; i++)
  {
    if (SERP[i] != SER[i])
    {
      if (SER[i] > SERP[i])
      {
        SERP[i]+=10;
        if (SERP[i] > SER[i] || SERP[i]>4095) { SERP[i]=SER[i]; }
        pwm.setPWM(i, 0, SERP[i]);
      }
      else if (SER[i] < SERP[i])
      {
        SERP[i]-=10;
        if (SERP[i] < SER[i] || SERP[i]<0) { SERP[i]=SER[i]; }
        pwm.setPWM(i, 0, SERP[i]);
      }
    }
  }
}
// MAKRA
    while (Serial.available() > 0){
    delay(2);
    char c = Serial.read();
    rec += c;}
    
    if (rec.length() > 0) 
    {
     if (rec[0]=='N')
        {
          String buf;
          for (int i=1; i<rec.length(); i++)
          {
            buf+=rec[i];
          }
            cSer = buf.toInt();
            Serial.println("Ustawiles nbr serva na: "+String(cSer));
        }

    if (rec[0]=='V')
        {
          String buf;
          for (int i=1; i<rec.length(); i++)
          {
            buf+=rec[i];
          }
            int sVal = buf.toInt();
            Serial.println("Ustawiles wartosc serva na: "+String(sVal));
            pwm.setPWM(cSer, 0, sVal);
        }
    rec="";
    }
  //--------MAKRA DUZE SERVO 110-590 PWM / MALE 80-600
  if (WiFi.status() != WL_CONNECTED) startWIFI();
}
