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
I2cMasterSclIo      = 2,                /*!< gpio number for I2C master clock */
I2cMasterSdaIo      = 14;               /*!< gpio number for I2C master data  */
i2c_port_t I2cMasterNum = I2C_NUM_0;    /*!< I2C port number for master dev */
char Afe4404Address = 0x58<<1;          /*!< slave address for Afe4404 sensor, shift by 1 for 8-bit representation of 7-bit address */
bool AckCheckEn = true;                 /*!< I2C master will check ack from slave*/
bool AchChackDis = false;               /*!< I2C master will not check ack from slave */
bool I2cPullup = true;                  /*!< I2C pullup */
char AckVal = 0x0;                      /*!< I2C ack value */
char NackVal = 0x1;                     /*!< I2C nack value */
char LastNackVal = 0x2;                 /*!< I2C last_nack value */
unsigned int RegisterEnteriesAfe4404 = 64;
//uint8_t Address[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x20, 0x21, 0x22, 0x23, 0x32, 0x33, 0x36, 0x37};
uint32_t Value[] =  
{0,                 //SW_RESET, TM_COUNT_RST, REG_READ
 100,               //LED2STC
 398,               //LED2ENDC
 800,               //LED1LEDSTC
 1198,              //LED1LEDENDC
 500,               //ALED2STC/LED3STC
 798,               //ALED2ENDC/LED3ENDC
 900,               //LED1STC
 1198,              //LED1ENDC
 0,                 //LED2LEDSTC
 398,               //LED2LEDENDC
 1300,              //ALED1STC
 1598,              //ALED1ENDC
 5608,              //LED2CONVST
 6067,              //LED2CONVEND
 6077,              //ALED2CONVST\LED3CONVST
 6536,              //ALED2CONVEND\LED3CONVEND
 6546,              //LED1CONVST
 7006,              //LED1CONVEND
 7016,              //ALED1CONVST
 7475,              //ALED1CONVEND
 5600,              //ADCRSTSTCT0
 5606,              //ADCRSTENDCT0
 6069,              //ADCRSTSTCT1
 6075,              //ADCRSTENDCT1
 6538,              //ADCRSTSTCT2
 6544,              //ADCRSTENDCT2
 7008,              //ADCRSTSTCT3
 7014,              //ADCRSTENDCT3
 39999,             //PRPCT
 0x000000,          //TIMEREN, NUMAV
 0x000000,          //ENSEPGAIN, TIA_CF_SEP, TIAGAIN_SEP
 0x000000,          //PROG_TG_EN, TIA_CF, TIA_GAIN
 0x000000,          //ILED_2X, DYNAMIC2, OSC_ENABLE, DYNAMIC3, DYNAMIC4, PDNRX, PDNAFE
 0x000000,          //ENABLE_CLKOUT, CLKDIV_CLKOUT
 0x000000,          //read only LED2VAL
 0x000000,          //read only ALED2VAL\LED3VAL
 0x000000,          //read only LED1VAL
 0x000000,          //read only ALED1VAL
 0x000000,          //read only LED2-ALED2VAL
 0x000000,          //read only LED1-ALED1VAL
 0x000000,          //PD_DISCONNECT, ENABLE_INPUT_SHORT, CLKDIV_EXTMODE
 7675,              //PDNCYCLESTC
 39199,             //PDNCYCLEENDC
 0x000000,          //PROG_TG_STC
 0x000000,          //PROG_TG_ENDC
 400,               //LED3LEDSTC
 798,               //LED3LEDENDC
 0x000000,          //CLKDIV_PRF
 0x000000,          //POL_OFFDAC_LED2, I_OFFDAC_LED2, POL_OFFDAC_AMB1, I_OFFDAC_AMB1, POL_OFFDAC_LED1, I_OFFDAC_LED1, POL_OFFDAC_AMB2/POL_OFFDAC_LED3, I_OFFDAC_AMB2/I_OFFDAC_LED3
 0x000000,          //DEC_EN, DEC_FACTOR
 0x000000,          //read only AVG_LED2-ALED2VAL
 0x000000};         //read only AVG_LED1-ALED1VAL