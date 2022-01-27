#pragma once

/*Radio Params*/
#define RF_FREQUENCY   					  915000000 // Hz

#define FSK_FDEV                          25e3      // Hz
#define FSK_DATARATE                      50e3      // bps
#define FSK_BANDWIDTH                     50e3      // Hz
#define FSK_AFC_BANDWIDTH                 83.333e3  // Hz
#define FSK_PREAMBLE_LENGTH               5         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON         false

#define LORA_BANDWIDTH                    2        	// [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR             7        	// [SF7..SF12]
#define LORA_CODINGRATE                   1         // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH              8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT               5         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON        false
#define LORA_IQ_INVERSION_ON              false

#define RX_TIMEOUT_VALUE                  3500
#define TX_OUTPUT_POWER                    2        // dBm //BML: set from 17 to 10 to 2 to run off launchpad / harvested power
#define BUFFER_SIZE                       256 		// Define the payload size here

#define MAC_HDR                           0xDF    
#define DEV_ID                            0x04

#define PACKET_SIZE                       255
#define HEADER_SIZE                       6
