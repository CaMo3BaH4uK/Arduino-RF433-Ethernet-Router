#include <EtherCard.h>

static byte MAC[] = { 0xDE,0xAD,0xC0,0xDE,0x00,0x00 };

byte Ethernet::buffer[500];
BufferFiller bfill;

void setup (){
  Serial.begin(57600);
  Serial.println(F("\n[Radio-Ethernet router]"));

  Serial.print("MAC: ");
  for (byte i = 0; i < 6; ++i) {
    Serial.print(MAC[i], HEX);
    if (i < 5)
      Serial.print(':');
  }
  Serial.println();

  if (ether.begin(sizeof Ethernet::buffer, MAC, SS) == 0)
    Serial.println(F("Failed to access Ethernet controller"));

  Serial.println(F("Setting up DHCP"));
  if (!ether.dhcpSetup())
    Serial.println(F("DHCP failed"));

  ether.printIp("My IP: ", ether.myip);
  ether.printIp("Netmask: ", ether.netmask);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);
}

static word returnData() {
  long t = millis() / 1000;
  word h = t / 3600;
  byte m = (t / 60) % 60;
  byte s = t % 60;
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
    "$D$D:$D$D:$D$D"),
    h/10, h%10, m/10, m%10, s/10, s%10);
  return bfill.position();
}

static word badRequest() {
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
    "HTTP/1.0 400 Bad Request\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
    "<h1>400 Bad Request</h1>"));
  return bfill.position();
}

void loop (){
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  if (pos){
    bfill = ether.tcpOffset();
    char* data = (char*) Ethernet::buffer + pos;
    if (strncmp("GET /?", data, 6) == 0){
      int headerPos = strstr(data, " HTTP")-data;
      int queryStartPos = strstr(data, " ")-data;
      int queryEndPos = strstr(data, "?")-data;
      data[headerPos] = 0;
      for (int i = queryStartPos; i < queryEndPos+1; i++){
        data[i] = 32;
      }
      int dataLen = strlen(data);
      int args = 1;
      int lastSpace = 0;
      for (int i = 0; i < dataLen; i++){
        if (data[i] == '&'){
          args++;
        }
        if (data[i] == ' '){
          lastSpace = i;
        }
      }
      if (args == 2) {
        memcpy(data, data+lastSpace+1, dataLen - lastSpace);
        dataLen = strlen(data);
        int delPos = strstr(data, "&")-data;
        int portIndex = strstr(data, "port=");
        int dataIndex = strstr(data, "data=");
        if (portIndex && dataIndex) {
          data[delPos] = 0;
          int port = atoi((char *)(portIndex+5));
          /*
           * Serial.println(port);
           * Serial.println(strlen(dataIndex+5));
           */
          ether.httpServerReply(returnData()); 
        } else {
          ether.httpServerReply(badRequest()); 
        }
      } else {
        ether.httpServerReply(badRequest()); 
      }
    } else {
      ether.httpServerReply(badRequest()); 
    }
  }
}
