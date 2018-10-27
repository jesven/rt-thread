/*
 * File      : lwp_gcc.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2018, RT-Thread Development Team
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
 */

#define  Mode_USR       0x10
#define  Mode_FIQ       0x11
#define  Mode_IRQ       0x12
#define  Mode_SVC       0x13
#define  Mode_MON       0x16
#define  Mode_ABT       0x17
#define  Mode_UDF       0x1B
#define  Mode_SYS       0x1F

#define A_Bit        0x100 
#define I_Bit        0x80 @; when I bit is set, IRQ is disabled
#define F_Bit        0x40 @; when F bit is set, FIQ is disabled
#define T_Bit        0x20

.cpu cortex-a9
.syntax unified
.text

/*
 * void* lwp_get_sys_api(rt_uint32_t number);
 */
.global lwp_get_sys_api
.global lwp_get_kernel_sp
.global lwp_set_kernel_sp


/*
 * void lwp_user_entry(u32 R0_text_addr, u32 R1_data_addr);
 */
.global lwp_user_entry
.type lwp_user_entry, % function
lwp_user_entry:

    mov     r9, sp              /* set usr mode sp */

    cps #Mode_SYS
    mov sp, r9
    cps #Mode_SVC

    mrs     r9, cpsr
    bic     r9, #0x1f
    orr     r9, #Mode_USR
    msr     spsr, r9

    /* set data address. */
    mov     r9, r2
    movs    pc, r1

/*
 * void SVC_Handler(void);
 */
.global SVC_Handler
.type SVC_Handler, % function
SVC_Handler:
    cps #Mode_SYS
    push {r0}
    ldr r0, =usr_stack_point
    str sp, [r0]
    pop {r0}
    cps #Mode_SVC
    ldr sp, =usr_stack_point
    ldr sp, [sp]
    add sp, #4

    push {r0}

    ldr r0, [lr, #-4]
    and r0, #0xff

    push {r1 - r3, lr}
    bl lwp_get_sys_api

    cmp r0, #0           /* r0 = api */
    beq svc_err_quit

    ldr r2, =svc_exit
    cps #Mode_SYS
    mov r1, lr           /* syscall caller*/
    mov lr, r2           /* after api return to svc_exit */
    cps #Mode_SVC
    ldr r2, =usr_lr
    str r1, [r2]          /* usr's lr -> usr_lr */

    pop {r1 - r3, lr}
    mov lr, r0         /* lr, prepare return to api */

    mrs  r0, spsr
    orr  r0, #Mode_SYS /* 0x1f, no need bic */
    msr  spsr, r0     /* prepare return to system mode */

    pop {r0}
    movs pc, lr

svc_err_quit: 
    pop {r1 - r3, lr}
    pop {r0}
    movs pc, lr

svc_exit:
    mrs lr, cpsr
    bic lr, #0x1f
    orr lr, #Mode_USR
    msr cpsr_scxf, lr
    ldr lr, =usr_lr
    ldr lr, [lr]
    bx lr

.bss
.align 2
usr_lr:
    .space 4
usr_stack_point:
    .space 4
