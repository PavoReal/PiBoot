
//
// Entry point
//
.globl _start
_start:
.func _start
    ldr pc,_reset_handler
    ldr pc,_undefined_handler
    ldr pc,_swi_handler
    ldr pc,_prefetch_handler
    ldr pc,_data_handler
    ldr pc,_unused_handler
    ldr pc,_irq_handler
    ldr pc,_fiq_handler
_reset_handler:      .word reset
_undefined_handler:  .word undefined_handler
_swi_handler:        .word swi_handler
_prefetch_handler:   .word prefetch_handler
_data_handler:       .word data_handler
_unused_handler:     .word unused_handler
_irq_handler:        .word irq_handler
_fiq_handler:        .word fiq_handler

reset:
    mov r0,#0x8000
    mov r1,#0x0000
    ldmia r0!,{r2,r3,r4,r5,r6,r7,r8,r9}
    stmia r1!,{r2,r3,r4,r5,r6,r7,r8,r9}
    ldmia r0!,{r2,r3,r4,r5,r6,r7,r8,r9}
    stmia r1!,{r2,r3,r4,r5,r6,r7,r8,r9}

    ;@ (PSR_IRQ_MODE|PSR_FIQ_DIS|PSR_IRQ_DIS)
    mov r0,#0xD2
    msr cpsr_c,r0
    mov sp,#0x8000

    ;@ (PSR_FIQ_MODE|PSR_FIQ_DIS|PSR_IRQ_DIS)
    mov r0,#0xD1
    msr cpsr_c,r0
    mov sp,#0x4000

    ;@ (PSR_SVC_MODE|PSR_FIQ_DIS|PSR_IRQ_DIS)
    mov r0,#0xD3
    msr cpsr_c,r0
    mov sp,#0x1C000000

    ;@ SVC MODE, IRQ ENABLED, FIQ DIS
    ;@mov r0,#0x53
    ;@msr cpsr_c, r0
    
    mov r0,pc
    bl start

hang: 
    b hang
.endfunc

.globl enable_irq
enable_irq:
    mrs r0,cpsr
    bic r0,r0,#0x80
    msr cpsr_c,r0
    bx lr

reset_handler:
    mov r0,#0x04
    b handler

undefined_handler:
    mov r0,#0x08
    b handler

swi_handler:
    mov r0,#0x0C
    b handler

prefetch_handler:
    mov r0,#0x10
    b handler

data_handler:
    mov r0,#0x14
    b handler

unused_handler:
    mov r0,#0x18
    b handler

irq_handler:
    push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
    bl c_irq_handler
    pop  {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
    subs pc,lr,#4

fiq_handler:
    mov r0,#0x1C
    b handler

handler:
    b hang

data_abort:
    b hang

.globl start_mmu
start_mmu:
    mov r2,#0
    mcr p15,0,r2,c7,c7,0 ;@ invalidate caches
    mcr p15,0,r2,c8,c7,0 ;@ invalidate tlb
    mcr p15,0,r2,c7,c10,4 ;@ DSB ??

    mvn r2,#0
    bic r2,#0xC
    mcr p15,0,r2,c3,c0,0 ;@ domain

    mcr p15,0,r0,c2,c0,0 ;@ tlb base
    mcr p15,0,r0,c2,c0,1 ;@ tlb base

    mrc p15,0,r2,c1,c0,0
    orr r2,r2,r1
    mcr p15,0,r2,c1,c0,0

    bx lr

.globl stop_mmu
stop_mmu:
    mrc p15,0,r2,c1,c0,0
    bic r2,#0x1000
    bic r2,#0x0004
    bic r2,#0x0001
    mcr p15,0,r2,c1,c0,0
    bx lr

.globl invalidate_tlbs
invalidate_tlbs:
    mov r2,#0
    mcr p15,0,r2,c8,c7,0  ;@ invalidate tlb
    mcr p15,0,r2,c7,c10,4 ;@ DSB ??
    bx lr

.globl PUT32
PUT32:
    str r1,[r0]
    bx lr

.globl GET32
GET32:
    ldr r0,[r0]
    bx lr

//
// Simply returns
//
.globl nop
nop:
.func nop
    bx lr
.endfunc

//
// See void dmb(void) in Arm.h
//
.globl dmb
dmb:
.func dmb
	mov	r12, #0
	mcr	p15, 0, r12, c7, c10, 5
	bx lr
.endfunc
