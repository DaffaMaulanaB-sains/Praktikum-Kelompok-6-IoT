# Praktikum-Kelompok-6-IoT

## UPN Veteran Jawa Timur
**Mata Kuliah:** Internet of Things B083

## Disusun oleh Kelompok 2
1. Hafizh Akbar Azzadhani — 23083010069  
2. Aditya Febri Pratama — 23083010096  
3. Radithya Hylmi Maulana Putra — 23083010083  
4. Daffa Maulana Briliansyah — 23083010098  

Repository ini merupakan dokumentasi dari berbagai hasil praktikum dan tugas kelompok pada mata kuliah Internet of Things. Setiap file diorganisasikan berdasarkan topik pembahasan untuk memudahkan dalam pencarian, penilaian, serta pengelolaan proyek.

## Daftar Folder

### 1. `01-Demo-Display-OLED-SSD1306`
Folder ini memuat proyek demonstrasi pemanfaatan layar OLED SSD1306 pada mikrokontroler. Di dalamnya terdapat kode program, dokumentasi hasil pengujian, serta berbagai file pendukung yang digunakan untuk menampilkan data maupun teks pada display OLED.

### 2. `02-Bacaan-Sensor-DHT-Melalui-WiFi`
Folder ini berisi project pembacaan sensor DHT11 menggunakan board ESP8266. Data suhu dan kelembapan ditampilkan pada OLED serta dapat diakses melalui web server. Folder ini memuat source code, dokumentasi hasil tampilan web, dan rangkaian hardware.

### 3. `03-Desain-Sistem-Sensor (ETS)`
Folder ini berisi dokumen desain sistem sensor yang disusun sebagai pengganti ETS. Isi folder mencakup file PDF utama beserta diagram atau file pendukung lain yang menjelaskan rancangan sistem, alur kerja, dan komponen yang digunakan.

### 4. `04-Project-UAS-Security-Home-System`
Folder ini berisi project UAS Smart Home Security System yang menggunakan ESP32 sebagai mikrokontroler utama dengan sensor ultrasonik HC-SR04 dan sensor suhu DS18B20 Waterproof. Sensor HC-SR04 digunakan untuk mendeteksi keberadaan objek atau intrusi pada area yang dipantau, sedangkan sensor DS18B20 digunakan untuk memantau suhu lingkungan secara real-time.

Sistem dilengkapi dengan buzzer sebagai alarm peringatan ketika intrusi terdeteksi. Selain itu, sistem juga memiliki fitur otomatisasi ventilasi, yaitu membuka jendela secara otomatis menggunakan servo motor ketika suhu lingkungan melebihi batas yang telah ditentukan. Seluruh data sensor dan status sistem dapat dipantau melalui aplikasi Blynk sehingga pengguna dapat menerima informasi dan notifikasi secara real-time melalui smartphone.

Isi Folder:

- Source Code: Kode program untuk menghubungkan ESP32 dengan sensor HC-SR04, sensor DS18B20, servo motor, buzzer, dan aplikasi Blynk.
- Dokumentasi Pengujian: Hasil pengujian sistem keamanan dan monitoring suhu, termasuk pengujian alarm, notifikasi, dan mekanisme buka-tutup jendela otomatis.
- Rangkaian Hardware: Diagram dan penjelasan koneksi antara ESP32, HC-SR04, DS18B20, servo motor, buzzer, dan komponen pendukung lainnya.
- Desain Sistem: Penjelasan alur kerja sistem, flowchart, serta logika deteksi intrusi dan kontrol ventilasi otomatis.
