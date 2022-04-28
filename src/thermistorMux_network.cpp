/*******************************************************************************
Copyright 2021
Steward Observatory Engineering & Technical Services, University of Arizona

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*******************************************************************************/

/**
 * @file thermistorMux_network.cpp
 * @author Rory Scobie (scobier@arizona.edu)
 * @brief Implements networking specific functions, to handle Ethernet, MQTT,
 * Sparkplug, and NTP functionality.
 * Originally created for VCM module, modified by Nestor, for thermistor Mux use.
 * @version (see THERMISTOR_MUX_VERSION in thermistorMux_global.h)
 * @date 2022-04-19
 *
 * @copyright Copyright (c) 2021
 */

#include "thermistorMux_network.h"
#include "thermistorMux_hardware.h"
#include "thermistorMux_global.h"
#include "thermistor_Mux.h"
#include <NativeEthernet.h>
#include <PubSubClient.h>
#include <NTPClient_Generic.h>
#include <sparkplugb_arduino.hpp>
#include "cf_sparkplug.h"

// Reset defines
#ifndef RESTART_ADDR
/*
Need to verify/update restart address
And check of these functions will work on this board
*/
#define RESTART_ADDR 0xE000ED0C
#endif
#define READ_RESTART() (*(volatile uint32_t *)RESTART_ADDR)
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))

#define NUMBER_MUX_CHANNELS 32
#define MUX_VERSION_COMPLETE "1v1"


// Common network configuration values: TBD
#define GATEWAY 128, 96, 11, 233
#define SUBNET 255, 255, 0, 0
#define DNS 128, 96, 11, 233
#define NUM_BROKERS  1

#if defined(production_TEST)
// MQTT broker definitions: TBD
#define MQTT_BROKER1 169,254,141,48
#define MQTT_BROKER1_PORT 1883

//NTP server address
#define NTP_IP  {169, 254, 39, 226}

#else
  #error A network configuration must be defined
#endif

// MAC address for device #0, adjusted according to ID pins: TBD
#define MUX0_MAC  {0xa, 0x0, 0x0, 0x0, 0x0, 0x0}

// IP address for device #0, adjusted according to ID pins: TBD
#define MUX0_IP   {169, 254, 84, 177}

// Sparkplug settings
#define GROUP_ID              "VI"              // This node's group ID
#define NODE_ID_TEMPLATE      "THERMISTORx"            // Template for this node's node ID
#define NODE_ID_TOKEN         'x'               // Character to be replaced with module ID

/*
  Private variables
*/
// NTP variables
static EthernetUDP ntpUDP;
static IPAddress ntpIP = NTP_IP;
// Can use "pool.ntp.org" if connected to internet, but might have outages.
// TODO: decrease sync frequency to avoid violation of pool.ntp.org terms of service
static NTPClient ntp(ntpUDP, ntpIP);

// MQTT variables
static EthernetClient enet[NUM_BROKERS];
static PubSubClient m_broker[NUM_BROKERS];

// Sparkplug node and topic names
static String node_id = NODE_ID_TEMPLATE;
static String nodeBirthTopic = NODE_TOPIC(NBIRTH_MESSAGE_TYPE, NODE_ID_TEMPLATE);
static String nodeDeathTopic = NODE_TOPIC(NDEATH_MESSAGE_TYPE, NODE_ID_TEMPLATE);
static String nodeDataTopic  = NODE_TOPIC(NDATA_MESSAGE_TYPE,  NODE_ID_TEMPLATE);
static String nodeCmdTopic   = NODE_TOPIC(NCMD_MESSAGE_TYPE,   NODE_ID_TEMPLATE);

// These variables hold the last published value of each metric
static uint64_t m_bdSeq[NUM_BROKERS] = {0};  // Node birth/death sequence numbers
static bool     m_nodeReboot         = false;
static bool     m_nodeRebirth        = false;
static bool     m_nodeNextServer     = false;
static bool     m_nodeCalibrated      = false;
static bool     m_nodeCalibrationINW  = false;
static uint64_t m_commsVersion       = COMMS_VERSION;
static const char *m_firmwareVersion = MUX_VERSION_COMPLETE;
static float    m_calTemp1            = {0.0};
static float    m_calTemp2            = {0.0};
static float    m_calData[NUMBER_MUX_CHANNELS]    = {0.00};
static const char *m_units           = "NOT SET";  // The user units
static float    m_THERMISTOR[NUMBER_MUX_CHANNELS] = {0.0};
static float    m_ADC_temperature = 0.0;

// Alias numbers for each of the node metrics
enum NodeMetricAlias {
    NMA_bdSeq = 0,
    NMA_Reboot,
    NMA_Rebirth,
    NMA_NextServer,
    NMA_CalibrationStatus,
    NMA_CalibrationTemp1,
    NMA_CalibrationTemp2,
    NMA_CalibrationData,
    NMA_CalibrationINW,
    NMA_CommsVersion,
    NMA_FirmwareVersion,
    NMA_Units,
    NMA_THERMISTOR1,
    NMA_THERMISTOR2,
    NMA_THERMISTOR3,
    NMA_THERMISTOR4,
    NMA_THERMISTOR5,
    NMA_THERMISTOR6,
    NMA_THERMISTOR7,
    NMA_THERMISTOR8,
    NMA_THERMISTOR9,
    NMA_THERMISTOR10,
    NMA_THERMISTOR11,
    NMA_THERMISTOR12,
    NMA_THERMISTOR13,
    NMA_THERMISTOR14,
    NMA_THERMISTOR15,
    NMA_THERMISTOR16,
    NMA_THERMISTOR17,
    NMA_THERMISTOR18,
    NMA_THERMISTOR19,
    NMA_THERMISTOR20,
    NMA_THERMISTOR21,
    NMA_THERMISTOR22,
    NMA_THERMISTOR23,
    NMA_THERMISTOR24,
    NMA_THERMISTOR25,
    NMA_THERMISTOR26,
    NMA_THERMISTOR27,
    NMA_THERMISTOR28,
    NMA_THERMISTOR29,
    NMA_THERMISTOR30,
    NMA_THERMISTOR31,
    NMA_THERMISTOR32,
    NMA_ADC_Temperature,
    EndNodeMetricAlias
};

// The bdseq metric for a single broker
static MetricSpec bdseqMetricsTemplate[] = {
    {"bdSeq", NMA_bdSeq, false, METRIC_DATA_TYPE_INT64, NULL, false, 0},
};

// The bdseq metrics for all brokers
static MetricSpec bdseqMetrics[NUM_BROKERS][NUM_ELEM(bdseqMetricsTemplate)];

// All node metrics
static MetricSpec NodeMetrics[] = {
    {"Node Control/Reboot",               NMA_Reboot,          true,  METRIC_DATA_TYPE_BOOLEAN, &m_nodeReboot,       false, 0},
    {"Node Control/Rebirth",              NMA_Rebirth,         true,  METRIC_DATA_TYPE_BOOLEAN, &m_nodeRebirth,      false, 0},
    {"Node Control/Next Server",          NMA_NextServer,      true,  METRIC_DATA_TYPE_BOOLEAN, &m_nodeNextServer,   false, 0},
    {"Node Control/Calibration INW",            NMA_CalibrationINW,      true,  METRIC_DATA_TYPE_BOOLEAN, &m_nodeCalibrationINW,    false, 0},
    {"Node Control/Calibration Status",            NMA_CalibrationStatus,      false,  METRIC_DATA_TYPE_BOOLEAN, &m_nodeCalibrated,    false, 0},
    {"Node Control/Calibration Temperature 1",      NMA_CalibrationTemp1,      true, METRIC_DATA_TYPE_FLOAT,   &m_calTemp1, false, 0},        
    {"Node Control/Calibration Temperature 2",      NMA_CalibrationTemp2,      true, METRIC_DATA_TYPE_FLOAT,   &m_calTemp2, false, 0},    
    {"Properties/Communications Version", NMA_CommsVersion,    false, METRIC_DATA_TYPE_INT64,   &m_commsVersion,     false, 0},
    {"Properties/Firmware Version",       NMA_FirmwareVersion, false, METRIC_DATA_TYPE_STRING,  &m_firmwareVersion,  false, 0},
    {"Properties/Units",                  NMA_Units,           false, METRIC_DATA_TYPE_STRING,  &m_units,            false, 0},
    {"Outputs/Calibration Data",                 NMA_CalibrationData,        false, METRIC_DATA_TYPE_FLOAT,   &m_calData[0],           false, 0},
    {"Inputs/THERMISTOR1",                       NMA_THERMISTOR1,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[0],           false, 0},
    {"Inputs/THERMISTOR2",                       NMA_THERMISTOR2,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[1],           false, 0},
    {"Inputs/THERMISTOR3",                       NMA_THERMISTOR3,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[2],           false, 0},
    {"Inputs/THERMISTOR4",                       NMA_THERMISTOR4,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[3],           false, 0},
    {"Inputs/THERMISTOR5",                       NMA_THERMISTOR5,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[4],           false, 0},
    {"Inputs/THERMISTOR6",                       NMA_THERMISTOR6,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[5],           false, 0},
    {"Inputs/THERMISTOR7",                       NMA_THERMISTOR7,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[6],           false, 0},
    {"Inputs/THERMISTOR8",                       NMA_THERMISTOR8,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[7],           false, 0},
    {"Inputs/THERMISTOR9",                       NMA_THERMISTOR9,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[8],           false, 0},
    {"Inputs/THERMISTOR10",                      NMA_THERMISTOR10,           false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[9],          false, 0},
    {"Inputs/THERMISTOR11",                      NMA_THERMISTOR11,           false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[10],          false, 0},
    {"Inputs/THERMISTOR12",                       NMA_THERMISTOR12,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[11],           false, 0},
    {"Inputs/THERMISTOR13",                       NMA_THERMISTOR13,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[12],           false, 0},
    {"Inputs/THERMISTOR14",                       NMA_THERMISTOR14,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[13],           false, 0},
    {"Inputs/THERMISTOR15",                       NMA_THERMISTOR15,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[14],           false, 0},
    {"Inputs/THERMISTOR16",                       NMA_THERMISTOR16,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[15],           false, 0},
    {"Inputs/THERMISTOR17",                       NMA_THERMISTOR17,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[16],           false, 0},
    {"Inputs/THERMISTOR18",                       NMA_THERMISTOR18,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[17],           false, 0},
    {"Inputs/THERMISTOR19",                       NMA_THERMISTOR19,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[18],           false, 0},
    {"Inputs/THERMISTOR20",                      NMA_THERMISTOR20,           false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[19],          false, 0},
    {"Inputs/THERMISTOR21",                      NMA_THERMISTOR21,           false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[20],          false, 0},
    {"Inputs/THERMISTOR22",                       NMA_THERMISTOR22,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[21],           false, 0},
    {"Inputs/THERMISTOR23",                       NMA_THERMISTOR23,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[22],           false, 0},
    {"Inputs/THERMISTOR24",                       NMA_THERMISTOR24,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[23],           false, 0},
    {"Inputs/THERMISTOR25",                       NMA_THERMISTOR25,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[24],           false, 0},
    {"Inputs/THERMISTOR26",                       NMA_THERMISTOR26,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[25],           false, 0},
    {"Inputs/THERMISTOR27",                       NMA_THERMISTOR27,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[26],           false, 0},
    {"Inputs/THERMISTOR28",                       NMA_THERMISTOR28,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[27],           false, 0},
    {"Inputs/THERMISTOR29",                       NMA_THERMISTOR29,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[28],           false, 0},
    {"Inputs/THERMISTOR30",                      NMA_THERMISTOR30,           false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[29],          false, 0},
    {"Inputs/THERMISTOR31",                      NMA_THERMISTOR31,           false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[30],          false, 0},
    {"Inputs/THERMISTOR32",                       NMA_THERMISTOR32,            false, METRIC_DATA_TYPE_FLOAT,   &m_THERMISTOR[31],           false, 0},
    {"Inputs/ADC Internal Temperature",          NMA_ADC_Temperature,     false, METRIC_DATA_TYPE_FLOAT,   &m_ADC_temperature,      false, 0},
};

//Verify validity of this function
void reset_teensy(){
    WRITE_RESTART(0x5FA0004);
}

// The Test Bench device is active if and only if TEST_BENCH_SUPPORT is defined.
// If the Test Bench device is not active then its DBIRTH, DDEATH and DDATA
// messages are never published and incoming DCMD messages are ignored.

// Publish the NBIRTH message and the DBIRTH message for any devices, with all
// metrics specified.
void publish_births(){
    for(int br_idx = 0; br_idx < NUM_BROKERS; br_idx++){
        // Create and publish the NBIRTH message containing the bdseq metric
        // for this broker together with all the node metrics
        set_up_nbirth_payload();
        if(!add_metrics(true, ARRAY_AND_SIZE(bdseqMetrics[br_idx])) ||
           !publish_metrics(&m_broker[br_idx], 1, nodeBirthTopic.c_str(),
                            true, ARRAY_AND_SIZE(NodeMetrics))){
            DebugPrintNoEOL("Failed to publish NBIRTH: ");
            DebugPrint(cf_sparkplug_error);
            // Continue anyway
        }
    }
}

// Publish the NDATA message with any node metrics that have been updated.
void publish_node_data(){
    // Publish any updated metrics in the NDATA message
    set_up_next_payload();
    if(!publish_metrics(ARRAY_AND_SIZE(m_broker), nodeDataTopic.c_str(), false,
                        ARRAY_AND_SIZE(NodeMetrics))){
        // An empty message means we aren't connected to any brokers, while the
        // no metrics message means no metrics have changed since the last time
        // we published - ignore both of these cases
        if(strcmp(cf_sparkplug_error, ""          ) != 0 &&
           strcmp(cf_sparkplug_error, "No metrics") != 0){
            DebugPrintNoEOL("Failed to publish NDATA: ");
            DebugPrint(cf_sparkplug_error);
        }
        return;
    }
}

/**
 * @brief Subscribe to the required topics on the given broker.
 *
 * @param broker the broker we're subscribing with
 * @return true if we successfully subscribe to all topics
 * @return false if we fail to subscribe to one or more topics
 */
bool subscribeTopics(PubSubClient* broker){
    bool success = true;
    if(!broker->subscribe(HOST_STATE_TOPIC))
        success = false;
    if(!broker->subscribe(nodeCmdTopic.c_str()))
        success = false;
    return success;
}

// Connect to the specified broker and send out initial messages.
bool connect_to_broker(PubSubClient *broker, int br_idx){
    // Increment the birth/death sequence number before creating the NDEATH
    // message
    m_bdSeq[br_idx]++;
    if(!update_metric(ARRAY_AND_SIZE(bdseqMetrics[br_idx]), &m_bdSeq[br_idx]))
        DebugPrint(cf_sparkplug_error);

    // Create the NDEATH message with its metrics
    set_up_ndeath_payload();
    if(!add_metrics(true, ARRAY_AND_SIZE(bdseqMetrics[br_idx]))){
        DebugPrint(cf_sparkplug_error);
        DebugPrint("Failed to add metrics to NDEATH");
        m_bdSeq[br_idx]--;
        return false;
    }

    // Connect to the broker, with the NDEATH message as our "will"
    if(!connect(broker, node_id.c_str(), nodeDeathTopic.c_str())){
        DebugPrint(cf_sparkplug_error);
        m_bdSeq[br_idx]--;
        return false;
    }

    // Subscribe to the topics we're interested in
    if(!subscribeTopics(broker)){
        DebugPrint("Unable to subscribe to topics on broker");
        // Disconnect gracefully from the broker
        disconnect(broker, nodeDeathTopic.c_str());
        return false;
    }

    // Success
    return true;
}

/***
/**
 * @brief Returns the seconds since Jan 1, 1970 from the NTP object.
 *
 * @return unsigned long
***/

unsigned long get_current_time(void){
    return ntp.getUTCEpochTime();
}

/**
 * @brief Returns the milliseconds since Jan 1, 1970 from the NTP object.
 *
 * @return unsigned long long
***/

unsigned long long get_current_time_millis(void){
    return ntp.getUTCEpochMillis();
}
// Check to see if a received message is a Node command (NCMD) message.  If it
// is, handle it and return true, even if it's invalid; otherwise return false.
bool process_node_cmd_message(char* topic, byte* payload, unsigned int len){
    Serial.println("Processing Command.");
    if(strcmp(topic, nodeCmdTopic.c_str()) != 0)
        // This is not a Node command message
        return false;

    // Decode the Sparkplug payload
    sparkplugb_arduino_decoder decoder;
    if(!decoder.decode(payload, len)){
        // Invalid payload - don't do anything
        decoder.free_payload();
        DebugPrint("Unable to decode Node command payload");
        // This was a Node command message
        return true;
    }

    // Process the metrics
    for(unsigned int idx = 0; idx < decoder.payload.metrics_count; idx++){
        Metric *metric = &decoder.payload.metrics[idx];
        MetricSpec *metric_spec = find_received_metric(ARRAY_AND_SIZE(NodeMetrics), metric);
        if(metric_spec == NULL){
            // Invalid metric - skip it
            DebugPrintNoEOL("Unrecognized Node metric: ");
            DebugPrint(cf_sparkplug_error);
            continue;
        }

        // Now handle the metric
        int64_t alias = metric_spec->alias;
        switch(alias){
        case NMA_Reboot:
            if(metric->value.boolean_value){
                DebugPrint("Reboot command received");
                // Reboot immediately - don't attempt to process the rest of
                // the message, publish data, send death certificate,
                // disconnect from broker, or close network
                reset_teensy();
            }
            break;

        case NMA_Rebirth:
            m_nodeRebirth = metric->value.boolean_value;
            if(!update_metric(ARRAY_AND_SIZE(NodeMetrics), &m_nodeRebirth))
                DebugPrint(cf_sparkplug_error);
            if(m_nodeRebirth)
                // Publish birth messages again
                DebugPrint("Node Rebirth command received");
            break;

        case NMA_NextServer:
            //### The Next Server command is part of the Sparkplug spec, but it
            //### has no real use here since we stay connected to all brokers.
            //### Should we just ignore this command (and remove metric and flag)?
            m_nodeNextServer = metric->value.boolean_value;
            if(!update_metric(ARRAY_AND_SIZE(NodeMetrics), &m_nodeNextServer))
                DebugPrint(cf_sparkplug_error);
            if(m_nodeNextServer)
                DebugPrint("NextServer command received");
            break;
        case NMA_CalibrationStatus:
            DebugPrint("Calibration status requested.");            
            break;
        case NMA_CalibrationTemp1:
            m_nodeCalibrationINW = metric->value.boolean_value;
            m_calTemp1 = metric->value.float_value;
            cal_thermistor(m_calTemp1, 1);
            m_nodeCalibrationINW = true;
            for(int br_idx = 0; br_idx < NUM_BROKERS; br_idx++){
                set_up_next_payload();
                publish_metrics(&m_broker[br_idx], 1, nodeBirthTopic.c_str(), true, ARRAY_AND_SIZE(NodeMetrics));
                m_nodeCalibrationINW = false; 
            }
            break;
        case NMA_CalibrationTemp2:
            for(int br_idx = 0; br_idx < NUM_BROKERS; br_idx++){
                set_up_next_payload();
                publish_metrics(&m_broker[br_idx], 1, nodeBirthTopic.c_str(), true, ARRAY_AND_SIZE(NodeMetrics));
                m_nodeCalibrationINW = false; 
            }
            m_nodeCalibrationINW = metric->value.boolean_value;
            m_calTemp2 = metric->value.float_value;
            cal_thermistor(m_calTemp2, 2);
            break;
        case NMA_CalibrationINW:
            m_nodeCalibrationINW = metric->value.boolean_value;
            if(!update_metric(ARRAY_AND_SIZE(NodeMetrics), &m_nodeCalibrationINW))
                DebugPrint(cf_sparkplug_error);
            break;
        default:
            DebugPrintNoEOL("Unhandled Node metric alias: ");
            DebugPrint(alias);
            break;
        }
    }

    // Free the decoder memory
    decoder.free_payload();

    // This was a Node command message
    return true;
}

/**
 * @brief Callback registered with broker to handle incoming data from
 * subscribed topics.
 *
 * @param topic the subscribed topic that we're getting data from
 * @param payload the incoming payload we're receiving on that topic
 * @param len the length, in bytes, of the payload
 */
void callback_worker(char* topic, byte* payload, unsigned int len){
    // Check parameters are valid
    if(topic == NULL || strcmp(topic, "") == 0){
        // Topic was not specified - don't do anything
        DebugPrint("No topic specified");
        return;
    }
    if(payload == NULL){
        // Payload was not specified - don't do anything
        DebugPrint("No payload specified");
        return;
    }
    if(len == 0){
        // Payload is empty - don't do anything
        DebugPrint("Payload length is zero");
        return;
    }

    // Determine message type
    bool host_online = false;
    if(process_host_state_message(topic, payload, len, &host_online)){
        // A non-empty error indicates the message was invalid
        if(strcmp(cf_sparkplug_error, "") != 0)
            DebugPrint(cf_sparkplug_error);
        if(host_online){
            // Primary Host is connected to this broker
            DebugPrint("Primary Host is ONLINE");
            //### Should we publish births to let the Primary Host know we're
            //### here, or wait for the Primary Host to send a Rebirth message?
            //### After all, we might have received this message because *we*
            //### just connected to the broker, not the Primary Host.
        }
        else{
            // Primary Host is not connected to this broker
            DebugPrint("Primary Host is OFFLINE");
            //### Enter safe state (not applicable for this module)
        }
    }
    else if(!process_node_cmd_message(topic, payload, len)) {
        // Unrecognized message
        char topic_short[40];
        snprintf(topic_short, sizeof(topic_short), "%s", topic);
        DebugPrintNoEOL("Unrecognized message topic: \"");
        DebugPrintNoEOL(topic_short);
        DebugPrint("\"");
    }
}

/**
 * @brief Publish metrics for THERMISTOR channels and temperature.  Note that we
 * publish this data even if it hasn't changed because the timestamp should
 * show when the data was last read, not when it last changed.
 *
 * @param THERMISTOR_data an array of NUM_THERMISTOR_CHANNELS floats representing the averaged
 * THERMISTOR voltages
 * @param the average temperature reading
 */
void publish_data(float* THERMISTOR_data, float ADC_temperature){
    // Store new THERMISTOR data, converting from raw THERMISTOR values to user units
    for(int i = 0; i < NUMBER_MUX_CHANNELS; i++){
        m_THERMISTOR[i] = THERMISTOR_data[i];
        if(!update_metric(ARRAY_AND_SIZE(NodeMetrics), &m_THERMISTOR[i]))
            DebugPrint(cf_sparkplug_error);
    }

    // Store new ADC temperature
    m_ADC_temperature = ADC_temperature;
    if(!update_metric(ARRAY_AND_SIZE(NodeMetrics), &m_ADC_temperature))
        DebugPrint(cf_sparkplug_error);
}

void publish_calibration_status(bool status){
    if (status == true){
        m_nodeCalibrated = true;
    }
}

/**
 * @brief Updates the NTP object's state, which will periodically sync time
 * with the NTP server.
 *
 * @return true if a sync event occurred TODO: verify this
 * @return false if we failed to sync
 */

bool update_ntp(void){
    ntp.update();
    return ntp.updated();
}

/**
 * @brief Sets the name of topics and the device ID based on the module ID
 * read from the jumpers.
 */
void generateNames(int hardware_id){
    char dev_id = hardware_id + '0';
    node_id.replace(NODE_ID_TOKEN, dev_id);
    nodeBirthTopic.replace(NODE_ID_TOKEN, dev_id);
    nodeDeathTopic.replace(NODE_ID_TOKEN, dev_id);
    nodeDataTopic.replace(NODE_ID_TOKEN, dev_id);
    nodeCmdTopic.replace(NODE_ID_TOKEN, dev_id);
}

/**
 * @brief Set up the arrays holding the separate birth/death sequence number
 * metrics for each broker.
 */
void setup_bdseq_metrics(void){
    for(int br_idx = 0; br_idx < NUM_BROKERS; br_idx++){
        // Copy the template metric data to the array for this broker
        memcpy(&bdseqMetrics[br_idx], &bdseqMetricsTemplate, sizeof(bdseqMetricsTemplate));

        // Set the variable pointer for the metric
        set_metric_variable(ARRAY_AND_SIZE(bdseqMetrics[br_idx]), NMA_bdSeq, &m_bdSeq[br_idx]);

        // Reset the sequence number so it starts at zero when incremented
        m_bdSeq[br_idx] = (uint64_t) -1;
    }
}

/**
 * @brief Initializes the network, sets up and checks the metric arrays, assigns
 * the IP and MAC addresses based on hardware ID jumpers, connects to NTP, and
 * sets up the broker objects.
 *
 * @return true on success
 * @return false if there's some failure
 */
bool network_init(void){
    // Set up the metrics arrays holding the node birth/death sequence numbers
    setup_bdseq_metrics();

    // We need to send at least the node metrics plus bdseq
    set_max_metrics(NUM_ELEM(bdseqMetrics[0]) + NUM_ELEM(NodeMetrics));

    // Check that the alias numbers in the metrics are valid and unique
    for(int i = 0; i < NUM_BROKERS; ++i)
        if(!check_metrics(ARRAY_AND_SIZE(bdseqMetrics[i]), NMA_bdSeq + 1)){
            DebugPrint(cf_sparkplug_error);
            return false;
        }
    if(!check_metrics(ARRAY_AND_SIZE(NodeMetrics     ), EndNodeMetricAlias     )){
        DebugPrint(cf_sparkplug_error);
        return false;
    }

    // Point to our function for getting timestamps
    set_gettimestamp_callback(get_current_time_millis);

    IPAddress ip = MUX0_IP;      // This device's IP
    IPAddress dns(DNS);          // DNS server
    IPAddress gateway(GATEWAY);  // Network gateway
    IPAddress subnet(SUBNET);    // Network subnet

    byte mac[6] = MUX0_MAC;      // This device's MAC address

    // Adjust network addresses based on module ID
    int hardware_id = get_hardware_id();
    if(hardware_id < 0 || hardware_id > MAX_BOARD_ID){
        DebugPrintNoEOL("Invalid hardware ID ");
        DebugPrint(hardware_id);
        return false;
    }
    ip[3]  += hardware_id;
    mac[5] += hardware_id;

    //Generate the MQTT topic names
    generateNames(hardware_id);

    Ethernet.begin(mac, ip, dns, gateway, subnet);
    if(Ethernet.hardwareStatus() == EthernetNoHardware){
        DebugPrint("Ethernet Shield is not connected");
        return false;
    }
    if(Ethernet.linkStatus() == LinkOFF){
        DebugPrint("Ethernet cable is unplugged");
        // This is not a fatal error
    }

    DebugPrintNoEOL("My IP address: ");
    DebugPrint(ip);

    // These should only get called once
    ntp.setUpdateInterval(SECS_IN_HR);
    ntp.begin();
    if(!ntp.updated()){
        DebugPrintNoEOL("Trying NTP update from ");
        DebugPrintNoEOL(ntpIP);
        DebugPrintNoEOL("... ");
        ntp.forceUpdate();
    }
    if(ntp.updated()){
        DebugPrintNoEOL("NTP updated.  Time is ");
        DebugPrint(ntp.getFormattedTime());
    }
    else
        DebugPrint("NTP not updated");

    for(int i = 0; i < NUM_BROKERS; ++i)
        m_broker[i].setClient(enet[i]);

    m_broker[0].setServer(IPAddress(MQTT_BROKER1), MQTT_BROKER1_PORT);

    for(int i = 0; i < NUM_BROKERS; ++i){
        m_broker[i].setCallback(callback_worker);
        m_broker[i].setBufferSize(BIN_BUF_SIZE);
    }

    // Network has been set up successfully
    return true;
}

/**
 * @brief Check each broker is connected, and if not then attempt to connect to
 * it.  Keep the connection to any connected brokers open, process incoming MQTT
 * messages, and publish birth and data messages as necessary.  This function
 * should be called periodically.
 */
void check_brokers(void){
    // Try to connect to any brokers that aren't currently connected
    bool new_connection = false;
    for(int i = 0; i < NUM_BROKERS; ++i){
        PubSubClient *broker = &m_broker[i];
        if(!broker->connected()){
            // Try to connect to the broker
            if(!connect_to_broker(broker, i))
                // Can't connect - ignore this broker
                continue;
            new_connection = true;
            DebugPrintNoEOL("Connected to broker");
            DebugPrint(i+1);
        }
    }

    // If we made a new connection to a broker, publish our birth messages to
    // all connected brokers.  Note that this must be done before handling any
    // incoming messages.
    if(new_connection)
        publish_births();

    // Handle any incoming messages, as well as maintaining our connection to
    // the brokers
    for(int i = 0; i < NUM_BROKERS; ++i){
        PubSubClient *broker = &m_broker[i];
        if(broker->connected())
            broker->loop();
    }

    // Have we been asked to re-publish our birth messages?
    bool rebirth = m_nodeRebirth;
    if(rebirth){
        // Don't publish birth messages if we just did that
        if(!new_connection)
            publish_births();

        // Reset the flags after publishing so that the birth message/s will
        // show which flags triggered them.  Note that an NDATA and/or DDATA
        // message will immediately follow with the flags reset to false.
        if(m_nodeRebirth){
            m_nodeRebirth = false;
            if(!update_metric(ARRAY_AND_SIZE(NodeMetrics), &m_nodeRebirth))
                DebugPrint(cf_sparkplug_error);
        }
    }
    // Publish any Node data that has changed
    publish_node_data();
    // Reset the next server flag if it was set
    if(m_nodeNextServer){
        m_nodeNextServer = false;
        if(!update_metric(ARRAY_AND_SIZE(NodeMetrics), &m_nodeNextServer))
            DebugPrint(cf_sparkplug_error);
    }
}
