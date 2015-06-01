#include <kernel/module.h>
#include <kernel/page.h>
#include <asm/usart.h>
#include <error.h>

#define BUF_SIZE	PAGE_SIZE

static struct fifo_t rxq, txq;

static DEFINE_SPINLOCK(rx_lock);
static DEFINE_SPINLOCK(tx_lock);

static int usart_close(unsigned int id)
{
	struct dev_t *dev = getdev(id);

	if (--dev->count == 0) {
		__usart_close();
		kfree(rxq.buf);
		kfree(txq.buf);
	}

	return dev->count;
}

static size_t usart_read(unsigned int id, void *buf, size_t size)
{
	int data;
	char *c = (char *)buf;
	unsigned int irqflag;

	spin_lock_irqsave(rx_lock, irqflag);
	data = fifo_get(&rxq, 1);
	spin_unlock_irqrestore(rx_lock, irqflag);

	if (data == -1)
		return 0;

	if (c)
		*c = data & 0xff;

	return 1;
}

static size_t usart_write_int(unsigned int id, void *buf, size_t size)
{
	char c = *(char *)buf;

	unsigned int irqflag;
	int err;

	spin_lock_irqsave(tx_lock, irqflag);
	err = fifo_put(&txq, c, 1);
	spin_unlock_irqrestore(tx_lock, irqflag);

	__usart_tx_irq_raise();

	if (err == -1) return 0;

	return 1;
}

static size_t usart_write_polling(unsigned int id, void *buf, size_t size)
{
	char c = *(char *)buf;
	unsigned int irqflag;

	spin_lock_irqsave(tx_lock, irqflag);
	__usart_putc(c);
	spin_unlock_irqrestore(tx_lock, irqflag);

	return 1;
}

static void isr_usart()
{
	int c;

	if (__usart_check_rx()) {
		c = __usart_getc();

		spin_lock(rx_lock);
		c = fifo_put(&rxq, c, 1);
		spin_unlock(rx_lock);

		if (c == -1) {
			/* overflow */
		}
	}

	if (__usart_check_tx()) {
		spin_lock(tx_lock);
		c = fifo_get(&txq, 1);
		spin_unlock(tx_lock);

		if (c == -1)
			__usart_tx_irq_reset();
		else
			__usart_putc(c);
	}
}

static int usart_open(unsigned int id, int mode)
{
	struct dev_t *dev = getdev(id);
	void *buf;

	if ((dev == NULL) || !(dev->count++)) { /* if it's the first access */
		int nr_irq = __usart_open(115200);

		if (nr_irq > 0) {
			if (mode & O_RDONLY) {
				if ((buf = kmalloc(BUF_SIZE)) == NULL)
					return -ERR_ALLOC;

				fifo_init(&rxq, buf, BUF_SIZE);
			}

			if (mode & O_WRONLY) {
				if ((buf = kmalloc(BUF_SIZE)) == NULL)
					return -ERR_ALLOC;

				fifo_init(&txq, buf, BUF_SIZE);
			}

			if (mode & O_NONBLOCK)
				dev->ops->write = usart_write_polling;
			else
				dev->ops->write = usart_write_int;

			register_isr(nr_irq, isr_usart);
		}
	}

	return dev->count;
}

int stdin, stdout, stderr;

int console_open(int mode)
{
	unsigned int id = 1;

	usart_open(id, mode);

	stdin = stdout = stderr = id;

	return id;
}

void console_putc(int c)
{
	__usart_putc(c);
}

/* well, think about how to deliver this kind of functions to user
static int kbhit()
{
	return (rxq.front != rxq.rear);
}

static void fflush()
{
	unsigned int irqflag;

	spin_lock_irqsave(rx_lock, irqflag);
	fifo_flush(&rxq);
	spin_unlock_irqrestore(rx_lock, irqflag);
}
*/

static struct dev_interface_t ops = {
	.open  = usart_open,
	.read  = usart_read,
	.write = usart_write_int,
	.close = usart_close,
};

#include <kernel/init.h>

static int __init console_init()
{
	unsigned int id = mkdev();

	register_dev(id, &ops, "console");

	usart_open(id, O_RDWR | O_NONBLOCK);

	stdin = stdout = stderr = id;

	return 0;
}
MODULE_INIT(console_init);
