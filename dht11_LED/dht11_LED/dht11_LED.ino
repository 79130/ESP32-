#include <WiFi.h>
#include <SimpleDHT.h>

#define TCP_SERVER_ADDR "bemfa.com"
#define TCP_SERVER_PORT "8344"

#define DEFAULT_STASSID  "roronoa"
#define DEFAULT_STAPSW "sora123456"

String UID = "c480536e8d9d4bb3bb95e28492ab6570";
String TOPIC = "temp";
String TOPIC2 = "light002";

const int LED_Pin = LED_BUILTIN;
#define pinDHT11 4 

#define upDataTime 2*1000

SimpleDHT11 dht11(pinDHT11);

#define MAX_PACKETSIZE 512

WiFiClient TCPclient;
String TcpClient_Buff = "";
unsigned int TcpClient_BuffIndex = 0;
unsigned long TcpClient_preTick = 0;
unsigned long preHeartTick = 0;
unsigned long preTCPStartTick = 0;
bool preTCPConnected = false;

// ---------------- 新增控制 ----------------
bool autoBlink = false;     // 是否进入自动闪烁
bool manualOn = false;      // 收到 on 命令
bool forceOff = false;      // 收到 off 命令
unsigned long lastBlinkTime = 0;
bool ledState = false;
unsigned long blinkInterval = 500;

String my_led_status = "off";

void doWiFiTick();
void startSTA();
void doTCPClientTick();
void startTCPClient();
void sendtoTCPServer(String p);
void turnOnLed();
void turnOffLed();
void updateLedState();

void sendtoTCPServer(String p) {
  if (!TCPclient.connected()) {
    Serial.println("Client is not ready");
    return;
  }
  TCPclient.print(p);
}

void startTCPClient() {
  if (TCPclient.connect(TCP_SERVER_ADDR, atoi(TCP_SERVER_PORT))) {
    Serial.printf("\nConnected to server: %s:%d\n", TCP_SERVER_ADDR, atoi(TCP_SERVER_PORT));
    String tcpTemp = "cmd=1&uid=" + UID + "&topic=" + TOPIC2 + "\r\n";
    sendtoTCPServer(tcpTemp);
    preTCPConnected = true;
    preHeartTick = millis();
    TCPclient.setNoDelay(true);
  } else {
    Serial.printf("Failed to connect to server: %s\n", TCP_SERVER_ADDR);
    TCPclient.stop();
    preTCPConnected = false;
  }
  preTCPStartTick = millis();
}

void doTCPClientTick() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (!TCPclient.connected()) {
    if (preTCPConnected) {
      preTCPConnected = false;
      preTCPStartTick = millis();
      TCPclient.stop();
    } else if (millis() - preTCPStartTick > 1000) {
      startTCPClient();
    }
  } else {
    if (TCPclient.available()) {
      char c = TCPclient.read();
      TcpClient_Buff += c;
      TcpClient_BuffIndex++;
      TcpClient_preTick = millis();

      if (TcpClient_BuffIndex >= MAX_PACKETSIZE - 1) {
        TcpClient_BuffIndex = MAX_PACKETSIZE - 2;
        TcpClient_preTick -= 200;
      }
      preHeartTick = millis();
    }
    if (millis() - preHeartTick >= upDataTime) {
      preHeartTick = millis();
      byte temperature = 0;
      byte humidity = 0;
      int err = dht11.read(&temperature, &humidity, NULL);
      if (err != SimpleDHTErrSuccess) {
        Serial.print("Read DHT11 failed, err="); Serial.println(err); delay(1000);
        return;
      }

      // ---------------- 根据温度决定自动闪烁 ----------------
      if (!manualOn && !forceOff) {
        autoBlink = (temperature > 30);
      }

      String upstr = "cmd=2&uid=" + UID + "&topic=" + TOPIC + "&msg=#" + temperature + "#" + humidity + "#" + my_led_status + "#\r\n";
      sendtoTCPServer(upstr);
    }
  }

  if ((TcpClient_Buff.length() >= 1) && (millis() - TcpClient_preTick >= 200)) {
    TCPclient.flush();
    if (TcpClient_Buff.indexOf("&msg=on") > 0) {
      manualOn = true;
      forceOff = false;
      autoBlink = false;
    } else if (TcpClient_Buff.indexOf("&msg=off") > 0) {
      manualOn = false;
      forceOff = true;
      autoBlink = false;
    }
    TcpClient_Buff = "";
    TcpClient_BuffIndex = 0;
  }

  updateLedState();
}

void updateLedState() {
  if (manualOn) {
    digitalWrite(LED_Pin, LOW);
    my_led_status = "on";
  } else if (forceOff) {
    digitalWrite(LED_Pin, HIGH);
    my_led_status = "off";
  } else if (autoBlink) {
    if (millis() - lastBlinkTime > blinkInterval) {
      lastBlinkTime = millis();
      ledState = !ledState;
      digitalWrite(LED_Pin, ledState ? LOW : HIGH);
      my_led_status = ledState ? "on" : "off";
    }
  } else {
    digitalWrite(LED_Pin, HIGH);
    my_led_status = "off";
  }
}

void startSTA() {
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(DEFAULT_STASSID, DEFAULT_STAPSW);
}

void doWiFiTick() {
  static bool startSTAFlag = false;
  static bool taskStarted = false;
  static uint32_t lastWiFiCheckTick = 0;

  if (!startSTAFlag) {
    startSTAFlag = true;
    startSTA();
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWiFiCheckTick > 1000) {
      lastWiFiCheckTick = millis();
    }
  } else {
    if (!taskStarted) {
      taskStarted = true;
      Serial.print("\nGet IP Address: ");
      Serial.println(WiFi.localIP());
      startTCPClient();
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_Pin, OUTPUT);
  digitalWrite(LED_Pin, HIGH);
}

void loop() {
  doWiFiTick();
  doTCPClientTick();
}

