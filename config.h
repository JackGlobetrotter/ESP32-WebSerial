//#define ASYNC_TCP_SSL_ENABLED // ASYNCTCP doers not support ssl for ESP32 yet

#define VERSION "1.00"

#ifdef ASYNC_TCP_SSL_ENABLED

const char * cert = "/rootSSL.key";
const char* key = "/rootSSL.pem";
const char * key_password = "password";

#endif

#define BAUDRATE 1500000
#define BUFFERSIZE 4096
#define MAX_CLIENTS 4

//Wifi credentials
const char* ssid = "ssid";
const char* pw = "wifipassword";

// Web interface Credentails
const char* username = "username";
const char* password = "password";

#define DEBUG