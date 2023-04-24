#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "amcom.h"

/// Start of packet character
const uint8_t  AMCOM_SOP         = 0xA1;
const uint16_t AMCOM_INITIAL_CRC = 0xFFFF;

static uint16_t AMCOM_UpdateCRC(uint8_t byte, uint16_t crc)
{
    byte ^= (uint8_t)(crc & 0x00ff);
    byte ^= (uint8_t)(byte << 4);
    return ((((uint16_t)byte << 8) | (uint8_t)(crc >> 8)) ^ (uint8_t)(byte >> 4) ^ ((uint16_t)byte << 3));
}


void AMCOM_InitReceiver(AMCOM_Receiver* receiver, AMCOM_PacketHandler packetHandlerCallback, void* userContext) {
    // TODO
    receiver->userContext = userContext;
    receiver->packetHandler = packetHandlerCallback;
}

size_t AMCOM_Serialize(uint8_t packetType, const void* payload, size_t payloadSize, uint8_t* destinationBuffer) {
    // DONE
    size_t size = payloadSize + sizeof(AMCOM_PacketHeader); // header(5 bytes) + payload
    uint16_t crc = AMCOM_INITIAL_CRC;

    crc = AMCOM_UpdateCRC(packetType, crc);
    crc = AMCOM_UpdateCRC((uint8_t) payloadSize, crc);
    if(NULL != payload){
        for(size_t i = 0; i < payloadSize; ++i){
            crc = AMCOM_UpdateCRC(((uint8_t *)payload)[i], crc);
        }
    }

    printf("Serializer crc: %#.4x\t", crc);

    uint8_t header[5] = {AMCOM_SOP, packetType, (uint8_t)payloadSize,(uint8_t)(crc & 0xFF), (uint8_t)(crc >> 8 & 0xFF)};

    if(NULL == payload){
        memcpy(destinationBuffer, header, sizeof(header));
        return sizeof(header);
    }
    else{
        uint8_t packet[size];

        // write header to packet
        for(size_t i = 0; i < sizeof(AMCOM_PacketHeader); ++i){
            packet[i] = (uint8_t) header[i];
        }

        // write payload to packet
        for(size_t i = 0; i < payloadSize; ++i){
            packet[i+5] = ((uint8_t *)payload)[i];
        }

        memcpy(destinationBuffer, packet, sizeof(packet));
        return size;
    }
}

void AMCOM_Deserialize(AMCOM_Receiver* receiver, const void* data, size_t dataSize) {
    // ------ DONE -------

    // state machine
    for(size_t j = 0; j < dataSize; ++j){
        switch(receiver->receivedPacketState){
            case AMCOM_PACKET_STATE_EMPTY:
                if(AMCOM_SOP == ((uint8_t *)data)[j]){
                    receiver->receivedPacket.header.sop = ((uint8_t *)data)[j];
                    receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_SOP;
                }
                break;

            case AMCOM_PACKET_STATE_GOT_SOP:
                receiver->receivedPacket.header.type = ((uint8_t *)data)[j];
                receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_TYPE;
                break;

            case AMCOM_PACKET_STATE_GOT_TYPE:
                if(200 >= ((uint8_t *)data)[j]){
                    receiver->receivedPacket.header.length = ((uint8_t *)data)[j];
                    receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_LENGTH;
                }
                break;

            case AMCOM_PACKET_STATE_GOT_LENGTH:
                // get first byte of crc
                receiver->receivedPacket.header.crc = ((uint8_t *)data)[j];
                receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_CRC_LO;
                break;

            case AMCOM_PACKET_STATE_GOT_CRC_LO:
                // got second byte of crc
                receiver->receivedPacket.header.crc = receiver->receivedPacket.header.crc | (((uint8_t *)data)[j] << 8);
                if(0 < receiver->receivedPacket.header.length){
                    receiver->receivedPacketState = AMCOM_PACKET_STATE_GETTING_PAYLOAD;
                    break;
                }else{
                    receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_WHOLE_PACKET;
                }


            case AMCOM_PACKET_STATE_GETTING_PAYLOAD:
                if(receiver->receivedPacket.header.length > 0){
                    receiver->receivedPacket.payload[receiver->payloadCounter] = ((uint8_t *)data)[j];
                    receiver->payloadCounter++;
                }
                if(receiver->payloadCounter == receiver->receivedPacket.header.length){
                    receiver->receivedPacketState = AMCOM_PACKET_STATE_GOT_WHOLE_PACKET;
                }else{
                    break;
                }


            case AMCOM_PACKET_STATE_GOT_WHOLE_PACKET:;
                uint16_t crc = AMCOM_INITIAL_CRC;
                crc = AMCOM_UpdateCRC((uint8_t)receiver->receivedPacket.header.type, crc);
                crc = AMCOM_UpdateCRC((uint8_t)receiver->receivedPacket.header.length, crc);

                if(0 < receiver->receivedPacket.header.length){
                    for(size_t i = 0; i < receiver->receivedPacket.header.length; ++i){
                        crc = AMCOM_UpdateCRC(receiver->receivedPacket.payload[i], crc);
                    }
                }
                if(crc == receiver->receivedPacket.header.crc){
                    receiver->packetHandler(&receiver->receivedPacket,receiver->userContext);
                }

                receiver->receivedPacketState = AMCOM_PACKET_STATE_EMPTY;
                receiver->payloadCounter = 0;
                memset(&receiver->receivedPacket, 0x00, sizeof(receiver->receivedPacket));
                break;
        }

    } // data[j]

}
