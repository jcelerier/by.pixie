#ifndef _PRO_DRIVER_H
#define _PRO_DRIVER_H


#include <windows.h>
#include <process.h>
#include <assert.h>
#include <tchar.h>
#include <time.h>
#include <conio.h>
#include <ftd2xx.h>

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <memory>

#define APP_VERSION "1.02"

/******************** PIXIE LABELS: ASSIGNED AS PER API *********************/

#define GET_CONFIG_LABEL		3
#define SET_CONFIG_LABEL		4
#define LED_UPDATE_LABEL	    6
#define SHOW_QUERY_LABEL		7
#define SHOW_UPDATE_LABEL		8
#define SHOW_READ_LABEL			9
#define GET_SN_LABEL			10
#define QUERY_HW_VERSION		14

/***********************************************************************************/
#define ONE_BYTE	1
#define DMX_START_CODE 0x7E 
#define DMX_END_CODE 0xE7 
#define OFFSET 0xFF
#define DMX_HEADER_LENGTH 4
#define BYTE_LENGTH 8
#define HEADER_RDM_LABEL 5
#define NO_RESPONSE 0
#define DMX_PACKET_SIZE 512
#define RX_BUFFER_SIZE 40960
#define TX_BUFFER_SIZE 40960
#define PIXIE_HW_VERSION	0x30
#define MAX_PIXIE_CHANNELS 510

#pragma pack(1)
struct PixieConfigGet {
        uint8_t FirmwareLSB;
        uint8_t FirmwareMSB;
		uint8_t Personality;
        uint8_t GroupSize;
        uint8_t PixelOrder;
        uint8_t LEDType;
        uint8_t PLayOnDMXLoss;
		uint16_t DroppedMessages;
};

struct PixieConfigSet {
		uint8_t Personality;
        uint8_t GroupSize;
        uint8_t PixelOrder;
        uint8_t LEDType;
        uint8_t PLayOnDMXLoss;
};
#pragma pack()

#define MAX_DEVICES 20
#define SEND_NOW 0
#define TRUE 1
#define FALSE 0
#define HEAD 0
#define IO_ERROR 9

struct Pixie
{
    FT_HANDLE handle{};
    PixieConfigGet config;
};

int FTDI_SendData(FT_HANDLE device_handle, int label, uint8_t *data, int length);
int FTDI_ReceiveData(FT_HANDLE device_handle, int label, uint8_t *data, unsigned int expected_length);
std::unique_ptr<Pixie> createPixie(int device_num);
void SendDMXToPixe(FT_HANDLE device_handle, uint8_t *);

void FTDI_ClosePort(FT_HANDLE device_handle);
void FTDI_PurgeBuffer(FT_HANDLE device_handle);
void FTDI_Reload();

#endif
