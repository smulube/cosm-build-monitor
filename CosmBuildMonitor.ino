/*

This example sketch implements a simple LED "traffic light" for monitoring the
current status of the Cosm continuous integration (CI) build server. It uses
the new Arduino library developed by Adrian McEwen to fetch data from Cosm, and
then uses that data to display the build status via some LEDs hooked up to
vaguely resemble the full size traffic light that does the same job at Cosm HQ.

This example just fetches data from Cosm, then renders that state via our LEDs,
and to make it work requires a feed to be published on Cosm having two
datastreams with ids: 'building' and 'failing', with the following states:

  stream_id | values
  ------------------------------------------------------------------------------
  building  | 0 if nothing currently building, 1 if one or more project is
            | currently being built 
  failing   | 0 if all projects currently passing, 1 if one or more project is
            | currently in a failing state

This example uses the new Cosm client
(https://github.com/amcewen/Cosm-Arduino), which in turn requires the new
HttpClient library (https://github.com/amcewen/HttpClient), both of which
depend on the Ethernet library, which means you'll need at least Arduino 1.0 to
have any joy with this.

We hope that the Cosm-Arduino library is going to be included as part of the
core Arduino distribution, but until then you'll have to install them manually
from the above URLs.

Circuit:

 * Red, green and yellow LEDs attached to pins 5, 6, 7 (with appropriate
   resistor connected in series)
 * Ethernet shield attached to pins 10, 11, 12, 13 (or an Arduino Ethernet)
 
Created: 01/06/2012
by Sam Mulube (using code derived from Adrian McEwen's Cosm-Arduino examples)
*/

#include <SPI.h>
#include <Ethernet.h>
#include <HttpClient.h>
#include <Cosm.h>

// MAC address for your Ethernet shield
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x6A, 0xB8 };

// Set up LED pins for our "traffic light"
int red = 7;
int yellow = 6;
int green = 5;

// Your Cosm key to let you download data - this key needs read only access to the feed
char cosmKey[] = "YOUR_API_KEY";

// The feed containing the build status
long feedId = 61031;

// Define the strings for our datastream IDs - these need to match the published IDs
String buildingId = "building";
String failingId = "failing";

// Our datastreams
CosmDatastream datastreams[] = {
  CosmDatastream(buildingId, DATASTREAM_INT),
  CosmDatastream(failingId, DATASTREAM_INT)
};

// Wrap the datastreams into a feed
CosmFeed feed(feedId, datastreams, 2 /* number of datastreams */);

// Create our Client objects
EthernetClient client;
CosmClient cosmclient(client);

// Some timer stuff to handle blinking and fetching from Cosm
long previousBlinkMillis = 0;
long blinkInterval = 1000;
long previousFetchMillis = 0;
long fetchInterval = 15000;

// Holds the current state of our CI server
int failing = 0;
int building = 0;

// Holds the current state of our LEDs
int redState = LOW;
int yellowState = LOW;
int greenState = LOW;

void setup() {
  // Setup our led output pins
  pinMode(red, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(green, OUTPUT);
  
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  Serial.println("Cosm Build Monitor");
  Serial.println();

  while (Ethernet.begin(mac) != 1) {
    Serial.println("Error getting IP address via DHCP, trying again...");
    delay(15000);
  }
}

void loop() {
  
  unsigned long currentMillis = millis();
  
  // Update LEDs - this means blinking the yellow one if required and
  // making sure the red and green represent the current state of the CI server
  if (currentMillis - previousBlinkMillis > blinkInterval) {
    previousBlinkMillis = currentMillis;
    
    // update yellow LED state (this is the only one that blinks, so we need to toggle the state)
    if (building == 1) {
      if (yellowState == LOW) {
        yellowState = HIGH;
      } else {
        yellowState = LOW;
      }
    } else {
      yellowState = LOW;
    }
    
    // update good/bad LEDs - these don't blink, so this is simpler
    if (failing == 1) {
      redState = HIGH;
      greenState = LOW;
    } else {
      redState = LOW;
      greenState = HIGH;
    }
    
    // write the current state to the LEDs
    digitalWrite(red, redState);
    digitalWrite(yellow, yellowState);
    digitalWrite(green, greenState);
  }
  
  // Attempt to fetch state from Cosm - we probably could have done this all within the previous
  // interval timer, but it seemed simpler to keep them separate
  if (currentMillis - previousFetchMillis > fetchInterval) {
    previousFetchMillis = currentMillis;
    
    // fetch data from Cosm and store state locally
    Serial.println("Fetching data from Cosm");

    int ret = cosmclient.get(feed, cosmKey);
    
    building = feed[0].getInt();
    failing = feed[1].getInt();
 
    // Got data - print out some debug output   
    Serial.print("Building: ");
    Serial.println(building);
    Serial.print("Failing: ");
    Serial.println(failing);
  }
}
