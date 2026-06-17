/* ============================================================
   HOME SECURITY ESP32 — BLYNK IOT (FINAL)
   
   Pin:
   - GPIO18 → HC-SR04 TRIG
   - GPIO19 → HC-SR04 ECHO
   - GPIO21 → Buzzer
   - GPIO4  → DS18B20 DATA (suhu air)
   - GPIO22 → Servo SG92R
   - GPIO23 → DHT22 DATA
   
   Blynk Virtual Pin:
   - V1 → Switch Mode Rumah (ARMED=Maling, DISARMED=Tamu)
   - V2 → Gauge Jarak (cm)
   - V3 → Label Status
   - V4 → Gauge Suhu Air (°C) - DS18B20
   - V5 → Gauge Suhu Ruangan (°C) - DHT22
   - V6 → Gauge Kelembaban (%) - DHT22
   - V7 → Switch Sistem Security ON/OFF
   
   Events:
   - motion_detected → maling (saat ARMED + V7 ON)
   - tamu_datang     → tamu (saat DISARMED + V7 ON)
   - suhu_air_bahaya → air > 44°C
   - suhu_kebakaran  → ruangan > 50°C
============================================================ */

#define BLYNK_TEMPLATE_ID   "TMPL6oY4vzk4j"
#define BLYNK_TEMPLATE_NAME "homesecurity"
#define BLYNK_AUTH_TOKEN    "TEfQB-5YnXC6Kc7sIwl7UmMVY0GdpOcf"
#define BLYNK_PRINT         Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
#include <DHT.h>

// ── WiFi ─────────────────────────────────────────────────────
const char* ssid = "A32";
const char* pass = "jirolupat";

// ── Pin ──────────────────────────────────────────────────────
#define TRIG_PIN      18
#define ECHO_PIN      19
#define BUZZER_PIN    21
#define ONE_WIRE_BUS   4
#define SERVO_PIN     22
#define DHT_PIN       23
#define DHT_TYPE      DHT22

// ── Virtual Pin ──────────────────────────────────────────────
#define VPIN_ARM        V1
#define VPIN_DISTANCE   V2
#define VPIN_STATUS     V3
#define VPIN_SUHU_AIR   V4
#define VPIN_SUHU_RUANG V5
#define VPIN_HUMIDITY   V6
#define VPIN_SYSTEM     V7

// ── Konstanta ────────────────────────────────────────────────
const float          JARAK_BAHAYA     = 20.0;
const float          JARAK_MIN_VALID  = 2.0;
const float          SUHU_RUANG_BATAS = 28.0;
const float          SUHU_KEBAKARAN   = 50.0;
const unsigned long  WARMUP_MS        = 3000;
const unsigned long  INTERVAL_JARAK   = 200;
const unsigned long  INTERVAL_SUHU    = 2000;
const unsigned long  COOLDOWN_NOTIF   = 5000;
const unsigned long  WIFI_TIMEOUT     = 15000;
const unsigned long  RECONNECT_CD     = 10000;
const int            KONFIRMASI_MAKS  = 3;

const float SUHU_AIR_AMAN_MIN  = 37.0;
const float SUHU_AIR_AMAN_MAX  = 40.0;
const float SUHU_AIR_PANAS_MAX = 44.0;

const unsigned long BUZZER_TAMU_MS = 1000;

// ── State ────────────────────────────────────────────────────
bool          isArmed            = true;
bool          systemOn           = true;
bool          sensorReady        = false;
bool          buzzerNyala        = false;
unsigned long buzzerOnTime       = 0;
bool          modeMaling         = false;
int           posServo           = -1;
int           konfirmasi         = 0;
unsigned long lastNotif          = 0;
unsigned long lastNotifAir       = 0;
unsigned long lastNotifKebakaran = 0;
unsigned long bootTime           = 0;
unsigned long lastReconnect      = 0;

// ── Objek ────────────────────────────────────────────────────
OneWire           oneWire(ONE_WIRE_BUS);
DallasTemperature sensorAir(&oneWire);
DHT               dht(DHT_PIN, DHT_TYPE);
Servo             myServo;
BlynkTimer        timer;

// ── Forward declarations ─────────────────────────────────────
void bacaSensor();
void bacaSuhuAir();
void bacaDHT();
void buzzerNyalaFn(bool maling);
void buzzerMatiFn();
void cekBuzzerTimeout();
void setServo(int derajat);
void sendStatus();
void sendStatusWithWindow(bool jendelaBuka, bool kebakaran);
bool connectWiFi();

// ════════════════════════════════════════════════════════════
//  UTILITY
// ════════════════════════════════════════════════════════════
void printSep(char c = '-') {
  for (int i = 0; i < 48; i++) Serial.print(c);
  Serial.println();
}

// ════════════════════════════════════════════════════════════
//  BUZZER
// ════════════════════════════════════════════════════════════
void buzzerNyalaFn(bool maling) {
  if (!buzzerNyala) {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerNyala  = true;
    modeMaling   = maling;
    buzzerOnTime = millis();
    if (maling) {
      Serial.println("  [BUZZER] >>> ALARM (terus menerus) <<<");
    } else {
      Serial.println("  [BUZZER] >>> TAMU (bunyi pendek) <<<");
    }
  } else if (maling) {
    // Update mode jadi maling kalau sebelumnya tamu tapi sekarang ada bahaya lebih besar
    modeMaling = true;
  }
}

void buzzerMatiFn() {
  if (buzzerNyala) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerNyala = false;
    Serial.println("  [BUZZER] Mati");
  }
}

void cekBuzzerTimeout() {
  if (buzzerNyala && !modeMaling) {
    if (millis() - buzzerOnTime >= BUZZER_TAMU_MS) {
      buzzerMatiFn();
      konfirmasi = 0;
    }
  }
}

// ════════════════════════════════════════════════════════════
//  SERVO
// ════════════════════════════════════════════════════════════
void setServo(int derajat) {
  if (posServo != derajat) {
    myServo.write(derajat);
    posServo = derajat;
    Serial.print("  [SERVO]  Bergerak ke ");
    Serial.print(derajat);
    Serial.println(" derajat");
  }
}

// ════════════════════════════════════════════════════════════
//  WIFI
// ════════════════════════════════════════════════════════════
bool connectWiFi() {
  Serial.print("  [WIFI] Menghubungkan ke: ");
  Serial.println(ssid);
  WiFi.disconnect(true);
  delay(500);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t >= WIFI_TIMEOUT) {
      Serial.println("\n  [WIFI] GAGAL! Pastikan:");
      Serial.println("         1. Hotspot/WiFi menyala");
      Serial.println("         2. Nama & password benar");
      Serial.println("         3. Frekuensi WiFi 2.4GHz (bukan 5GHz)");
      return false;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("  [WIFI] Terhubung! IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

// ════════════════════════════════════════════════════════════
//  BLYNK
// ════════════════════════════════════════════════════════════
void sendStatus() {
  if (!Blynk.connected()) return;
  Blynk.virtualWrite(VPIN_STATUS, isArmed ? "ARMED" : "DISARMED");
  Blynk.virtualWrite(VPIN_ARM,    isArmed ? 1 : 0);
  Blynk.virtualWrite(VPIN_SYSTEM, systemOn ? 1 : 0);
  Serial.println("  [BLYNK] Status dikirim ke dashboard");
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(VPIN_ARM);
  Blynk.syncVirtual(VPIN_SYSTEM);
  sendStatus();
  printSep('=');
  Serial.println("  [BLYNK] Terhubung ke server Blynk IoT!");
  printSep('=');
}

// V1 — Mode Rumah (Tamu/Maling)
BLYNK_WRITE(VPIN_ARM) {
  isArmed = param.asInt();
  printSep();
  if (isArmed) {
    Serial.println("  [MODE]   RUMAH KOSONG (deteksi = Maling)");
  } else {
    Serial.println("  [MODE]   ADA PENGHUNI (deteksi = Tamu)");
  }
  sendStatus();
  printSep();
}

// V7 — Sistem Security ON/OFF
BLYNK_WRITE(VPIN_SYSTEM) {
  systemOn = param.asInt();
  printSep();
  if (systemOn) {
    Serial.println("  [SISTEM] AKTIF — Sensor jarak & buzzer berjalan");
  } else {
    Serial.println("  [SISTEM] NONAKTIF — Sensor jarak & buzzer dimatikan");
    buzzerMatiFn();
    konfirmasi = 0;
    if (Blynk.connected()) Blynk.virtualWrite(VPIN_DISTANCE, 0);
  }
  sendStatus();
  printSep();
}

// ════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(2000);

  printSep('=');
  Serial.println("  HOME SECURITY ESP32 - BLYNK IOT (FINAL)");
  printSep('=');

  pinMode(TRIG_PIN,   OUTPUT);
  pinMode(ECHO_PIN,   INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(TRIG_PIN,   LOW);
  digitalWrite(BUZZER_PIN, LOW);

  sensorAir.begin();
  int jumlah = sensorAir.getDeviceCount();
  Serial.print("  [DS18B20] Sensor suhu air ditemukan: ");
  Serial.println(jumlah);
  if (jumlah == 0) {
    Serial.println("  [DS18B20] WARNING: Cek kabel & pull-up 4.7k di GPIO4");
  }

  dht.begin();
  Serial.println("  [DHT22]  Sensor suhu ruangan & kelembaban siap di GPIO23");

  myServo.attach(SERVO_PIN);
  setServo(90);

  Serial.println("  [INIT] Semua perangkat siap");
  printSep();

  if (connectWiFi()) {
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect(3000);
  } else {
    Serial.println("  [WARN] Blynk tidak aktif, sensor tetap jalan lokal");
  }

  bootTime = millis();

  timer.setInterval(INTERVAL_JARAK, bacaSensor);
  timer.setInterval(INTERVAL_SUHU,  bacaSuhuAir);
  timer.setInterval(INTERVAL_SUHU,  bacaDHT);

  printSep();
  Serial.println("  [BOOT] Sistem siap, warm-up 3 detik...");
  printSep();
}

// ════════════════════════════════════════════════════════════
//  LOOP
// ════════════════════════════════════════════════════════════
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnect > RECONNECT_CD) {
      lastReconnect = millis();
      Serial.println("  [WIFI] Terputus, reconnect...");
      connectWiFi();
    }
    timer.run();
    return;
  }

  if (!Blynk.connected()) {
    if (millis() - lastReconnect > RECONNECT_CD) {
      lastReconnect = millis();
      Serial.println("  [BLYNK] Reconnecting...");
      Blynk.connect(3000);
    }
  }

  Blynk.run();
  timer.run();
  cekBuzzerTimeout();
}

// ════════════════════════════════════════════════════════════
//  UKUR JARAK
// ════════════════════════════════════════════════════════════
float ukurJarak() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long durasi = pulseIn(ECHO_PIN, HIGH, 30000);
  if (durasi == 0) return -1.0;
  return durasi * 0.034f / 2.0f;
}

// ════════════════════════════════════════════════════════════
//  TASK: BACA JARAK (setiap 200ms)
// ════════════════════════════════════════════════════════════
void bacaSensor() {
  if (!sensorReady) {
    if (millis() - bootTime >= WARMUP_MS) {
      sensorReady = true;
      Serial.println("  [SENSOR] HC-SR04 siap memantau!");
      sendStatus();
    }
    return;
  }

  if (!systemOn) {
    return;
  }

  float jarak = ukurJarak();

  if (jarak < 0) {
    konfirmasi = 0;
    return;
  }

  if (jarak < JARAK_MIN_VALID) {
    konfirmasi = 0;
    if (Blynk.connected()) Blynk.virtualWrite(VPIN_DISTANCE, 0);
    return;
  }

  if (Blynk.connected()) Blynk.virtualWrite(VPIN_DISTANCE, jarak);

  Serial.print("  [JARAK]  ");
  Serial.print(jarak, 1);
  Serial.print(" cm");

  if (jarak < JARAK_BAHAYA) {
    konfirmasi++;
    Serial.print("  | Objek terdeteksi! Konfirmasi: ");
    Serial.print(konfirmasi);
    Serial.print("/");
    Serial.print(KONFIRMASI_MAKS);

    if (konfirmasi >= KONFIRMASI_MAKS) {
      if (isArmed) {
        Serial.print(" >>> ALARM MALING!");
        buzzerNyalaFn(true);

        if (Blynk.connected() && millis() - lastNotif >= COOLDOWN_NOTIF) {
          Blynk.logEvent("motion_detected", "WARNING! Penyusup terdeteksi saat rumah kosong!");
          lastNotif = millis();
        }
      } else {
        Serial.print(" >>> Tamu datang");
        buzzerNyalaFn(false);

        if (Blynk.connected() && millis() - lastNotif >= COOLDOWN_NOTIF) {
          Blynk.logEvent("tamu_datang", "Ada tamu di depan rumah");
          lastNotif = millis();
        }
      }
    }
  } else {
    Serial.print("  | Aman");
    konfirmasi = 0;
    if (buzzerNyala && modeMaling) {
      buzzerMatiFn();
    }
  }
  Serial.println();
}

// ════════════════════════════════════════════════════════════
//  TASK: BACA SUHU AIR — DS18B20 (setiap 2000ms)
// ════════════════════════════════════════════════════════════
void bacaSuhuAir() {
  sensorAir.requestTemperatures();
  float suhu = sensorAir.getTempCByIndex(0);

  if (suhu == DEVICE_DISCONNECTED_C || suhu < -10.0 || suhu > 100.0) {
    Serial.println("  [AIR]    ERROR: DS18B20 tidak terbaca! Cek kabel & pull-up 4.7k di GPIO4");
    return;
  }

  Serial.print("  [AIR]    Suhu air: ");
  Serial.print(suhu, 1);
  Serial.print(" C  |  Kategori: ");

  if (suhu > SUHU_AIR_PANAS_MAX) {
    Serial.println("BAHAYA! Risiko luka bakar");
    if (Blynk.connected() && millis() - lastNotifAir >= COOLDOWN_NOTIF) {
      Blynk.logEvent("suhu_air_bahaya", "BAHAYA! Suhu air > 44C, risiko luka bakar!");
      lastNotifAir = millis();
    }
  } else if (suhu >= (SUHU_AIR_AMAN_MAX + 0.1) && suhu <= SUHU_AIR_PANAS_MAX) {
    Serial.println("PANAS - hati-hati");
  } else if (suhu >= SUHU_AIR_AMAN_MIN && suhu <= SUHU_AIR_AMAN_MAX) {
    Serial.println("AMAN - nyaman untuk mandi");
  } else {
    Serial.println("Di luar rentang monitoring");
  }

  if (Blynk.connected()) Blynk.virtualWrite(VPIN_SUHU_AIR, suhu);
}

// ════════════════════════════════════════════════════════════
//  TASK: BACA DHT22 — Suhu Ruangan + Kelembaban (setiap 2000ms)
// ════════════════════════════════════════════════════════════
void bacaDHT() {
  float suhu       = dht.readTemperature();
  float kelembaban = dht.readHumidity();

  if (isnan(suhu) || isnan(kelembaban)) {
    Serial.println("  [DHT22]  ERROR: Sensor tidak terbaca! Cek kabel di GPIO23");
    return;
  }

  Serial.print("  [DHT22]  Suhu ruangan: ");
  Serial.print(suhu, 1);
  Serial.print(" C  |  Kelembaban: ");
  Serial.print(kelembaban, 1);
  Serial.print(" %  |  Jendela: ");

  bool kebakaran   = (suhu > SUHU_KEBAKARAN);
  bool jendelaBuka = (suhu > SUHU_RUANG_BATAS);

  if (kebakaran) {
    Serial.println("!!! SUHU EKSTREM - KEMUNGKINAN KEBAKARAN !!!");
    setServo(0); // buka jendela untuk ventilasi asap
    buzzerNyalaFn(true);

    if (Blynk.connected() && millis() - lastNotifKebakaran >= COOLDOWN_NOTIF) {
      Blynk.logEvent("suhu_kebakaran", "BAHAYA! Suhu ruangan ekstrem, indikasi kebakaran!");
      lastNotifKebakaran = millis();
    }
  } else if (jendelaBuka) {
    setServo(0);
    Serial.println("BUKA (0°) — suhu > 28C");
  } else {
    setServo(90);
    Serial.println("TUTUP (90°) — suhu <= 28C");
  }

  if (Blynk.connected()) Blynk.virtualWrite(VPIN_SUHU_RUANG, suhu);
  if (Blynk.connected()) Blynk.virtualWrite(VPIN_HUMIDITY,   kelembaban);

  sendStatusWithWindow(jendelaBuka, kebakaran);
}

// ════════════════════════════════════════════════════════════
//  Update status text gabungan sistem + jendela + kebakaran
// ════════════════════════════════════════════════════════════
void sendStatusWithWindow(bool jendelaBuka, bool kebakaran) {
  if (!Blynk.connected()) return;

  String status;
  if (kebakaran) {
    status = "BAHAYA KEBAKARAN!";
  } else if (!systemOn) {
    status = "SISTEM OFF";
  } else {
    status = isArmed ? "ARMED" : "DISARMED";
  }

  status += jendelaBuka ? " | Jendela BUKA" : " | Jendela TUTUP";
  Blynk.virtualWrite(VPIN_STATUS, status);
}