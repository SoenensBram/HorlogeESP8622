#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"

#include "driver/i2c.h"
#include "driver/gpio.h"

/*
Pins Used In Pinout
Pin 20 GPIO11   RxSupplyEn
Pin 21 GPIO6    TxsupplyEn
Pin 22 GPIO7    PowerEn
Pin 23 GPIO8    ResetAfe
Pin 24 GPIO5    DataReadyInterupt
Pin 9  IO14     I2C_SCL
Pin 14 IO2      I2C_SDA
*/
static uint32_t i2c_frequency = 100000;
unsigned int
RxSupplyEnable      = 11,
TxSupplyEnable      = 6,
PowerEnable         = 7,
ResetAfe            = 8,
DataReadyInterupt   = 5,
I2cMasterSclIo      = 9,                /*!< gpio number for I2C master clock */
I2cMasterSdaIo      = 14;               /*!< gpio number for I2C master data  */
int I2cMasterNum = I2C_NUM_0;           /*!< I2C port number for master dev */
char Afe4404Address = 0x58<<1;          /*!< slave address for Afe4404 sensor, shift by 1 for 8-bit representation of 7-bit address */
bool AckCheckEn = true;                 /*!< I2C master will check ack from slave*/
bool AchChackDis = false;               /*!< I2C master will not check ack from slave */
bool I2cPullup = false;                 /*!< I2C pullup */
char AckVal = 0x0;                      /*!< I2C ack value */
char NackVal = 0x1;                     /*!< I2C nack value */
char LastNackVal = 0x2;                 /*!< I2C last_nack value */
unsigned int RegisterEnteriesAfe4404 = 38;
uint8_t Address[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x20, 0x21, 0x22, 0x23, 0x32, 0x33, 0x36, 0x37};
uint32_t Value[] =  {0x000050, 0x00018F, 0x000320, 0x0004AF, 0x0001E0, 0x00031F, 0x000370, 0x0004AF, 0x000000, 0x00018F, 0x0004FF, 0x00063E, 0x000198, 0x0005BB, 0x0005C4, 0x0009E7, 0x0009F0, 0x000E13, 0x000E1C, 0x00123F, 0x000191, 0x000197, 0x0005BD, 0x0005C3, 0x0009E9, 0x0009EF, 0x000E15, 0x000E1B, 0x009C3F, 0x000103, 0x008003, 0x000003, 0x000400, 0x000000, 0x00155F, 0x00991F, 0x000190, 0x00031F, 0x000050, 0x00018F, 0x000320, 0x0004AF, 0x0001E0, 0x00031F, 0x000370, 0x0004AF, 0x000000, 0x00018F, 0x0004FF, 0x00063E, 0x000198, 0x0005BB, 0x0005C4, 0x0009E7, 0x0009F0, 0x000E13, 0x000E1C, 0x00123F, 0x000191, 0x000197, 0x0005BD, 0x0005C3, 0x0009E9, 0x0009EF, 0x000E15, 0x000E1B, 0x009C3F, 0x000103, 0x008003, 0x000003, 0x000400, 0x000000, 0x00155F, 0x00991F, 0x000190, 0x00031F};