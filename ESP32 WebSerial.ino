#include "config.h"
#include <esp_wifi.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "SPIFFS.h"
#include <ArduinoOTA.h> 

bool shouldReboot = false;
int cleanup_counter = 0;
uint8_t buffer[BUFFERSIZE];
uint16_t buffer_pointer = 0;

AsyncWebServer asyncserver(80);
AsyncWebSocket ws("/data");

void checkAuth(AsyncWebServerRequest* request) {
	if (!request->authenticate(username, password))
	{
		return request->requestAuthentication();
	}
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {

	if (type == WS_EVT_CONNECT) {
		if (ws.getClients().length() > MAX_CLIENTS)
			client->close();
	}
	else if (type == WS_EVT_DISCONNECT) {
#ifdef DEBUG
		Serial.println(ws.getClients().length());
#endif
		if (ws.getClients().length() == 0) {
#ifdef DEBUG
			Serial.println("Log out");
#endif
			Serial2.write((byte)0x04);
			Serial2.write((byte)0x04);
			Serial2.write((byte)0x04);
		}

	}
	else if (type == WS_EVT_DATA) {
		{
			flash(2, 10);
			//uint8_t lnbk[]{ 13,10 };
			Serial2.write(data, len);
		}
	}

}

void setup() {

	delay(500);
	pinMode(2, OUTPUT);
	flash(10, 50);

	Serial.begin(115200); //debug
	Serial2.begin(BAUDRATE); // GPIO 25/26

	flash(2, 200);

	// Initialize SPIFFS
	if (!SPIFFS.begin(true)) {
#ifdef DEBUG
		Serial.println("An Error has occurred while mounting SPIFFS");
#endif
			return;
	}

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, pw);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
	}

	flash(3, 200);


	// Configure Async Webserver

	asyncserver.serveStatic("js/setup.js", SPIFFS, "/js/setup.js");
	asyncserver.serveStatic("js/fn.js", SPIFFS, "/js/fn.js");
	asyncserver.serveStatic("style/index.css", SPIFFS, "/style/index.css");

	asyncserver.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
		checkAuth(request);
		AsyncWebServerResponse* response = request->beginResponse(SPIFFS, "/index.htm");
		response->addHeader("Connection", "close");
		request->send(response);
		});

	asyncserver.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(200, "text/plain", "OK");
		flash(5, 200);
		ESP.restart();
		});

	asyncserver.on("/logout", HTTP_GET, [](AsyncWebServerRequest* request) {
		AsyncWebServerResponse* res = request->beginResponse(401);
		res->addHeader("WWW-Authenticate", "Basic realm='asyncesp'");
		res->addHeader("Authorization", "Basic false");
		request->send(res);
		});

	asyncserver.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
		checkAuth(request);
		request->send(SPIFFS, "/update.htm");
		});

	asyncserver.on("/update", HTTP_POST, [](AsyncWebServerRequest* request) { //Upload handler
		shouldReboot = !Update.hasError();
		AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
		response->addHeader("Connection", "close");
		request->send(response);
		ESP.restart();
		}, [](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
			byte type = U_FLASH;
			if (request->header("type") == "SPIFFS")
				type = U_SPIFFS;
			if (!index) {
				Serial.printf("Update Start: %s\n", filename.c_str());
				if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000, type, 2)) {
					Update.printError(Serial);
				}
			}
			if (!Update.hasError()) {
				if (Update.write(data, len) != len) {
					Update.printError(Serial);
				}
			}
			if (final) {
				if (Update.end(true)) {
					Serial.printf("Update Success: %uB\n", index + len);
					shouldReboot = true;
				}
				else {
					Update.printError(Serial);
				}
			}
		});
	ws.onEvent(onWsEvent);
	asyncserver.addHandler(&ws);
	asyncserver.begin();

	// OTA

	ArduinoOTA.setMdnsEnabled(true);
	ArduinoOTA.setHostname("serverterm");
	ArduinoOTA
		.onStart([]() {
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH)
			type = "sketch";
		else // U_SPIFFS
			type = "filesystem";
		// Clean SPIFFS
		SPIFFS.end();

		// Disable client connections    
		ws.enable(false);

		// Advertise connected clients what's going on
		ws.textAll("OTA Update Started");

		// Close them
		ws.closeAll();
		// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
		Serial.println("Start updating " + type);
			})
		.onEnd([]() {
				Serial.println("\nEnd");
			})
				.onProgress([](unsigned int progress, unsigned int total) {
				Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
					})
				.onError([](ota_error_t error) {
						Serial.printf("Error[%u]: ", error);
						if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
						else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
						else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
						else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
						else if (error == OTA_END_ERROR) Serial.println("End Failed");
					});

					ArduinoOTA.setPasswordHash((const char*)"a53a9898f4ab940ba77b035dace30539");
					ArduinoOTA.begin();

					esp_err_t esp_wifi_set_max_tx_power(50);

					flash(4);

}



void loop()
{
	if (shouldReboot) {
#ifdef DEBUG
		Serial.println("Rebooting...");
#endif
		delay(100);
		ESP.restart();
	}

	ArduinoOTA.handle();

	if (cleanup_counter < 100000)
		cleanup_counter++;
	else
	{
		cleanup_counter = 0;
		ws.cleanupClients();
	}
	if (Serial2.available() && buffer_pointer < BUFFERSIZE - 1)
	{
		while (Serial2.available() && buffer_pointer < BUFFERSIZE - 1)
		{
			buffer[buffer_pointer] = Serial2.read(); // read char from UART(num)
			if (buffer_pointer < BUFFERSIZE - 1)
				buffer_pointer++;
		}

	}
#ifdef DEBUG
	if (buffer_pointer >= BUFFERSIZE - 1) {
		Serial.println("Buffer full");
	}
#endif 

	if (buffer_pointer > 0 && ws.getClients().length() > 0) {
		ws.binaryAll((uint8_t*)buffer, buffer_pointer);
		buffer_pointer = 0;
	}
}


void flash(int count) { flash(count, 50); }
void flash(int count, int del) {
	for (int i = 0; i < count; i++)
	{
		digitalWrite(2, 1);
		delay(del);
		digitalWrite(2, 0);
		delay(del);
	}
}
