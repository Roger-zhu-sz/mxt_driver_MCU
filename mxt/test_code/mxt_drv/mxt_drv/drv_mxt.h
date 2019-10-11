/*
 * drv_mxt.h
 *
 * Created: 9/3/2019 9:34:50 AM
 *  Author: A20657
 */ 


#ifndef DRV_MXT_H_
#define DRV_MXT_H_

#define MXT_T15_INS_SIZE        2
#define MXT_BLOCK_INFO_SIZE     260

#define MXT_CONFIG_UPDATE_WHEN_BOOT
#define MXT_SLAVE_ADDRESS       0x4a            /*schematic depends on */

enum{
    MXT_UNINIT = 0,
    MXT_START,
    MXT_NORMAL,
    MXT_UPDATE_CFG,
	MXT_UPDATE_CFG_WAIT,
    MXT_UPDATE_CFG_SUCCESS,
    MXT_UPDATE_CFG_FAIL,
    MXT_SW_RESET,
    MXT_ERROR,
};



/*--------------------------------------*/
/*ERROR CODE*/
/*--------------------------------------*/
#define ERR_MXT_INFO_RAM_SIZE           -1
#define ERR_MXT_OBJECT_SIZE             -2
#define ERR_MXT_IIC_TRANSFER            -3
#define ERR_MXT_T5_T44_ADDR             -4
#define ERR_MXT_CRC_INPUT_DATA          -5
#define ERR_MXT_CRC_MARCH               -6
#define ERR_MXT_DEVICE_ID               -7
#define ERR_MXT_MAGIC_CODE              -8
#define ERR_MXT_DEVICE_INFO             -9
#define ERR_MXT_BLK_INFO                -10


struct s_mxt_touch_msg
{
    //touches
    uint8_t id;
    uint8_t status;
    uint16_t x_pos;
    uint16_t y_pos;
    //keys
    uint16_t key_status[MXT_T15_INS_SIZE];
};

extern struct s_mxt_touch_msg mxt_touch_msg;




typedef int32_t (*p_mxt_touch_cb)(uint8_t *mxt_msg);

extern p_mxt_touch_cb mxt_touch_cb,mxt_keys_cb;


void drv_mxt_init();
uint8_t drv_mxt_read_state();
void drv_mxt_set_state(uint8_t state);
int32_t drv_mxt_read_info();
int32_t drv_mxt_irq();
int32_t drv_mxt_update_cfg();
int32_t drv_mxt_sw_reset();


#endif /* DRV_MXT_H_ */