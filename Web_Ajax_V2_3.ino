
// Upload this with arduino 5v 328

#include "SPI.h"
#include "Ethernet.h"
#include <EthernetUdp.h>
#include "WebServer.h"
#include <Udp.h>
#include <stdio.h>
#include <Time.h>

#include <VirtualWire.h>
#include <OneWire.h> // OnWire for numeric temperature
#define DS18B20 0x28     // Adress 1-Wire DS18B20 for temperature
#define BROCHE_ONEWIRE 7 // 1-Wire connector Temperature

// Powerfix Aussenfunksteckdosen
#define AON         "light on"
#define AOFF        "light off"
#define FOOD        "feed cat"
 
#define TIMER2 60000
#define AlarmHMS(_hr_, _min_, _sec_) (_hr_ * SECS_PER_HOUR + _min_ * SECS_PER_MIN + _sec_)
 
static uint8_t mac[6] = { 0x02, 0xAA, 0xBB, 0xCC, 0x00, 0x22 }; // MAC address
static uint8_t ip[] = { 192, 168, 1, 77 };                       // IP address
unsigned int localPort = 45;                                  // local port to listen for UDP packets
byte timeServer[] = { 192, 168, 1, 1};                          // IP address of time server (local router)
const int NTP_PACKET_SIZE= 48;                                  // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE];                            // buffer to hold incoming and outgoing packets 
short rc_pin=8;                                                 // data pin of sender
short vc_pin=10;                                                // power pin for sender (only power up sender before sending)

//File myFile;  // Use it to read sd files

/* Set this to the offset (in seconds) to your local time
   This example is GMT - 6 */
const long timeZoneOffset = -21600L;   

unsigned long epoch = 0;
int NTPCounter = 0;
boolean NTPUpdate = false;
int pos = 0;
float temp;
OneWire ds(BROCHE_ONEWIRE); // objet OneWire ds creation

EthernetUDP Udp;

EthernetClient client;
int cou = 0;
int coupleTimes = 21;

int serverPort=8080;
EthernetServer server(serverPort);
char inData[20]; // Allocate some space for the string
byte index = 0;


// IR & RF
int RF_TX_PIN = 4;
int RF_RX_PIN = 3; // receiver

char inChar=-1;


#define PREFIX ""
WebServer webserver(PREFIX, 80);
 
boolean getTemperature(float *temp){
  byte data[9], addr[8];
  if (!ds.search(addr)) { // Recherche un module 1-Wire
    ds.reset_search();    // Réinitialise la recherche de module
    return false;         // Retourne une erreur
  }
  if (OneWire::crc8(addr, 7) != addr[7]) // check if adresse correctly set
    return false;                       
  if (addr[0] != DS18B20) // check DS18B20
    return false;         
  ds.reset();             // reset bus 1-Wire
  ds.select(addr);        // select DS18B20
  ds.write(0x44, 1);      // start ask for temperature
  delay(800);             // wait
  ds.reset();             
  ds.select(addr);       
  ds.write(0xBE);         // ask to read scratchpad
  for (byte i = 0; i < 9; i++) // scratchpad reading
    data[i] = ds.read();       // save octects
  *temp = ((data[1] << 8) | data[0]) * 0.0625;
  return true;
}

// called by Timer2 every 60 sec, checks whether to switch on/off the remote control switches (Master)
void checkTimer() {
 
  NTPCounter++;
 
  // Update Time every hour
  if (NTPCounter == 60) {
    NTPUpdate = true;
    NTPCounter = 0;
  }
}
 
void remoteCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  if (type == WebServer::POST)
  {
    bool repeat;
    char name[16], value[16];
    do
    {
      repeat = server.readPOSTparam(name, 16, value, 16);
 
      if (strcmp(name, "remote") == 0)
      {
         int val = strtoul(value, NULL, 10);
         char code[24];
         switch(val) {
           case 11: // A on
             strcpy(code, AON);
           break;
           case 51: // A off
             strcpy(code, AOFF);
           break;
           case 70: // feed cat
            strcpy(code, FOOD);
           break;
          // case 75: // trace event
          //   Serial.println("Interrupteur event!");//log
          // break;
           default:
              sprintf(code, "%d", val) ;
           break;
         }
         
     //    sprintf(MyTemp, "%d", val);
         sendCode(code);
         vw_send((uint8_t *)code, strlen(code));
      }
    } while (repeat);
 
    server.httpSeeOther(PREFIX "/remote.html");
    return;
  }
 
  server.httpSuccess();
 
  if (type == WebServer::GET)
  {
    // myFile = SD.open("index1.txt",FILE_WRITE);
    // server.print(myFile);
P(remote) =
"<html>"
"<head>"
"<meta name='viewport' content='width=device-width, initial-scale=1' />"
"<title>Kouskala's Home</title>"
"<link rel='stylesheet' href='http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.css' type='text/css' />"
"<script src='http://code.jquery.com/jquery-1.9.1.min.js' type='text/javascript'></script>"
"<script src='http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.js' type='text/javascript'></script>"
"<script type='text/javascript'>$(document).bind('mobileinit',function(){$.mobile.touchOverflowEnabled=true});$.mobile.page.prototype.options.domCache=true;</script>"
"</head><body><div data-role='page'><div data-role='header' data-position='fixed'><h1>Sweet Home</h1>"
"</div><div data-role='content' align='center' data-mini='true'><div data-role='collapsible-set' data-theme='c' data-content-theme='d' data-mini='true'>"
"<div data-role='collapsible' data-collapsed='false'><h3>Habitat</h3><fieldset class='ui-grid-a'><div class='ui-block-a'><iframe frameborder='0' scrolling='NO' height='320' src='http://soft-and-com.com/Thermo/index.html?";
server.printP(remote);

getTemperature(&temp);
server.print(temp);

//myFile = SD.open("index2.txt",FILE_WRITE);
//server.print(myFile);

P(remote2) =
"'></iframe></div><div class='ui-block-b'><form action='/' method='post' data-ajax='false'><div data-role='fieldcontain'>"
"<div data-role='controlgroup' data-type='horizontal'><legend>Lampe Salon</legend> <button name='remote' value='11' data-icon='check'>On<button name='remote' value='51' data-icon='delete'>Off</button></button>"
"</div></div><div data-role='fieldcontain'><div data-role='controlgroup' data-type='horizontal'><legend>Lampe 2</legend><button name='remote' value='61' data-icon='check'>On<button name='remote' value='20' data-icon='delete'>Off</button></button>"
"</div></div><div data-role='fieldcontain'><div data-role='controlgroup' data-type='horizontal'><legend>Distributeur croquettes</legend> <button name='remote' value='70' data-icon='check'>Go!</button></div>"
"Last feed on "
"<fieldset>"
/*"  <div data-role='fieldcontain'>"
"    <label for='checkbox-based-flipswitch'>Checkbox-based:</label>"
"    <select id='flip1' data-role='flipswitch'>"
"      <option value='76'>On</option>"
"      <option value='75'>Off</option>"
"    </select>"
"  </div>"
"</fieldset>"*/


;
server.printP(remote2);

server.print(day());
server.print("d");
server.print(month());
server.print("m");
server.print(hour());
server.print("h");
server.print(minute());
server.print("m");
server.print(second());
server.print("s");

//myFile = SD.open("index3.txt",FILE_WRITE);
//server.print(myFile);

/*server.print("");
server.print(getNtpMinute());
server.print(getNtpHour());*/
//server.print(day().' '.month().' '.year().' '.hour().' '.minute());
P(remote4) =
"</div></form></div></fieldset></div><div data-role='collapsible' data-mini='true'><h5>T&eacute;l&eacute;commande</h5><form action='/' method='post' data-ajax='false'>"
"<div data-role='fieldcontain'><div align='right'><div data-role='controlgroup' data-type='horizontal'><button name='remote' value='17' data-mini='true' data-theme='a'>Mute"
"<button name='remote' value='16' data-theme='a' data-mini='true'>Power</button></button></div>"
"</div><div data-role='controlgroup' data-type='horizontal'><button name='remote' value='1'>1 <button name='remote' value='2'>2<button name='remote' value='3'>3</button></button></button>"
"</div><div data-role='controlgroup' data-type='horizontal'><button name='remote' value='4'>4 <button name='remote' value='5'>5<button name='remote' value='6'>6</button></button></button>"
"</div><div data-role='controlgroup' data-type='horizontal'><button name='remote' value='7'>7 <button name='remote' value='8'>8<button name='remote' value='9'>9</button></button></button>"
"</div><div data-role='controlgroup' data-type='horizontal'><button name='remote' value='10'>0</button></div><div data-role='controlgroup' data-type='horizontal'>"
"<button name='remote' value='13' data-theme='b' data-icon='minus' data-iconpos='right'>Volume<button name='remote' value='12' data-theme='b' data-icon='plus' data-iconpos='right'>Volume</button></button>"
"</div><div data-role='controlgroup' data-type='horizontal'><button name='remote' value='15' data-theme='b' data-icon='minus' data-iconpos='right'>Chaine &nbsp; <button name='remote' value='14' data-theme='b' data-icon='plus' data-iconpos='right'>Chaine&nbsp;</button></button>"
"</div></div></form></div></div></div><div data-role='footer' data-position='fixed'><div data-role='navbar' data-iconpos='top'>"
"<ul><li><a href='#' data-icon='home' class='ui-btn-active'>Controle</a></li><li><a href='timer.html' data-icon='gear' data-transition='slide'>Timer</a></li></ul>"
"</div></div></div>;</body></html>";   

server.printP(remote4);
//server.printP(remote2);

  }
}
 
void timerCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
 
  server.httpSuccess();
 
 
}
 
 
// Send code to remote socket switch
boolean sendCode(char code[]){
  digitalWrite(vc_pin, HIGH);
  for(short z = 0; z<6; z++){
    for(short i = 0; i<24; i++){
      sendByte(code[i]);
    }
    sendByte('x');
  }
  digitalWrite(vc_pin, LOW);
  return true;
}
 
// Send byte to remote socket switch
void sendByte(char i) {
  switch(i){
  case '0':
    {
    
      return;
    }
  case '1':
    {
    /*  digitalWrite(rc_pin,HIGH);
      delayMicroseconds(500);
      digitalWrite(rc_pin,LOW);
      delayMicroseconds(1000);*/
      return;
    }
  case 'x':
    {
    /*  digitalWrite(rc_pin,HIGH);
      delayMicroseconds(3000);
      digitalWrite(rc_pin,LOW);
      delayMicroseconds(7000);*/
    }
  }
}


 void setup()
{
  Serial.begin(9600);
 
  pinMode(rc_pin, OUTPUT);
  pinMode(vc_pin, OUTPUT);
 
  getTemperature(&temp);
 
  vw_set_tx_pin(RF_TX_PIN); // Setup transmit pin
  vw_setup(2000); // Transmission speed in bits per second.
  
  vw_set_rx_pin(RF_RX_PIN);  // Setup receive pin.
  vw_setup(2000); // Transmission speed in bits per second.
  vw_rx_start(); // Start the PLL receiver.
  
  Ethernet.begin(mac, ip);
 
  // Start Tcp server
  server.begin();
  Serial.println("Server started");//log
  
  // Activate WebServer and register 3 websites
  webserver.setDefaultCommand(&remoteCmd);
  webserver.addCommand("remote.html", &remoteCmd);
  webserver.addCommand("timer.html", &timerCmd);
 // webserver.addCommand("options.html", &optionsCmd);
 
  webserver.begin();
 
  Udp.begin(localPort);

  //epoch = getNTP();

  //setTime(getNtpHour(), getNtpMinute(), getNtpSecond(), 1, 1, 11);
 // checkTimer();
  
}

// Convert String to int
int stringToInt(String str) {
  char this_char[str.length()+1];
  str.toCharArray(this_char, sizeof(this_char));
  return atoi(this_char);
}



// Get time from NTP-Server
unsigned long getNTP() {
  sendNTPpacket(timeServer); // send an NTP packet to a time server
 
  delay(1000);
 
  if ( Udp.available() ) {  
    Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer
 
    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
 
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;  
 
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     
    // subtract seventy years + one hour (GMT+1)
    // unsigned long epoch = secsSince1900 - seventyYears;
    return secsSince1900 - seventyYears + 3600;
  }
}    

// send an NTP request to the time server at the given address 
unsigned long sendNTPpacket(byte *address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE); 
  // Initialize values needed to form NTP request
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
 
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp: 		   
 // Udp.write( packetBuffer,NTP_PACKET_SIZE,  address, 123); //NTP requests are to port 123
}


void loop()
{
  
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;
  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
    int i;

    for (i = 0; i < buflen; i++)
    {
      //  Serial.print(buf[i], HEX);
       // Serial.print((char)buf[i]);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     
        inChar = (char)buf[i]; // Read a character
        inData[index] = inChar; // Store it
        index++; // Increment where to write next
        
    }
    inData[index] = '\0'; // Null terminate the string
    
    char comp2[20] = {'f','e','e','d',' ','o','k','\0'};
    Serial.print(inData);
    if (strcmp(inData,comp2)  == 0) {
      Serial.print(inData);
      Serial.println("YEAH!");      
    }
    for (i = 0; i < 21; i++)
    {
      inData[i] = ' ';
    }
    index = 0;    
  }

  
  getNTP();
  webserver.processConnection();
 //asm volatile ("  jmp 0"); 
  String outMessage = ""; 
  int i=0;
  
  if (NTPUpdate == true) {
    epoch = getNTP();
    Serial.println(epoch);
   // setTime(getNtpHour(), getNtpMinute(), getNtpSecond(), 1, 1, 11);
    NTPUpdate == false;
  }
   
  
  cou++;  
  //Serial.println(cou);
  if (cou > 32000) {  // 10000 = 3sec
     coupleTimes++;
     cou=-32000;
    
  }
  
  // Read TCP connexions
    EthernetClient client = server.available();
    if (client) {
      
      String clientMsg ="";
      
      while (client.connected()) {
        if (client.available()) {
          i++;
          char c = client.read();
         
          //read char by char HTTP request
        if (outMessage.length() < 100) {
          if (i > 3) {       
             if (c!='\0'){
              outMessage.concat(c);
             }                             
          }
                      
         //  Serial.println(outMessage);
           if (outMessage  == "101") {
              Serial.println("Sentss:  " + outMessage); // see in Serial Monitor
              const char *msg = "light on";
              vw_send((uint8_t *)msg, strlen(msg));
          }
          if (outMessage  == "102") {
              const char *msg = "light off";
              vw_send((uint8_t *)msg, strlen(msg));
          }
          if (outMessage  == "103") {
            Serial.println("Sentss:  " + outMessage); // see in Serial Monitor
            const char *msg = "feed cat";
             vw_send((uint8_t *)msg, strlen(msg));
            
          }
        } 
          
          //if the character is an "end of line" the whole message is recieved
          if (c == '&') {
            //Serial.println("Message from Client:"+clientMsg+":"+outMessage);//print it to the serial
            
            Serial.println("read:  " + outMessage); // see in Serial Monitor
            
            
            client.println("You said:"+outMessage);//modify the string and send it back
            clientMsg="";
             
          }
        }
    }
   // Serial.println("finality :"+outMessage);
  
    // give the Client time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    outMessage="";
    }
    /* if (!client.connected()) {
        Serial.println();
        Serial.println("disconnecting.");
        client.stop();
    
        // do nothing forevermore:
        for(;;)
          ;
      }*/
     
}

