/*
 * drv_mxt.c
 *
 * Created: 9/3/2019 9:34:35 AM
 *  Author: A20657
 */ 
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "driver_init.h"
#include "utils.h"
#include "drv_i2c_master.h"
#include "drv_mxt.h"
#include "config.h"


#define MXT_FAMILY_ID           0xA4
#define MXT_OBJECT_START        0x07
#define MXT_OBJECT_SIZE         6
#define MXT_INFO_CHECKSUM_SIZE  3
#define MXT_MAX_BLOCK_WRITE     255


/* Object types */
#define MXT_DEBUG_DIAGNOSTIC_T37    37
#define MXT_GEN_MESSAGE_T5        5
#define MXT_GEN_COMMAND_T6        6
#define MXT_GEN_POWER_T7        7
#define MXT_GEN_ACQUIRE_T8        8
#define MXT_GEN_DATASOURCE_T53        53
#define MXT_TOUCH_MULTI_T9        9
#define MXT_TOUCH_KEYARRAY_T15        15
#define MXT_TOUCH_PROXIMITY_T23        23
#define MXT_TOUCH_PROXKEY_T52        52
#define MXT_PROCI_GRIPFACE_T20        20
#define MXT_PROCG_NOISE_T22        22
#define MXT_PROCI_ONETOUCH_T24        24
#define MXT_PROCI_TWOTOUCH_T27        27
#define MXT_PROCI_GRIP_T40        40
#define MXT_PROCI_PALM_T41        41
#define MXT_PROCI_TOUCHSUPPRESSION_T42    42
#define MXT_PROCI_STYLUS_T47        47
#define MXT_PROCG_NOISESUPPRESSION_T48    48
#define MXT_SPT_COMMSCONFIG_T18        18
#define MXT_SPT_GPIOPWM_T19        19
#define MXT_SPT_SELFTEST_T25        25
#define MXT_SPT_CTECONFIG_T28        28
#define MXT_SPT_USERDATA_T38        38
#define MXT_SPT_DIGITIZER_T43        43
#define MXT_SPT_MESSAGECOUNT_T44    44
#define MXT_SPT_CTECONFIG_T46        46
#define MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71 71
#define MXT_PROCI_SYMBOLGESTUREPROCESSOR    92
#define MXT_PROCI_TOUCHSEQUENCELOGGER    93
#define MXT_TOUCH_MULTITOUCHSCREEN_T100 100
#define MXT_PROCI_ACTIVESTYLUS_T107    107

/* MXT_GEN_MESSAGE_T5 object */
#define MXT_RPTID_NOMSG        0xff
#define MXT_MSG_SIZE           0xA

/* MXT_GEN_COMMAND_T6 field */
#define MXT_COMMAND_RESET    0
#define MXT_COMMAND_BACKUPNV    1
#define MXT_COMMAND_CALIBRATE    2
#define MXT_COMMAND_REPORTALL    3
#define MXT_COMMAND_DIAGNOSTIC    5

/* Define for T6 status byte */
#define MXT_T6_STATUS_RESET     (1 << 7)
#define MXT_T6_STATUS_OFL       (1 << 6)
#define MXT_T6_STATUS_SIGERR    (1 << 5)
#define MXT_T6_STATUS_CAL       (1 << 4)
#define MXT_T6_STATUS_CFGERR    (1 << 3)
#define MXT_T6_STATUS_COMSERR    1 << 2)

/* Define for MXT_GEN_COMMAND_T6 */
#define MXT_BOOT_VALUE		0xa5
#define MXT_RESET_VALUE		0x01
#define MXT_BACKUP_VALUE	0x55


/* MXT_TOUCH_MULTI_T9 field */
#define MXT_T9_CTRL		0
#define MXT_T9_ORIENT		9
#define MXT_T9_RANGE		18

/* MXT_TOUCH_MULTI_T9 status */
#define MXT_T9_UNGRIP		BIT(0)
#define MXT_T9_SUPPRESS		BIT(1)
#define MXT_T9_AMP		BIT(2)
#define MXT_T9_VECTOR		BIT(3)
#define MXT_T9_MOVE		BIT(4)
#define MXT_T9_RELEASE		BIT(5)
#define MXT_T9_PRESS		BIT(6)
#define MXT_T9_DETECT		BIT(7)



#define MXT_CFG_MAGIC_CODE      "OBP_RAW V1"


uint8_t mxt_block_info[MXT_BLOCK_INFO_SIZE];


struct s_mxt_object
{
    uint8_t type;
    uint16_t start_address;
    uint8_t size_minus_one;
    uint8_t instances_minus_one;
    uint8_t num_report_ids;
    
} __attribute__((packed));


struct s_mxt_info {
    uint8_t family_id;
    uint8_t variant_id;
    uint8_t version;
    uint8_t build;
    uint8_t matrix_xsize;
    uint8_t matrix_ysize;
    uint8_t object_num;
};

struct s_mxt_data
{
    uint8_t slave_addr;
    uint8_t state;
    struct s_mxt_info mxt_info;
    
    uint16_t mem_size;
    uint8_t multitouch;
    uint8_t num_touchids;
    uint8_t max_reportid;
    uint32_t info_crc;
    uint32_t config_crc;
    
    uint16_t max_x;
    uint16_t max_y;
    
    uint8_t t6_status;
    uint8_t t25_info[7];
    
    /* Cached parameters from object table */
    uint16_t T5_address;
    uint16_t T6_address;
    uint16_t T7_address;
    uint16_t T9_address;
    uint16_t T18_address;
    uint16_t T25_address;
    uint16_t T38_address;
    uint16_t T44_address;
    uint16_t T100_address;
    uint8_t T5_msg_size;
    uint8_t T6_reportid;
    uint8_t T9_reportid_min;
    uint8_t T9_reportid_max;
    uint8_t T15_reportid_min;
    uint8_t T15_reportid_max;
    uint8_t T25_reportid;
    uint8_t T19_reportid;
    uint8_t T100_reportid_min;
    uint8_t T100_reportid_max;
     
    uint8_t msg_buf[MXT_MSG_SIZE];
};

struct s_mxt_data mxt_data;



//touch msg;
struct s_mxt_touch_msg mxt_touch_msg;

p_mxt_touch_cb mxt_touch_cb,mxt_keys_cb;


/*
drv_mxt_proc_t6_messages
*/
static void drv_mxt_proc_t6_message(uint8_t * msg)
{
    mxt_data.t6_status = msg[1];
    mxt_data.config_crc = msg[2] | (msg[3] << 8) | (msg[4] << 16);
	
	if(mxt_data.state == MXT_UPDATE_CFG)
	{	
		if(mxt_data.config_crc == file_cfg_crc)
		{
			//update config success
			mxt_data.state = MXT_UPDATE_CFG_SUCCESS;
		}
		else
		{
			//update config fail
			mxt_data.state = MXT_UPDATE_CFG_FAIL;
		}
	}
	
#ifdef MXT_CONFIG_UPDATE_WHEN_BOOT
    if(MXT_T6_STATUS_RESET&mxt_data.t6_status)
    {
        //check crc after reset
        if(mxt_data.config_crc != file_cfg_crc)
        {
            mxt_data.state = MXT_UPDATE_CFG;
        }
    }
#endif

}

/*
drv_mxt_proc_t25_messages
*/
static void drv_mxt_proc_t25_message(uint8_t * msg)
{
    memcpy(mxt_data.t25_info, &msg[1], 7);
}

/*
drv_mxt_proc_t9_message
*/
static void drv_mxt_proc_t9_message(uint8_t * msg)
{
    uint16_t x;
    uint16_t y;
    struct s_mxt_data *data = &mxt_data;
    
    mxt_touch_msg.id = msg[0] - mxt_data.T9_reportid_min;
    mxt_touch_msg.status = msg[1];
	x = (msg[2] << 4) | ((msg[4] >> 4) & 0xf);
	y = (msg[3] << 4) | ((msg[4] & 0xf));
    if(data->max_x < 1024)
        x >>= 2;
    if(data->max_y < 1024)
        y >>= 2;
    
    mxt_touch_msg.x_pos = x;
    mxt_touch_msg.y_pos = y;
}

/*
drv_mxt_proc_t100_message
*/
static void drv_mxt_proc_t100_message(uint8_t * msg)
{
    uint16_t x;
    uint16_t y;
    struct s_mxt_data *data = &mxt_data;
    
    mxt_touch_msg.id = msg[0] - data->T100_reportid_min - 2;
    mxt_touch_msg.status = msg[1];
    x = msg[2] | msg[3] << 8;
    y = msg[4] | msg[5] << 8;
    
    mxt_touch_msg.x_pos = x;
    mxt_touch_msg.y_pos = y;
}


/*
drv_mxt_proc_t15_message
*/
static void drv_mxt_proc_t15_message(uint8_t * msg)
{
    uint8_t ins;
    struct s_mxt_data *data = &mxt_data;
    if(data->T9_address)
    {
        mxt_touch_msg.key_status[0] = msg[2] | msg[3] << 8;
        mxt_touch_msg.key_status[1] = msg[4] | msg[5] << 8;
    }
    else if(data->T100_address)
    {
        ins = msg[0] - data->T15_reportid_min;
        if(ins > MXT_T15_INS_SIZE - 1)
        {
            //T15 report ID overflow, please check the MXT_T15_INS_SIZE
            return;
        }
        mxt_touch_msg.key_status[ins] = msg[2+ins] | msg[3+ins] << 8;
    }
}
/*
drv_mxt_process_msg
*/
static int32_t drv_mxt_process_msg(uint8_t *message)
{
    int32_t ret = 0;
	uint8_t report_id = message[0];
    struct s_mxt_data *data = &mxt_data;
    bool b_keystatus = false,b_touch = false;
    
    if(report_id == 0xff)
        return 0;
    if (report_id == data->T6_reportid)
    {
		drv_mxt_proc_t6_message(message);
	} 
    else if(report_id >= data->T9_reportid_min &&
		   report_id <= data->T9_reportid_max)
    {
        //touch
		drv_mxt_proc_t9_message(message);
        b_touch = true;
	} 
    else if (report_id >= data->T100_reportid_min &&
		   report_id <= data->T100_reportid_max)
    {
        //touch
		drv_mxt_proc_t100_message(message);
        b_touch = true;
	}
    else if (report_id == data->T19_reportid) 
    {
        //reserved
	} 
    else if (report_id == data->T25_reportid) 
    {
        drv_mxt_proc_t25_message(message);
	} 
    else if (report_id >= data->T15_reportid_min
		   && report_id <= data->T15_reportid_max) 
    {
        //T15 keys
		drv_mxt_proc_t15_message(message);
        b_keystatus = true;
	}
    if((mxt_touch_cb)&&b_touch)
        mxt_touch_cb(message);
    if((mxt_keys_cb)&&b_keystatus)
        mxt_keys_cb(message);
    return ret;
}


static int32_t mxt_parse_T9()
{
    int32_t ret;
    uint8_t pos[4];
    struct s_mxt_data *data = &mxt_data;
    
    ret = drv_i2c_read(data->slave_addr, data->T9_address + MXT_T9_RANGE, pos, 4);
    if(ret)
        return ret;
    data->max_x = pos[1] << 8 | pos[0];
    data->max_y = pos[3] << 8 | pos[2];
    return 0;
}


static int32_t mxt_parse_T100()
{
    
    return 0;
}

/*
mxt_parse_object_table
*/
static int32_t mxt_parse_object_table(uint8_t * object_table)
{
    int32_t ret = 0,i;
    uint8_t reportid = 1, min_id, max_id;
    uint16_t end_address;
    
    struct s_mxt_object *object;
    struct s_mxt_data *data = &mxt_data;
    
    for(i = 0; i < data->mxt_info.object_num; i ++)
    {
        object = (struct s_mxt_object *)object_table + i;
        
        //get report id
        if(object->num_report_ids)
        {
            min_id = reportid;
            reportid += object->num_report_ids * (object->instances_minus_one + 1);
            max_id = reportid - 1;
        }
        else
        {
            min_id = 0;
            max_id = 0;
        }
        
        switch(object->type)
        {
            case MXT_GEN_MESSAGE_T5:
                data->T5_msg_size = object->size_minus_one;
                data->T5_address = object->start_address;
                break;
            case MXT_GEN_COMMAND_T6:
                data->T6_reportid = min_id;
                data->T6_address = object->start_address;
                break;
            case MXT_GEN_POWER_T7:
                data->T7_address = object->start_address;
                break;
            case MXT_TOUCH_MULTI_T9:
                data->T9_address = object->start_address;
                data->multitouch = MXT_TOUCH_MULTI_T9;
                /* Only handle messages from first T9 instance */
                data->T9_reportid_min = min_id;
                data->T9_reportid_max = min_id + object->num_report_ids - 1;
                data->num_touchids = object->num_report_ids;
                ret = mxt_parse_T9();
                if(ret)
                    return ret;
                break;
            case MXT_TOUCH_KEYARRAY_T15:
                data->T15_reportid_min = min_id;
                data->T15_reportid_max = max_id;
                break;
            case MXT_SPT_SELFTEST_T25:
                data->T25_reportid = min_id;
                data->T25_address = object->start_address;
                break;
                
            case MXT_SPT_MESSAGECOUNT_T44:
                data->T44_address = object->start_address;
                break;
            case MXT_SPT_GPIOPWM_T19:
                data->T19_reportid = min_id;
                break;
            case MXT_SPT_USERDATA_T38:
                data->T38_address = object->start_address;
                break;
            case MXT_TOUCH_MULTITOUCHSCREEN_T100:
                data->T100_address = object->start_address;
                data->multitouch = MXT_TOUCH_MULTITOUCHSCREEN_T100;
                data->T100_reportid_min = min_id;
                data->T100_reportid_max = max_id;
                /* first two report IDs reserved */
                data->num_touchids = object->num_report_ids - 2;
                ret = mxt_parse_T100();
                break;
            default:
                break;
            
        }
        end_address = object->start_address + (object->size_minus_one + 1) * (object->instances_minus_one + 1) - 1;
        if (end_address >= data->mem_size)
        {
            data->mem_size = end_address + 1;
        }
    }
    
    /* Store maximum reportid */
    data->max_reportid = reportid;
    
    if (data->T44_address && (data->T5_address != data->T44_address + 1))
    {
        //T44 or T5 address error
        return ERR_MXT_T5_T44_ADDR;
    }
    return ret;
}


/*
mxt_calc_crc24
*/
static void mxt_calc_crc24(uint32_t *crc, uint8_t firstbyte, uint8_t secondbyte)
{
    static const unsigned int crcpoly = 0x80001B;
    uint32_t result;
    uint32_t data_word;

    data_word = (secondbyte << 8) | firstbyte;
    result = ((*crc << 1) ^ data_word);

    if (result & 0x1000000)
        result ^= crcpoly;

    *crc = result;
}

/*
mxt_calculate_crc
*/
static int32_t mxt_calculate_crc(uint8_t *base, uint16_t start_off, uint16_t end_off)
{
    uint32_t crc = 0;
    uint8_t *ptr = base + start_off;
    uint8_t *last_val = base + end_off - 1;

    if (end_off < start_off)
        return ERR_MXT_CRC_INPUT_DATA;

    while (ptr < last_val) {
        mxt_calc_crc24(&crc, *ptr, *(ptr + 1));
        ptr += 2;
    }

    /* if len is odd, fill the last byte with 0 */
    if (ptr == last_val)
        mxt_calc_crc24(&crc, *ptr, 0);

    /* Mask to 24-bit */
    crc &= 0x00FFFFFF;

    return crc;
}


/*
drv_mxt_looking_for
search *object in info block 
*/
static struct s_mxt_object *drv_mxt_looking_for(struct s_mxt_data *data, uint8_t *object_table, uint8_t object_no)
{
	uint8_t i;
	struct s_mxt_object *object;
	for(i = 0; i < data->mxt_info.object_num; i++)
	{
		object = (struct s_mxt_object *)object_table +i;
		if(object_no == object->type)
			return object;
	}
	return NULL;
}

static int32_t drv_mxt_send_T6_cmd(struct s_mxt_data *data, uint8_t offset, uint8_t cmd)
{
    return drv_i2c_write(data->slave_addr, data->T6_address + offset, &cmd, 1);
}
/*
drv_mxt_update_cfg
update cfg from .h
*/
int32_t drv_mxt_update_cfg()
{
	uint8_t object_no,object_no_old = 0,ins_no,size;
	uint8_t *point;
	uint16_t offset,len = sizeof(file_cfg_data);
	struct s_mxt_object *object = NULL;
	int32_t ret = 0;
	
	struct s_mxt_data *data = &mxt_data;
	
    /*
    if(data->config_crc)
    {
        //IC not ready OR no config
        return -1;
    } 
    */
    
    //check config data
    //magic code
    if(memcmp(MXT_CFG_MAGIC_CODE,file_magic_code,sizeof(MXT_CFG_MAGIC_CODE)))
    {
        //magic code mis-march
        return ERR_MXT_MAGIC_CODE;
    }
    //check device info
    if(memcmp(&data->mxt_info,file_device_info,sizeof(data->mxt_info)))
    {
        //device mis-march
        return ERR_MXT_DEVICE_INFO;
    }
    if(data->config_crc == file_cfg_crc)
    {
        //same crc, don't need update
        return 0;
    }
    
    //disable chg interrupt
    //#disable chg interrupt
	point = (uint8_t *)file_cfg_data;
	for(offset = 0; offset < len; )
	{
		object_no = *point++;
		ins_no = *point++;
		size = *point++;
		offset += size + 3;
		if(object_no == object_no_old)
			goto skip_looking;
		object = drv_mxt_looking_for(data, mxt_block_info + MXT_OBJECT_START, object_no);
		if(!object)
		{
			//fail to find the object
			continue;
		}
skip_looking:
		if(ins_no > object->instances_minus_one)
		{
			//ins# over object ins
			continue;
		}		
		ret = drv_i2c_write(data->slave_addr, object->start_address + ins_no*(object->size_minus_one+1), point, size);
		if(ret)
		{
			return ret;
		}
		object_no_old = object->type;
		point += size;
		
	}
	
	//send backup cmd
    ret = drv_mxt_send_T6_cmd(data, MXT_COMMAND_BACKUPNV, MXT_BACKUP_VALUE);
    
    //#enalbe chg interrupt
    //#wait_for_crc_update
	
	mxt_data.state = MXT_UPDATE_CFG_WAIT;
    return ret;
    
}


/*
drv_mxt_read_info
read mxt device info block
*/
int32_t drv_mxt_read_info()
{
    int32_t ret, crc_rd, crc_cal;
    uint16_t size;
	struct s_mxt_data *data = &mxt_data;
    
    if(MXT_OBJECT_SIZE != sizeof(struct s_mxt_object))
    {
        //s_mxt_object error
        //s_mxt_oject element align error, it will cause parse object fail.
        return ERR_MXT_OBJECT_SIZE;
    }
	
    ret = drv_i2c_read(data->slave_addr, 0, mxt_block_info, 7);
    if(ret)
    {
        return ret;
    }
	
    if(MXT_FAMILY_ID != mxt_block_info[0])
    {
        //it's not a mxt chip
        return ERR_MXT_DEVICE_ID;
    }
    
    size = MXT_OBJECT_START + MXT_OBJECT_SIZE * mxt_block_info[6] + MXT_INFO_CHECKSUM_SIZE;
	if(size > MXT_BLOCK_INFO_SIZE)
	{
		//ram size for mxt_block_info is too small, need enlarge 
		return ERR_MXT_INFO_RAM_SIZE;
	}
    
    ret = drv_i2c_read(data->slave_addr, MXT_OBJECT_START, &mxt_block_info[MXT_OBJECT_START], size - MXT_OBJECT_START);
    if(ret)
    {
        return ret;
    }
    
    //crc check
    crc_rd = mxt_block_info[size - 3] | (mxt_block_info[size - 2] << 8) | (mxt_block_info[size - 1] << 16);
    crc_cal = mxt_calculate_crc(mxt_block_info, 0, size - MXT_INFO_CHECKSUM_SIZE);
    if(crc_cal != crc_rd)
    {
        //crc caculate failed.
        return ERR_MXT_CRC_MARCH;
    }
    mxt_data.info_crc = crc_rd;
    memcpy(&mxt_data.mxt_info, mxt_block_info, MXT_OBJECT_START);
    
    ret = mxt_parse_object_table(mxt_block_info + MXT_OBJECT_START);
    if(ret)
    {
        return ret;
    }
    mxt_data.state = MXT_START;
    return 0;
}


/*
drv_mxt_irq
read msg in chg interrupt
*/
int32_t drv_mxt_irq(void)
{
	int32_t ret = 0;
	uint8_t count = 1;
	struct s_mxt_data *data = &mxt_data;
	
	if(!data->state)
	{
		return ERR_MXT_BLK_INFO;
	}
	if(data->T44_address)
	{
		//read T44 count
		ret = drv_i2c_read(data->slave_addr, data->T44_address, &count, 1);
		if(ret)
		{
			return ret;
		}
		if(count == 0)
		{
			return ret;
		}
	}
	
	while(count)
	{
		//read T5
		ret = drv_i2c_read(data->slave_addr, data->T5_address, data->msg_buf, data->T5_msg_size);
		if(ret)
		{
			return ret;
		}
		ret = drv_mxt_process_msg(data->msg_buf);
		count --;
	}
	return 0;
}


void drv_mxt_init()
{
    mxt_data.slave_addr = MXT_SLAVE_ADDRESS;
    mxt_data.state = MXT_UNINIT;
}

uint8_t drv_mxt_read_state()
{
    return mxt_data.state;
}

void drv_mxt_set_state(uint8_t state)
{
    mxt_data.state = state;
}

int32_t drv_mxt_sw_reset()
{
	return drv_mxt_send_T6_cmd(&mxt_data, MXT_COMMAND_RESET, MXT_RESET_VALUE);
}

//example
/*
void drv_mxt_task()
{
    int32_t ret;
    switch(mxt_data.state)
    {
        case MXT_START:
            mxt_data.state ++;
            //system setting, include gpio,por,isr, etc.
            
            break;
            
        case MXT_INIT:
            ret = drv_mxt_init();
            if(ret)
            {
                mxt_data.state = MXT_ERROR;
            }
            mxt_data.state ++;
            break;
            
        case MXT_NORMAL:
            drv_mxt_irq();
            break;
            
        case MXT_UPDATE_CFG:
            ret = drv_mxt_update_cfg(&mxt_data);
            mxt_data.state ++;
            break;
            
        case MXT_WAIT_CHG_INT:
            mxt_data.state = MXT_NORMAL;
            break;
            
        case MXT_SW_RESET:
            drv_mxt_send_T6_cmd(&mxt_data, MXT_COMMAND_RESET, MXT_RESET_VALUE);
            mxt_data.state = MXT_NORMAL;
            break;
            
        case MXT_ERROR:
        default:
            break;
    }
}


void drv_mxt_main(void)
{   
    while(1)
    {
        drv_mxt_task();
    }
}
*/

