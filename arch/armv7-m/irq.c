#include <io.h>
#include <kernel/task.h>
#include <error.h>

static void ISR_irq();

void *irq_vectors[]
__attribute__((section(".vector_irq"), aligned(4), used, weak)) = {
			/* nVEC(nIRQ): ADDR - DESC */
			/* ----------------------- */
	ISR_irq,	/*  16(0)   : 0x40  - WWDG */
	ISR_irq,	/*  17(1)   : 0x44  - PVD */
	ISR_irq,	/*  18(2)   : 0x48  - TAMPER */
	ISR_irq,	/*  19(3)   : 0x4c  - RTC */
	ISR_irq,	/*  20(4)   : 0x50  - FLASH */
	ISR_irq,	/*  21(5)   : 0x54  - RCC */
	ISR_irq,	/*  22(6)   : 0x58  - EXTI0 */
	ISR_irq,	/*  23(7)   : 0x5c  - EXTI1 */
	ISR_irq,	/*  24(8)   : 0x60  - EXTI2 */
	ISR_irq,	/*  25(9)   : 0x64  - EXTI3 */
	ISR_irq,	/*  26(10)  : 0x68  - EXTI4 */
	ISR_irq,	/*  27(11)  : 0x6c  - DMA1_Channel1 */
	ISR_irq,	/*  28(12)  : 0x70  - DMA1_Channel2 */
	ISR_irq,	/*  29(13)  : 0x74  - DMA1_Channel3 */
	ISR_irq,	/*  30(14)  : 0x78  - DMA1_Channel4 */
	ISR_irq,	/*  31(15)  : 0x7c  - DMA1_Channel5 */
	ISR_irq,	/*  32(16)  : 0x80  - DMA1_Channel6 */
	ISR_irq,	/*  33(17)  : 0x84  - DMA1_Channel7 */
	ISR_irq,	/*  34(18)  : 0x88  - ADC1 | ADC2 */
	ISR_irq,	/*  35(19)  : 0x8c  - USB High Priority | CAN TX */
	ISR_irq,	/*  36(20)  : 0x90  - USB Low Priority | CAN RX0 */
	ISR_irq,	/*  37(21)  : 0x94  - CAN RX1 */
	ISR_irq,	/*  38(22)  : 0x98  - CAN SCE */
	ISR_irq,	/*  39(23)  : 0x9c  - EXTI[9:5] */
	ISR_irq,	/*  40(24)  : 0xa0  - TIM1 Break */
	ISR_irq,	/*  41(25)  : 0xa4  - TIM1 Update */
	ISR_irq,	/*  42(26)  : 0xa8  - TIM1 Trigger | Communication */
	ISR_irq,	/*  43(27)  : 0xac  - TIM1 Capture Compare */
	ISR_irq,	/*  44(28)  : 0xb0  - TIM2 */
	ISR_irq,	/*  45(29)  : 0xb4  - TIM3 */
	ISR_irq,	/*  46(30)  : 0xb8  - TIM4 */
	ISR_irq,	/*  47(31)  : 0xbc  - I2C1 Event */
	ISR_irq,	/*  48(32)  : 0xc0  - I2C1 Error */
	ISR_irq,	/*  49(33)  : 0xc4  - I2C2 Event */
	ISR_irq,	/*  50(34)  : 0xc8  - I2C2 Error */
	ISR_irq,	/*  51(35)  : 0xcc  - SPI1 */
	ISR_irq,	/*  52(36)  : 0xd0  - SPI2 */
	ISR_irq,	/*  53(37)  : 0xd4  - USART1 */
	ISR_irq,	/*  54(38)  : 0xd8  - USART2 */
	ISR_irq,	/*  55(39)  : 0xdc  - USART3 */
	ISR_irq,	/*  56(40)  : 0xe0  - EXTI[15:10] */
	ISR_irq,	/*  57(41)  : 0xe4  - RTC Alarm */
	ISR_irq,	/*  58(42)  : 0xe8  - USB Wakeup */
	ISR_irq,	/*  59(43)  : 0xec  - TIM8 Break */
	ISR_irq,	/*  60(44)  : 0xf0  - TIM8 Update */
	ISR_irq,	/*  61(45)  : 0xf4  - TIM8 Trigger | Communication */
	ISR_irq,	/*  62(46)  : 0xf8  - TIM8 Capture Compare */
	ISR_irq,	/*  63(47)  : 0xfc  - ADC3 */
	ISR_irq,	/*  64(48)  : 0x100 - FSMC */
	ISR_irq,	/*  65(49)  : 0x104 - SDIO */
	ISR_irq,	/*  66(50)  : 0x108 - TIM5 */
	ISR_irq,	/*  67(51)  : 0x10c - SPI3 */
	ISR_irq,	/*  68(52)  : 0x110 - UART4 */
	ISR_irq,	/*  69(53)  : 0x114 - UART5 */
	ISR_irq,	/*  70(54)  : 0x118 - TIM6 */
	ISR_irq,	/*  71(55)  : 0x11c - TIM7 */
	ISR_irq,	/*  72(56)  : 0x120 - DMA2_Channel1 */
	ISR_irq,	/*  73(57)  : 0x124 - DMA2_Channel2 */
	ISR_irq,	/*  74(58)  : 0x128 - DMA2_Channel3 */
	ISR_irq,	/*  75(59)  : 0x12c - DMA2_Channel4,5 */
};

void ISR_null(int nvector)
{
	error("ISR is not yet registered: %x", nvector);
}

static int (*secondary_irq_registers[PRIMARY_IRQ_MAX])(int, void (*)(int));

#ifdef CONFIG_COMMON_IRQ_FRAMEWORK
void (*primary_irq_table[PRIMARY_IRQ_MAX])(int);

static void __attribute__((naked)) ISR_irq()
{
	__asm__ __volatile__(
			"sub	sp, sp, #8		\n\t"
			"str	lr, [sp]		\n\t"
			"mrs	r0, ipsr		\n\t"
			"ldr	r1, =primary_irq_table	\n\t"
			"sub	r2, r0, #16		\n\t"
			"ldr	r3, [r1, r2, lsl #2]	\n\t"
			"blx	r3			\n\t"
			"ldr	lr, [sp]		\n\t"
			"add	sp, sp, #8		\n\t"
			"bx	lr			\n\t"
			::: "memory");
}

static int register_isr_primary(int lvector, void (*handler)(int))
{
	void (*p)(int);
	void (*f)(int);

	if (lvector < NVECTOR_IRQ)
		return EACCES;

	p = (void *)&primary_irq_table[lvector - NVECTOR_IRQ];

	do {
		f = (void *)__ldrex(p);

		if (f != ISR_null && f != ISR_irq) /* recursive if ISR_irq */
			return EEXIST;
	} while (__strex(handler, p));

	return 0;
}
#else /* !CONFIG_COMMON_IRQ_FRAMEWORK */
static void ISR_irq()
{
	ISR_null(__get_psr());
}

static int register_isr_primary(int lvector, void (*handler)(int))
{
	if (lvector < NVECTOR_IRQ)
		return EACCES;

	extern int _ram_start;
	void (*f)(int);
	unsigned int *p = (unsigned int *)&_ram_start;

	p += lvector;

	do {
		f = __ldrex(p);
		if (f != ISR_irq)
			return EEXIST;
	} while (__strex(handler, p));

	return 0;
}
#endif /* CONFIG_COMMON_IRQ_FRAMEWORK */


/* NOTE: calling multiple handlers is possible chaining handlers to a list,
 * which sounds flexible. but I don't think it would be useful since such use
 * cases don't look nice but causing latency and complexity. */
/* it will also be unregister if force=1 */
int register_isr_register(int lvector, int (*cb)(int, void (*)(int)), bool force)
{
	int (*p)(int);
	int (*f)(int);

	if (get_current_rank() != TF_PRIVILEGED)
		return EPERM;

	p = (void *)&secondary_irq_registers[get_primary_vector(lvector) - NVECTOR_IRQ];

	do {
		f = (void *)__ldrex(p);

		if (!force && f) {
			error("already exist or no room");
			return EEXIST;
		}
	} while (__strex(cb, p));

	return 0;
}

/* it will also be unregister if handler is null */
static int register_isr_secondary(int lvector, void (*handler)(int))
{
	if (get_secondary_vector(lvector) >= SECONDARY_IRQ_MAX)
		return ERANGE;

	if (get_primary_vector(lvector) < NVECTOR_IRQ)
		return EACCES;

	int (*f)(int, void (*)());

	if ((f = secondary_irq_registers[get_primary_vector(lvector) - NVECTOR_IRQ]))
		return f(lvector, handler);

	error("no irq register for %d:%d",
			get_primary_vector(lvector), get_secondary_vector(lvector));

	return EANYWAY;
}

int register_isr(int lvector, void (*handler)(int))
{
	int ret;

	if (get_current_rank() != TF_PRIVILEGED)
		return EPERM;

	if (lvector < PRIMARY_IRQ_MAX)
		ret = register_isr_primary(lvector, handler);
	else
		ret = register_isr_secondary(lvector, handler);

	dsb();
	isb();

	return ret;
}

int unregister_isr(int lvector)
{
	int ret, i;

	if (get_current_rank() != TF_PRIVILEGED)
		return EPERM;

	if (lvector < PRIMARY_IRQ_MAX) {
		/* unregister all the secondaries. it is time cunsuming since
		 * calling the unregister SECONDARY_IRQ_MAX times even if not
		 * registered. */
		for (i = 0; i < SECONDARY_IRQ_MAX; i++)
			register_isr_secondary(mkvector(lvector, i), NULL);

		ret = register_isr_primary(lvector, ISR_irq);
		register_isr_register(lvector, NULL, 1);
	} else {
		ret = register_isr_secondary(lvector, NULL);
	}

	dsb();
	isb();

	return ret;
}

#ifdef CONFIG_SYSCALL
#include <kernel/syscall.h>

#ifdef CONFIG_DEBUG_SYSCALL
unsigned int syscall_count = 0;
#endif

#ifndef CONFIG_SYSCALL_THREAD
#ifdef CONFIG_DEBUG_SYSCALL_NESTED
#include <error.h>
void syscall_nested(int sysnum)
{
	error("syscall %d nested!! current %x %s",
			sysnum, current, current->name);
}
#endif
#endif

void __attribute__((naked)) ISR_svc()
{
	__asm__ __volatile__(
#ifdef CONFIG_DEBUG_SYSCALL
			"ldr	r0, =syscall_count	\n\t"
			"ldr	r1, [r0]		\n\t"
			"add	r1, #1			\n\t"
			"str	r1, [r0]		\n\t"
#endif
			"mrs	r12, psp		\n\t"
			/* get the sysnum requested */
			"ldr	r0, [r12]		\n\t"
			"teq	r0, %0			\n\t"
			"beq	sys_schedule		\n\t"
			/* #24 = r0-r3, lr and padding(4) */
			"sub	sp, sp, #24		\n\t"
			"str	lr, [sp, #20]		\n\t"
#ifndef CONFIG_SYSCALL_THREAD
#ifdef CONFIG_DEBUG_SYSCALL_NESTED
			"stmia	sp, {r0-r3}		\n\t"
			"ldr	r1, =current		\n\t"
			"ldr	r2, [r1]		\n\t"
			"ldr	r1, [r2, #4]		\n\t"
			"tst	r1, %2			\n\t"
			"it	ne			\n\t"
			"blne	syscall_nested		\n\t"
			"ldmia	sp, {r0-r3}		\n\t"
#endif
#endif
			/* save context that are not yet saved by hardware.
			 * you can remove this overhead if not using
			 * `syscall_delegate_atomic()` but using only
			 * CONFING_SYSCALL_THREAD. */
#ifndef CONFIG_SYSCALL_THREAD
			"sub	r1, r12, #32		\n\t"
			"msr	psp, r1			\n\t"
			"stmdb	r12, {r4-r11}		\n\t"
#endif
			/* if nr >= SYSCALL_NR */
			"cmp	r0, %1			\n\t"
			"it	ge			\n\t"
			/* then nr = 0 */
			"movge	r0, #0			\n\t"
			/* get handler address */
			"ldr	r3, =syscall_table	\n\t"
			"ldr	r3, [r3, r0, lsl #2]	\n\t"
			/* arguments in place */
			"ldr	r0, [r12, #4]		\n\t"
			"ldr	r1, [r12, #8]		\n\t"
			"ldr	r2, [r12, #12]		\n\t"
			"blx	r3			\n\t"
			"mrs	r12, psp		\n\t"
#ifndef CONFIG_SYSCALL_THREAD
			/* check if delegated task */
			"ldr	r1, =current		\n\t"
			"ldr	r2, [r1]		\n\t" /* read flags */
			"ldr	r1, [r2, #4]		\n\t"
			"ands	r1, %2			\n\t"
			/* restore saved context if not */
			"itt	eq			\n\t"
			"ldmiaeq	r12!, {r4-r11}		\n\t"
			"msreq	psp, r12		\n\t"
#endif
			/* store return value */
			"str	r0, [r12]		\n\t"
			"dsb				\n\t"
			"isb				\n\t"
			"ldr	lr, [sp, #20]		\n\t"
			"add	sp, sp, #24		\n\t"
			"bx	lr			\n\t"
			:: "I"(SYSCALL_SCHEDULE), "I"(SYSCALL_NR), "I"(TF_SYSCALL)
			: "r12", "memory");
}
#endif /* CONFIG_SYSCALL */

void nvic_set(int nirq, int on)
{
	reg_t *reg;
	unsigned int bit, base;

	bit  = nirq % 32;
	nirq = nirq / 32 * 4;
	base = on? NVIC_BASE : NVIC_BASE + 0x80;
	reg  = (reg_t *)(base + nirq);

	*reg = 1 << bit;

	dsb();
}

void nvic_set_pri(int nirq, int pri)
{
	reg_t *reg;
	unsigned int bit, val;

	bit  = nirq % 4 * 8;
	reg  = (reg_t *)((NVIC_BASE + 0x300) + (nirq / 4 * 4));

	do {
		val = __ldrex(reg);
		val &= ~(0xff << bit);
		val |= ((pri & 0xf) << 4) << bit;
	} while (__strex(val, reg));

	dsb();
}

void __attribute__((naked)) sys_schedule()
{
	SCB_ICSR |= 1 << 28; /* raising pendsv for scheduling */
	__ret();
}

#include <kernel/init.h>

void __init __irq_init()
{
	int i;
	for (i = 0; i < PRIMARY_IRQ_MAX; i++)
		secondary_irq_registers[i] = NULL;
}

#ifdef CONFIG_COMMON_IRQ_FRAMEWORK
void __init irq_init()
{
	int i;
	for (i = 0; i < PRIMARY_IRQ_MAX; i++)
		primary_irq_table[i] = ISR_null;

	__irq_init();
}
#endif
