#include <Arduino.h>
#include "controller.h"

// I2C address (try 0x3C if not worki
HardwareSerial Serial2(USART2);  // PA2 = TX, PA3 = RX

HardwareSerial Serial3(USART3);  // PA2 = TX, PA3 = RX

TwoWire i2c(MCU_SDA_Pin,MCU_SCL_Pin);
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &i2c, -1);
DHT dht(TH_SENSOR_Pin, DHTTYPE);

char gsm_rx_buffer[500]; // Buffer for receiving data from GSM module
uint8_t receive_bytes = 0;
uint8_t receive_req = 0;

void setup() {
  // Initialize USB Serial (CDC) for USB debugging
  DEBUG.begin(115200);
  delay(2000); // Wait for USB enumeration
  
  // Initialize USART1 for GSM module
  GSM_SERIAL.begin(9600);
  delay(500);

  // put your setup code here, to run once:
  // delay(5000);
  init_controler();

  i2c.begin();  // SDA, SCL (VERY IMPORTANT)
  i2c.setClock(50000);  // 50kHz

  init_display();

  // DEBUG.println("I2C Scanner");
  // byte error, address;
  // for(address = 1; address < 127; address++) {
  //   i2c.beginTransmission(address);
  //   error = i2c.endTransmission();
  //   Serial.printf("I2C scane aadr-%x\n\r",address);
  //   if (error == 0) {
  //     Serial.print("Found at 0x");
  //     DEBUG.println(address, HEX);
  //   }
  //   delay(1000);
  // }
  // Serial.printf("Scan completed...\n\r");
  
  //Initialize T/H sensor
  dht.begin();
  
}

void loop() {
 
  static unsigned long gsm_sync_timeout = 0; // Variable to track GSM synchronization timing
  static unsigned long th_sensor_timeout = 0; // Variable to track sensor reading timing
  static unsigned long display_update_timeout = 0; // Variable to track display update timing
  static unsigned long init_pulse_sensor_timeout = 0; // Variable to track pulse sensor initialization timing
  static unsigned long display_timout = 0; // Variable to track display update timing
  static unsigned long gsm_call_timeout = 0; 
  static unsigned long gsm_sms_timeout = 0;
  static unsigned long gsm_clean_storage_timeout = 0;
  /*Function for receiving data from GSM module*/
  gsm_receive_data();

  if(millis() - gsm_sync_timeout > 3000) { // Check if 0.5 seconds have passed since the last GSM synchronization
    gsm_sync_timeout = millis(); // Update the last synchronization time
    // Toggle LED state using STM32 HAL
    digitalToggle(STATUS_LED_Pin);      // Push to OLED
    if(rlm.sta.init_gsm != 0xFF) { // Sync GSM every 0.1 seconds
      sync_gsm();
    }
  }

  if(rlm.opt.gsm_sms_ctrl && rlm.sta.init_gsm == 0xFF
  && (millis() - gsm_clean_storage_timeout > 60000)) { // Clean GSM storage every 60 seconds if GSM is fully initialized
    gsm_clean_storage_timeout = millis(); // Update the last storage cleaning time
    clean_sim_storage();
    DEBUG.println("GSM storage cleaned");
  }

  if(rlm.opt.gsm_sms_ctrl && rlm.sta.init_gsm == 0xFF
  && (millis() > gsm_sms_timeout + 5000L)) { // If GSM SMS control is active and 60 seconds have passed since the last SMS attempt
    
    DEBUG.println("Attempting to send GSM SMS... Timout: " + String(millis() - gsm_sms_timeout) + " ms");
    DEBUG.println("GSM SMS control state: " + String(rlm.opt.gsm_sms_ctrl));
    gsm_sms_timeout = millis(); // Update the last SMS attempt timem 
    // Implement GSM SMS control logic here (e.g., send an SMS if not already sent)
    if(rlm.opt.gsm_sms_ctrl==1) {
      rlm.opt.gsm_sms_ctrl = 0; // Reset SMS control to allow future SMS messages
      gsm_sms_timeout = millis() + 120000L; // Update the last SMS attempt time to avoid sending multiple SMS messages in quick succession  
      if(strlen(rlm.conf.home_mob) > 8) { // If home mobile number is configured, send SMS to it first
        char buff[30];
        memset(buff,0,sizeof(buff));
        sprintf(buff,"AT+CMGS=\"%s\"",rlm.conf.home_mob);
        gsm_send_data(buff);
        delay(500);
        gsm_send_dashboard();
        delay(500);
        GSM_SERIAL.write(26); // Send Ctrl+Z to indicate end of SMS message
      }
    }
  }
  
  // DEBUG.println("GSM synchronization completed");
  if(rlm.opt.gsm_call_ctrl && !rlm.opt.gsm_sms_ctrl && rlm.sta.init_gsm == 0xFF
  && (millis() > gsm_call_timeout + 30000L)) { // If GSM call control is active and 30 seconds have passed since the last call attempt
    
    DEBUG.println("Attempting to make GSM call... Timout: " + String(millis() - gsm_call_timeout) + " ms");
    DEBUG.println("GSM call control state: " + String(rlm.opt.gsm_call_ctrl));
    gsm_call_timeout = millis(); // Update the last call attempt time
    // Implement GSM call control logic here (e.g., make a call if not already in progress)
    if(rlm.opt.gsm_call_ctrl == 1) {
      rlm.opt.gsm_call_ctrl = 0; // Reset call control to allow future calls
      gsm_call_timeout = millis() + 120000L; // Update the last call attempt time to avoid making multiple calls in quick succession
      if(strlen(rlm.conf.home_mob) > 8) { // If home mobile number is configured, call it next
        char buff[20];
        memset(buff,0,sizeof(buff));
        sprintf(buff,"ATD%s;",rlm.conf.home_mob);
        gsm_send_data(buff);
      }
      
    }
    
  }

  rlm.opt.lamp_ctrl? digitalWrite(RELAY_LAMP_Pin, LOW):digitalWrite(RELAY_LAMP_Pin, HIGH);
  rlm.opt.fan_ctrl? digitalWrite(RELAY_FAN_Pin, LOW):digitalWrite(RELAY_FAN_Pin, HIGH);
  rlm.opt.fence_ctrl? digitalWrite(RELAY_FENCE_Pin, LOW):digitalWrite(RELAY_FENCE_Pin, HIGH);
  rlm.opt.valve_ctrl? digitalWrite(RELAY_VALVE_Pin, LOW):digitalWrite(RELAY_VALVE_Pin, HIGH);
  
  if(millis() - th_sensor_timeout > 2000){
    th_sensor_timeout = millis();
    read_th_sensor();
    read_tds_value();
  }


  if(millis() > display_update_timeout + 3000){
    display_update_timeout = millis();
    update_display();
  }


  // put your main code here, to run repeatedly:
  if (DEBUG.available()) {
    char usbChar[200];
    DEBUG.readBytes(usbChar, sizeof(usbChar));
    DEBUG.print("TTL received: ");
    DEBUG.println(usbChar);
    // Forward USB data to GSM
    GSM_SERIAL.print(usbChar);
    GSM_SERIAL.print("\r\n");
    DEBUG.println("Sending AT command to GSM...");
  }
}
