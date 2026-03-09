
#ifndef CONTROLLER_H
#define CONTROLLER_H

extern UART_HandleTypeDef huart1;

/*structure for GSM communication*/
struct GSMS_STATUS {
    uint8_t init; // Initialization status (0 or 1)
    int rssi; // Signal strength (0-31)
    uint8_t sim_status; // SIM card status (0-3)
    char registration_status[2]; // Registration status (0-5)
    char operator_name[20]; // Operator name
}gsmSta;

/*Structure for GSM communication settings*/
struct GSMS_SETTINGS {
    char apn[20]; // Access Point Name for GSM network
    char username[20]; // Username for GSM network
    char password[20]; // Password for GSM network
}gsmConf;

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
    if(!gsmSta.init) gsm_send_data("AT\r\n"); // Basic command to check communication
    else if(!gsmSta.rssi) gsm_send_data("AT+CSQ\r\n"); // Check signal quality
    else if(!gsmSta.sim_status) gsm_send_data("AT+CCID?\r\n"); // Check SIM card status
}
#endif // CONTROLLER_H