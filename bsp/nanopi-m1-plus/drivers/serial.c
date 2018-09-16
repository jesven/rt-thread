/*
 *  serial.c UART driver
 *
 * COPYRIGHT (C) 2013, Shanghai Real-Thread Technology Co., Ltd
 *
 *  This file is part of RT-Thread (http://www.rt-thread.org)
 *  Maintainer: bernard.xiong <bernard.xiong at gmail.com>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2013-03-30     Bernard      the first verion
 */

#include <rthw.h>
#include <rtdevice.h>

#include "serial.h"

struct hw_uart_device
{
    rt_uint32_t hw_base;
    rt_uint32_t irqno;
};

#define IRQ_UART0               32
#define IRQ_UART1               33
#define SUNXI_UART0_BASE        0x01c28000
#define SUNXI_UART1_BASE        0x01c28400

#define UART_REG(x) unsigned int x

struct NS16550 {
    UART_REG(rbr);      /* 0 */
    UART_REG(ier);      /* 1 */
    UART_REG(fcr);      /* 2 */
    UART_REG(lcr);      /* 3 */
    UART_REG(mcr);      /* 4 */
    UART_REG(lsr);      /* 5 */
    UART_REG(msr);      /* 6 */
    UART_REG(spr);      /* 7 */
};

#define thr rbr
#define iir fcr
#define dll rbr
#define dlm ier

#define UART_LSR_TEMT   0x40    /* Transmitter empty */
#define UART_LSR_THRE   0x20    /* Transmit-hold-register empty */
#define UART_LSR_BI 0x10    /* Break interrupt indicator */
#define UART_LSR_FE 0x08    /* Frame error indicator */
#define UART_LSR_PE 0x04    /* Parity error indicator */
#define UART_LSR_OE 0x02    /* Overrun error indicator */
#define UART_LSR_DR 0x01    /* Receiver data ready */

#define UART_IER_PTIME 0x80
#define UART_IER_ELSI  0x4
#define UART_IER_ETBEI 0x2
#define UART_IER_ERBFI 0x1

typedef struct NS16550 *NS16550_t;

static unsigned int readl(volatile void *addr)
{
    return *(volatile unsigned int *)addr;
}

static void writel(unsigned int v, volatile void *addr)
{
    *(volatile unsigned int *)addr = v;
}

static void NS16550_putc(NS16550_t com_port, char c)
{
    while ((readl(&com_port->lsr) & UART_LSR_THRE) == 0)
        ;
    writel(c, &com_port->thr);
}

static int NS16550_getc(NS16550_t com_port)
{
    if ((readl(&com_port->lsr) & UART_LSR_DR) == 0)
	{
		return -1;
	}
    return readl(&com_port->thr);
}

////////////////////////////////////////////////////////

 void rt_hw_uart_isr(int irqno, void *param)
{
    struct rt_serial_device *serial = (struct rt_serial_device *)param;

    rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
}

static rt_err_t uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    return RT_EOK;
}

static rt_err_t uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct hw_uart_device *uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct hw_uart_device *)serial->parent.user_data;

    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        /* disable rx irq */
		((NS16550_t)uart->hw_base)->ier &= ~UART_IER_ERBFI;
        rt_hw_interrupt_mask(uart->irqno);
        break;

    case RT_DEVICE_CTRL_SET_INT:
        /* enable rx irq */
		((NS16550_t)uart->hw_base)->ier |= UART_IER_ERBFI;
        rt_hw_interrupt_umask(uart->irqno);
        break;
    }

    return RT_EOK;
}

static int uart_putc(struct rt_serial_device *serial, char c)
{
    struct hw_uart_device *uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct hw_uart_device *)serial->parent.user_data;
    
	NS16550_putc((NS16550_t)uart->hw_base, c);

    return 1;
}

static int uart_getc(struct rt_serial_device *serial)
{
    int ch;
    struct hw_uart_device *uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct hw_uart_device *)serial->parent.user_data;

	ch = NS16550_getc((NS16550_t)uart->hw_base);
    return ch;
}

static const struct rt_uart_ops _uart_ops =
{
    uart_configure,
    uart_control,
    uart_putc,
    uart_getc,
};

#ifdef RT_USING_UART0
/* UART device driver structure */
static struct hw_uart_device _uart0_device =
{
    SUNXI_UART0_BASE,
    IRQ_UART0,
};
static struct rt_serial_device _serial0;
#endif

#ifdef RT_USING_UART1
/* UART1 device driver structure */
static struct hw_uart_device _uart1_device =
{
    SUNXI_UART1_BASE,
    IRQ_UART1,
};
static struct rt_serial_device _serial1;
#endif

int rt_hw_uart_init(void)
{
    struct hw_uart_device *uart;
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

#ifdef RT_USING_UART0
    uart = &_uart0_device;

    _serial0.ops    = &_uart_ops;
    _serial0.config = config;

    /* register UART1 device */
    rt_hw_serial_register(&_serial0, "uart0",
                          RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX,
                          uart);
    rt_hw_interrupt_install(uart->irqno, rt_hw_uart_isr, &_serial0, "uart0");
    /* enable Rx and Tx of UART */
#endif

#ifdef RT_USING_UART1
    uart = &_uart1_device;
    _serial1.ops = &_uart_ops;
    _serial1.config = config;

    /* register UART1 device */
    rt_hw_serial_register(&_serial1, "uart1",
                          RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX, uart);
    /* enable Rx and Tx of UART */
    rt_hw_interrupt_install(uart->irqno, rt_hw_uart_isr, &_serial1, "uart1");
#endif

    return 0;
}
INIT_BOARD_EXPORT(rt_hw_uart_init);
