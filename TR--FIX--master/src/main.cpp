#include "header.h"
// input dr ko martin: task untuk baca berat dan nunjukin di oled bedain 
// jgn disatuin sama ngirim realtime database biar kalo misalkan g dpt wifi
// dia masi bisa show berat di oled
// sama input satu lg kalo bs wifi nya kalo emg putus bisa nyari ssid sama
// password lain lewat rtdb idk how?

// fungsi setup untuk koneksi ke wifi
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

// fungsi untuk membaca jarak dari sensor ultrasonik atas yang nantinya jarak dijadikan persentase kepenuhan tong sampah
void ultrasonic_atas()
{
  digitalWrite(TRIG_PIN_2, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN_2, HIGH);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN_2, LOW);

  duration_2 = pulseIn(ECHO_PIN_2, HIGH);
  distance_2 = duration_2 * SOUND_SPEED / 2;

  percentage = (height - distance_2) * 100 / height;
}

// fungsi membaca jarak dari sensor ultrasonik depan untuk menentukan servo membuka tong sampah atau tidak
void ultrasonic_depan()
{
  digitalWrite(TRIG_PIN_1, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN_1, HIGH);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN_1, LOW);

  duration_1 = pulseIn(ECHO_PIN_1, HIGH);
  distance_1 = duration_1 * SOUND_SPEED / 2;
}

// fungsi membuka tutup tong sampah menggerakkan servo sebesar 155 derajat
void Open_Bin()
{
  servo.write(155);
}

// fungsi untuk data logging ke firebase setiap 24 jam
void send24h()
{
  String timestamp;
  std::string databasePath = "/LogTest";
  String countPath = "/Counter/count"; 

  while (!timeClient.update())
  {
    timeClient.forceUpdate();
  }

  // Mendapatkan tanggal waktu saat itu juga dengan NTP client
  timestamp = timeClient.getFormattedDate();
  timestamp.replace("T", " ");
  timestamp.replace("Z", "");
  // diambil waktu dan tanggal saja dari timestamp yaitu timeStr dan dateStr
  char timeStr[9];
  char dateStr[11];
  sscanf(timestamp.c_str(), "%10s %8s", dateStr, timeStr);
  Serial.print("time: ");
  Serial.println(timestamp);

  // mengambil integer dari path /Counter pada firebase rtdb
  Firebase.getInt(firebaseData, countPath);
    int lastCount = firebaseData.intData();
    // parentNode adalah path yang nantinya akan diupload ke LogTest dengan urutan dari 0,1,2,3 dst
    // kenapa digunakan 0,1,2,3 dst karena untuk datagrid dan chart di flutter hanya bisa membaca jika pathnya berurutan angka
    int currentCount = lastCount + 1;
    int parentNode = currentCount - 1;

    // nama path yang akan diupload = /Logtest/(parentNode) -> (/Logtest/0, /LogTest/1, dst)
    std::string parentPath = databasePath + "/" + std::to_string(parentNode);

    Serial.print("Current counter value: ");
    Serial.println(parentNode);

    // child yang ada didalam parentPath
    // set status, weight, percentage, date dan time ke child pathnya
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

    //upload ke rtdb 
    Serial.printf("Uploading json... %s\n", Firebase.RTDB.setJSON(&firebaseData, parentPath.c_str(), &json) ? "Success" : firebaseData.errorReason().c_str());

    //mengupdate path counter di rtdb
    Firebase.setInt(firebaseData, countPath, currentCount);

  
}

// fungsi untuk mengubah value di path capacity dan weight1 setiap 1 detik untuk realtime monitoring
void realtimeSend()
{
  rts.set("/capacity", percentage);
  Firebase.updateNode(firebaseData, "/Read/Tong1", rts);
  rts.set("/weight1", weight);
  Firebase.updateNode(firebaseData, "/Read/Tong1", rts);
  delay(1000);
}

// fungsi inisialisasi menghubungkan ke firebase
void setup_firebase()
{
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
  Serial.println("Firebase setup success");
}

// fungsi untuk menunjukkan bacaan sensor ke serial monitor
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

// fungsi inisialisasi untuk OLED
void setup_oled()
{
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
  Serial.println(F("SSD1306 allocation success"));
  Serial.println("Initializing the scale");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(395);
  scale.tare();
}

// fungsi untuk menunjukkan berat dari timbangan pada OLED
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

// Task RTOS untuk menjalankan fungsi- fungsi yang sebelumnya ada di atas
void taskRealtimeSerial(void *parameter)
{
  while (1)
  {
    ultrasonic_atas();
    print_status();
    realtimeSend();
    displayWeight();
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// task RTOS untuk menjalankan fungsi send24h hanya sekali setiap harinya pada interval jam 20:59:00 - 21:00:00
void task24h(void *parameter)
{
  bool hasUploaded = false;
  while (1)
  {
    String timestamp;
    while (!timeClient.update())
    {
      timeClient.forceUpdate();
    }
    timestamp = timeClient.getFormattedDate();
    timestamp.replace("T", " ");
    timestamp.replace("Z", "");
    char timeStr[9];
    char dateStr[11];
    sscanf(timestamp.c_str(), "%10s %8s", dateStr, timeStr);
    Serial.print("time: ");
    Serial.println(timestamp);

    int currentHour, currentMinute, currentSecond;
    sscanf(timeStr, "%d:%d:%d", &currentHour, &currentMinute, &currentSecond);

    if (currentHour == 20 && currentMinute == 59 && currentSecond >= 0 && currentSecond <= 59 && hasUploaded == false)
    {
      send24h();
      hasUploaded = true; // Set the flag to indicate that upload has been done
    }
    else if (currentHour >= 21 && currentMinute >= 00 && currentSecond >= 0)
    {
      Serial.print("no upload\n\n");
      hasUploaded = false; // Reset the flag to allow upload in the next interval
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// setup 
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
  //Create task RTOS 
  xTaskCreate(taskRealtimeSerial, "RtSerial", configMINIMAL_STACK_SIZE+15360, NULL, 5 , NULL);
  xTaskCreate(task24h, "24h", configMINIMAL_STACK_SIZE+10240, NULL, 5 , NULL);
}

// fungsi loop hanya untuk membuka tutup tong sampah 
void loop()
{
  ultrasonic_depan();

  if (distance_1 < distance)
  {
    Open_Bin();
  }
  else if (distance_1 > distance)
  {
    servo.write(0);
    delay(100);
  }
}
