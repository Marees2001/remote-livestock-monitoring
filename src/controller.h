
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>
#include <stdio.h>
#include <Arduino.h>
#include "define.h"
#include <Wire.h>
#include "U8glib.h"
#include "flash.h"
#include "DHT.h"

extern DHT dht;

extern char gsm_rx_buffer[200]; // Buffer for receiving data from GSM module
extern uint8_t receive_bytes; // Number of bytes received from GSM module

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
};

/*Structure for controller settings*/
struct CONTROLLER_SETTINGS {
    int version;
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

// GSM function for send data via UART
void gsm_send_data(const char* data){
    Serial.print(">> Sending: ");
    Serial.println(data);
    GSM_SERIAL.print(data);
    GSM_SERIAL.print("\r\n");
}

void init_controler(){
    Serial.println("\n\n=== Remote Livestock Monitoring System ===");
    read_from_flash(&rlm.conf,sizeof(rlm.conf));
    if(rlm.conf.version != FIRMWARE_VERSION){
        rlm.conf.version = FIRMWARE_VERSION;
        rlm.conf.mx_temp = 40.0;
        rlm.conf.mn_temp = 35.0;
        rlm.conf.mx_hum  = 50;
        rlm.conf.mn_hum  = 70;
        rlm.conf.mx_tds  = 1000;
        rlm.conf.mn_tds  = 3000;
        //Store the value
        save_to_flash(&rlm.conf,sizeof(rlm.conf));
    }
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
    Serial.println("System initialized successfully");

    
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
    if(GSM_SERIAL.available()) {
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
        uint8_t lst_sms_idx = 0;
        sscanf(gsm_rx_buffer, "+CMTI: %*s,%d", &lst_sms_idx);
        char buff[20];
        memset(buff,0,sizeof(buff));
        sprintf(buff,"AT+CMGD=%d",lst_sms_idx);
        gsm_send_data(buff);
    }else if(strstr(gsm_rx_buffer, "+CMGR:")) {
        sscanf(gsm_rx_buffer, "+CMGR: %*s,%s,%*s,%*s", &rlm.opt.lst_mob_req);
    }else if(strstr(gsm_rx_buffer, "Dashboard")) {
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
        gsm_send_data("Fill the value. 1.Owner:");
        delay(500);
        GSM_SERIAL.write(26);
    }else if(strstr(gsm_rx_buffer, "Fill the value. 1.Owner:")) {
        char buff[100],num[15];
        memset(num,0,sizeof(num));
        sscanf(gsm_rx_buffer, "Fill the value. 1.Owner:%s", &num);
        memset(buff,0,sizeof(buff));
        sprintf(buff,"AT+CPBW=1,\"%s\",145,\"Owner\"",num);
        gsm_send_data(buff);
        delay(500);
        memset(buff,0,sizeof(buff));
        sprintf(buff,"AT+CMGS=\"%s\"",rlm.opt.lst_mob_req);
        gsm_send_data(buff);
        delay(500);
        gsm_send_data("The modified successfully!!");
        delay(500);
        GSM_SERIAL.write(26);
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
    }else if(strstr(gsm_rx_buffer, "2.Set TDS")){
        char buff[30];
        memset(buff,0,sizeof(buff));
        sprintf(buff,"AT+CMGS=\"%s\"",rlm.opt.lst_mob_req);
        gsm_send_data(buff);
        delay(500);
        gsm_send_data("Fill the value. 1.TDSmax: 2.TDSmin: ");
        delay(500);
        GSM_SERIAL.write(26);
    }else if(strstr(gsm_rx_buffer, "Fill the value. 1.Tmax:")) {
        char buff[30];
        sscanf(gsm_rx_buffer, "Fill the value. 1.Tmax:%f 2.Tmin:%f 3.Hmax:%d 4.Hmin:%d",
            &rlm.conf.mx_temp,&rlm.conf.mn_temp,&rlm.conf.mx_hum,&rlm.conf.mn_hum);
        memset(buff,0,sizeof(buff));
        sprintf(buff,"AT+CMGS=\"%s\"",rlm.opt.lst_mob_req);
        gsm_send_data(buff);
        delay(500);
        gsm_send_data("The modified successfully!!");
        delay(500);
        GSM_SERIAL.write(26);
        Serial.printf("1.Tmax:%f 2.Tmin:%f 3.Hmax:%d 4.Hmin:%d\r\n",
        rlm.conf.mx_temp,rlm.conf.mn_temp,rlm.conf.mx_hum,rlm.conf.mn_hum);
        //Store the value
        save_to_flash(&rlm.conf,sizeof(rlm.conf));
    }else if(strstr(gsm_rx_buffer, "Fill the value. 1.TDSmax:")) {
        char buff[30];
        sscanf(gsm_rx_buffer, "Fill the value. 1.TDSmax:%d 2.TDSmin:%d ",
            &rlm.conf.mx_tds,&rlm.conf.mn_tds);
        memset(buff,0,sizeof(buff));
        sprintf(buff,"AT+CMGS=\"%s\"",rlm.opt.lst_mob_req);
        gsm_send_data(buff);
        delay(500);
        gsm_send_data("The modified successfully!!");
        delay(500);
        GSM_SERIAL.write(26);
        Serial.printf("1.TDSmax:%d 2.TDSmin:%d \r\n",
        rlm.conf.mx_tds,rlm.conf.mn_tds);
        //Store the value
        save_to_flash(&rlm.conf,sizeof(rlm.conf));
    }else if(strstr(gsm_rx_buffer, "1.Fan control:")) {
        
    }
}

// Send initialization commands to GSM module in sequence and update status bits accordingly
void sync_gsm() {
    BITSET(rlm.sta.init_gsm, 2); // Mark UART configured as done
	Serial.printf("GSM Send cmd -%x\n\r",rlm.sta.init_gsm);
    if(!BITREAD(rlm.sta.init_gsm, 0)) gsm_send_data("ATE0"); // Basic command to check communication
    else if(!BITREAD(rlm.sta.init_gsm, 1)) gsm_send_data("AT+CSMINS?"); // Check SIM card status
    else if(!BITREAD(rlm.sta.init_gsm, 3)) gsm_send_data("AT+CSQ"); // Check signal quality
    else if(!BITREAD(rlm.sta.init_gsm, 4)) gsm_send_data("AT+CMGF=1"); // Set SMS mode
    else if(!BITREAD(rlm.sta.init_gsm, 5)) gsm_send_data("AT+CMGS=\"+918122606532\""); // Set SMS recipient
    else if(!BITREAD(rlm.sta.init_gsm, 6)) gsm_send_data("Hi Boss I am alive!"); // Sent test SMS
    else if(!BITREAD(rlm.sta.init_gsm, 7)) GSM_SERIAL.write(26); // set end of SMS message (Ctrl+Z)
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

    Serial.print("Voltage: ");
    Serial.print(voltage, 3);
    Serial.print(" V | TDS: ");
    Serial.print(rlm.sta.tds_value, 0);
    Serial.println(" ppm");
}

void read_th_sensor(){
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  rlm.sta.temp_value = dht.readHumidity();
  // Read temperature as Celsius (the default)
  rlm.sta.hum_value = dht.readTemperature();
  //Calculate the THI(Temprature Humidity Index)
  rlm.sta.thi = (1.8 * rlm.sta.temp_value + 32) - ((0.55 - 0.0055 * rlm.sta.hum_value ) 
    * (1.8 * rlm.sta.temp_value - 26));
  Serial.printf("Tempratur :%0.2f\n\r",rlm.sta.temp_value);
  Serial.printf("Humitidy :%d\n\r",rlm.sta.hum_value);
  Serial.printf("THI :%0.2f\n\r",rlm.sta.thi);
}

#endif // CONTROLLER_H
