#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_task_wdt.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"

#include "driver/i2c.h"
#include "driver/gpio.h"

#include "afe4404.h"
 
static const char *TAG2 = "AFE4404"; 


/**
 * @brief I2C ESP Master initializion
 * 
 * Setting the pins and installing the master i2c.
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
 */
static esp_err_t I2cMasterInit(){
    i2c_config_t configI2c;
    configI2c.mode = I2C_MODE_MASTER;
    configI2c.sda_io_num = I2cMasterSdaIo;
    configI2c.scl_io_num = I2cMasterSclIo;
    configI2c.sda_pullup_en = I2cPullup;
    configI2c.scl_pullup_en = I2cPullup;
    configI2c.clk_stretch_tick = 300; // 300 ticks, Clock stretch is about 210us
    i2c_driver_install(I2cMasterNum, I2C_MODE_MASTER);
    ESP_ERROR_CHECK(i2c_param_config(I2cMasterNum, &configI2c));
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
 * @param DataLength        data length in bytes
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
 */
static esp_err_t I2cMasterAfe4404Write(uint8_t RegisterAddress, uint32_t *Data, size_t DataLength){
    int returnValue;
    i2c_cmd_handle_t commandI2c = i2c_cmd_link_create();
    i2c_master_start(commandI2c);
    i2c_master_write_byte(commandI2c, Afe4404Address| I2C_MASTER_WRITE, AckCheckEn);
    i2c_master_write_byte(commandI2c, RegisterAddress, AckCheckEn);
    i2c_master_write(commandI2c, Data, DataLength, AckCheckEn);
    i2c_master_stop(commandI2c);
    returnValue = i2c_master_cmd_begin(I2cMasterNum, commandI2c, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(commandI2c);
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
 * @param RegisterAddress   slave reg addres
 * @param Data              data to send
 * @param DataLength        data length in bytes
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
 */
static esp_err_t I2cMasterAfe4404Read(uint8_t RegisterAddress, uint32_t *Data, size_t DataLength){
    int returnValue;
    uint8_t readData[DataLength];
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
    i2c_master_read(commandI2c, readData, DataLength, LastNackVal);
    i2c_master_stop(commandI2c);
    returnValue = i2c_master_cmd_begin(I2cMasterNum, commandI2c, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(commandI2c);
    for(uint i = 0; i<DataLength; i++){
        *Data = (*Data << 8) | readData[i];
    }
    return returnValue;
}

/**
 * @brief code for initializing AFE4404 Register
 * 
 * They are defined in the H-file in the 3 arrays: 
 *  - Value:                For Registry Values
 *  - Address:              For Registry addresses
 *  - WriteableRegister:    Knowing if the register is writable
 * 
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
 */
static esp_err_t I2cMasterAfe4404InitializeRegister(){
    uint8_t j = 0;
    for(unsigned int i = 0; i < RegisterEnteriesAfe4404; i++){
        if(WriteableRegister[i]){
             uint8_t configData[3];
            configData[0]=(uint8_t)(Value[i] >>16);
            configData[1]=(uint8_t)(((Value[i] & 0x00FFFF) >>8));
            configData[2]=(uint8_t)(((Value[i] & 0x0000FF)));
            ESP_ERROR_CHECK(I2cMasterAfe4404Write(Address[i], configData, 3));
            }else{
            GetAddress[j] = Address[i];
            j ++;
        }
    }
    return ESP_OK;
}

/**
 * @brief code for initializing AFE4404 Interuptcall
 */
static void InterruptRoutine(void* arg){
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    DataReady = true;
    DataReadyCount ++;
}

/**
 * @brief code for initializing AFE4404 Pins for power control 
 * 
 * This code is for setting up the power control of the AFE4404.
 *
 *  @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
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
    return ESP_OK;
}

/**
 * @brief code for initializing AFE4404 Interups
 * 
 * This code is needed for initalizing the interups and needs to be called once at the start of the code.
 *
 *  @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
 */
static esp_err_t InitInteruptPortDataReady(){
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    ESP_ERROR_CHECK(gpio_set_intr_type(DataReadyInterupt,GPIO_INTR_POSEDGE));
    ESP_ERROR_CHECK(gpio_set_direction(DataReadyInterupt,GPIO_MODE_DEF_INPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(DataReadyInterupt,GPIO_FLOATING));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(DataReadyInterupt, InterruptRoutine, (void *) DataReadyInterupt));
    return ESP_OK;
}

/**
 * @brief code for AFE4404 Powerup Sequence
 * 
 * This code is needed for a correct startup of the AFE4404.
 *
 *  @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
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
    vTaskDelay(100 / portTICK_RATE_MS);
    //I2cMasterAfe4404Write(Address[34], &Value[34]+1 ,3);
    return ESP_OK;
}

/**
 * @brief AFE4404 powerup code
 * 
 * This code is needed for powering up the AFE4404 chip.
 *
 *  @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
 */
static esp_err_t Afe4404PowerUp(){
    Afe4404InitializePowerUp();
    I2cMasterAfe4404InitializeRegister();
    DataReadyCount = 0;
    ESP_LOGI(TAG2, "AFE Powerup!");
    return ESP_OK;
}

/**
 * @brief AFE4404 Powerdown code
 * 
 * This code is needed for powering down the AFE4404 chip.
 *
 *  @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
 */
static esp_err_t Afe4404PowerDown(){
    ESP_ERROR_CHECK(gpio_set_level(PowerEnable,UINT32_MAX));
    ESP_ERROR_CHECK(gpio_set_level(RxSupplyEnable,0));
    ESP_ERROR_CHECK(gpio_set_level(TxSupplyEnable,0));
    ESP_ERROR_CHECK(gpio_set_level(ResetAfe,0));
    ESP_LOGI(TAG2, "AFE Powerdown");
    return ESP_OK;
}

/**
 * @brief AFE4404 initialization
 * 
 * This code is needed for all the initialization of the AFE4404 chip.  
 *
 *  @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
 */
static esp_err_t Afe4404Init(){
    MasterAfe4404InitializePorts();
    InitInteruptPortDataReady();
    I2cMasterInit();
    ESP_LOGI(TAG2, "esp init for AFE");
    return ESP_OK;
}

/**
 * @brief code for getting data from the AFE4404 with the specified sensor
 * 
 * @param readout   This is the sensor that will be readout, these are specified in the h-File under the enum Sensor array
 *
 * @return This is the data that is gotten from the specified data
 */
static uint32_t AfeGetData(enum Sensor readout){
    uint32_t data1=0;
    uint8_t length = 3;
    I2cMasterAfe4404Read(GetAddress[readout], &data1, length);
    return data1;
}

/**
 * @brief reading data Array from AFE4404
 *
 * @param size      Here the size of the array is defined
 * @param Data      This is the Pointer where we will write data to
 * @param readout   This is the sensor that will be readout, these are specified in the h-File under the enum Sensor array
 */
static void AfeGetDataArray(uint16_t size, uint32_t *Data, enum Sensor readout){
    Afe4404PowerUp();
    uint i = 0;
    while (i<size){
        if(DataReadyCount>5 && DataReady == true){
            Data[i] = AfeGetData(readout);
            i++;
            esp_task_wdt_reset();
            DataReady = false;
        }
    }
    Afe4404PowerDown();
}

/**
 * @brief reading data Array from AFE4404
 *
 * @param readout   This is the sensor that will be readout, these are specified in the h-File under the enum Sensor array
 */
static void serialContinuisPrint(enum Sensor readout){
    Afe4404PowerUp();
    uint32_t bob =0;
    while(1){
        if(DataReady == true){
            bob = AfeGetData(readout);
            //ESP_LOGI("bob","%u;",bob);
            printf("$%d;\r\n", bob);
            esp_task_wdt_reset();
            DataReady = false;
        }
    }
}