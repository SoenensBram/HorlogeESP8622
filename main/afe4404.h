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

static xQueueHandle gpio_evt_queue = NULL;

static uint32_t i2c_frequency = 100000;
unsigned int
RxSupplyEnable      = 16,
TxSupplyEnable      = 5,
PowerEnable         = 4,
ResetAfe            = 13,
DataReadyInterupt   = 12,
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
unsigned int RegisterEnteriesAfe4404 = 54;
uint32_t Value[] =  {
0,                 //SW_RESET, TM_COUNT_RST, REG_READ
100,                //LED2STC
399,               //LED2ENDC
802,               //LED1LEDSTC
1201,              //LED1LEDENDC
501,               //ALED2STC/LED3STC
800,               //ALED2ENDC/LED3ENDC
902,               //LED1STC
1201,              //LED1ENDC
0,                 //LED2LEDSTC
399,               //LED2LEDENDC
1303,              //ALED1STC
1602,              //ALED1ENDC
409,               //LED2CONVST
1468,              //LED2CONVEND
1478,              //ALED2CONVST\LED3CONVST
2537,              //ALED2CONVEND\LED3CONVEND
2547,              //LED1CONVST
3606,              //LED1CONVEND
3616,              //ALED1CONVST
4675,              //ALED1CONVEND
401,               //ADCRSTSTCT0
407,               //ADCRSTENDCT0
1470,              //ADCRSTSTCT1
1476,              //ADCRSTENDCT1
2539,              //ADCRSTSTCT2
2545,              //ADCRSTENDCT2
3608,              //ADCRSTSTCT3
3614,              //ADCRSTENDCT3
39999,               //PRPCT
259,               //TIMEREN, NUMAV
32771,             //ENSEPGAIN, TIA_CF_SEP, TIAGAIN_SEP
3,                 //PROG_TG_EN, TIA_CF, TIA_GAIN
12495,             //ILED3,ILED2, ILED1
1065496,               //DYNAMIC1, ILED_2X, DYNAMIC2, OSC_ENABLE, DYNAMIC3, DYNAMIC4, PDNRX, PDNAFE
512,               //ENABLE_CLKOUT, CLKDIV_CLKOUT
0,       //read only LED2VAL
0,       //read only ALED2VAL\LED3VAL
0,       //read only LED1VAL
0,       //read only ALED1VAL
0,       //read only LED2-ALED2VAL
0,       //read only LED1-ALED1VAL
0,                 //PD_DISCONNECT, ENABLE_INPUT_SHORT, CLKDIV_EXTMODE
5475,              //PDNCYCLESTC
39199,             //PDNCYCLEENDC
0,                 //PROG_TG_STC
0,                 //PROG_TG_ENDC
409,               //LED3LEDSTC
800,               //LED3LEDENDC
1,                 //CLKDIV_PRF
0,                 //POL_OFFDAC_LED2, I_OFFDAC_LED2, POL_OFFDAC_AMB1, I_OFFDAC_AMB1, POL_OFFDAC_LED1, I_OFFDAC_LED1, POL_OFFDAC_AMB2/POL_OFFDAC_LED3, I_OFFDAC_AMB2/I_OFFDAC_LED3
0,                 //DEC_EN, DEC_FACTOR
0,       //read only AVG_LED2-ALED2VAL
0};      //read only AVG_LED1-ALED1VAL
uint8_t Address[] = {
0x00,              //SW_RESET, TM_COUNT_RST, REG_READ
0x01,              //LED2STC
0x02,              //LED2ENDC
0x03,              //LED1LEDSTC
0x04,              //LED1LEDENDC
0x05,              //ALED2STC/LED3STC
0x06,              //ALED2ENDC/LED3ENDC
0x07,              //LED1STC
0x08,              //LED1ENDC
0x09,              //LED2LEDSTC
0x0A,              //LED2LEDENDC
0x0B,              //ALED1STC
0x0C,              //ALED1ENDC
0x0D,              //LED2CONVST
0x0E,              //LED2CONVEND
0x0F,              //ALED2CONVST\LED3CONVST
0x10,              //ALED2CONVEND\LED3CONVEND
0x11,              //LED1CONVST
0x12,              //LED1CONVEND
0x13,              //ALED1CONVST
0x14,              //ALED1CONVEND
0x15,              //ADCRSTSTCT0
0x16,              //ADCRSTENDCT0
0x17,              //ADCRSTSTCT1
0x18,              //ADCRSTENDCT1
0x19,              //ADCRSTSTCT2
0x1A,              //ADCRSTENDCT2
0x1B,              //ADCRSTSTCT3
0x1C,              //ADCRSTENDCT3
0x1D,              //PRPCT
0x1E,              //TIMEREN, NUMAV
0x20,              //ENSEPGAIN, TIA_CF_SEP, TIAGAIN_SEP
0x21,              //PROG_TG_EN, TIA_CF, TIA_GAIN
0x22,              //ILED3,ILED2, ILED1
0x23,              //DYNAMIC1, ILED_2X, DYNAMIC2, OSC_ENABLE, DYNAMIC3, DYNAMIC4, PDNRX, PDNAFE              
0x29,              //ENABLE_CLKOUT, CLKDIV_CLKOUT
0x2A,              //LED2VAL
0x2B,              //ALED2VAL\LED3VAL
0x2C,              //LED1VAL
0x2D,              //ALED1VAL
0x2E,              //LED2-ALED2VAL
0x2F,              //LED1-ALED1VAL
0x31,              //PD_DISCONNECT, ENABLE_INPUT_SHORT, CLKDIV_EXTMODE
0x32,              //PDNCYCLESTC
0x33,              //PDNCYCLEENDC
0x34,              //PROG_TG_STC
0x35,              //PROG_TG_ENDC
0x36,              //LED3LEDSTC
0x37,              //LED3LEDENDC
0x39,              //CLKDIV_PRF
0x3A,              //POL_OFFDAC_LED2, I_OFFDAC_LED2, POL_OFFDAC_AMB1, I_OFFDAC_AMB1, POL_OFFDAC_LED1, I_OFFDAC_LED1, POL_OFFDAC_AMB2/POL_OFFDAC_LED3, I_OFFDAC_AMB2/I_OFFDAC_LED3
0x3D,              //DEC_EN, DEC_FACTOR
0x3F,              //AVG_LED2-ALED2VAL
0X40};             //AVG_LED1-ALED1VAL
bool WriteableRegister[] = {
true,             //SW_RESET, TM_COUNT_RST, REG_READ
true,             //LED2STC
true,             //LED2ENDC
true,             //LED1LEDSTC
true,             //LED1LEDENDC
true,             //ALED2STC/LED3STC
true,             //ALED2ENDC/LED3ENDC
true,             //LED1STC
true,             //LED1ENDC
true,             //LED2LEDSTC
true,             //LED2LEDENDC
true,             //ALED1STC
true,             //ALED1ENDC
true,             //LED2CONVST
true,             //LED2CONVEND
true,             //ALED2CONVST\LED3CONVST
true,             //ALED2CONVEND\LED3CONVEND
true,             //LED1CONVST
true,             //LED1CONVEND
true,             //ALED1CONVST
true,             //ALED1CONVEND
true,             //ADCRSTSTCT0
true,             //ADCRSTENDCT0
true,             //ADCRSTSTCT1
true,             //ADCRSTENDCT1
true,             //ADCRSTSTCT2
true,             //ADCRSTENDCT2
true,             //ADCRSTSTCT3
true,             //ADCRSTENDCT3
true,             //PRPCT
true,             //TIMEREN, NUMAV
true,             //ENSEPGAIN, TIA_CF_SEP, TIAGAIN_SEP
true,             //PROG_TG_EN, TIA_CF, TIA_GAIN
true,             //ILED3,ILED2, ILED1
true,             //DYNAMIC1, ILED_2X, DYNAMIC2, OSC_ENABLE, DYNAMIC3, DYNAMIC4, PDNRX, PDNAFE
true,             //ENABLE_CLKOUT, CLKDIV_CLKOUT
false,  //read only LED2VAL
false,  //read only ALED2VAL\LED3VAL
false,  //read only LED1VAL
false,  //read only ALED1VAL
false,  //read only LED2-ALED2VAL
false,  //read only LED1-ALED1VAL
true,             //PD_DISCONNECT, ENABLE_INPUT_SHORT, CLKDIV_EXTMODE
true,             //PDNCYCLESTC
true,             //PDNCYCLEENDC
true,             //PROG_TG_STC
true,             //PROG_TG_ENDC
true,             //LED3LEDSTC
true,             //LED3LEDENDC
true,             //CLKDIV_PRF
true,             //POL_OFFDAC_LED2, I_OFFDAC_LED2, POL_OFFDAC_AMB1, I_OFFDAC_AMB1, POL_OFFDAC_LED1, I_OFFDAC_LED1, POL_OFFDAC_AMB2/POL_OFFDAC_LED3, I_OFFDAC_AMB2/I_OFFDAC_LED3
true,             //DEC_EN, DEC_FACTOR
false,   //read only AVG_LED2-ALED2VAL
false};  //read only AVG_LED1-ALED1VAL

