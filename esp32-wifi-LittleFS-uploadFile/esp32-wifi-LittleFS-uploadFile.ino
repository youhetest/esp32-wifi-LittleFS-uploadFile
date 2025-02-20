#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// 定义你的 WiFi 网络的 SSID 和密码
const char* WIFI_SSID = "you-wifi-ssid";
const char* WIFI_PASSWORD = "password";

AsyncWebServer server(80);

void setupLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    Serial.println("Formatting LittleFS...");
    if (LittleFS.format()) {
      Serial.println("LittleFS formatted successfully");
    } else {
      Serial.println("LittleFS format failed");
    }
  }
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("WiFi connection failed");
  }
}

void handleFileUpload(AsyncWebServerRequest *request) {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>File Upload</title>
    </head>
    <body>
      <h1>File Upload</h1>
      <form method="POST" action="/upload" enctype="multipart/form-data">
        <input type="file" name="file">
        <input type="submit" value="Upload">
      </form>
    </body>
    </html>
  )rawliteral";
  request->send(200, "text/html", html);
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  static File uploadFile;

  if (!index) {
    Serial.printf("UploadStart: %s\n", filename.c_str());
    // LittleFS 只能使用 const char*
    if (!filename.startsWith("/")) filename = "/" + filename;
    uploadFile = LittleFS.open(filename.c_str(), "w"); // 使用 c_str()
    if (!uploadFile) {
      request->send(500, "text/plain", "Failed to open file for writing");
      return;
    }
  }
  if (uploadFile) {
    uploadFile.write(data, len);
  }
  if (final) {
    Serial.printf("UploadEnd: %s, %u bytes\n", filename.c_str(), index+len);
    uploadFile.close();
    request->send(200, "text/plain", "File uploaded successfully: " + filename);
    printLittleFSFiles();
  }
}

void printLittleFSFiles() {
  Serial.println("Files in LittleFS:");
  File root = LittleFS.open("/");
  File file = root.openNextFile();

  while (file) {
    Serial.print("  File: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }
  Serial.println("Finished listing files");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  setupLittleFS();
  connectWiFi();

  server.on("/", HTTP_GET, handleFileUpload);
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){}, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
      handleUpload(request, filename, index, data, len, final);
  });
  server.begin();

  Serial.println("Async Web server started");
}

void loop() {
}
