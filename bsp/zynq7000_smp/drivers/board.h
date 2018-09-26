/*
 * File      : board.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2013, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2013-07-06     Bernard    the first version
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include "zynq7000.h"

/* Freq of all peripherals */
#define APU_FREQ     666666667
#define DDR_FREQ     533333313
#define DCI_FREQ     10159000
#define QSPI_FREQ    200000000
#define SMC_FREQ     100000000
#define ENET0_FREQ   125000000
#define ENET1_FREQ   125000000
#define USB0_FREQ    60000000
#define USB1_FREQ    60000000
#define SDIO_FREQ    50000000
#define UART_FREQ    50000000
#define SPI_FREQ     166666666
#define I2C_FREQ     25000000
#define WDT_FREQ     133333333
#define TTC_FREQ     50000000
#define CAN_FREQ     100000000
#define PCAP_FREQ    200000000
#define TPIU_FREQ    0
#define FPGA0_FREQ   100000000
#define FPGA1_FREQ   200000000
#define FPGA2_FREQ   200000000
#define FPGA3_FREQ   80000000

#if defined(__CC_ARM)
extern int Image$$RW_IRAM1$$ZI$$Limit;
#define HEAP_BEGIN      ((void*)&Image$$RW_IRAM1$$ZI$$Limit)
#elif defined(__GNUC__)
extern int __bss_end;
#define HEAP_BEGIN      ((void*)&__bss_end)
#endif

#define HEAP_END        (void*)(0x40000000)

void rt_hw_board_init(void);

#endif
