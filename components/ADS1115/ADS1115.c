/* Apache License 2.0  | Author Contact: jdwifwaf@gmail.com */
#include <stdio.h>
#include <driver/i2c.h>
#include <esp_log.h>
#include <string.h>

#include "ADS1115.h"

static const char *ADS_TAG = "ADS1115";
static ads1115_t *ads_cfg;

static esp_err_t ADS1115_read_to_rwbuff(uint8_t reg_adr);          // Move these to the header file if you need additional r/w capabilities
static esp_err_t ADS1115_write_reg(uint16_t val, uint16_t reg);    // Move these to the header file if you need additional r/w capabilities
static esp_err_t i2c_handle_write(uint8_t dev_adr, uint8_t w_adr, uint8_t w_len, uint8_t *buff) ;
static esp_err_t i2c_handle_read(uint8_t dev_adr, uint8_t r_adr, uint8_t r_len, uint8_t *buff);

//inline implementations
extern inline esp_err_t ADS1115_request_single_ended_AIN0();
extern inline esp_err_t ADS1115_request_single_ended_AIN1();
extern inline esp_err_t ADS1115_request_single_ended_AIN2();
extern inline esp_err_t ADS1115_request_single_ended_AIN3();

extern inline esp_err_t ADS1115_request_diff_AIN0_AIN1();
extern inline esp_err_t ADS1115_request_diff_AIN0_AIN3();
extern inline esp_err_t ADS1115_request_diff_AIN1_AIN3();
extern inline esp_err_t ADS1115_request_diff_AIN2_AIN3();

static inline esp_err_t ADS1115_set_lo_thresh(uint16_t value);
static inline esp_err_t ADS1115_set_hi_thresh(uint16_t value);

esp_err_t ADS1115_initiate(ads1115_t *cfg)
{
    esp_log_level_set(ADS_TAG, ADS1115_DEBUG_LEVEL);   

    return ADS1115_set_config(cfg);
} 

esp_err_t ADS1115_set_config(ads1115_t *cfg)
{
    if(!cfg) 
        return ESP_ERR_INVALID_ARG;
        
    ads_cfg = cfg;
    return ESP_OK;    
} 

bool ADS1115_get_conversion_state()
{
    ADS1115_read_to_rwbuff(ADS1115_REG_CFG);
    return (ads_cfg->rw_buff[0] & 0x80) ? true : false;
} 

esp_err_t ADS1115_request_by_definition(uint8_t def)
{
    ads_cfg->reg_cfg &= ADS1115_CFG_MS_MUX_OMASK;
    ads_cfg->reg_cfg |= (def << 8) & 0xFF00;
    ads_cfg->reg_cfg |= ADS1115_CFG_MS_OS_ACTIVE & 0xFF00;
    return ADS1115_write_reg(ads_cfg->reg_cfg, ADS1115_REG_CFG);
}

int16_t ADS1115_get_conversion()
{
    ADS1115_read_to_rwbuff(ADS1115_REG_CONV);
    return (int16_t)BYTES_INT(ads_cfg->rw_buff[0], ads_cfg->rw_buff[1]);

}

esp_err_t ADS1115_set_thresh_by_definition(uint8_t thresh, uint16_t val)
{
    if(thresh != ADS1115_REG_LO_THRESH || thresh != ADS1115_REG_HI_THRESH)
        return ESP_ERR_INVALID_ARG;
        
    return ADS1115_write_reg(val, thresh);
}

static esp_err_t ADS1115_write_reg(uint16_t val, uint16_t reg)
{  
    ads_cfg->rw_buff[0] = (uint8_t)(val >> 8) & 0xFF;
    ads_cfg->rw_buff[1] = (uint8_t)val & 0xFF;

    return i2c_handle_write(ads_cfg->dev_addr, reg, 2, ads_cfg->rw_buff);
}

static esp_err_t ADS1115_read_to_rwbuff(uint8_t reg_adr)
{
    return i2c_handle_read(ads_cfg->dev_addr, reg_adr, 2, ads_cfg->rw_buff);
}

static esp_err_t i2c_handle_write(uint8_t dev_adr, uint8_t w_adr, uint8_t w_len, uint8_t *buff) 
{
  esp_err_t ret_err = ESP_OK;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  ret_err += i2c_master_start(cmd);    

  ret_err += i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_WRITE, true);
  ret_err += i2c_master_write_byte(cmd, w_adr, true);
  ret_err += i2c_master_write(cmd, buff, w_len, true);
  ret_err += i2c_master_stop(cmd);

  ret_err += i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(500));
  i2c_cmd_link_delete(cmd);
  
  return ret_err;
}

static esp_err_t i2c_handle_read(uint8_t dev_adr, uint8_t r_adr, uint8_t r_len, uint8_t *buff) 
{
  memset(buff, 0, sizeof(*buff));
  
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  esp_err_t ret_err = ESP_OK;

  ret_err += i2c_master_start(cmd);

  ret_err += i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_WRITE, true);
  ret_err += i2c_master_write_byte(cmd, r_adr, true);
  ret_err += i2c_master_start(cmd);
  ret_err += i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_READ, true);

  if (r_len > 1)
      ret_err += i2c_master_read(cmd, buff, r_len-1, I2C_MASTER_ACK);        
  ret_err += i2c_master_read_byte(cmd, buff + r_len-1, I2C_MASTER_NACK);
  ret_err += i2c_master_stop(cmd);

  ret_err += i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(500));
  i2c_cmd_link_delete(cmd);    

  return ret_err; 
}