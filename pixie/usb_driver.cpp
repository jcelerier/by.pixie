#include <ext.h>
#include <usb_driver.h>
#undef error
#undef post

/* Function : FTDI_ClosePort
 * Author	: ENTTEC
 * Purpose  : Closes the Pixie Device Handle
 * Parameters: none
 **/
void FTDI_ClosePort(FT_HANDLE device_handle)
{
    if (device_handle != NULL)
        FT_Close(device_handle);
}

void FTDI_Reload()
{
    WORD wVID = 0x0403;
    WORD wPID = 0x6001;
    FT_STATUS ftStatus;

    printf ("\nReloading devices for use with drivers ");
    ftStatus = FT_Reload(wVID,wPID);
    // Must wait a while for devices to be re-enumerated
    Sleep(3500);
    if(ftStatus != FT_OK)
    {
        post("Reloading Driver FAILED");
    }
    else
        post("Reloading Driver D2XX PASSED");
}

/* Function : FTDI_SendData
 * Author	: ENTTEC
 * Purpose  : Send Data (DMX or other packets) to the USB Device
 * Parameters: Label, Pointer to Data Structure, Length of Data
 **/
int FTDI_SendData(FT_HANDLE handle, int label, uint8_t *data, int length)
{
    uint8_t end_code = DMX_END_CODE;
    FT_STATUS res = 0;
    DWORD bytes_to_write = length;
    DWORD bytes_written = 0;
    HANDLE event = NULL;
    int size=0;
    // Form Packet Header
    uint8_t header[DMX_HEADER_LENGTH];
    header[0] = DMX_START_CODE;
    header[1] = label;
    header[2] = length & OFFSET;
    header[3] = length >> BYTE_LENGTH;
    // Write The Header
    res = FT_Write(	handle,(uint8_t *)header,DMX_HEADER_LENGTH,&bytes_written);
    if (bytes_written != DMX_HEADER_LENGTH) return  NO_RESPONSE;
    // Write The Data
    res = FT_Write(	handle,(uint8_t *)data,length,&bytes_written);
    if (bytes_written != length) return  NO_RESPONSE;
    // Write End Code
    res = FT_Write(	handle,(uint8_t *)&end_code,ONE_BYTE,&bytes_written);
    if (bytes_written != ONE_BYTE) return  NO_RESPONSE;
    if (res == FT_OK)
        return TRUE;
    else
        return FALSE;
}

/* Function : FTDI_ReceiveData
 * Author	: ENTTEC
 * Purpose  : Receive Data (DMX or other packets) from the USB DEVICE
 * Parameters: Label, Pointer to Data Structure, Length of Data
 **/
int FTDI_ReceiveData(FT_HANDLE handle, int label, uint8_t *data, unsigned int expected_length)
{

    FT_STATUS res = 0;
    DWORD length = 0;
    DWORD bytes_to_read = 1;
    DWORD bytes_read =0;
    uint8_t byte = 0;
    HANDLE event = NULL;
    char buffer[600];
    // Check for Start Code and matching Label
    while (byte != label)
    {
        while (byte != DMX_START_CODE)
        {
            res = FT_Read(handle,(uint8_t *)&byte,ONE_BYTE,&bytes_read);
            if(bytes_read== NO_RESPONSE) return  NO_RESPONSE;
        }
        res = FT_Read(handle,(uint8_t *)&byte,ONE_BYTE,&bytes_read);
        if (bytes_read== NO_RESPONSE) return  NO_RESPONSE;
    }
    // Read the rest of the Header Byte by Byte -- Get Length
    res = FT_Read(handle,(uint8_t *)&byte,ONE_BYTE,&bytes_read);
    if (bytes_read== NO_RESPONSE) return  NO_RESPONSE;
    length = byte;
    res = FT_Read(handle,(uint8_t *)&byte,ONE_BYTE,&bytes_read);
    if (res != FT_OK) return  NO_RESPONSE;
    length += ((uint32_t)byte)<<BYTE_LENGTH;
    // Check Length is not greater than allowed
    if (length > DMX_PACKET_SIZE)
        return  NO_RESPONSE;
    // Read the actual Response Data
    res = FT_Read(handle,buffer,length,&bytes_read);
    if(bytes_read != length) return  NO_RESPONSE;
    // Check The End Code
    res = FT_Read(handle,(uint8_t *)&byte,ONE_BYTE,&bytes_read);
    if(bytes_read== NO_RESPONSE) return  NO_RESPONSE;
    if (byte != DMX_END_CODE) return  NO_RESPONSE;
    // Copy The Data read to the buffer passed
    memcpy(data,buffer,expected_length);
    return TRUE;
}


/* Function : FTDI_PurgeBuffer
 * Author	: ENTTEC
 * Purpose  : Clears the buffer used internally by the USB DEVICE
 * Parameters: none
 **/
void FTDI_PurgeBuffer(FT_HANDLE device_handle)
{
    FT_Purge (device_handle,FT_PURGE_TX);
    FT_Purge (device_handle,FT_PURGE_RX);
}

std::unique_ptr<Pixie> createPixie(int device_num)
{
    Pixie pixie;
    int ReadTimeout = 120;
    int WriteTimeout = 100;
    uint8_t Serial[4];
    uint8_t HWVersion = 0;
    long version;
    uint8_t major_ver,minor_ver,build_ver;
    int recvd =0;
    uint8_t byte = 0;
    int size = 0;
    int res = 0;
    int tries =0;
    uint8_t latencyTimer;
    FT_STATUS ftStatus;

    // Try at least 3 times
    do  {
        ftStatus = FT_Open(device_num, &pixie.handle);
        Sleep(500);
        tries ++;
    } while ((ftStatus != FT_OK) && (tries < 3));

    if (ftStatus == FT_OK)
    {
        // D2XX Driver Version
        ftStatus = FT_GetDriverVersion(pixie.handle,(LPDWORD)&version);
        if (ftStatus == FT_OK)
        {
            major_ver = (uint8_t) version >> 16;
            minor_ver = (uint8_t) version >> 8;
            build_ver = (uint8_t) version & 0xFF;
            post("D2XX Driver Version:: %02X.%02X.%02X ",major_ver,minor_ver,build_ver);
        }
        else
            post("Unable to Get D2XX Driver Version") ;

        // Latency Timer -- used to minimize timeouts
        ftStatus = FT_GetLatencyTimer (pixie.handle, (PUCHAR)&latencyTimer);

        // These are important values that can be altered to suit your needs
        // Timeout in microseconds: Too high or too low value should not be used
        FT_SetTimeouts(pixie.handle, ReadTimeout,WriteTimeout);
        // Buffer size in bytes (multiple of 4096)
        FT_SetUSBParameters(pixie.handle, RX_BUFFER_SIZE,TX_BUFFER_SIZE);
        // Good idea to purge the buffer on initialize
        FT_Purge (pixie.handle, FT_PURGE_RX);

        // Check Version of Device to vaildate it's a Pixie
        res = NO_RESPONSE;
        tries = 0;
        do {
            res = FTDI_SendData(pixie.handle, QUERY_HW_VERSION,(uint8_t *)&size,0);
            Sleep(100);
            res = FTDI_ReceiveData(pixie.handle, QUERY_HW_VERSION,&HWVersion,1);
            tries ++;
        } while ((res == NO_RESPONSE) && (tries < 3));

        if ((HWVersion & 0xF0) == PIXIE_HW_VERSION)
            post(" ---- PIXIE Found! Getting Device Information ------");
        else
        {
            post(" error: USB-Device is not Pixie: hw-version read: %d ",(HWVersion & 0xF0));
            return FALSE;
        }

        // Send Get Config to get Device Info
        post("Sending GET_CONFIG to PIXIE ... ");
        res = FTDI_SendData(pixie.handle, GET_CONFIG_LABEL,(uint8_t *)&size,0);
        if (res == NO_RESPONSE)
        {
            // try again - reset buffer on device
            FT_Purge (pixie.handle, FT_PURGE_TX);
            res = FTDI_SendData(pixie.handle, GET_CONFIG_LABEL,(uint8_t *)&size,0);
            if (res == NO_RESPONSE)
            {
                FTDI_ClosePort(pixie.handle);
                post(" USB Device did not reply");
                return  NO_RESPONSE;
            }
        }
        else
            post(" Pixie Connected Succesfully");
        // Receive Config Response
        post("Waiting for GET_CONFIG REPLY packet... ");
        res = FTDI_ReceiveData(pixie.handle, GET_CONFIG_LABEL,(uint8_t *)&pixie.config, sizeof(PixieConfigGet));
        if (res == NO_RESPONSE)
        {
            // try again - reset buffer on device
            FT_Purge (pixie.handle, FT_PURGE_TX);
            res = FTDI_ReceiveData(pixie.handle, GET_CONFIG_LABEL,(uint8_t *)&pixie.config, sizeof(PixieConfigGet));
            if (res == NO_RESPONSE)
            {
                FTDI_ClosePort(pixie.handle);
                post(" USB Device did not reply");
                return  NO_RESPONSE;
            }
        }
        else
            post(" GET_CONFIG REPLY Received ... ");

        res = FTDI_SendData(pixie.handle, GET_SN_LABEL,(uint8_t *)&size,0);
        res = FTDI_ReceiveData(pixie.handle, GET_SN_LABEL,(uint8_t *)&Serial,4);
        // Display All Info available
        post("-----------::PIXIE Connected [Information Follows]::------------");
        post("  FIRMWARE VERSION: %d.%d",pixie.config.FirmwareMSB, pixie.config.FirmwareLSB);
        post("  PERSONALITY: %d", pixie.config.Personality);
        post("  LED Group Size: %d", pixie.config.GroupSize);
        post("  RGB Order: %d", pixie.config.PixelOrder);
        post("  LED STRIP TYPE: %d", pixie.config.LEDType);
        post("  START Show On DMX LOSS: %d", pixie.config.PLayOnDMXLoss);
        post("----------------------------------------------------------------\n\n");
        return std::make_unique<Pixie>(pixie);
    }
    else // Can't open Device
    {
        return {};
    }
}

/* Function : SendDMXToPixe
 * Author	: ENTTEC
 * Purpose  : Sends the passed data array to Pixie; Define output config and universe as required (above)
 * Parameters: dmx data array (510 bytes)
 **/
void SendDMXToPixe(FT_HANDLE handle, uint8_t* dmxdata)
{
    // Use this to target the correct section of each output (refer to API for more details)
    // Currently set to output 2 universes on both outputs
    uint8_t PixieOutputs[] = {0,1,2,3};
    uint8_t PixieOutputConfig = 1;

    uint8_t output_count = sizeof(PixieOutputs)/sizeof(PixieOutputs[0]);
    uint8_t output_universe = 0;
    for (uint8_t i=0; i< output_count; i++)
    {
        output_universe = PixieOutputs[i];
        dmxdata[510] = 0 + (PixieOutputConfig << 2);
        if (output_universe > 0)
            dmxdata[510] += (output_universe << 0);
        if (output_count == 1)
            Sleep(25);
        // actuall send to Pixie now ..
        FTDI_SendData(handle, LED_UPDATE_LABEL, dmxdata,MAX_PIXIE_CHANNELS+1);
    }
}
/*

// our main function with everything to do the test
int main(int argc, char**argv)
{

        DWORD Num_Devices =0;
        uint16_t device_connected =0;
        int i=0;
        int device_num=0;
        BOOL res = 0;
        FT_STATUS ftStatus;
        FT_HANDLE DevHandler;
        DWORD Flags;
        DWORD ID;
        DWORD Type;
        DWORD LocId;
        char SerialNumber[16];
        char Description[64];
        uint8_t output_config = 2;
        uint8_t output_universe = 0;
        uint8_t DMX[MAX_PIXIE_CHANNELS+1];

        post("Enttec PIXIE - C - Windows - Sample Test - VERSION %s \n",APP_VERSION);
        post(" Press Enter to Look for and connect to Pixie :");
        _getch();

        // If you face problems identifying the USB DEVICE: Use this code to reload device drivers: takes a few secs
        FTDI_Reload();

        // Create the device information list
        ftStatus = FT_CreateDeviceInfoList(&Num_Devices);
        if (ftStatus == FT_OK)
                post(" Number of USB Devices found: %d\n",Num_Devices);
        if (Num_Devices > 0)
        {
                // got through all devices found to get info
                 for (i=0;i<Num_Devices;i++)
                 {
                        device_num = i;
                        post(" Testing Device: %d\n",device_num);
                        // Returns usb-level info
                        FT_GetDeviceInfoDetail(0, &Flags, &Type, &ID, &LocId, SerialNumber, Description, &DevHandler);
                        if (ftStatus == FT_OK)
                        {
                                post(" SerialNumber: %s\n",SerialNumber);
                                post(" Description: %s\n",Description);
                                FT_Close(DevHandler);
                        }
                        device_connected = isPixie(device_num);
                        if (device_connected)
                                break;
                 }
        }

        if (device_connected)
        {
                // Send DMX to Pixie Examples

                // Send All to white
                memset(DMX,0,sizeof(DMX));
                for (int x=0; x< MAX_PIXIE_CHANNELS; x++)
                        DMX[x] = 255;
                SendDMXToPixe(DMX);
                Sleep(999);

                // Send All to green
                memset(DMX,0,sizeof(DMX));
                SendDMXToPixe(DMX);
                for (int x=0; x< MAX_PIXIE_CHANNELS; x++)
                {
                        if (x%3 == 0)
                                DMX[x] = 199;
                }
                SendDMXToPixe(DMX);
                Sleep(999);

                // Send All to pink
                memset(DMX,0,sizeof(DMX));
                SendDMXToPixe(DMX);
                for (int x=0; x< MAX_PIXIE_CHANNELS; x++)
                {
                        if (x%3 == 0)
                                DMX[x] = 0;
                        else if (x%2 == 0)
                                DMX[x] = 99;
                        else
                                DMX[x] = 0;
                }
                SendDMXToPixe(DMX);

                // Send Dynamic Examples
                SendRGBChase();
                SendRainbowFades();

        }

        // Finish all done
        post(" Press Enter to Exit :");
        _getch();
        return 0;
}
*/
