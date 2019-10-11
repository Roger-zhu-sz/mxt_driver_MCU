/*
 * drv_i2c_master.c
 *
 * Created: 9/3/2019 9:35:24 AM
 *  Author: A20657
*/ 

#include <stdbool.h>
#include <string.h>
#include "driver_init.h"
#include "utils.h"
#include "drv_i2c_master.h"

void drv_i2c_master_init(void)
{
    
}

/*
drv_i2c_read
i2c read to *data
*/
int32_t drv_i2c_write(uint8_t slave_addr, uint16_t reg_addr, uint8_t * data, uint16_t len)
{
    int32_t ret = 0;
    struct io_descriptor *i2c_0_io;
    uint8_t addr_buf[2], w_buf[255],length;
    bool retry = false;
    
	if(len > 250)
		return -1;
	length = len + 2;
    addr_buf[0]= reg_addr & 0xff;
    addr_buf[1]= (reg_addr >> 8) & 0xff;
    
    i2c_m_sync_get_io_descriptor(&I2C_0, &i2c_0_io);
    i2c_m_sync_enable(&I2C_0);
    i2c_m_sync_set_slaveaddr(&I2C_0, slave_addr, I2C_M_SEVEN);
	memcpy(w_buf, addr_buf, 2);
	memcpy(w_buf+2, data, len);
    
    //write reg address + data
write_reg:
    ret = io_write(i2c_0_io, w_buf, length);
    if(ret != length)
    {
        if(!retry)
        {
            retry = true;
            //retry
            goto write_reg;
        }
        return ERR_I2C;
    }
    
    //write data
// write_data:    
//     ret = io_write(i2c_0_io, data, len);
//     if(ret != len)
//     {
//         if(!retry)
//         {
//             retry = true;
//             //retry
//             goto write_data;
//         }
//         return ERR_I2C;
//     }
    return 0;
}


/*
drv_i2c_read
i2c read to *data
*/
int32_t drv_i2c_read(uint8_t slave_addr, uint16_t reg_addr, uint8_t * data, uint16_t len)
{
    int32_t ret = 0;
    struct io_descriptor *i2c_0_io;
    uint8_t addr_buf[2];
    bool retry = false;
    
    addr_buf[0]= reg_addr & 0xff;
    addr_buf[1]= (reg_addr >> 8) & 0xff;
    
    i2c_m_sync_get_io_descriptor(&I2C_0, &i2c_0_io);
    i2c_m_sync_enable(&I2C_0);
    i2c_m_sync_set_slaveaddr(&I2C_0, slave_addr, I2C_M_SEVEN);
    
    //write reg address
write_reg:
    ret = io_write(i2c_0_io, addr_buf, 2);
    if(ret != 2)
    {
        if(!retry)
        {
            retry = true;
            //retry 
            goto write_reg;
        }
        return ERR_I2C;
    }
    
    //read data
read_data:
    ret = io_read(i2c_0_io, data, len);
    if(ret != len)
    {
        if(!retry)
        {
            retry = true;
            //retry
            goto read_data;
        }
        return ERR_I2C;
    }    
    return 0;
}



