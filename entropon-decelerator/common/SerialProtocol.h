// ==========================================
// SerialProtocol.h - Communication protocol
// ==========================================

#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include <Arduino.h>
#include "TimeTypes.h"

class SerialProtocol {
public:
    // Message types
    static const uint8_t MSG_STATE = 0x01;
    static const uint8_t MSG_TIME = 0x02;
    static const uint8_t MSG_STOP = 0x03;
    static const uint8_t MSG_ACK = 0x04;
    static const uint8_t MSG_ERROR = 0xFF;
    
    // Protocol constants
    static const uint8_t START_BYTE = 0xAA;
    static const uint8_t END_BYTE = 0x55;
    static const uint16_t MAX_PAYLOAD_SIZE = 64;
    
private:
    Stream* serial;
    uint8_t receiveBuffer[MAX_PAYLOAD_SIZE + 5];  // Start + Type + Length + Payload + Checksum + End
    uint16_t receiveIndex;
    uint32_t lastReceiveTime;
    
public:
    // Constructor
    SerialProtocol(Stream* serialPort) : serial(serialPort), receiveIndex(0), lastReceiveTime(0) {}
    
    // Send state update
    void sendStateUpdate(const ExhibitState& state) {
        uint8_t payload[sizeof(ExhibitState)];
        memcpy(payload, &state, sizeof(ExhibitState));
        sendMessage(MSG_STATE, payload, sizeof(ExhibitState));
    }
    
    // Send time update
    void sendTimeUpdate(const TimeValue& outside, const TimeValue& chamber) {
        uint8_t payload[sizeof(TimeValue) * 2];
        memcpy(payload, &outside, sizeof(TimeValue));
        memcpy(payload + sizeof(TimeValue), &chamber, sizeof(TimeValue));
        sendMessage(MSG_TIME, payload, sizeof(TimeValue) * 2);
    }
    
    // Send stop signal
    void sendStop() {
        sendMessage(MSG_STOP, nullptr, 0);
    }
    
    // Send acknowledgment
    void sendAck() {
        sendMessage(MSG_ACK, nullptr, 0);
    }
    
    // Receive message (non-blocking)
    bool receiveMessage(uint8_t* msgType, uint8_t* payload, uint16_t* payloadLength) {
        // Check for timeout
        if (receiveIndex > 0 && millis() - lastReceiveTime > 100) {
            receiveIndex = 0;  // Reset on timeout
        }
        
        // Read available bytes
        while (serial->available()) {
            uint8_t byte = serial->read();
            lastReceiveTime = millis();
            
            // Look for start byte
            if (receiveIndex == 0) {
                if (byte == START_BYTE) {
                    receiveBuffer[0] = byte;
                    receiveIndex = 1;
                }
                continue;
            }
            
            // Store byte
            if (receiveIndex < sizeof(receiveBuffer)) {
                receiveBuffer[receiveIndex++] = byte;
                
                // Check if we have minimum message length (Start + Type + Length + Checksum + End)
                if (receiveIndex >= 5) {
                    uint8_t length = receiveBuffer[2];
                    uint16_t expectedLength = 5 + length;  // Full message length
                    
                    if (receiveIndex >= expectedLength) {
                        // Verify end byte
                        if (receiveBuffer[expectedLength - 1] == END_BYTE) {
                            // Calculate checksum
                            uint8_t checksum = 0;
                            for (uint16_t i = 1; i < expectedLength - 2; i++) {
                                checksum ^= receiveBuffer[i];
                            }
                            
                            // Verify checksum
                            if (checksum == receiveBuffer[expectedLength - 2]) {
                                // Valid message received
                                *msgType = receiveBuffer[1];
                                *payloadLength = length;
                                
                                if (length > 0) {
                                    memcpy(payload, &receiveBuffer[3], length);
                                }
                                
                                receiveIndex = 0;  // Reset for next message
                                return true;
                            }
                        }
                        
                        // Invalid message, reset
                        receiveIndex = 0;
                    }
                }
            } else {
                // Buffer overflow, reset
                receiveIndex = 0;
            }
        }
        
        return false;
    }
    
    // Process incoming messages (helper for common patterns)
    bool processIncoming(ExhibitState* state, TimeValue* outside, TimeValue* chamber) {
        uint8_t msgType;
        uint8_t payload[MAX_PAYLOAD_SIZE];
        uint16_t payloadLength;
        
        if (receiveMessage(&msgType, payload, &payloadLength)) {
            switch (msgType) {
                case MSG_STATE:
                    if (payloadLength == sizeof(ExhibitState)) {
                        memcpy(state, payload, sizeof(ExhibitState));
                        return true;
                    }
                    break;
                    
                case MSG_TIME:
                    if (payloadLength == sizeof(TimeValue) * 2) {
                        memcpy(outside, payload, sizeof(TimeValue));
                        memcpy(chamber, payload + sizeof(TimeValue), sizeof(TimeValue));
                        return true;
                    }
                    break;
                    
                case MSG_STOP:
                    // Set flag or call callback
                    return true;
                    
                case MSG_ACK:
                    // Handle acknowledgment if needed
                    return true;
            }
        }
        
        return false;
    }
    
private:
    // Send a message with the protocol format
    void sendMessage(uint8_t msgType, const uint8_t* payload, uint16_t payloadLength) {
        if (payloadLength > MAX_PAYLOAD_SIZE) return;
        
        // Send start byte
        serial->write(START_BYTE);
        
        // Send message type
        serial->write(msgType);
        
        // Send payload length
        serial->write((uint8_t)payloadLength);
        
        // Calculate checksum
        uint8_t checksum = msgType ^ (uint8_t)payloadLength;
        
        // Send payload and update checksum
        for (uint16_t i = 0; i < payloadLength; i++) {
            serial->write(payload[i]);
            checksum ^= payload[i];
        }
        
        // Send checksum
        serial->write(checksum);
        
        // Send end byte
        serial->write(END_BYTE);
        
        // Flush to ensure immediate transmission
        serial->flush();
    }
};

#endif // SERIAL_PROTOCOL_H