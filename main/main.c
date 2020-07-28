#include <stdio.h>

#include <esp_log.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ADS1115.h"

#define SDA_IO (21)                      /*!< gpio number for I2C master data  */
#define SCL_IO (22)                      /*!< gpio number for I2C master clock */

#define FREQ_HZ (100000)                 /*!< I2C master clock frequency */
#define TX_BUF_DISABLE (0)               /*!< I2C master doesn't need buffer */
#define RX_BUF_DISABLE (0)               /*!< I2C master doesn't need buffer */

#define I2C_NUM I2C_NUM_0                /*!< I2C number */
#define I2C_MODE I2C_MODE_MASTER         /*!< I2C mode to act as */
#define I2C_RX_BUF_STATE RX_BUF_DISABLE  /*!< I2C set rx buffer status */
#define I2C_TX_BUF_STATE TX_BUF_DISABLE  /*!< I2C set tx buffer status */
#define I2C_INTR_ALOC_FLAG (0)           /*!< I2C set interrupt allocation flag */

static const char *TAG = "Main";

/* i2c setup ----------------------------------------- */
// Config profile for espressif I2C lib
i2c_config_t i2c_cfg = {                     
  .mode = I2C_MODE_MASTER, 
  .sda_io_num = SDA_IO,
  .scl_io_num = SCL_IO,
  .sda_pullup_en = GPIO_PULLUP_DISABLE,
  .scl_pullup_en = GPIO_PULLUP_DISABLE,
  .master.clk_speed = FREQ_HZ,
};

/* ADS1115 setup ------------------------------------- */
// Below uses the default values speficied by the datasheet
ads1115_t ads1115_cfg = {
  .reg_cfg =  ADS1115_CFG_LS_COMP_MODE_TRAD | // Comparator is traditional
              ADS1115_CFG_LS_COMP_LAT_NON |   // Comparator is non-latching
              ADS1115_CFG_LS_COMP_POL_LOW |   // Alert is active low
              ADS1115_CFG_LS_COMP_QUE_DIS |   // Compator is disabled
              ADS1115_CFG_LS_DR_1600SPS |     // No. of samples to take
              ADS1115_CFG_MS_MODE_SS,         // Mode is set to single-shot
  .dev_addr = 0x48,
};

void xADS1115_thred(void *arg); // FreeRTOS task for ADC loop

void app_main(void) 
{
  ESP_LOGD(TAG,"Starting ADS1115 example..");

  // Setup I2C
  i2c_param_config(I2C_NUM, &i2c_cfg);
  i2c_driver_install(I2C_NUM, I2C_MODE, I2C_RX_BUF_STATE, I2C_TX_BUF_STATE, I2C_INTR_ALOC_FLAG);

  // Setup ADS1115
  ADS1115_initiate(&ads1115_cfg);
  
  // Start ADC loop
  xTaskCreate(xADS1115_thred, "xADS1115_thred",2048, NULL, 3, NULL);
}

void xADS1115_thred(void *arg)
{
 // Buffer for result
  uint16_t result = 0;

  for(;;) 
  {
    // Request single ended on pin AIN0  
    ADS1115_request_single_ended_AIN1();      // all functions except for get_conversion_X return 'esp_err_t' for logging

    // Check conversion state - returns true if conversion is complete 
    while(!ADS1115_get_conversion_state()) 
      vTaskDelay(pdMS_TO_TICKS(5));          // wait 5ms before check again
    
    // Return latest conversion value
    result = ADS1115_get_conversion();   
    ESP_LOGD(TAG,"Conversion Value: %d", result);  
  }

  ESP_LOGD(TAG,"Should not reach here!");
  vTaskDelete(NULL);  
}