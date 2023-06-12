#include "header.h"

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.mode(WIFI_STA);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void ultrasonic_atas()
{
  digitalWrite(TRIG_PIN_2, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN_2, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN_2, LOW);

  duration_2 = pulseIn(ECHO_PIN_2, HIGH);
  distance_2 = duration_2 * SOUND_SPEED / 2;

  percentage = (height - distance_2) * 100 / height;
}

void ultrasonic_depan()
{
  digitalWrite(TRIG_PIN_1, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN_1, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN_1, LOW);

  duration_1 = pulseIn(ECHO_PIN_1, HIGH);
  distance_1 = duration_1 * SOUND_SPEED / 2;
}

void Open_Bin()
{
  servo.write(155);
}

void send24h()
{
  String timestamp;
  std::string databasePath = "/LogTest";
  String countPath = "/Counter/count";

  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();

    while (!timeClient.update())
    {
      timeClient.forceUpdate();
    }
    // Get current timestamp
    timestamp = timeClient.getFormattedDate();
    timestamp.replace("T", " ");
    timestamp.replace("Z", "");
    char timeStr[9];
    char dateStr[11];
    sscanf(timestamp.c_str(), "%10s %8s", dateStr, timeStr);
    Serial.print("time: ");
    Serial.println(timestamp);

    Firebase.getInt(firebaseData, countPath);
    int lastCount = firebaseData.intData();

    int currentCount = lastCount + 1;
    int parentNode = currentCount - 1;

    std::string parentPath = databasePath + "/" + std::to_string(parentNode);

    Serial.print("Current counter value: ");
    Serial.println(parentNode);

    if (percentage >= 100)
    {
      json.set(statusPath.c_str(), "FULL");
    }
    else
    {
      json.set(statusPath.c_str(), "AVAILABLE");
    }
    json.set(weightPath, weight);
    json.set(percentPath, percentage);

    json.set(datePath, String(dateStr));
    json.set(timePath, String(timeStr));

    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&firebaseData, parentPath.c_str(), &json) ? "ok" : firebaseData.errorReason().c_str());

    Firebase.setInt(firebaseData, countPath, currentCount);
  }
}

void realtimeSend()
{
  rts.set("/capacity", percentage);
  Firebase.updateNode(firebaseData, "/Read/Tong1", rts);
  rts.set("/weight1", weight);
  Firebase.updateNode(firebaseData, "/Read/Tong1", rts);
  delay(1000);
}

void setup_firebase()
{
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
  Serial.println("------------------------------------");
  Serial.println("Connected to firebase");
}

void print_status()
{
  weight = scale.get_units();

  float lastReading;

  if (scale.wait_ready_timeout(2000))
  {
    Serial.print("Weight: ");
    Serial.print(weight);
    Serial.println(" g");

    Serial.print("Distance Depan: ");
    Serial.print(distance_1);
    Serial.println(" cm");

    Serial.print("Distance Atas: ");
    Serial.print(distance_2);
    Serial.println(" cm");

    Serial.print("Percentage: ");
    Serial.print(percentage);
    Serial.println(" %");

    lastReading = weight;
  }
  else
  {
    Serial.println("HX711 Hilang cokkk");
  }
  scale.power_down(); // put the ADC in sleep mode
  delay(100);
  scale.power_up();
}

void setup_oled()
{
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  else {
    Serial.println("SSD1306 allocation success");
    }
  display.clearDisplay();
  display.setTextColor(WHITE);

  Serial.println("Initializing the scale");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(371);
  scale.tare();
}

void displayWeight()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("Weight:");
  display.display();
  display.setCursor(0, 30);
  display.setTextSize(2);
  display.print(weight);
  display.print(" ");
  display.print("grams");
  display.display();
}

void setup()
{
  Serial.begin(115200);

  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(ECHO_PIN_1, INPUT);

  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(ECHO_PIN_2, INPUT);

  pinMode(SERVO_PIN, OUTPUT);

  servo.attach(SERVO_PIN);
  servo.write(0); // Close the bin

  setup_wifi();
  setup_firebase();
  setup_oled();

  timeClient.begin();
  timeClient.update();
  timeClient.setTimeOffset(25200);
}

void loop()
{
  ultrasonic_depan();
  if (distance_1 < distance)
  {
    Open_Bin();
  }
  else
  {
    servo.write(0);
  }
  ultrasonic_atas();
  
  print_status();
  send24h();
  realtimeSend();
  displayWeight();
}