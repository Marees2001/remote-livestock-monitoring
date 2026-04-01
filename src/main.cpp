#include <Arduino.h>
#include "controller.h"

// I2C address (try 0x3C if not worki
HardwareSerial Serial2(USART2);  // PA2 = TX, PA3 = RX
TwoWire i2c(MCU_SDA_Pin,MCU_SCL_Pin);
U8GLIB_SSD1306_128X64 oled(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);
DHT dht(TH_SENSOR_Pin, DHTTYPE);

char gsm_rx_buffer[500]; // Buffer for receiving data from GSM module
uint8_t receive_bytes = 0;
uint8_t receive_req = 0;

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
  Serial.println("\n\n=== Remote Livestock Monitoring System ===");
  Serial.println("System initialized successfully");
  // delay(5000);
  init_controler();

  i2c.begin();  // SDA, SCL (VERY IMPORTANT)
  i2c.setClock(100000);  // 100kHz

  Serial.println("Initializing OLED...");
  // Initialize OLED
  // assign default color value
  if ( oled.getMode() == U8G_MODE_R3G3B2 ) oled.setColorIndex(255);     // white
  else if ( oled.getMode() == U8G_MODE_GRAY2BIT ) oled.setColorIndex(3);         // max intensity
  else if ( oled.getMode() == U8G_MODE_BW ) oled.setColorIndex(1);         // pixel on
  else if ( oled.getMode() == U8G_MODE_HICOLOR )oled.setHiColorByRGB(255,255,255);
  // graphic commands to redraw the complete screen should be placed here  
  oled.setFont(u8g_font_unifont);
  //u8g.setFont(u8g_font_osb21);
  Serial.println("OLED Initialized!");

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
  
  //Initialize T/H sensor
  dht.begin();
  
}

void loop() {
 
  static unsigned long gsm_sync_timeout = 0; // Variable to track GSM synchronization timing
  static unsigned long sensor_read_timeout = 0; // Variable to track sensor reading timing
  static unsigned long display_update_timeout = 0; // Variable to track display update timing
  /*Function for receiving data from GSM module*/
  gsm_receive_data();

  if(millis() - gsm_sync_timeout > 3000) { // Check if 0.5 seconds have passed since the last GSM synchronization
    gsm_sync_timeout = millis(); // Update the last synchronization time
    // Toggle LED state using STM32 HAL
    digitalToggle(STATUS_LED_Pin);      // Push to OLED
    if(rlm.sta.init_gsm != 0xFF) { // Sync GSM every 0.1 seconds
      sync_gsm();
    }else { // If GSM is fully initialized, perform main operations
      // Main operations here (e.g., read sensors, update display, control actuators)
      printf("GSM fully initialized\n\r");
    }
  }
  
  // if(millis() - sensor_read_timeout > 3000){
  //   sensor_read_timeout = millis();
  //   read_th_sensor();
  //   read_tds_value();
  // }

  


  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    char usbChar[200];
    Serial.readBytes(usbChar, sizeof(usbChar));
    Serial.print("USB received: ");
    Serial.println(usbChar);
    // Forward USB data to GSM
    GSM_SERIAL.print(usbChar);
    GSM_SERIAL.print("\r\n");
    Serial.println("Sending AT command to GSM...");
  }

  oled.firstPage();
  do {
    update_display();
  } while (oled.nextPage());  // MCU stuck here for ~15ms
}
