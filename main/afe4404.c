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
 
static const char *TAG2 = "I2C"; 





/**
 * @brief I2C ESP Master initilization
 */
static esp_err_t I2cMasterInit(){
    i2c_config_t configI2c;
    configI2c.mode = I2C_MODE_MASTER;
    configI2c.sda_io_num = I2cMasterSdaIo;
    configI2c.scl_io_num = I2cMasterSclIo;
    ESP_LOGI(TAG2, "Pins Assined!");
    configI2c.sda_pullup_en = I2cPullup;
    configI2c.scl_pullup_en = I2cPullup;
    ESP_LOGI(TAG2, "Pullup Set!");
    configI2c.clk_stretch_tick = 300; // 300 ticks, Clock stretch is about 210us
    ESP_LOGI(TAG2, "I2C clock set!");
    i2c_driver_install(I2cMasterNum, I2C_MODE_MASTER);
    ESP_LOGI(TAG2, "Driver installed!");
    ESP_ERROR_CHECK(i2c_param_config(I2cMasterNum, &configI2c));
    ESP_LOGI(TAG2, "I2C Paramaters initialized");
    //ESP_ERROR_CHECK(i2c_driver_install(I2cMasterNum, configI2c.mode));
    ESP_LOGI(TAG2, "Done I2C Init!");
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
    ESP_LOGI(TAG2, "Done I2C Write!");
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
    ESP_LOGI(TAG2, "I2C Read!");
    i2c_cmd_handle_t commandI2c = i2c_cmd_link_create();
    i2c_master_start(commandI2c);
    i2c_master_write_byte(commandI2c, Afe4404Address <<1 | I2C_MASTER_WRITE, AckCheckEn);
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
    vTaskDelay(100 / portTICK_RATE_MS);
    I2cMasterInit();
    for(unsigned int i = 0; i < RegisterEnteriesAfe4404; i++){
        ESP_ERROR_CHECK(I2cMasterAfe4404Write(Address[i], &Value[i], 3));
    }
    ESP_LOGI(TAG2, "Done I2C InitRegister!");
    return ESP_OK;
}

/**
 * @brief code for initializing AFE4404 Pins
Pins Used In Pinout
Pin 20 GPIO11   RxSupplyEn
Pin 21 GPIO6    TxsupplyEn
Pin 22 GPIO7    PowerEn
Pin 23 GPIO8    ResetAfe
Pin 24 GPIO5    DataReadyInterupt
 * @return
 *     - ESP_OK Success
 */
static esp_err_t MasterAfe4404InitializePorts(){
    ESP_ERROR_CHECK(gpio_set_direction(DataReadyInterupt,GPIO_MODE_DEF_INPUT));
    //ESP_ERROR_CHECK(gpio_intr_enable(DataReadyInterupt));
    ESP_ERROR_CHECK(gpio_set_direction(TxSupplyEnable,GPIO_MODE_DEF_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(RxSupplyEnable,GPIO_MODE_DEF_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(PowerEnable,GPIO_MODE_DEF_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(ResetAfe,GPIO_MODE_DEF_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(TxSupplyEnable,0));
    ESP_ERROR_CHECK(gpio_set_level(RxSupplyEnable,0));
    ESP_ERROR_CHECK(gpio_set_level(PowerEnable,0));
    ESP_ERROR_CHECK(gpio_set_level(ResetAfe,UINT32_MAX));
    ESP_LOGI(TAG2, "Done I2C Ports Init!");
    return ESP_OK;
}

/**
 * @brief code for initializing AFE4404 Pins
Pins Used In Pinout
Pin 20 GPIO11   RxSupplyEn
Pin 21 GPIO6    TxsupplyEn
Pin 22 GPIO7    PowerEn
Pin 23 GPIO8    ResetAfe
Pin 24 GPIO5    DataReadyInterupt
 * @return
 *     - ESP_OK Success
 */
static esp_err_t Afe4404InitializePowerUp(){
    ets_delay_us(100000);
    ESP_ERROR_CHECK(gpio_set_level(PowerEnable,UINT32_MAX));
    ets_delay_us(100000);
    ESP_ERROR_CHECK(gpio_set_level(RxSupplyEnable,UINT32_MAX));
    ets_delay_us(10000);
    ESP_ERROR_CHECK(gpio_set_level(TxSupplyEnable,UINT32_MAX));
    ets_delay_us(20000);
    ESP_ERROR_CHECK(gpio_set_level(ResetAfe,0));
    ets_delay_us(35000);
    ESP_ERROR_CHECK(gpio_set_level(ResetAfe,UINT32_MAX));
    ESP_LOGI(TAG2, "Done I2C Hard Slave Powerup!");
    return ESP_OK;
}

/**
 * @brief code for Startupsequense AFE4404
 * @return
 *     - ESP_OK Success
 */
static esp_err_t Afe4404PowerUp(){
    MasterAfe4404InitializePorts();
    Afe4404InitializePowerUp();
    I2cMasterAfe4404InitializeRegister();
    ESP_LOGI(TAG2, "Done I2C Soft Slave Powerup!");
    return ESP_OK;
}

/**
 * @brief code for getting and calculating data from AFE4404
 */
static esp_err_t EspSpo2Data(){
    //I2cMasterAfe4404Write(Address[0], 0xD00000, 3);
    uint8_t data1 = 0;
    uint8_t length = 24;
    I2cMasterAfe4404Read(42, data1, length);
    ESP_LOGI(TAG2, "sensor_data%d: %d",42 , (uint32_t)data1);
    I2cMasterAfe4404Read(43, data1, length);
    ESP_LOGI(TAG2, "sensor_data%d: %d",43 , (uint32_t)data1);
    I2cMasterAfe4404Read(44, data1, length);
    ESP_LOGI(TAG2, "sensor_data%d: %d",44 , (uint32_t)data1);
    I2cMasterAfe4404Read(45, data1, length);
    ESP_LOGI(TAG2, "sensor_data%d: %d",45 , (uint32_t)data1);
    I2cMasterAfe4404Read(46, data1, length);
    ESP_LOGI(TAG2, "sensor_data%d: %d",46 , (uint32_t)data1);
    I2cMasterAfe4404Read(47, data1, length);
    ESP_LOGI(TAG2, "sensor_data%d: %d",47 , (uint32_t)data1);
    I2cMasterAfe4404Read(63, data1, length);
    ESP_LOGI(TAG2, "sensor_data%d: %d",63 , (uint32_t)data1);
    I2cMasterAfe4404Read(64, data1, length);
    ESP_LOGI(TAG2, "sensor_data%d: %d",64 , (uint32_t)data1);
    ESP_LOGI(TAG2, "Done read SPO2!");
    //code goes here
    return ESP_OK;
}