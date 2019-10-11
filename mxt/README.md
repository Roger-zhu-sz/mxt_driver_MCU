Description & usage
==================
MCU driver for maxtouch guidance

1.	Hardware.
Maxtouch connect to MCU through I2C.  AVDD/VDD/RESET/SDA/SCL/CHG/GND

2.	Software
File: drv_mxt.c drv_mxt.h
Flow:
    a.	MCU HW setup.  Eg.  iic init, CHG/RESET GPIO init.
    b.	Mxt init. Include POR, CHG IRQ process setting, mxt info block read.
    c.	Mxt msg process by detecting CHG(low level or failing).  Failing may suit MCU better.
    d.	update config if needed when receive T6 msg (by checking crc value).
    e. process CHG irq.

Eg.
    MCU preparing:
        drv_i2c_read(u8 i2c_slaveaddr, u16 reg_addr, u8 *data, u16 len);
        drv_i2c_write(u8 i2c_slaveaddr, u16 reg_addr, u8 *data, u16 len);
        MCU_init();  (reset/chg setting, power control pin init).
        
        Mxt control process:
            Mxt_POR()
            Mxt_CHG_IRQ_INIT()
            Mxt_data.slave_addr setting
            Drv_mxt_read_info
            CHG IRQ process in loop

              
