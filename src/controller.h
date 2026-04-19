
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>
#include <stdio.h>
#include <Arduino.h>
#include "define.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "flash.h"
#include "DHT.h"

extern DHT dht;
extern char gsm_rx_buffer[500]; // Buffer for receiving data from GSM module
extern uint8_t receive_bytes; // Number of bytes received from GSM module
extern uint8_t receive_req;

/*Structure for controller status*/
struct CONTROLLER_STATUS {
    uint8_t init_gsm; // GSM initialization status by bitread operations: 0: GSM module initialized,
    //1: SIM card ready, 2: UART configured, 3: RSSI received,4: set SMS mode, 5: set SMS recipient
    float tds_value; // TDS sensor value
    float temp_value; // Temperature sensor value
    uint8_t hum_value; // Humidity sensor value
    uint8_t thi;
    uint8_t rssi; // GSM signal strength (0-31)
    uint8_t sim_status; // SIM card status (0: not inserted, 1: ready)
    uint8_t alm_sta; // Alarm status (0 or 1)
    uint8_t lam_status; // Lamp status (0 or 1)
    uint8_t fan_status; // Fan status (0 or 1)
    uint8_t fence_status; // Fence status (0 or 1)
    uint8_t lst_msg_idx;
    uint8_t temp_alarm; // 0: no alarm, 1: temperature out of range
    uint8_t hum_alarm; // 0: no alarm, 1: humidity out
    uint8_t tds_alarm; // 0: no alarm, 1: TDS out of range
};

/*Structure for controller settings*/
struct CONTROLLER_SETTINGS {
    uint32_t magic;
    char home_mob[15]; // Home mobile number for GSM communication
    float mx_temp; // Max threshold for temperature
    float mn_temp; // Min threshold for temperature
    uint8_t mx_hum; // Max threshold for humidity
    uint8_t mn_hum; // Min threshold for humidity
    int mx_tds; // Max threshold for TDS; 
    //range 0-1000 ppm, where 0-300 ppm is considered good, 300-600 ppm is fair, and above 600 ppm is poor water quality
    int mn_tds; // Min threshold for TDS
};

/*Structure for controller operation*/
struct CONTROLLER_OPERATION {
    uint8_t lamp_ctrl; // Lamp control (0 or 1)
    uint8_t fan_ctrl; // Fan control (0 or 1)
    uint8_t fence_ctrl; // Fence control (0 or 1) 
    uint8_t alarm_ctrl; // Alarm control (0 or 1)
    uint8_t valve_ctrl; // Valve control (0 or 1)
    uint8_t gsm_call_ctrl; // GSM call control (0 or 1)
    uint8_t gsm_sms_ctrl; // GSM SMS control (0 or 1)
    char lst_mob_req[15];
};

struct REMOTE_LIVESTOCK_MONITORING {
    struct CONTROLLER_STATUS sta; // Current status of the controller
    struct CONTROLLER_SETTINGS conf; // Current settings of the controller
    struct CONTROLLER_OPERATION opt; // Current operation of the controller
}rlm;

void read_th_sensor(){
    rlm.sta.temp_value = dht.readTemperature();
    // Read temperature as Celsius (the default)
    rlm.sta.hum_value = dht.readHumidity();

    rlm.sta.thi = (1.8 * rlm.sta.temp_value + 32) - ((0.55 - 0.0055 * rlm.sta.hum_value ) 
    * (1.8 * rlm.sta.temp_value - 26)); 

    Serial.println("Tempratur : " + String(rlm.sta.temp_value) + "C");
    Serial.println("Humitidy : " + String(rlm.sta.hum_value) + "%");
    Serial.println("THI: " + String(rlm.sta.thi));

    if((rlm.sta.temp_value > rlm.conf.mx_temp || rlm.sta.temp_value < rlm.conf.mn_temp)
    && rlm.sta.temp_value != 0) {
        rlm.opt.alarm_ctrl = 1; // Activate alarm control
        if(!rlm.opt.gsm_call_ctrl) rlm.opt.gsm_call_ctrl = 1; // Activate GSM call control
        if(!rlm.opt.gsm_sms_ctrl)  rlm.opt.gsm_sms_ctrl = 1; // Activate GSM SMS control
        rlm.sta.temp_alarm = 1; // Set temperature alarm status
        Serial.println("Temperature out of range! Alarm activated.");
    } else if((rlm.sta.hum_value > rlm.conf.mx_hum || rlm.sta.hum_value < rlm.conf.mn_hum)
    && rlm.sta.hum_value != 0) {
        rlm.opt.alarm_ctrl = 1; // Activate alarm control if humidity is out of range
        rlm.sta.hum_alarm = 1; // Set humidity alarm status
        if(!rlm.opt.gsm_call_ctrl) rlm.opt.gsm_call_ctrl = 1; // Activate GSM call control
        if(!rlm.opt.gsm_sms_ctrl)  rlm.opt.gsm_sms_ctrl = 1; // Activate GSM SMS control
        Serial.println("Humidity out of range! Alarm activated.");
    } else {
        rlm.opt.alarm_ctrl = 0; // Clear alarm control if all readings are normal
        rlm.sta.temp_alarm = 0; // Clear temperature alarm status
        rlm.sta.hum_alarm = 0; // Clear humidity alarm status
    }

}

void read_tds_value(){
    int adcValue = analogRead(TDS_SENSOR_Pin);

    // Convert ADC value to voltage
    float voltage = adcValue * VREF / ADC_RESOLUTION;

    // Temperature compensation
    float compensationCoefficient = 1.0 + 0.02 * (rlm.sta.temp_value - 25.0);
    float compensatedVoltage = voltage / compensationCoefficient;

    // TDS calculation
    rlm.sta.tds_value = (133.42 * compensatedVoltage * compensatedVoltage * compensatedVoltage
                    - 255.86 * compensatedVoltage * compensatedVoltage
                    + 857.39 * compensatedVoltage) * 0.5;
    
    Serial.println("Voltage: " + String(voltage, 3) + " V");
    Serial.println("TDS sensor reading:"+String(rlm.sta.tds_value)+" ppm"); 
    if((rlm.sta.tds_value > rlm.conf.mx_tds || rlm.sta.tds_value < rlm.conf.mn_tds)    && rlm.sta.tds_value != 0) {
        rlm.opt.alarm_ctrl = 1; // Activate alarm control
        rlm.sta.tds_alarm = 1; // Set TDS alarm status
        if(!rlm.opt.gsm_call_ctrl) rlm.opt.gsm_call_ctrl = 1; // Activate GSM call control
        if(!rlm.opt.gsm_sms_ctrl)  rlm.opt.gsm_sms_ctrl = 1; // Activate GSM SMS control
        Serial.println("TDS out of range! Alarm activated.");
    } else {
        rlm.sta.tds_alarm = 0; // Clear TDS alarm status
    }  
}

// GSM function for send data via UART
void gsm_send_data(const char* data){
    Serial.print(">> Sending: ");
    Serial.println(data);
    GSM_SERIAL.print(data);
    GSM_SERIAL.print("\r\n");
}

void clean_sim_storage() {
    gsm_send_data("AT+CMGD=1,4"); // Delete all SMS messages from SIM storage
    GSM_SERIAL.print("\r\n");
    Serial.println("Sent command to clean SIM storage");
}


void gsm_send_dashboard(){
    char buff[200];
    memset(buff,0,sizeof(buff));
    sprintf(buff,"1.Temprature:%.2f C  2.Humidity:%d%% 3.TDS: %d bpm 4.THI: %d",
        rlm.sta.temp_value,rlm.sta.hum_value,rlm.sta.tds_value,rlm.sta.thi);
    gsm_send_data(buff);
}

// GSM function for receive data via UART
void gsm_receive_data(){
    // Re-arm DMA reception for the next payload
    memset(gsm_rx_buffer, 0, sizeof(gsm_rx_buffer));
    // Check for GSM responses
    while(GSM_SERIAL.available()) {
        int size = GSM_SERIAL.readBytes(gsm_rx_buffer, sizeof(gsm_rx_buffer) - 1); // Read GSM response into buffer
        if (size >= sizeof(gsm_rx_buffer) - 1) {
            memset(gsm_rx_buffer, 0, sizeof(gsm_rx_buffer)); // Clear buffer if overflow
            Serial.println("GSM receive buffer overflow, clearing buffer");
        }
         Serial.printf("GSM received: %s\n\r", gsm_rx_buffer); // Print the received GSM response for debu
    }
    
    // Process the received data as needed
    if(!BITREAD(rlm.sta.init_gsm, 0) && strstr(gsm_rx_buffer, "OK")) {
        BITSET(rlm.sta.init_gsm, 0); // Basic command to check communication
    } else if(!BITREAD(rlm.sta.init_gsm, 1) && strstr(gsm_rx_buffer, "+CSMINS:")) {
        sscanf(gsm_rx_buffer, "+CSMINS: %*d,%d", &rlm.sta.sim_status);
        BITSET(rlm.sta.init_gsm, 1); // Mark SIM card status as received
    } else if(!BITREAD(rlm.sta.init_gsm, 3) && strstr(gsm_rx_buffer, "+CSQ:")) {
        sscanf(gsm_rx_buffer, "+CSQ: %d,%*d", &rlm.sta.rssi);
        BITSET(rlm.sta.init_gsm, 3); // Mark RSSI as received
    } else if(!BITREAD(rlm.sta.init_gsm, 4) && strstr(gsm_rx_buffer,"OK")) {
        BITSET(rlm.sta.init_gsm, 4); // Mark SMS mode set as done
    } else if(!BITREAD(rlm.sta.init_gsm, 5) && strstr(gsm_rx_buffer,">")) {
        BITSET(rlm.sta.init_gsm, 5); // Mark SMS recipient set as done
    }else if(!BITREAD(rlm.sta.init_gsm, 6) && strstr(gsm_rx_buffer, ">")) {
        BITSET(rlm.sta.init_gsm, 6); // Mark test SMS sent as done
    } else if(!BITREAD(rlm.sta.init_gsm, 7) && strstr(gsm_rx_buffer, "OK")) {
        BITSET(rlm.sta.init_gsm, 7); // Mark end of SMS message set as done
    }else if(strstr(gsm_rx_buffer, "+CMTI:")) {
        int lst_sms_idx = 0;
        char *cmti_ptr = strstr(gsm_rx_buffer, "+CMTI:");
        if(cmti_ptr) {
            sscanf(cmti_ptr, "+CMTI: %*[^,],%d", &lst_sms_idx);
        }
        char buff[20];
        memset(buff,0,sizeof(buff));
        sprintf(buff,"AT+CMGR=%d",lst_sms_idx);
        gsm_send_data(buff);
        Serial.printf("lst_sms_idx->%d\n\r",lst_sms_idx);
    }else if(strstr(gsm_rx_buffer, "+CMGR:")) {
        char *cmgr_ptr = strstr(gsm_rx_buffer, "+CMGR:");
        Serial.printf("RAW: [%s]\n\r", cmgr_ptr);
        if(cmgr_ptr) {
            sscanf(cmgr_ptr, "+CMGR: %*[^,],\"%[^\"]\"", rlm.opt.lst_mob_req);
            Serial.printf("Phone parsed: [%s]\n\r", rlm.opt.lst_mob_req);
        }
        if(strstr(gsm_rx_buffer, "Dashboard")) {
            char buff[30];
            memset(buff,0,sizeof(buff));
            sprintf(buff,"AT+CMGS=\"%s\"",rlm.opt.lst_mob_req);
            gsm_send_data(buff);
            delay(500);
            gsm_send_dashboard();
            delay(500);
            GSM_SERIAL.write(26);
        }else if(strstr(gsm_rx_buffer, "Modify")){
            char buff[30];
            memset(buff,0,sizeof(buff));
            sprintf(buff,"AT+CMGS=\"%s\"",rlm.opt.lst_mob_req);
            gsm_send_data(buff);
            delay(500);
            gsm_send_data("What you want to modify? 1.Contact 2.Sensors");
            delay(500);
            GSM_SERIAL.write(26);
        }else if(strstr(gsm_rx_buffer, "1.Contact")) {
            char buff[30];
            memset(buff,0,sizeof(buff));
            sprintf(buff,"AT+CMGS=\"%s\"",rlm.opt.lst_mob_req);
            gsm_send_data(buff);
            delay(500);
            gsm_send_data("What you want to modify? 1.Owner:");
            delay(500);
            GSM_SERIAL.write(26);
        }else if(strstr(gsm_rx_buffer, "1.Owner:")) {
            char buff[100],num[15];
            memset(num,0,sizeof(num));
            char *owner_ptr = strstr(gsm_rx_buffer, "1.Owner:");
            if(owner_ptr) {
                sscanf(owner_ptr, "1.Owner: %s", num);  // num not &num
                // trim any \r\n at end
                num[strcspn(num, "\r\n")] = 0;
                Serial.printf("Owner num: [%s]\n\r", num);
                memccpy(rlm.conf.home_mob, num, '\0', sizeof(rlm.conf.home_mob)); // Store the new home mobile number in configuration
                save_to_flash(&rlm.conf,sizeof(rlm.conf));
            }

            // memset(buff, 0, sizeof(buff));
            // sprintf(buff, "AT+CPBW=1,\"%s\",145,\"Owner\"", num);
            // gsm_send_data(buff);
            // delay(500);
            memset(buff,0,sizeof(buff));
            sprintf(buff,"AT+CMGS=\"%s\"",rlm.opt.lst_mob_req);
            gsm_send_data(buff);
            delay(500);
            gsm_send_data("The modified successfully!!");
            delay(500);
            GSM_SERIAL.write(26);
            BITCLEAR(rlm.sta.init_gsm, 5);
            BITCLEAR(rlm.sta.init_gsm, 6);
            BITCLEAR(rlm.sta.init_gsm, 7);
        }else if(strstr(gsm_rx_buffer, "2.Sensors")) {
            char buff[30];
            memset(buff,0,sizeof(buff));
            sprintf(buff,"AT+CMGS=\"%s\"",rlm.opt.lst_mob_req);
            gsm_send_data(buff);
            delay(500);
            gsm_send_data("I can help you.What I do? 1.Set T/H 2.Set TDS");
            delay(500);
            GSM_SERIAL.write(26);
        }else if(strstr(gsm_rx_buffer, "1.Set T/H")){
            char buff[30];
            memset(buff,0,sizeof(buff));
            sprintf(buff,"AT+CMGS=\"%s\"",rlm.opt.lst_mob_req);
            gsm_send_data(buff);
            delay(500);
            gsm_send_data("Fill the value. 1.Tmax: 2.Tmin: 3.Hmax: 4.Hmin:");
            delay(500);
            GSM_SERIAL.write(26);
        }else if(strstr(gsm_rx_buffer, "Fill the value. 1.Tmax:")) {
            char *th = strstr(gsm_rx_buffer, "Fill the value.");
            if(th) {
                Serial.printf("RAW: [%s]\n\r", th);
                sscanf(th, "Fill the value. 1.Tmax:%d 2.Tmin:%d 3.Hmax:%d 4.Hmin:%d",
                &rlm.conf.mx_temp,&rlm.conf.mn_temp,&rlm.conf.mx_hum,&rlm.conf.mn_hum);
            }
            char buff[30];
            memset(buff,0,sizeof(buff));
            sprintf(buff,"AT+CMGS=\"%s\"",rlm.opt.lst_mob_req);
            gsm_send_data(buff);
            delay(500);
            gsm_send_data("The modified successfully!!");
            delay(500);
            GSM_SERIAL.write(26);
            Serial.println("1.Tmax:" + String(rlm.conf.mx_temp) 
            + " 2.Tmin:" + String(rlm.conf.mn_temp) 
            + " 3.Hmax:" + String(rlm.conf.mx_hum) 
            + " 4.Hmin:" + String(rlm.conf.mn_hum));
            //Store the value
            save_to_flash(&rlm.conf,sizeof(rlm.conf));
        }else if(strstr(gsm_rx_buffer, "2.Set TDS")){
            char buff[30];
            memset(buff,0,sizeof(buff));
            sprintf(buff,"AT+CMGS=\"%s\"",rlm.opt.lst_mob_req);
            gsm_send_data(buff);
            delay(500);
            gsm_send_data("Fill the value. 1.HRTmax: 2.HRTmin: 3.SPO2max: 4.SPO2min:");
            delay(500);
            GSM_SERIAL.write(26);
        }else if(strstr(gsm_rx_buffer, "Fill the value. 1.TDSmax:")) {
            char *hbt = strstr(gsm_rx_buffer, "Fill the value.");
            if(hbt) {
                Serial.printf("RAW: [%s]\n\r", hbt);
                sscanf(hbt, "Fill the value. 1.TDSmax:%d 2.TDSmin:%d",
                &rlm.conf.mx_tds,&rlm.conf.mn_tds);
            }
            char buff[30];
            memset(buff,0,sizeof(buff));
            sprintf(buff,"AT+CMGS=\"%s\"",rlm.opt.lst_mob_req);
            gsm_send_data(buff);
            delay(500);
            gsm_send_data("The modified successfully!!");
            delay(500);
            GSM_SERIAL.write(26);
            Serial.println("1.TDSmax:" + String(rlm.conf.mx_tds) + " 2.TDSmin:" + String(rlm.conf.mn_tds));
            //Store the value
            save_to_flash(&rlm.conf,sizeof(rlm.conf));
        }

    }
}

// Send initialization commands to GSM module in sequence and update status bits accordingly
void sync_gsm() {
    BITSET(rlm.sta.init_gsm, 2); // Mark UART configured as done
	Serial.printf("GSM Send cmd -%x\n\r",rlm.sta.init_gsm);
    if(!BITREAD(rlm.sta.init_gsm, 0)) gsm_send_data("ATE0"); // Basic command to check communication
    else if(!BITREAD(rlm.sta.init_gsm, 1)) gsm_send_data("AT+CSMINS?"); // Check SIM card status
    else if(!BITREAD(rlm.sta.init_gsm, 3)) gsm_send_data("AT+CSQ"); // Check signal quality
    else if(!BITREAD(rlm.sta.init_gsm, 4)) gsm_send_data("AT+CMGF=1"); // Set SMS mode is string mode
    else if(!BITREAD(rlm.sta.init_gsm, 5)) {
        char buff[30];
        memset(buff,0,sizeof(buff));
        sprintf(buff,"AT+CMGS=\"%s\"",rlm.conf.home_mob);
        gsm_send_data(buff); // Set SMS recipient
    }
    else if(!BITREAD(rlm.sta.init_gsm, 6)) gsm_send_data("Hi Boss I am alive!"); // Sent test SMS
    else if(!BITREAD(rlm.sta.init_gsm, 7)) GSM_SERIAL.write(26); // set end of SMS message (Ctrl+Z)
}

void init_controler(){
    Serial.println("\n\n=== Remote Livestock Monitoring System ===");
    // Initialize LED pin
    pinMode(STATUS_LED_Pin, OUTPUT);
    pinMode(RELAY_FENCE_Pin, OUTPUT);
    pinMode(RELAY_LAMP_Pin, OUTPUT);
    pinMode(RELAY_FAN_Pin, OUTPUT);

    digitalWrite(STATUS_LED_Pin, LOW); // LED off (inverted logic on Blue Pill)
    digitalWrite(RELAY_FENCE_Pin, HIGH); 
    digitalWrite(RELAY_LAMP_Pin, HIGH); 
    digitalWrite(RELAY_FAN_Pin, HIGH); 

    pinMode(TH_SENSOR_Pin,INPUT_PULLUP);

    memset(&rlm.conf, 0, sizeof(rlm.conf)); // Clear the entire rlm structure
    memset(&rlm.sta, 0, sizeof(rlm.sta)); // Clear status structure
    memset(&rlm.opt, 0, sizeof(rlm.opt)); // Clear operation structure
    if(!read_from_flash(&rlm.conf,sizeof(rlm.conf))){ // Load configuration from flash memory
        rlm.conf.magic = FLASH_MAGIC;
        // Initialize default values if flash read fails
        memset(rlm.conf.home_mob, 0, sizeof(rlm.conf.home_mob));
        // memccpy(rlm.conf.home_mob, "", '\0', sizeof(rlm.conf.home_mob)); // Default home mobile number
        // memccpy(rlm.conf.emg_mob, "", '\0', sizeof(rlm.conf.emg_mob)); // Default emergency mobile number
        rlm.conf.mx_temp = 38;
        rlm.conf.mn_temp = 30;
        rlm.conf.mx_hum = 80;
        rlm.conf.mn_hum = 20;
        rlm.conf.mx_tds = 100;
        rlm.conf.mn_tds = 60;
        Serial.println("No valid configuration in flash. Loaded default configuration.");
        save_to_flash(&rlm.conf,sizeof(rlm.conf)); // Save default configuration to flash memory
    }

    //By default alive message not sent, for reason mobile number may not be set by user, so set the GSM initialization bits related to SMS to avoid sending SMS until user sets mobile number
    if(strlen(rlm.conf.home_mob) < 8) { // Check if home mobile number is not set (assuming valid numbers are at least 8 characters long)
        BITSET(rlm.sta.init_gsm, 5);
        BITSET(rlm.sta.init_gsm, 6);
        BITSET(rlm.sta.init_gsm, 7);
       // rlm.sta.init_gsm |= 0xE0; // Set bits 5, 6, and 7 to indicate SMS setup is done
    }
    Serial.printf("GSM Send cmd -%x\n\r",rlm.sta.init_gsm);

    Serial.println("Controller initialized with following configuration:");
    Serial.printf("Home Mobile: %s\n\r", rlm.conf.home_mob);
    Serial.printf("Temperature Thresholds: Min=%d C, Max=%d C\n\r", rlm.conf.mn_temp, rlm.conf.mx_temp);
    Serial.printf("Humidity Thresholds: Min=%d %%, Max=%d %%\n\r", rlm.conf.mn_hum, rlm.conf.mx_hum);
    Serial.printf("TDS Thresholds: Min=%d %%, Max=%d %%\n\r", rlm.conf.mn_tds, rlm.conf.mx_tds);
    Serial.println("System initialized successfully");

    
}

void update_display(){
    
}
#endif // CONTROLLER_H
