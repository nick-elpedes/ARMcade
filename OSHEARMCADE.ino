#include <SPI.h>
#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_ILI9341.h>     // Hardware-specific library
#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <Adafruit_ImageReader.h> // Image-reading functions


// Pins on the Nucleo   (USE arduino header, NOT the ST-Morpho!)
#define TFT_CLK PA_5    //D13
#define TFT_MISO PA_6   //D12
#define TFT_MOSI PA_7   //D11
#define TFT_CS PB_6     //D10
#define TFT_DC PC_7     //D9
#define TFT_RST PA_9    //D8
#define NES_DATA PA_8   //D7
#define NES_LATCH PB_10 //D6
#define NES_CLOCK PB_4  //D5
#define tonePin PB_3    //D3
#define SD_CS PA_10     //D2

// GLOBALS

SdFat SD;         // SD card filesystem
Adafruit_ImageReader reader(SD); // Image-reader object, pass in SD filesys

// define screen using hardware SPI (D11,D12,D13)
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_Image img;
//int32_t width = 0, height = 0;

// game flag
bool game = false;
byte NESButtonData; // byte to hold nes button states
byte PrevNESButtonData = 0b00000000; // byte to hold nes button states for previous update

void setup() {
  // define buzzer
  pinMode(tonePin, OUTPUT);
  noTone(tonePin);

  ImageReturnCode stat; // Status from image-reading functions

  Serial.begin(9600);
  while(!Serial);       // Wait for Serial Monitor before continuing
  // init pins for controller
  pinMode(NES_LATCH, OUTPUT);
  pinMode(NES_CLOCK, OUTPUT);
  pinMode(NES_DATA, INPUT);

  // force NES out low
  digitalWrite(NES_CLOCK, LOW);
  digitalWrite(NES_LATCH, LOW);

  // startup sequence
  // init screen
  tft.begin();

  // init files
  Serial.print(F("Initializing filesystem..."));
  if(!SD.begin(SD_CS, SD_SCK_MHZ(25))) { // ESP32 requires 25 MHz limit
    Serial.println(F("SD begin() failed"));
    // display error text
    tft.setCursor(0, 0);
    tft.setTextSize(3);
    tft.setTextColor(ILI9341_RED);
    tft.println("SD CARD ERROR");
    tone(tonePin, 1000); // evil wii crash noise gif
    for(;;); // Fatal error, do not continue
  }
  Serial.println(F("OK!"));
  tft.setRotation(1); // make sure we are in landscape for boot screen and controller screen

  // clear screen
  tft.fillScreen(ILI9341_BLACK);

  // beep on startup
  tone(tonePin, 1000, 1000);

  // display OSHE logo for a few seconds
  Serial.print(F("Loading the logo..."));
  stat = reader.drawBMP("/logo.bmp", tft, 40, 0);
  reader.printStatus(stat);

  // wait a bit
  delay(2000);

  // check for game (todo)
  // Leaving false for now since game detection/format does not exist
  // good luck Shane
  if(false) { // some check for if game is inserted
    game = true;
    // code to boot into game
  } else {
    game = false;

    tft.fillScreen(ILI9341_BLACK);
    Serial.println(F("No game file present, showing default screen"));

    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(0,0);
    tft.setTextSize(2);
    tft.println("No game file present");
    tft.setTextSize(1);
    tft.setTextWrap(true);
    tft.println("Please flash the Nucleo-64 board with the game code and the TFT SD card with game assets to continue.");
    tft.println("Wait until the tone finishes to enter the controller test menu");
    midi(); // funny but can be removed and replaced with a delay. This is here as a proof of concept for different tones for project spec.
    tft.fillScreen(ILI9341_BLACK); // reset screen
    drawScreen(); // draw the controller screen in the setup code so we don't redraw the screen every loop
  }

}

// loop contains the code for the controller test menu.
void loop() {
  GetNESControllerData();
  updateMismatches();
}

// NOTE: to set cursor to a specific position, characters are 6x8 pixels
// (Multiplied by 2 for readability), so make sure to account for that
// when using set cursor.

// draw initial screen
void drawScreen() {
  // set desired text settings
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  // start drawing at top left of screen
  tft.setCursor(0, 0);
  tft.write(0xC8); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xBA); tft.println();
  tft.write(0xB9); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x18); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0xB9); tft.println();
  tft.write(0xB9); tft.write(0x20); tft.write(0x1B); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x1A); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x42); tft.write(0x20); tft.write(0x20); tft.write(0x41); tft.write(0x20); tft.write(0x20); tft.write(0xB9); tft.println();
  tft.write(0xB9); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x19); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x16); tft.write(0x20); tft.write(0x20); tft.write(0x16); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0x20); tft.write(0xB9); tft.println();
  tft.write(0xC7); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xCC); tft.write(0xBB); tft.println();
  // tft.println("╔════════════════════╗");
  // tft.println("║   ↑   TEST MENU    ║");
  // tft.println("║ ←   →        B  A  ║");
  // tft.println("║   ↓    ▬  ▬        ║");
  // tft.println("╚════════════════════╝");
}

// NESButtonData bits correspond to RLDUSSBA
// update all visual mismatches from the last controller state to this controller state
// This is done to prevent the entire screen from having to be redrawn every cycle (causes flicker)
void updateMismatches() {
  // Make sure text display settings are set to how we want them to be
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);

  // for checking if mismatches exist
  byte mismatches;
  mismatches = NESButtonData ^ PrevNESButtonData;

  // no mismatches, skip visual update
  if(mismatches == 0b00000000) {

  // Had issues with the tone function not respecting the duration setting, so this is to prevent persisting noise
  noTone(tonePin);
  PrevNESButtonData = NESButtonData;
  return; // break if no mismatch
  }

  // play tone on any button pressed
  tone(tonePin, 1000, 10);
  

  // A
  tft.setCursor(216, 32);
  if(checkNthBit(NESButtonData, 0)) {
    tft.write(0x41);
  } else {
    tft.write(0xDA);
  }

  // B
  tft.setCursor(180, 32);
  if(checkNthBit(NESButtonData, 1)) {
    tft.write(0x42);
  } else {
    tft.write(0xDA);
  }


  // Select
  tft.setCursor(108, 48);
  if(checkNthBit(NESButtonData, 2)) {
    tft.write(0x16);
  } else {
    tft.write(0xDA);
  }

  // Start
  tft.setCursor(144, 48);
  if(checkNthBit(NESButtonData, 3)) {
    tft.write(0x16);
  } else {
    tft.write(0xDA);
  }

  // UP
  tft.setCursor(48, 16);
  if(checkNthBit(NESButtonData, 4)) {
    tft.write(0x18);
  } else {
    tft.write(0xDA);
  }

  // DOWN
  tft.setCursor(48, 48);
  if(checkNthBit(NESButtonData, 5)) {
    tft.write(0x19);
  } else {
    tft.write(0xDA);
  }

  // LEFT
  tft.setCursor(24, 32);
  if(checkNthBit(NESButtonData, 6)) {
    tft.write(0x1B);
  } else {
    tft.write(0xDA);
  }

  // RIGHT
  tft.setCursor(72, 32);
  if(checkNthBit(NESButtonData, 7)) {
    tft.write(0x1A);
  } else {
    tft.write(0xDA);
  }

  PrevNESButtonData = NESButtonData;
}



// hehe
void midi() {

    tone(tonePin, 659, 27.8072265625);
    delay(30.8969184028);
    delay(17.6553819444);
    tone(tonePin, 698, 174.78828125);
    delay(194.209201389);
    delay(17.6553819444);
    tone(tonePin, 698, 79.44921875);
    delay(88.2769097222);
    delay(17.6553819444);
    tone(tonePin, 698, 79.44921875);
    delay(88.2769097222);
    delay(123.587673611);
    tone(tonePin, 698, 79.44921875);
    delay(88.2769097222);
    delay(123.587673611);
    tone(tonePin, 698, 174.78828125);
    delay(194.209201389);
    delay(17.6553819444);
    tone(tonePin, 587, 79.44921875);
    delay(88.2769097222);
    delay(123.587673611);
    tone(tonePin, 587, 460.80546875);
    delay(512.006076389);
    delay(17.6553819444);
    tone(tonePin, 698, 174.78828125);
    delay(194.209201389);
    delay(17.6553819444);
    tone(tonePin, 698, 79.44921875);
    delay(88.2769097222);
    delay(17.6553819444);
    tone(tonePin, 698, 79.44921875);
    delay(88.2769097222);
    delay(123.587673611);
    tone(tonePin, 783, 79.44921875);
    delay(88.2769097222);
    delay(123.587673611);
    tone(tonePin, 830, 174.78828125);
    delay(194.209201389);
    delay(17.6553819444);
    tone(tonePin, 783, 79.44921875);
    delay(88.2769097222);
    delay(17.6553819444);
    tone(tonePin, 698, 79.44921875);
    delay(88.2769097222);
    delay(17.6553819444);
    tone(tonePin, 587, 79.44921875);
    delay(88.2769097222);
    delay(17.6553819444);
    tone(tonePin, 698, 79.44921875);
    delay(88.2769097222);
    delay(17.6553819444);
    tone(tonePin, 783, 79.44921875);
    delay(88.2769097222);
    delay(229.519965278);
    tone(tonePin, 698, 174.78828125);
    delay(194.209201389);
    delay(17.6553819444);
    tone(tonePin, 698, 79.44921875);
    delay(88.2769097222);
    delay(17.6553819444);
    tone(tonePin, 698, 79.44921875);
    delay(88.2769097222);
    delay(123.587673611);
    tone(tonePin, 783, 174.78828125);
    delay(194.209201389);
    delay(17.6553819444);
    tone(tonePin, 830, 174.78828125);
    delay(194.209201389);
    delay(17.6553819444);
    tone(tonePin, 880, 174.78828125);
    delay(194.209201389);
    delay(17.6553819444);
    tone(tonePin, 1046, 79.44921875);
    delay(88.2769097222);
    delay(123.587673611);
    tone(tonePin, 880, 174.78828125);
    delay(194.209201389);
    delay(123.587673611);
    tone(tonePin, 1174, 174.78828125);
    delay(194.209201389);
    delay(17.6553819444);
    tone(tonePin, 1174, 79.44921875);
    delay(88.2769097222);
    delay(123.587673611);
    tone(tonePin, 1174, 79.44921875);
    delay(88.2769097222);
    delay(17.6553819444);
    tone(tonePin, 880, 79.44921875);
    delay(88.2769097222);
    delay(17.6553819444);
    tone(tonePin, 1174, 79.44921875);
    delay(88.2769097222);
    delay(17.6553819444);
    tone(tonePin, 1046, 842.16171875);
    delay(935.735243056);
    delay(17.6553819444);
}

void GetNESControllerData(){			// this is where it all happens as far as grabbing the NES control pad data
	digitalWrite(NES_LATCH, HIGH);		// we need to send a clock pulse to the latch (strobe connection)...
  bitWrite(NESButtonData,0,digitalRead(NES_DATA)); // A button fix, apparently pulsing the latch sends the A button signal thru the data line (NKE)
	delayMicroseconds(2);   // this line may not be needed!!!!!!!!!!
  digitalWrite(NES_LATCH, LOW);		// this will cause the status of all eight buttons to get saved within the 4021 chip in the NES control pad.
	for(int x=1; x<=7; x++){			// Now we need to transmit the eight bits of data serially from the NES control pad to the Arduino

		digitalWrite(NES_CLOCK, HIGH);					// once each bit is saved, we send a clock pulse to the NES clock connection...
    delayMicroseconds(2); // this line may not be needed!!!!!!!!!!
		bitWrite(NESButtonData,x,digitalRead(NES_DATA)); // one by one, we will read from the NESData line and store each bit in the NESButtonData variable.
		digitalWrite(NES_CLOCK, LOW);					// this will now shift all bits in the 4021 chip in the NES control pad, so we can read the next bit.
    delayMicroseconds(2); // this line may not be needed!!!!!!!!!!
  }
}

// Returns the state of the selected bit in a byte
bool checkNthBit(byte input, int bit) {
  byte checkBit = NESButtonData >> bit;

  if ((checkBit & 1) != 0) {
    return true;
  }
  return false;
}