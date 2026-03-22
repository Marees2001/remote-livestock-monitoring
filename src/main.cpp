#include <Arduino.h>
#include "controller.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// I2C address (try 0x3C if not worki
TwoWire i2c(MCU_SDA_Pin,MCU_SCL_Pin);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &i2c, -1);

char gsm_rx_buffer[200]; // Buffer for receiving data from GSM module
uint8_t receive_bytes = 0;

void setup() {
  // Initialize USB Serial (CDC) for USB debugging
  Serial.begin(9600);
  delay(2000); // Wait for USB enumeration
  
  // Initialize USART1 for GSM module
  GSM_SERIAL.begin(9600);
  delay(500);

  // Initialize LED pin
  pinMode(STATUS_LED_Pin, OUTPUT);
  digitalWrite(STATUS_LED_Pin, HIGH); // LED off (inverted logic on Blue Pill)

  // put your setup code here, to run once:
  Serial.println("\n\n=== IoT Livestock Monitoring System ===");
  Serial.println("System initialized successfully");
  // delay(5000);
  i2c.begin();  // SDA, SCL (VERY IMPORTANT)
  i2c.setClock(100000);  // 100kHz

  Serial.println("Initializing OLED...");
  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 init failed!");
    while (1);
  }
  Serial.println("OLED Initialized!");

  // Clear screen
  display.clearDisplay();   // Clear buffer
  display.setCursor(0, 0);  // Reset cursor
  display.print("Hello");
  display.display();        // Push to OLED

  // Serial.println("I2C Scanner");
  // byte error, address;
  // for(address = 1; address < 127; address++) {
  //   i2c.beginTransmission(address);
  //   error = i2c.endTransmission();
  //   Serial.printf("I2C scane aadr-%x\n\r",address);
  //   if (error == 0) {
  //     Serial.print("Found at 0x");
  //     Serial.println(address, HEX);
  //   }
  //   delay(1000);
  // }
  // Serial.printf("Scan completed...\n\r");
  

  
}

void loop() {
 
  static unsigned long gsm_sync_timeout = 0; // Variable to track GSM synchronization timing
  static unsigned long sensor_read_timeout = 0; // Variable to track sensor reading timing
  static unsigned long display_update_timeout = 0; // Variable to track display update timing
  /*Function for receiving data from GSM module*/
  //gsm_receive_data();

  if(millis() - gsm_sync_timeout > 2000) { // Check if 0.5 seconds have passed since the last GSM synchronization
    gsm_sync_timeout = millis(); // Update the last synchronization time
    // Toggle LED state using STM32 HAL
    //digitalToggle(STATUS_LED_Pin);      // Push to OLED
    if(rlm.sta.init_gsm != 0xFF) { // Sync GSM every 0.1 seconds
      //sync_gsm();
    }else { // If GSM is fully initialized, perform main operations
      // Main operations here (e.g., read sensors, update display, control actuators)
      printf("GSM fully initialized\n\r");
    }
  }
  


  // // put your main code here, to run repeatedly:
  // if (Serial.available()) {
  //   char usbChar[200];
  //   Serial.readBytes(usbChar, sizeof(usbChar));
  //   Serial.print("USB received: ");
  //   Serial.println(usbChar);
  //   // Forward USB data to GSM
  //   GSM_SERIAL.print(usbChar);
  //   GSM_SERIAL.print("\r\n");
  //   Serial.println("Sending AT command to GSM...");
  // }
}
