/*
 * delay.S
 *
 *  Created on: Jan 27, 2015
 *      Author: wella
 */

.syntax unified
.cpu cortex-m3
.fpu softvfp
.thumb
.text
.section	.text.delay,"ax",%progbits
.align	2
.global	delay
.thumb
.thumb_func

delay:
.cfi_startproc
.L3:
subs	r0, r0, #1
cmp	r0, #0
bne	.L3
bx	lr
.cfi_endproc
