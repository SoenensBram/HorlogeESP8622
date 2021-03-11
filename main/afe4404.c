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

#include "afe4404.h"
 
static const char *TAG2 = "AFE4404"; 


/**
 * @brief I2C ESP Master initilization
 */
static esp_err_t I2cMasterInit(){
    i2c_config_t configI2c;
    configI2c.mode = I2C_MODE_MASTER;
    configI2c.sda_io_num = I2cMasterSdaIo;
    configI2c.scl_io_num = I2cMasterSclIo;
    //ESP_LOGI(TAG2, "Pins Assined!");
    configI2c.sda_pullup_en = I2cPullup;
    configI2c.scl_pullup_en = I2cPullup;
    //ESP_LOGI(TAG2, "Pullup Set!");
    configI2c.clk_stretch_tick = 300; // 300 ticks, Clock stretch is about 210us
    //ESP_LOGI(TAG2, "I2C clock set!");
    i2c_driver_install(I2cMasterNum, I2C_MODE_MASTER);
    //ESP_LOGI(TAG2, "Driver installed!");
    ESP_ERROR_CHECK(i2c_param_config(I2cMasterNum, &configI2c));
    //ESP_LOGI(TAG2, "I2C Paramaters initialized");
    //ESP_ERROR_CHECK(i2c_driver_install(I2cMasterNum, configI2c.mode));
    //ESP_LOGI(TAG2, "Done I2C Init!");
    return ESP_OK;
}

/**
 * @brief code for writing data to AFE4404
 *
 * 1. send data
 * ___________________________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write reg_address + ack | write data_len byte + ack  | stop |
 * --------|---------------------------|-------------------------|----------------------------|------|
 *
 * @param I2cNum            I2C port number
 * @param RegisterAddress   slave reg address
 * @param Data              data to send
 * @param DataLength        data length
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
 */
static esp_err_t I2cMasterAfe4404Write(uint8_t RegisterAddress, uint8_t *Data, size_t DataLength){
    int returnValue;
    i2c_cmd_handle_t commandI2c = i2c_cmd_link_create();
    i2c_master_start(commandI2c);
    i2c_master_write_byte(commandI2c, Afe4404Address| I2C_MASTER_WRITE, AckCheckEn);
    i2c_master_write_byte(commandI2c, RegisterAddress, AckCheckEn);
    i2c_master_write(commandI2c, Data, DataLength, AckCheckEn);
    i2c_master_stop(commandI2c);
    returnValue = i2c_master_cmd_begin(I2cMasterNum, commandI2c, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(commandI2c);
    //ESP_LOGI(TAG2, "Done I2C Write!");
    return returnValue;
}

/**
 * @brief code for reading data from AFE4404
 *
 * 1. send reg address
 * ______________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write reg_address + ack | stop |
 * --------|---------------------------|-------------------------|------|
 *
 * 2. read data
 * ___________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | read data_len byte + ack(last nack)  | stop |
 * --------|---------------------------|--------------------------------------|------|
 *
 * @param I2cNum            I2C port number
 * @param RegisterAddress   slave reg address
 * @param Data              data to send
 * @param DataLength        data length
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
 */
static esp_err_t I2cMasterAfe4404Read(uint8_t RegisterAddress, uint8_t *Data, size_t DataLength){
    int returnValue;
    //ESP_LOGI(TAG2, "I2C Read!");
    i2c_cmd_handle_t commandI2c = i2c_cmd_link_create();
    i2c_master_start(commandI2c);
    i2c_master_write_byte(commandI2c, Afe4404Address | I2C_MASTER_WRITE, AckCheckEn);
    i2c_master_write_byte(commandI2c, RegisterAddress, AckCheckEn);
    i2c_master_stop(commandI2c);
    returnValue = i2c_master_cmd_begin(I2cMasterNum, commandI2c, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(commandI2c);
    if(returnValue != ESP_OK) return returnValue;
    commandI2c = i2c_cmd_link_create();
    i2c_master_start(commandI2c);
    i2c_master_write_byte(commandI2c, Afe4404Address | I2C_MASTER_READ, AckCheckEn);
    i2c_master_read(commandI2c, Data, DataLength, LastNackVal);
    i2c_master_stop(commandI2c);
    returnValue = i2c_master_cmd_begin(I2cMasterNum, commandI2c, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(commandI2c);
    return returnValue;
}

/**
 * @brief code for initializing AFE4404 Register
 *
 * 1. send data
 * ___________________________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write reg_address + ack | write data_len byte + ack  | stop |
 * --------|---------------------------|-------------------------|----------------------------|------|
 *
 * @param I2cNum            I2C port number
 * 
 * @return
 *     - ESP_OK Success
 */
static esp_err_t I2cMasterAfe4404InitializeRegister(){
    uint32_t data1 = 0;
    uint8_t j = 0;
   //  //basic_settings ======================================================
    uint8_t configData[3];
    uint8_t *point;
    
    configData[0]=(uint8_t)(8 >>16);
    configData[1]=(uint8_t)(((8 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((8 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x00, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x00,0/*,data2/200,data3/200,data4/200,data5/200*/);

    uint32_t reg_val =0;
    reg_val |= (0 << 20);       //  0: Transmitter not pwrd dwn |  1: pwrd dwn in dynamic pwrd-dwn mode 
    reg_val |= (1 << 17);       //  0: LED crrnt range 0-50 |   1: range 0-100
    reg_val |= (1 << 14);       //  0: ADC not pwrd dwn |  1: ADC pwrd dwn in dynamic pwrd-dwn mode
    reg_val |= (1 << 9);        //  0: External Clock (clk pin acts as input in this mode) | 1: Internal Clock (uses 4Mhz internal osc)
    reg_val |= (1 << 4);        //  0: TIA not pwrd dwn |  1: TIA pwrd dwn in dynamic pwrd-dwn mode
    reg_val |= (0 << 3);        //  0: Rest of ADC is not pwrd dwn |  1: Is pwrd dwn in dynamic pwrd-dwn mode
    reg_val |= (0 << 1);        //  0: Normal Mode |  1: RX portion of AFE pwrd dwn  
    reg_val |= (0 << 0);        //  0: Normal Mode |  1: Entire AFE is pwrd dwn   
    configData[0]=(uint8_t)(16912 >>16);
    configData[1]=(uint8_t)(((16912 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((16912 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x23, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x23,16912/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_led2_start_end ======================================================
    configData[0]=(uint8_t)(0 >>16);
    configData[1]=(uint8_t)(((0 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((0 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x09, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x09,0/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(399 >>16);
    configData[1]=(uint8_t)(((399 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((399 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x0A, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x0A,399/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_led2sample_start_end ======================================================
    configData[0]=(uint8_t)(80 >>16);
    configData[1]=(uint8_t)(((80 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((80 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x01, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x01,80/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(399 >>16);
    configData[1]=(uint8_t)(((399 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((399 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x02, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x02,399/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_adc_reset0_start_end ======================================================
    configData[0]=(uint8_t)(401 >>16);
    configData[1]=(uint8_t)(((401 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((401 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x15, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x15,401/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(407 >>16);
    configData[1]=(uint8_t)(((407 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((407 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x16, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x16,407/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_led2_converter_start_end ======================================================
    configData[0]=(uint8_t)(408 >>16);
    configData[1]=(uint8_t)(((408 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((408 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x0D, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x0D,408/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(1467 >>16);
    configData[1]=(uint8_t)(((1467 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((1467 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x0E, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x0E,1467/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_led3_start_stop ======================================================
    configData[0]=(uint8_t)(400 >>16);
    configData[1]=(uint8_t)(((400 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((400 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x36, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x36,400/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(799 >>16);
    configData[1]=(uint8_t)(((799 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((799 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x37, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x37,799/*,data2/200,data3/200,data4/200,data5/200*/);

     //set_led3_sample_start_end ======================================================
    configData[0]=(uint8_t)(480 >>16);
    configData[1]=(uint8_t)(((480 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((480 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x05, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x05,480/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(799 >>16);
    configData[1]=(uint8_t)(((799 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((799 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x06, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x06,799/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_adc_reset1_start_end ======================================================
    configData[0]=(uint8_t)(1469 >>16);
    configData[1]=(uint8_t)(((1469 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((1469 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x17, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x17,1469/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(1475 >>16);
    configData[1]=(uint8_t)(((1475 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((1475 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x18, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x18,1475/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_led3_convert_start_end ======================================================
    configData[0]=(uint8_t)(1476 >>16);
    configData[1]=(uint8_t)(((1476 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((1476 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x0F, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x0F,1476/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(2535 >>16);
    configData[1]=(uint8_t)(((2535 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((2535 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x10, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x10,2535/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_led1_start_end ======================================================
    configData[0]=(uint8_t)(800 >>16);
    configData[1]=(uint8_t)(((800 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((800 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x03, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x03,800/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(1199 >>16);
    configData[1]=(uint8_t)(((1199 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((1199 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x04, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x04,1199/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_led1_sample_start_end ======================================================
    configData[0]=(uint8_t)(880 >>16);
    configData[1]=(uint8_t)(((880 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((880 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x07, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x07,880/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(1199 >>16);
    configData[1]=(uint8_t)(((1199 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((1199 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x08, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x08,1199/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_adc_reset2_start_end ======================================================
    configData[0]=(uint8_t)(2537 >>16);
    configData[1]=(uint8_t)(((2537 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((2537 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x19, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x19,2537/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(2543 >>16);
    configData[1]=(uint8_t)(((2543 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((2543 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x1A, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x1A,2543/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_led1_convert_start_end ======================================================
    configData[0]=(uint8_t)(2544 >>16);
    configData[1]=(uint8_t)(((2544 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((2544 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x11, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x11,2544/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(3603 >>16);
    configData[1]=(uint8_t)(((3603 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((3603 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x12, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x12,3603/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_amb1_sample_start_end ======================================================
    configData[0]=(uint8_t)(1279 >>16);
    configData[1]=(uint8_t)(((1279 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((1279 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x0B, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x0B,1279/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(1598 >>16);
    configData[1]=(uint8_t)(((1598 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((1598 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x0C, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x0C,1598/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_adc_reset3_start_end ======================================================
    configData[0]=(uint8_t)(3605 >>16);
    configData[1]=(uint8_t)(((3605 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((3605 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x1B, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x1B,3605/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(3611 >>16);
    configData[1]=(uint8_t)(((3611 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((3611 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x1C, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x1C,3611/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_amb1_convert_start_end ======================================================
    configData[0]=(uint8_t)(3612 >>16);
    configData[1]=(uint8_t)(((3612 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((3612 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x13, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x13,3612/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(4671 >>16);
    configData[1]=(uint8_t)(((4671 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((4671 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x14, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x14,4671/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_pdn_cycle_start_end ======================================================
    configData[0]=(uint8_t)(5471 >>16);
    configData[1]=(uint8_t)(((5471 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((5471 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x32, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x32,5471/*,data2/200,data3/200,data4/200,data5/200*/);

    configData[0]=(uint8_t)(39199 >>16);
    configData[1]=(uint8_t)(((39199 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((39199 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x33, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x33,39199/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_prpct_count ======================================================
    configData[0]=(uint8_t)(39999 >>16);
    configData[1]=(uint8_t)(((39999 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((39999 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x1D, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x1D,39999/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_timer_and_average_num ======================================================
    uint32_t temp = 0;
    temp |= (1 << 8);  // Timer Enable bit -> to use internal timing engine to do sync for sampling, data conv etc.
    temp |= 3;
    configData[0]=(uint8_t)(259 >>16);
    configData[1]=(uint8_t)(((259 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((259 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x1E, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value:%u", 0x1E,259/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_seperate_tia_gain ======================================================
    temp = 0;
    temp |= (0 << 15);   //  Separate TIA gains if this bit = 1; else single gain if = 0;
    temp |= (0 << 3);  //  Control of C2 settings (3 bits -> 0-7)
    temp |= (4 << 0);
    configData[0]=(uint8_t)(32772 >>16);
    configData[1]=(uint8_t)(((32772 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((32772 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x20, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value:%u", 0x20,temp/*,data2/200,data3/200,data4/200,data5/200*/);

   //set_tia_gain ======================================================
    temp = 0;
    temp |= (0 << 8);          //  making 1 will replace ADC_RDY output with signal from timing engine.                        //  controlled by PROG_TG_STC and PROG_TG_ENDC regs.
    temp |= (0 << 3);       //  Control of C1 settings (3 bits -> 0-7)
    temp |= (3 << 0); 
    configData[0]=(uint8_t)(temp >>16);
    configData[1]=(uint8_t)(((temp & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((temp & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x21, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value:%u", 0x21,temp/*,data2/200,data3/200,data4/200,data5/200*/);

   //set_led_currents ======================================================
    temp = 0;
    temp |= (15 << 0);     // LED 1 addrss space -> 0-5 bits
    temp |= (3 << 6);     // LED 2 addrss space -> 6-11 bits
    temp |= (3 << 12);
    configData[0]=(uint8_t)(12495 >>16);
    configData[1]=(uint8_t)(((12495 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((12495 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x22, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value:%u", 0x22,12495/*,data2/200,data3/200,data4/200,data5/200*/);

   //set_led_currents ======================================================
    temp = 0;
    temp |= (0 << 9);   //  Enable clock output if enable = 1
    temp |= (2 << 1);  
    configData[0]=(uint8_t)(4 >>16);
    configData[1]=(uint8_t)(((4 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((4 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x29, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value:%u", 0x29,temp/*,data2/200,data3/200,data4/200,data5/200*/);

    //set_prpct_count ======================================================
    configData[0]=(uint8_t)(1 >>16);
    configData[1]=(uint8_t)(((1 & 0x00FFFF) >>8));
    configData[2]=(uint8_t)(((1 & 0x0000FF)));
    ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x39, &configData, 3));
    ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x39,1/*,data2/200,data3/200,data4/200,data5/200*/);

    // //PD_DISCONNECT, ENABLE_INPUT_SHORT, CLKDIV_EXTMODE ======================================================
    // configData[0]=(uint8_t)(32 >>16);
    // configData[1]=(uint8_t)(((32 & 0x00FFFF) >>8));
    // configData[2]=(uint8_t)(((32 & 0x0000FF)));
    // ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x31, &configData, 3));
    // ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x31,32/*,data2/200,data3/200,data4/200,data5/200*/);

    // //PROG_TG_STC ======================================================
    // configData[0]=(uint8_t)(0 >>16);
    // configData[1]=(uint8_t)(((0 & 0x00FFFF) >>8));
    // configData[2]=(uint8_t)(((0 & 0x0000FF)));
    // ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x34, &configData, 3));
    // ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x34,0/*,data2/200,data3/200,data4/200,data5/200*/);

    // //PROG_TG_ENDC ======================================================
    // configData[0]=(uint8_t)(0 >>16);
    // configData[1]=(uint8_t)(((0 & 0x00FFFF) >>8));
    // configData[2]=(uint8_t)(((0 & 0x0000FF)));
    // ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x35, &configData, 3));
    // ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x35,0/*,data2/200,data3/200,data4/200,data5/200*/);

    // //POL_OFFDAC_LED2, I_OFFDAC_LED2, POL_OFFDAC_AMB1, I_OFFDAC_AMB1, POL_OFFDAC_LED1, I_OFFDAC_LED1, POL_OFFDAC_AMB2/POL_OFFDAC_LED3, I_OFFDAC_AMB2/I_OFFDAC_LED3 ======================================================
    // configData[0]=(uint8_t)(1048575 >>16);
    // configData[1]=(uint8_t)(((1048575 & 0x00FFFF) >>8));
    // configData[2]=(uint8_t)(((1048575 & 0x0000FF)));
    // ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x3A, &configData, 3));
    // ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x3A,1048575/*,data2/200,data3/200,data4/200,data5/200*/);

    // //DEC_EN, DEC_FACTOR ======================================================
    // configData[0]=(uint8_t)(46 >>16);
    // configData[1]=(uint8_t)(((46 & 0x00FFFF) >>8));
    // configData[2]=(uint8_t)(((46 & 0x0000FF)));
    // ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x39, &configData, 3));
    // ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x39,46/*,data2/200,data3/200,data4/200,data5/200*/);

















    // ESP_ERROR_CHECK(I2cMasterAfe4404Read(Address[i], &data1, 3));
    // ESP_LOGI(TAG2, "Read $%d;%x", data1,Address[i]/*,data2/200,data3/200,data4/200,data5/200*/);









    // configData[0]=(uint8_t)(1 >>16);
    // configData[1]=(uint8_t)(((1 & 0x00FFFF) >>8));
    // configData[2]=(uint8_t)(((1 & 0x0000FF)));
    // ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x00, &configData, 3));
    // ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x00,1/*,data2/200,data3/200,data4/200,data5/200*/);



    // for(unsigned int i = 0; i < RegisterEnteriesAfe4404; i++){
    //     if(WriteableRegister[i]){
    //         // ESP_ERROR_CHECK(I2cMasterAfe4404Write(Address[i], &Value[i], 3));
    //         // ESP_LOGI(TAG2, "Write Addres: %x Value: %d", Value[i],Address[i]/*,data2/200,data3/200,data4/200,data5/200*/);
    //         //data1 = 0;
    //         //ESP_ERROR_CHECK(I2cMasterAfe4404Read(Address[i], &data1, 3));
    //         ESP_LOGI(TAG2, "Address: %x Read Value: %d ", Address[i],data1/*,data2/200,data3/200,data4/200,data5/200*/);
    //     }else{
    //         GetAddress[j] = Address[i];
    //         j ++;
    //     }
    // }
    // ESP_LOGI(TAG2, "Done I2C InitRegister!");

    // configData[0]=(uint8_t)(0 >>16);
    // configData[1]=(uint8_t)(((0 & 0x00FFFF) >>8));
    // configData[2]=(uint8_t)(((0 & 0x0000FF)));
    // ESP_ERROR_CHECK(I2cMasterAfe4404Write(0x00, &configData, 3));
    // ESP_LOGI(TAG2, "Write Addres: %x Value: %d", 0x00,0/*,data2/200,data3/200,data4/200,data5/200*/);

    return ESP_OK;
}

//Routine that gets exicuted when the device interupt is called
static void InterruptRoutine(void* arg){
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    DataReady = true;
    DataReadyCount ++;
}


/**
 * @brief code for getting and calculating data from AFE4404 Debugging only
 */
static void EspSpo2Data(){;
    uint8_t data1[3];
    uint8_t length = 3;


    // for(unsigned int i = 0; i < RegisterEnteriesAfe4404; i++){
    //     if(!WriteableRegister[i]){
    //         I2cMasterAfe4404Read(Address[i], &data1, length);
    //         ESP_LOGI(TAG2, "sensor_data %d: %d",Address[i] , data1);
    //     }
    // }
    I2cMasterAfe4404Read(0x2A, &data1, length);
    uint32_t retVal;
    retVal = data1[0];
    retVal = (retVal << 8) | data1[1];
    retVal = (retVal << 8) | data1[2];
    
      if (retVal & 0x00200000)  // check if the ADC value is positive or negative
      {
        retVal &= 0x003FFFFF;   // convert it to a 22 bit value
        return (retVal^0xFFC00000);
      }
  


    ESP_LOGI(TAG2, "$%d;", retVal/*,data2/200,data3/200,data4/200,data5/200*/);
    


    //ESP_LOGI(TAG2, "sensor_data %d: %d",Address[36] , data1);
    //I2cMasterAfe4404Read(Address[37], &data1, length);
    //ESP_LOGI(TAG2, "sensor_data %d: %d",Address[37] , data1);
    //I2cMasterAfe4404Read(Address[38], &data1, length);
    //ESP_LOGI(TAG2, "sensor_data %d: %d",Address[38] , data1);
    //I2cMasterAfe4404Read(Address[39], &data1, length);
    //ESP_LOGI(TAG2, "sensor_data %d: %d",Address[39] , data1);
    DataReady = false;
    //ESP_LOGI(TAG2, "Done read SPO2!");
}

/**
 * @brief code for initializing AFE4404 Pins
 * @return
 *     - ESP_OK Success
 */
static esp_err_t MasterAfe4404InitializePorts(){
    ESP_ERROR_CHECK(gpio_set_direction(TxSupplyEnable,GPIO_MODE_DEF_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(RxSupplyEnable,GPIO_MODE_DEF_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(PowerEnable,GPIO_MODE_DEF_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(ResetAfe,GPIO_MODE_DEF_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(TxSupplyEnable,0));
    ESP_ERROR_CHECK(gpio_set_level(RxSupplyEnable,0));
    ESP_ERROR_CHECK(gpio_set_level(PowerEnable,0));
    ESP_ERROR_CHECK(gpio_set_level(ResetAfe,UINT32_MAX));
    //ESP_LOGI(TAG2, "Done I2C Ports Init!");
    return ESP_OK;
}

//initialize the dataReady interupt, this only needs to be called once on boot
static esp_err_t InitInteruptPortDataReady(){
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    ESP_ERROR_CHECK(gpio_set_intr_type(DataReadyInterupt,GPIO_INTR_POSEDGE));
    ESP_ERROR_CHECK(gpio_set_direction(DataReadyInterupt,GPIO_MODE_DEF_INPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(DataReadyInterupt,GPIO_FLOATING));
    //ESP_LOGI(TAG2, "Interupts configured");
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    //ESP_LOGI(TAG2, "Interupt service installed");
    ESP_ERROR_CHECK(gpio_isr_handler_add(DataReadyInterupt, InterruptRoutine, (void *) DataReadyInterupt));
    return ESP_OK;
}

/**
 * @brief code for initializing AFE4404 Pins
 * @return
 *     - ESP_OK Success
 */
static esp_err_t Afe4404InitializePowerUp(){
    ets_delay_us(100000);
    ESP_ERROR_CHECK(gpio_set_level(PowerEnable,0));
    ets_delay_us(100000);
    ESP_ERROR_CHECK(gpio_set_level(RxSupplyEnable,UINT32_MAX));
    ets_delay_us(10000);
    ESP_ERROR_CHECK(gpio_set_level(TxSupplyEnable,UINT32_MAX));
    ets_delay_us(20000);
    ESP_ERROR_CHECK(gpio_set_level(ResetAfe,0));
    ets_delay_us(35000);
    ESP_ERROR_CHECK(gpio_set_level(ResetAfe,UINT32_MAX));
    //ESP_LOGI(TAG2, "Done I2C Hard Slave Powerup!");
    vTaskDelay(100 / portTICK_RATE_MS);
    I2cMasterAfe4404Write(Address[34], &Value[34]+1 ,3);
    return ESP_OK;
}

/**
 * @brief code for Startupsequense AFE4404
 * @return
 *     - ESP_OK Success
 */
static esp_err_t Afe4404PowerUp(){
    Afe4404InitializePowerUp();
    I2cMasterAfe4404InitializeRegister();
    DataReadyCount = 0;
    ESP_LOGI(TAG2, "AFE Powerup!");
    return ESP_OK;
}

//Power AFe down
static esp_err_t Afe4404PowerDown(){
    ESP_ERROR_CHECK(gpio_set_level(PowerEnable,UINT32_MAX));
    ESP_ERROR_CHECK(gpio_set_level(RxSupplyEnable,0));
    ESP_ERROR_CHECK(gpio_set_level(TxSupplyEnable,0));
    ESP_ERROR_CHECK(gpio_set_level(ResetAfe,0));
    ESP_LOGI(TAG2, "AFE Powerdown");
    return ESP_OK;
}

//initalizing data and setting u pins for ESP
static esp_err_t Afe4404Init(){
    MasterAfe4404InitializePorts();
    InitInteruptPortDataReady();
    I2cMasterInit();
    ESP_LOGI(TAG2, "esp init for AFE");
    return ESP_OK;
}

//getting data from a single sensor
static int32_t AfeGetData(enum Sensor readout){
    int32_t data1;
    uint8_t length = 24;
    I2cMasterAfe4404Read(GetAddress[readout], &data1, length);
    return data1;
}

//Only methode that needs to be called to read an array of results
static void AfeGetDataArray(uint16_t size, int32_t *Data, enum Sensor readout){
    Afe4404PowerUp();
    uint i = 0;
    while (i<size){
        if(DataReadyCount>10 && DataReady == true){
            Data[i] = AfeGetData(readout);
            i++;
            DataReady = false;
        }
    }
    Afe4404PowerDown();
    //ESP_LOGI(TAG2, "AFE array written");
}

