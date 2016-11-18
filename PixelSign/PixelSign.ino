/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#define PIN 4 //PIN D2 on NodeMCU - connected to ws2812 strand.

typedef void (*func)();

// Config for matrix.
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(10, 10, PIN,
  NEO_MATRIX_BOTTOM     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);

const uint16_t colors[] = {
  matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255) };
  

// config for wifi 
const char *ssid = "";
const char *password = "";
// 60:01:94:06:61:a3 

char message[] = "                                                                                                                       ";
uint32_t image[100] = {};
int message_length = sizeof(message)/sizeof(char);
int repeat_times = 3;
int frame_delay = 100;
enum color_style {fixed = 0, rando = 1, cycle = 2};
byte cycle_idx = 1;
int style = rando;
uint16_t t_color = colors[0];
int x = matrix.width();
int pass = 0;
int prev_frame = millis();
func doMatrixUpdate;

ESP8266WebServer server ( 80 );

const int led = 13;

void handleText() {
  digitalWrite ( led, 1 );
  char temp[400];
  doMatrixUpdate = doTextUpdate;
  message_length = server.arg("message").length();
  repeat_times = server.arg("repeat").toInt();
  frame_delay = server.arg("speed").toInt();
  style = server.arg("colorstyle").toInt();
  String t_str = server.arg("colour");
  t_str.toCharArray(temp, 7);
  t_color = (uint16_t) strtol( &temp[1], NULL, 16);
  pass = 0;
  matrix.setTextColor(t_color);
  String Smessage = urldecode(server.arg("message"));
  Smessage.toCharArray(message, message_length + 1);
  x = matrix.width();
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
  digitalWrite ( led, 0 );
}

void doTextUpdate() {
  int curtime = millis();
  if (curtime > prev_frame + frame_delay) {
    prev_frame = curtime;
    if(--x < (message_length * -6)) {
      x = matrix.width();
      if (style == rando) matrix.setTextColor(colors[pass % 3]);
      if (++pass >= repeat_times && repeat_times > 0) {
        strcpy(message, "\0");
      }
    }
  }
  if (style == cycle && curtime > prev_frame + 10) matrix.setTextColor(Wheel(++cycle_idx));
  matrix.fillScreen(0);
  matrix.setCursor(x, 2);
  matrix.print(message);
  matrix.show();
}

void doBlankScreen() {
  matrix.fillScreen(0);
  matrix.show();
}


void handleImage() {
  char temp[1024];
  digitalWrite ( led, 1 );
  doMatrixUpdate = doImageUpdate;
  server.arg("data").toCharArray(temp, server.arg("data").length());
  repeat_times = server.arg("duration").toInt();
  int i = 0;
  char* sep = strchr(temp, '#');
  while ( sep != 0 ) {
    char hexcolor[7]; // 6 chars plus terminator
    memcpy(hexcolor, sep + 1, 6);
    hexcolor[6] = 0;
    image[i] = (uint32_t) strtol( hexcolor, NULL, 16);
    sep = strchr(sep + 1, '#');
    i++;
  }
  pass = 0;
  frame_delay = 1000;
  server.send ( 200, "text/html", "Ok");
  digitalWrite ( led, 0 );
}

void doImageUpdate() {
  int curtime = millis();
  if (curtime > prev_frame + frame_delay || pass == 0) {
    if ( pass == 0) {
      matrix.fillScreen(0);
      for (int i = 0; i < 100; i++) {
        matrix.setPixelColor(i, image[i]);  
      }
      matrix.show();
    } else if (pass > repeat_times && repeat_times != 0) {
        matrix.fillScreen(0);
        matrix.show();
    }
    prev_frame = millis();    
    pass++;
  }
}

void handleRoot() {
	digitalWrite ( led, 1 );
	char temp[1024];
  snprintf ( temp, 1024,
"<html>\
    <head>\
        <title>Pixel Web Sign Thing!</title>\
    </head>\
    <body>\
        <h3>Current Message:</h3>\
        <p>%s</p>\
        <form action=\"/text\">\
            <input type=\"text\" name=\"message\">\
            <select name=\"colorstyle\">\
                <option value=\"0\">One Color (set below)</option>\
                <option value=\"1\">Random</option>\
                <option value=\"2\">Cycle</option>\
            </select><br>\
            <label for=\"repeat\">Repeat x times (0 for infinite):</label><input type=\"number\" name=\"repeat\" value=\"3\"><br>\
            <label for=\"colour\">Colour: </label><input type=\"color\" name=\"colour\" value=\"#FF8833\"><br>\
            <label for=\"speed\">Fast</label> <input type=\"range\" name=\"speed\" min=\"10\" max=\"200\"> <label for=\"speed\">Slow</label><br>\
            <input type=\"submit\">\
        </form>\
    </body>\
</html>",

		message
	);
	server.send ( 200, "text/html", temp );
	digitalWrite ( led, 0 );
}

void handleNotFound() {
	digitalWrite ( led, 1 );
	String content = "File Not Found\n\n";
	content += "URI: ";
	content += server.uri();
	content += "\nMethod: ";
	content += ( server.method() == HTTP_GET ) ? "GET" : "POST";
	content += "\nArguments: ";
	content += server.args();
	content += "\n";

	for ( uint8_t i = 0; i < server.args(); i++ ) {
		content += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
	}

	server.send ( 404, "text/plain", content );
	digitalWrite ( led, 0 );
}

void setup ( void ) {
	pinMode ( led, OUTPUT );
	digitalWrite ( led, 0 );
	Serial.begin ( 115200 );
	WiFi.begin ( ssid, password );
	Serial.println ( "" );

	// Wait for connection
	while ( WiFi.status() != WL_CONNECTED ) {
		delay ( 500 );
		Serial.print ( "." );
	}

	Serial.println ( "" );
	Serial.print ( "Connected to " );
	Serial.println ( ssid );
	Serial.print ( "IP address: " );
	Serial.println ( WiFi.localIP() );

	if ( MDNS.begin ( "esp8266" ) ) {
		Serial.println ( "MDNS responder started" );
	}

	server.on ( "/", handleRoot );
  server.on ( "/text", handleText );
  server.on ( "/image", handleImage );
	server.onNotFound ( handleNotFound );
	server.begin();
	Serial.println ( "HTTP server started" );

 // setup matrix
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(80);
  matrix.setTextColor(colors[0]);
  message_length = snprintf(message, message_length, "%i.%i.%i.%i\0", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  doMatrixUpdate = doTextUpdate;
}

void loop ( void ) {
	server.handleClient();
  doMatrixUpdate();
}

String urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}

unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return matrix.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return matrix.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return matrix.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
