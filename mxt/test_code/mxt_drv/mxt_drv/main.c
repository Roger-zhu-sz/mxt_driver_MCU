#include <atmel_start.h>
#include "drv_mxt.h"
extern uint8_t tiny1617_fw[];
uint8_t ii;

int32_t user_touch_cb(uint8_t *msg)
{
	int32_t ret = 0;
	
	//touch data, after T series
	//msg[0]: report ID
	//msg[1]: status
	//msg[2-3]: x
	//msg[4-5]: y
	//check protocol for others & detail
	
	return ret;
}


int32_t user_keys_cb(uint8_t *msg)
{
	int32_t ret = 0;
	
	//key data, after T series
	//msg[0]: report ID
	//msg[1-2]: status
	//check protocol for others & detail
	
	return ret;
}

/*========================
mxt_init
========================*/
static int32_t mxt_init(void)
{
    int32_t ret = 0;
    /*
    //POR&interrupt setting
    drv_mxt_gpio_init();
    drv_mxt_power(0);
    SET_RESET_OUTPUT;
    drv_mxt_set_reset(0);
    SET_CHG_INPUT();
    
    drv_mxt_set_chg_irq();
    disable_chg_irq();
    
    drv_mxt_power(1);
    drv_mxt_set_reset(1);
    delay_ms(100);
    */
    
    mxt_touch_cb = user_touch_cb;
    mxt_keys_cb = user_keys_cb;
    
    drv_mxt_init();
	
    ret = drv_mxt_read_info();
    
    if(ret)
    {
        //failed to read mxt info 
		//user can add repeat read here. re-POR is recommended before retry. 
        return ret;
    }
    
    
    /*
    enable_chg_irq();
    */
    return 0;
}

void set_chg()
{
	
}

void set_reset()
{
	
}

void set_power()
{
	
}
bool read_chg()
{	
	return false;
}

void drv_mxt_task()
{
	int32_t ret;
	uint8_t state = drv_mxt_read_state();
	
	switch(state)
	{
		case MXT_UPDATE_CFG:
			ret = drv_mxt_update_cfg();
			if(ret)
			{
				//send update data fail
				drv_mxt_set_state(MXT_NORMAL);
			}
			break;
			
		case MXT_UPDATE_CFG_WAIT:
			drv_mxt_set_state(MXT_NORMAL);
			break;
		//cfg update sucess
		case MXT_UPDATE_CFG_SUCCESS:
			drv_mxt_set_state(MXT_NORMAL);
			break;		
		
		//cfg update fail	
		case MXT_UPDATE_CFG_FAIL:
			drv_mxt_set_state(MXT_NORMAL);
			break;
		
		case MXT_SW_RESET:
			drv_mxt_sw_reset();
			drv_mxt_set_state(MXT_NORMAL);
		break;
		
		case MXT_ERROR:
		default:
		break;
	}
	
	if(!read_chg())
	{
		drv_mxt_irq();
	}
}


void drv_mxt_main(void)
{
	int32_t ret;
		
	set_chg();
	set_reset();
	set_power();
	
	ret = mxt_init();
	//!! must add  delay after power on, check the datasheet for delay time
	
	if(ret)
	{
		//init fail
		drv_mxt_set_state(MXT_ERROR);
	}
	
	while(1)
	{
		drv_mxt_task();
	}
}

int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
//	ii = tiny1617_fw[0];


	drv_mxt_main();
	
	/* Replace with your application code */
	while (1) {
		
	}
}
