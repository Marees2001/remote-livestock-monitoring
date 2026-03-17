#ifndef Define_H
#define Define_H

#define BITREAD(value, bit) ((value >> bit) & 0x01) // Macro to read a specific bit from a value
#define BITSET(value, bit) (value |= (1 << bit)) // Macro to set a specific bit in a value
#define BITCLEAR(value, bit) (value &= ~(1 << bit)) // Macro to clear a specific bit in a value
#define BITTOGGLE(value, bit) (value ^= (1 << bit)) // Macro to toggle

//Definition for GSM message from controller to user
#define VIEW_DAHBOARD "Are you want to view the dashboard? Reply with 'YES' to see real-time sensor data and equipment status."


#define MSG_LAMP_ON "LAMP ON"
#define MSG_LAMP_OFF "LAMP OFF"
#define MSG_FAN_ON "FAN ON"
#define MSG_FAN_OFF "FAN OFF"
#define MSG_FENCE_ON "FENCE ON"
#define MSG_FENCE_OFF "FENCE OFF"


#endif // Define_H