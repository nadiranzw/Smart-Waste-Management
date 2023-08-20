#include <Keypad.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --------komponen pendukung--------

#define motor_organik 16
#define motor_plastik 17
#define pompa 18

int stateOrganik;
int statePlastik;
int statePompa;
const int oneWireBus = 4;
LiquidCrystal_I2C lcd(0x27,20,4);  // Pemilihan Pin dan pendeklarasian variabel
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);


// ------------WiFi------------
const char* ssid = "(put ur id wifi here)";
const char* password = "(put ur pass wifi here)";
const char* mqtt_server = "ee.unsoed.ac.id";

String ip;
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];


// --------Pengaturan Keypad--------

const byte rows = 4;
const byte cols = 4;
char keys[rows][cols] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[rows] = {23,32,33,25};
byte colPins[cols] = {26,27,14,13};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);
char key;
String mode;


// --------Pengaturan Durasi--------

// setting waktu
String inMM;
String inSS;
int setMM = 0;
int setSS = 0;
int setmS = 0;
int curMM = 0;
int curSS = 0;
int i; // dataTimer perulangan

unsigned long inputTimer; // mqtt
unsigned long dataTimer;
unsigned long prevmS = 0;
unsigned long currentSS, currentmS;
const long interval = 1;

// mode button
int dataSelection = 0;


// --------mode input nilai--------

// display
void SetupDisplay(){
  lcd.setCursor(0,0);
  if (mode == "organik"){
    lcd.print(mode);
  }
  else if (mode == "plastik"){
    lcd.print(mode);
  }
  lcd.setCursor(0,1);
  lcd.print("Durasi: ");
  lcd.setCursor(0,2);
  lcd.print("(MM) ");
  lcd.setCursor(10,2);
  lcd.print("(SS) ");
}

// delete (*)
void reset(){
  inMM = "";
  inSS = "";
  setMM = 0;
  setSS = 0;
  dataSelection = 0;
  lcd.clear();
  SetupDisplay();
}

// display saat running
void runningDisplay(){
  lcd.clear();
  lcd.setCursor(0,0);
  if (mode == "organik"){
    lcd.print(mode);
  }
  else if (mode == "plastik"){
    lcd.print(mode);
  }
  lcd.setCursor(0,1);
  lcd.print("Running Time: ");
  // lcd.setCursor(0,2);
  // lcd.print(i);
}

void modeOrganik(){
  // suhu pupuk
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  Serial.print(temperatureC);
  Serial.println("ºC");
  lcd.setCursor(0,3);
  lcd.print("suhu pupuk: ");
  lcd.setCursor(12,3);
  lcd.print(temperatureC);
  snprintf(msg, MSG_BUFFER_SIZE, String(temperatureC).c_str());
  client.publish("capstone/R/mesinpencacah/suhu", msg);
}

//------------------Network------------------

void connectWifi(){
  delay(100);
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  
  Serial.println("");
}


//------------------MQTT Configuration------------------

// Dipanggil ketika mengirim/menerima data mqtt
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println();
  
  if ((char)payload[0] == '1') {
    Serial.println("message arrived");
    //snprintf (msg, MSG_BUFFER_SIZE, "%d",KondisiLED);
  } 
  else {
    Serial.println("no message arrived");
    //snprintf (msg, MSG_BUFFER_SIZE, "%d",KondisiLED);
  }
}

// Memastikan tetap terhubung dengan server mqtt
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup(){
  Serial.begin(115200);
  pinMode(motor_organik, OUTPUT);
  pinMode(motor_plastik, OUTPUT);
  pinMode(pompa, OUTPUT);

  connectWifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  digitalWrite(motor_organik, LOW);
  digitalWrite(motor_plastik, LOW);
  digitalWrite(pompa, LOW);
  stateOrganik = 0;
  statePlastik = 0;
  statePompa = 0;

  lcd.begin();
  lcd.backlight();
  sensors.begin();
}


void loop(){
  if (!client.connected()){
    reconnect();
  }
  client.loop();
  unsigned long now = millis();
  if (now - lastMsg > 2000){
    lastMsg = now;

    ip = String(WiFi.localIP().toString());
    snprintf(msg, MSG_BUFFER_SIZE, ip.c_str());
    client.publish("capstone/R/mesinpencacah/IPAddress", msg);
    
    snprintf(msg, MSG_BUFFER_SIZE, String(mode).c_str());
    client.publish("capstone/R/mesinpencacah/mode", msg);

    snprintf(msg, MSG_BUFFER_SIZE, "%d", stateOrganik);
    client.publish("capstone/R/mesinpencacah/organik", msg);
    snprintf(msg, MSG_BUFFER_SIZE, "%d", statePlastik);
    client.publish("capstone/R/mesinpencacah/plastik", msg);
    snprintf(msg, MSG_BUFFER_SIZE, "%d", statePompa);
    client.publish("capstone/R/mesinpencacah/pompa", msg); 
    
  }

  char key = keypad.getKey();
  if (key){
    if (key == 'A'){
      mode = "organik";
      Serial.println(mode);
    }
    if (key == 'B'){
      mode = "plastik";
      Serial.println(mode);
    }
    SetupDisplay();
    // ga bisa kalo dimasukin ke fungsi
    if (key >= '0' && key <= '9'){
      Serial.println(key);
      switch (dataSelection){
        case 0:
          inMM += key;
          setMM = inMM.toInt();
          lcd.setCursor(5,2);
          lcd.print(inMM);
          break;
        case 1:
          inSS += key;
          setSS = inSS.toInt();
          lcd.setCursor(15,2);
          lcd.print(inSS);
          break;
      }
    }
    if (key == '*'){
      reset();
    }

    if (key == '#'){
      switch (dataSelection){
        case 0:
          curMM = setMM;
          inMM = "";
          setMM = 0;
          Serial.print("Data menit yang tersimpan: ");
          Serial.println(curMM);
          lcd.setCursor(5,2);
          lcd.print(curMM);
          dataSelection++;
          break;
        case 1:
          curSS = setSS;
          inSS = "";
          setSS = 0;
          Serial.print("Data detik yang tersimpan: ");
          Serial.println(curSS);
          lcd.setCursor(15,2);
          lcd.print(curSS);

          dataTimer = (curMM*60) + curSS;
          client.subscribe("capstone/R/mesinpencacah/timerIn");
          runningDisplay();
          
          for (i=dataTimer; i>=0; i--){
            // print timer pada mqtt
            snprintf(msg, MSG_BUFFER_SIZE, String(i).c_str());
            client.publish("capstone/R/mesinpencacah/timerOut", msg);

            if (mode == "organik"){
              digitalWrite(motor_organik, HIGH);
              stateOrganik = 1;
              snprintf(msg, MSG_BUFFER_SIZE, "%d", stateOrganik);
              client.publish("capstone/R/mesinpencacah/organik", msg);
              if (i <= 60){
                statePompa = 1;
                snprintf(msg, MSG_BUFFER_SIZE, "%d", statePompa);
                client.publish("capstone/R/mesinpencacah/pompa", msg);
                digitalWrite(pompa, HIGH);
                // Serial.println("pompa on");
                modeOrganik();
              }
            }
            else if (mode == "plastik"){
              digitalWrite(motor_plastik, HIGH);
              statePlastik = 1;
              snprintf(msg, MSG_BUFFER_SIZE, "%d", statePlastik);
              client.publish("capstone/R/mesinpencacah/plastik", msg);
            }
              
            Serial.println(i);
            if (i == 0){
              stateOrganik = 0;
              statePlastik = 0;
              statePompa = 0;
              digitalWrite(pompa, LOW);
              digitalWrite(motor_organik, LOW);
              digitalWrite(motor_plastik, LOW);
              mode = "";
              // Serial.println("pompa off");
            }
            lcd.setCursor(0,2);
            lcd.print(i);
            lcd.print("   ");
            delay(1000);
          }

          dataSelection++;
          if (dataSelection == 2){
            dataSelection = 0;
          }
          break;
      }          
    }
  }
}
