/*
 * drv_i2c_master.h
 *
 * Created: 9/3/2019 9:35:36 AM
 *  Author: A20657
 */ 


#ifndef DRV_I2C_MASTER_H_
#define DRV_I2C_MASTER_H_


//error code
#define ERR_I2C                 -1

int32_t drv_i2c_write(uint8_t slave_addr, uint16_t reg_addr, uint8_t * data, uint16_t len);
int32_t drv_i2c_read(uint8_t slave_addr, uint16_t reg_addr, uint8_t * data, uint16_t len);


#endif /* DRV_I2C_MASTER_H_ */