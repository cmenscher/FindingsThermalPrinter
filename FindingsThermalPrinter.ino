// Libraries, Constants, and Globals ----------------------------------------

#include <WiServer.h>
#include <SoftwareSerial.h>
#include <Adafruit_Thermal.h>

#define WIRELESS_MODE_INFRA	1
#define WIRELESS_MODE_ADHOC	2
//#define SSID "Betaworks"
//#define SSID_PASSWORD "betawork$416"
#define SSID "Intertrode"
#define SSID_PASSWORD "makisup"
#define ledPin 13
#define printSwitchPin 8
#define printer_RX_Pin 2
#define printer_TX_pin 3

unsigned long updateInterval = 60000;  // delay between updates, in milliseconds
unsigned long lastConnectionTime = 0;          // last time you connected to the server, in milliseconds
int switchVal = 0;
boolean initialRun = true;

//---------------------------------------------------------------------------



// Printer configuration parameters ----------------------------------------

Adafruit_Thermal printer(printer_RX_Pin, printer_TX_pin);

//---------------------------------------------------------------------------



// Wireless configuration parameters ----------------------------------------
unsigned char local_ip[] = {
  192,168,116,225};	// IP address of WiShield
unsigned char gateway_ip[] = {
  192,168,116,1};	// router or gateway IP address
unsigned char subnet_mask[] = {
  255,255,255,0};	// subnet mask for the local network
const prog_char ssid[] PROGMEM = {
  SSID};
unsigned char security_type = 3;	// 0 - open; 1 - WEP; 2 - WPA; 3 - WPA2

// WPA/WPA2 passphrase
const prog_char security_passphrase[] PROGMEM = {
  SSID_PASSWORD};	// max 64 characters

// WEP 128-bit keys
// sample HEX keys
prog_uchar wep_keys[] PROGMEM = {	
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,	// Key 0
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00,	// Key 1
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00,	// Key 2
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00	// Key 3
};

// setup the wireless mode
// infrastructure - connect to AP
// adhoc - connect to another WiFi device
unsigned char wireless_mode = WIRELESS_MODE_INFRA;
unsigned char ssid_len;
unsigned char security_passphrase_len;
//---------------------------------------------------------------------------



// Get the Findings stream---------------------------------------------------

//Production server
uint8 findings_ip[] = {50,16,203,11};

//local dev
//uint8 findings_ip[] = {192,168,116,163};

//Get streams!
//GETrequest getStream(findings_ip, 80, "findings.com", "/api/v1/last.stream");
GETrequest getLastID(findings_ip, 80, "findings.com", "/api/v1/last.stream?type=id");
GETrequest getContent(findings_ip, 80, "findings.com", "/api/v1/last.stream?type=content");
GETrequest getTitle(findings_ip, 80, "findings.com", "/api/v1/last.stream?type=title");
GETrequest getAuthor(findings_ip, 80, "findings.com", "/api/v1/last.stream?type=author");
GETrequest getUser(findings_ip, 80, "findings.com", "/api/v1/last.stream?type=user");
//---------------------------------------------------------------------------




// Handle/parse the stream data----------------------------------------------
boolean printing = false;
boolean inClip = false;
boolean skip = true;
char lastChar = 0;
String lastID = "";
String thisID = "";

void handleLastID(char* data, int len) {
  char currChar;
  //Serial.println("***ID***");
  if(len == 0) {
    if(thisID != lastID) {
     Serial.print("\nNew quote! ");
     Serial.print("(");
     Serial.print(thisID);
     Serial.print(" != ");
     Serial.print(lastID);
     Serial.print(") ");
     Serial.println("Getting content...");
     printer.print("--------------------------------");

     getContent.submit();
    } else {
     Serial.print("Duplicate quote! ");
     Serial.print("(");
     Serial.print(thisID);
     Serial.print(" == ");
     Serial.print(lastID);
     Serial.print(") ");
     Serial.println("Skipping print...");
    }
    lastID = thisID;
    thisID = "";
    clipReset();
    Serial.print("\n");
  } else {
    while (len-- > 0) {
      currChar = *(data++);

      if(currChar == '|' && lastChar == '|') {
        //Serial.println("END OF HEADER!");
        inClip = true;
        skip = true;
      }
      
      if(inClip) {
        if(!skip) {
          thisID += currChar;
        } else {
          skip = false;
        }
      }
      lastChar = currChar;
    }
  }
}


void printContent(char* data, int len) {
  char currChar;
  //Serial.println("\n***CONTENT***");
  if(len == 0) {
    Serial.print("\n");
    printer.print("\n");
    printer.underlineOn(); //turn on underlining for title
    delay(500); //try to keep the printer buffer from filling up
    clipReset();
    getTitle.submit();
  } else {
    printing = true;
    while (len-- > 0) {
      currChar = *(data++);

      if(currChar == '|' && lastChar == '|') {
        //Serial.println("END OF HEADER!");
        inClip = true;
        skip = true;
      }
      
      if(inClip) {
        if(!skip) {
          Serial.print(currChar);
          printer.justify('L');
          printer.print(currChar);
        } else {
          skip = false;
        }
      }
      lastChar = currChar;
    }
  }
}

void printTitle(char* data, int len) {
  char currChar;
  //Serial.println("\n***TITLE***");

  if(len == 0) {
    Serial.print("\n");
    printer.underlineOff(); //turn off underlining
    printer.print("\n");
    delay(250); //try to keep the printer buffer from filling up
    clipReset();
    getAuthor.submit();
  } else {
    while (len-- > 0) {
      currChar = *(data++);
      
      if(currChar == '|' && lastChar == '|') {
        //Serial.println("END OF HEADER!");
        inClip = true;
        skip = true;
      }
      
      if(inClip) {
        if(!skip) {
          Serial.print(currChar);
          printer.justify('R');
          printer.print(currChar);
        } else {
          skip = false;
        }
      }
      lastChar = currChar;
    }
  }
}

void printAuthor(char* data, int len) {
  char currChar;
  //Serial.println("\n***AUTHOR***");
  
  if(len == 0) {
    Serial.print("\n");
    printer.print("\n");
    delay(250); //try to keep the printer buffer from filling up
    clipReset();
    getUser.submit();
  } else {
    while (len-- > 0) {
      currChar = *(data++);
      
      if(currChar == '|' && lastChar == '|') {
        //Serial.println("END OF HEADER!");
        inClip = true;
        skip = true;
      }
      
      if(inClip) {
        if(!skip) {
          Serial.print(currChar);
          printer.justify('R');
          printer.print(currChar);
        } else {
          skip = false;
        }
      }
      lastChar = currChar;
    }
  }
}

void printUser(char* data, int len) {
  char currChar;
  //Serial.println("\n***USER***");
  
  if(len == 0) {
    Serial.print("\n");
    printer.print("\n\n\n");
    delay(250); //try to keep the printer buffer from filling up
    clipReset();
    printReset();
  } else {
    while (len-- > 0) {
      currChar = *(data++);
      
      if(currChar == '|' && lastChar == '|') {
        //Serial.println("END OF HEADER!");
        inClip = true;
        skip = true;
      }
      
      if(inClip) {
        if(!skip) {
          Serial.print(currChar);
          printer.justify('R');
          printer.print(currChar);
        } else {
          skip = false;
        }
      }
      lastChar = currChar;
    }
  }
}


// A little testing function
void doNothing(char* data, int len) {
  Serial.println("ain't doin' nothin'...");
  clipReset();
  printReset();
}

void clipReset() {
  inClip = false;
  skip = false;
  lastChar = 0;
  //printer.flush();
}

void printReset() {
  printer.print("--------------------------------");
  printer.print("\n\n\n");
  printing = false;
  uip_close();
  Serial.println("\n\nWaiting for next quote...");
//  Serial.print("lastConnectionTime=");
//  Serial.print(lastConnectionTime);
//  Serial.print(" updateInterval=");
//  Serial.println(updateInterval);
}

//---------------------------------------------------------------------------



// RUN!----------------------------------------------------------------------
void setup()
{
  // start serial port:
  Serial.begin(57600);
  freeMem("\n\nFree mem at boot (bytes)");
  Serial.print("Update Interval: ");
  Serial.println(updateInterval);

  printer.begin();
  printer.justify('L');
  //printer.println("Findings Quote Printer is GO!");
  
//  printer.println("\"Early modern Englishmen read in fits and starts and jumped from book to book.\nThey broke texts into fragments and assembled them into new patterns by transcribing\nthem in different sections of their notebooks.\nThen they reread the copies and rearranged the patterns while adding more excerpts.\nReading and writing were therefore inseparable activities.\"");
//  printer.setAbsoluteSize(8);
//  printer.justify('R');
//  printer.println("The Case for Books");
//  printer.println("Robert Darton");
  printer.println("\n\n\n");
  printer.flush();
  //printer.upsideDownOn();

  // give the ethernet module time to boot up:
  delay(1000);
  Serial.println("Initializing WiFi...");
  Serial.print("Looking for SSID: ");
  Serial.println(SSID);

  WiServer.init(NULL);
  WiServer.enableVerboseMode(false);

  getLastID.setReturnFunc(handleLastID);
  getContent.setReturnFunc(printContent);
  getTitle.setReturnFunc(printTitle);
  getAuthor.setReturnFunc(printAuthor);
  getUser.setReturnFunc(printUser);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  pinMode(printSwitchPin, INPUT);

  Serial.println("Ready...........FIGHT!\n\n\n");
}

void loop() {
  switchVal = digitalRead(printSwitchPin);

  if(switchVal == HIGH) {
    if(printing == false) {
      Serial.println("Manual print activated! Getting Findings feed...");
      
      //get the content, not the ID...we're forcing the print of the last quote regardless
      getContent.submit();
      lastConnectionTime = millis();
      if(initialRun) {
        initialRun = false;
      }
    }
  } else if (initialRun || (millis() - lastConnectionTime > updateInterval)) {
    if(printing == false) {
      Serial.println("Update interval exceeded!  Checking last quote ID...");
      getLastID.submit();
  
      lastConnectionTime = millis();
      if(initialRun) {
        initialRun = false;
      }
    }
  }

  // Run WiServer
  WiServer.server_task();

  delay(10);
}
//---------------------------------------------------------------------------


