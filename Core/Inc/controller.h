
#ifndef CONTROLLER_H
#define CONTROLLER_H

extern UART_HandleTypeDef huart1;

/*structure for GSM communication*/
struct GSMS_INITIALIZATION {
    uint8_t init; // command:AT ,Initialization status (0 or 1)
    uint8_t sim_sta; //command: AT-CSPIN? ,SIM card status (0-3). 0: Not checked, 1: Ready, 2: Not ready, 3: Error
    uint8_t set_uart_conf; // command: AT+ICF?,UART configuration status.
    // Bit read operations: 0: NOT SET, 1:Baud rate 2: Word length, 3: Stop bits, 4: Parity
    uint8_t req_rssi; // command: AT+CSQ?, Request signal quality .
    uint8_t set_mob_num; // command: AT+CMGF=1, Set mobile number format (0 or 1)
    uint8_t send_sms; // command: AT+CMGS="number", Send SMS status (0 or 1)
}gsmInit;

/*Structure for controller status*/
struct CONTROLLER_STATUS {
    uint8_t init_gsm; // GSM initialization status (0 or 1)
    float tds_value; // TDS sensor value
    float temp_value; // Temperature sensor value
    float hum_value; // Humidity sensor value
    uint8_t rssi; // GSM signal strength (0-31)
    uint8_t alm_sta; // Alarm status (0 or 1)
    uint8_t lam_status; // Lamp status (0 or 1)
    uint8_t fan_status; // Fan status (0 or 1)
    uint8_t fence_status; // Fence status (0 or 1)
};

/*Structure for controller settings*/
struct CONTROLLER_SETTINGS {
    uint8_t mx_temp; // Max threshold for temperature
    uint8_t mn_temp; // Min threshold for temperature
    uint8_t mx_hum; // Max threshold for humidity
    uint8_t mn_hum; // Min threshold for humidity
    uint8_t mx_tds; // Max threshold for TDS; 
    //range 0-1000 ppm, where 0-300 ppm is considered good, 300-600 ppm is fair, and above 600 ppm is poor water quality
    uint8_t mn_tds; // Min threshold for TDS
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
};

struct REMOTE_LIVESTOCK_MONITORING {
    struct CONTROLLER_STATUS sta; // Current status of the controller
    struct CONTROLLER_SETTINGS conf; // Current settings of the controller
    struct CONTROLLER_OPERATION opt; // Current operation of the controller
}rlm;

// GSM function for send data via UART
void gsm_send_data(const char* data){
    HAL_UART_Transmit(&huart1, (uint8_t*)data, strlen(data), 1000);
}

// GSM function for receive data via UART
void gsm_receive_data(char* buffer, size_t buffer_size){
    HAL_UART_Receive(&huart1, (uint8_t*)buffer, buffer_size, 1000);
}

void gsm_init() {
    // Send initialization commands to GSM module
    if(!gsmInit.init) gsm_send_data("AT\r\n"); // Basic command to check communication
    else if(!gsmInit.sim_sta) gsm_send_data("AT+CCID?\r\n"); // Check SIM card status
    else if(!gsmInit.req_rssi) gsm_send_data("AT+CSQ\r\n"); // Check signal quality
}
#endif // CONTROLLER_H