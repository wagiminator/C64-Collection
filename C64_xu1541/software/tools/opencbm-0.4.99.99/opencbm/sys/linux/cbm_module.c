/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2002 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 1997-2005 Joe Forster <sta(at)c64(dot)org> (Device Detection Code)
 *  Copyright 1997-2005 Wolfgang Moser (http://d81.de)   (Device Detection Code)
 *  Copyright 2000-2005 Markus Brenner                   (Parallel Burst Routines)
 *  Copyright 2000-2005 Pete Rittwage                    (Parallel Burst Routines)
 *  Copyright 2005      Tim Schürmann                    (Parallel Burst Routines)
 *  Copyright 2005-2006,2009 Spiro Trikaliotis           (Parallel Burst Routines)
 *  Copyright 2007-2009 Frédéric Brière                  (Adjustments on newer Linux kernels, abstraction from real hardware)
 *  Copyright 2009      Arnd Menge <arnd(at)jonnz(dot)de> (Parallel Burst Routines)
 *
 *
 */

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
#include <linux/config.h>
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
#include <linux/autoconf.h>
#else
#include <generated/autoconf.h>
#endif

#include <linux/module.h>

/*
 * Starting with 2.3.10, the IRQ and bi-directional bits are uncoupled from
 * the control byte.
 */
#ifndef DIRECT_PORT_ACCESS
# define FOUR_BIT_CONTROL
#endif

#ifdef DIRECT_PORT_ACCESS
# include <asm/io.h>
# include <linux/ioport.h>
#else
# include <linux/parport.h>
#endif

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>

#include <asm/uaccess.h>

#include "cbm_module.h"

/* forward references for parallel burst routines */
int cbm_parallel_burst_read_track(unsigned char *buffer);
int cbm_parallel_burst_read_track_var(unsigned char *buffer);
int cbm_parallel_burst_write_track(unsigned char *buffer, int length);
unsigned char cbm_parallel_burst_read(void);
int cbm_parallel_burst_write(unsigned char c);
int cbm_handshaked_read(int toggle);
int cbm_handshaked_write(char data, int toggle);

/* Defines needed for parallel burst end */

#ifdef DIRECT_PORT_ACCESS
unsigned int port = 0x378;	/* lpt port address             */
unsigned int irq = 7;		/* lpt irq line                 */
#else
unsigned int lp = 0;		/* parport number               */
#endif /* DIRECT_PORT_ACCESS */

int cable = -1;			/* <0 => autodetect             */
					/* =0 => non-inverted (XM1541)  */
					/* >0 => inverted     (XA1541)  */

#ifdef DIRECT_PORT_ACCESS
int reset = -1;			/* <0 => smart reset            */
#else /* =0 => no reset in cbm_init() */
int reset = 1;			/* >1 => force reset            */
#endif

int hold_clk = 1;		/* >0 => strict C64 behaviour   */
					/* =0 => release CLK when idle  */

#ifdef DIRECT_PORT_ACCESS
module_param(port, int, 0444);
MODULE_PARM_DESC(port, "IO portnumber of parallel port. (default 0x378)");

module_param(irq, int, 0444);
MODULE_PARM_DESC(irq, "IRQ number of parallel port. (default 7)");
#else
module_param(lp, int, 0444);
MODULE_PARM_DESC(lp, "parallel port number. (default 0)");
#endif /* DIRECT_PORT_ACCESS */

module_param(cable, int, 0444);
MODULE_PARM_DESC(cable,
		 "cable type: <0=autodetect, 0=non-inverted (XM1541), >0=inverted (XA1541). (default -1)");

module_param(reset, int, 0444);
MODULE_PARM_DESC(reset,
		 "reset at module load: <0=smart reset, 0=no reset, >0 force reset (default: "
#ifdef DIRECT_PORT_ACCESS
		 "-1"
#else /* =0 => no reset in cbm_init() */
		 "1"
#endif
		 ")");
module_param(hold_clk, int, 0444);
MODULE_PARM_DESC(hold_clk,
		 "0=release CLK when idle, >0=strict C64 behaviour. (default 1)");

MODULE_AUTHOR("Michael Klein");
MODULE_DESCRIPTION("Serial CBM bus driver module");
MODULE_LICENSE("GPL");

#define NAME      "cbm"
#define CBM_MINOR 177

MODULE_ALIAS("opencbm");

MODULE_ALIAS_MISCDEV(CBM_MINOR);

#define IEC_DATA   1
#define IEC_CLOCK  2
#define IEC_ATN    4
#define IEC_RESET  8

/* lpt output lines */
#define ATN_OUT    0x01
#define CLK_OUT    0x02
#ifndef OLD_C4L_CABLE
# define DATA_OUT  0x04
# define RESET     0x08
#else
# define DATA_OUT  0x08
# define RESET     0x04
#endif
#ifndef FOUR_BIT_CONTROL
#define LP_IRQ     0x10
#define LP_BIDIR   0x20
#endif

/* lpt input lines */
#define ATN_IN     0x10
#define CLK_IN     0x20
#define DATA_IN    0x40

static unsigned char out_bits, out_eor;
static int busy;
static int data_reverse;

#ifdef DIRECT_PORT_ACCESS
static int in_port;
static int out_port;
#endif

#ifndef DIRECT_PORT_ACCESS
static struct pardevice *cbm_device;
#endif

#define GET(line)        ((POLL() & line) == 0 ? 1 : 0)
#define SET(line)        (CTRL_WRITE(out_eor ^ (out_bits |= line)))
#define RELEASE(line)    (CTRL_WRITE(out_eor ^ (out_bits &= ~(line))))
#define SET_RELEASE(s,r) (CTRL_WRITE(out_eor ^ \
				(out_bits = (out_bits | (s)) & ~(r))))

#ifdef DIRECT_PORT_ACCESS
# define POLL()           (inb(in_port))
# define XP_READ()        (inb(port))
# define XP_WRITE(c)      (outb(c,port))
# define CTRL_READ()      (inb(out_port))
# define CTRL_WRITE(c)    (outb(c,out_port))
#else
# define POLL()           (parport_read_status(cbm_device->port))
# define XP_READ()        (parport_read_data(cbm_device->port))
# define XP_WRITE(c)      (parport_write_data(cbm_device->port,c))
# define CTRL_READ()      (parport_read_control(cbm_device->port))
# define CTRL_WRITE(c)    (parport_write_control(cbm_device->port,c))
#endif

#ifdef FOUR_BIT_CONTROL
# define set_data_forward() do { parport_data_forward(cbm_device->port); \
                                 data_reverse = 0; } while (0)
# define set_data_reverse() do { parport_data_reverse(cbm_device->port); \
                                 data_reverse = 1; } while (0)
#else
# define set_data_forward() do { RELEASE(LP_BIDIR); data_reverse = 0; } while (0)
# define set_data_reverse() do { SET(LP_BIDIR); data_reverse = 1; } while (0)
#endif

#ifdef DEBUG
# define DPRINTK(fmt,args...)     printk(fmt, ## args)
# define SHOW(str)                show(str)
#else
# define DPRINTK(fmt,args...)
# define SHOW(str)
#endif

#ifdef DEBUG_INTERRUPT
# define DPRINTK_INT(fmt,args...)     printk(fmt, ## args)
#else
# define DPRINTK_INT(fmt,args...)
#endif

static wait_queue_head_t cbm_wait_q;
volatile static int eoi;
volatile static int cbm_irq_count;

#if defined(DIRECT_PORT_ACCESS) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18))
# define SA_INTERRUPT IRQF_DISABLED
#endif

/*
 *  dump input lines
 */
#ifdef DEBUG
static void show(char *s)
{
	printk("%s: data=%d, clk=%d, atn=%d\n", s,
	       GET(DATA_IN), GET(CLK_IN), GET(ATN_IN));
}
#endif /* DEBUG */

static void timeout_us(int us)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule_timeout(HZ / 1000000 * us);
}

static int check_if_bus_free(void)
{
	int ret = 0;

	do {
		RELEASE(ATN_OUT | CLK_OUT | DATA_OUT | RESET);

		/* wait for the drive to have time to react */
		timeout_us(100);

		/* assert ATN */
		SET(ATN_OUT);

		/* now, wait for the drive to have time to react */
		timeout_us(100);

		/* if DATA is still unset, we have a problem. */
		if (!GET(DATA_IN))
			break;

		/* ok, at least one drive reacted. Now, test releasing ATN: */

		RELEASE(ATN_OUT);
		timeout_us(100);

		if (!GET(DATA_IN))
			ret = 1;

	} while (0);

	RELEASE(ATN_OUT | CLK_OUT | DATA_OUT | RESET);

	return ret;
}

static void wait_for_free_bus(void)
{
	int i = 1;

	while (1) {
		if (check_if_bus_free()) {
			printk("cbm: bus is free!\n");
			break;
        }

		++i;

		if (i == 1000) {
			printk("cbm: quitting because of timeout\n");
			break;
		}
		timeout_us(1000);
	}
}

static void do_reset(void)
{
	printk("cbm: resetting devices\n");
#ifdef FOUR_BIT_CONTROL
	RELEASE(DATA_OUT | ATN_OUT | CLK_OUT);
	parport_data_forward(cbm_device->port);
	parport_disable_irq(cbm_device->port);
#else
	RELEASE(DATA_OUT | ATN_OUT | CLK_OUT | LP_BIDIR | LP_IRQ);
#endif
	data_reverse = 0;
	SET(RESET);
	current->state = TASK_INTERRUPTIBLE;
	schedule_timeout(HZ / 10);	/* 100ms */
	RELEASE(RESET);

	printk("cbm: waiting for free bus...\n");
	wait_for_free_bus();
}

/*
 *  send byte
 */
static int send_byte(int b)
{
	int i, ack = 0;
	unsigned long flags;

	DPRINTK("send_byte %02x\n", b);

	local_irq_save(flags);
	for (i = 0; i < 8; i++) {
		udelay(70);
		if (!((b >> i) & 1))
			SET(DATA_OUT);
		RELEASE(CLK_OUT);
		udelay(20);
		SET_RELEASE(CLK_OUT, DATA_OUT);
	}
	local_irq_restore(flags);

	for (i = 0; (i < 20) && !(ack = GET(DATA_IN)); i++)
		udelay(100);

	DPRINTK("ack=%d\n", ack);

	return ack;
}

/*
 *  wait until listener is ready to receive
 */
static void wait_for_listener(void)
{
	DECLARE_WAITQUEUE(wait, current);

#ifdef FOUR_BIT_CONTROL
	parport_enable_irq(cbm_device->port);
#else
	SET(LP_IRQ);
#endif
	add_wait_queue(&cbm_wait_q, &wait);
	DPRINTK_INT("cbm: wait_for_listener() waits for interrupt\n");
	current->state = TASK_INTERRUPTIBLE;
	RELEASE(CLK_OUT);
	while (cbm_irq_count && !signal_pending(current))
		schedule();
	remove_wait_queue(&cbm_wait_q, &wait);
#ifdef FOUR_BIT_CONTROL
	parport_disable_irq(cbm_device->port);
#else
	RELEASE(LP_IRQ);
#endif
	DPRINTK_INT("cbm: wait_for_listener() got an interrupt\n");
}

static ssize_t cbm_read(struct file *f, char *buf, size_t count, loff_t *ppos)
{
	size_t received = 0;
	int i, b, bit;
	int ok = 0;
	unsigned long flags;

	DPRINTK("cbm_read: %zu bytes\n", count);

	if (eoi)
		return 0;

	do {
		i = 0;
		while (GET(CLK_IN)) {
			if (i >= 50) {
				current->state = TASK_INTERRUPTIBLE;
				schedule_timeout(HZ / 50);
				if (signal_pending(current))
					return -EINTR;
			} else {
				i++;
				udelay(20);
			}
		}
		local_irq_save(flags);
		RELEASE(DATA_OUT);
		for (i = 0; (i < 40) && !(ok = GET(CLK_IN)); i++)
			udelay(10);
		if (!ok) {
			/* device signals eoi */
			eoi = 1;
			SET(DATA_OUT);
			udelay(70);
			RELEASE(DATA_OUT);
		}
		for (i = 0; i < 100 && !(ok = GET(CLK_IN)); i++)
			udelay(20);
		for (bit = b = 0; (bit < 8) && ok; bit++) {
			for (i = 0; (i < 200) && !(ok = (GET(CLK_IN) == 0));
			     i++)
				udelay(10);
			if (ok) {
				b >>= 1;
				if (GET(DATA_IN) == 0)
					b |= 0x80;
				for (i = 0; i < 100 && !(ok = GET(CLK_IN)); i++)
					udelay(20);
			}
		}
		if (ok)
			SET(DATA_OUT);
		local_irq_restore(flags);
		if (ok) {
			received++;
			put_user((char)b, buf++);

			if (received % 256)
				udelay(50);
			else
				schedule();
		}

	} while (received < count && ok && !eoi);

	if (!ok) {
		printk("cbm_read: I/O error\n");
		return -EIO;
	}

	DPRINTK("received=%zu, count=%zu, ok=%d, eoi=%d\n",
		received, count, ok, eoi);

	return received;
}

static int cbm_raw_write(const char *buf, size_t cnt, int atn, int talk)
{
	unsigned char c;
	int i;
	int rv = 0;
	size_t sent = 0;
	unsigned long flags;

	eoi = cbm_irq_count = 0;

	DPRINTK("cbm_write: %zu bytes, atn=%d\n", cnt, atn);

	RELEASE(DATA_OUT);
	SET(CLK_OUT | (atn ? ATN_OUT : 0));

	for (i = 0; (i < 100) && !GET(DATA_IN); i++)
		udelay(10);

	if (!GET(DATA_IN)) {
		printk("cbm_write: no devices found\n");
		RELEASE(CLK_OUT | ATN_OUT);
		return -ENODEV;
	}

	current->state = TASK_INTERRUPTIBLE;
	schedule_timeout(HZ / 50);	/* 20ms */

	while (cnt > sent && rv == 0) {
		if (atn == 0)
			get_user(c, buf++);
		else
			c = *buf++;
		udelay(50);
		if (GET(DATA_IN)) {
			cbm_irq_count = ((sent == (cnt - 1))
					 && (atn == 0)) ? 2 : 1;
			wait_for_listener();

			if (signal_pending(current)) {
				rv = -EINTR;
			} else {
				if (send_byte(c)) {
					sent++;
					udelay(100);
				} else {
					printk("cbm_write: I/O error\n");
					rv = -EIO;
				}
			}
		} else {
			printk("cbm_write: device not present\n");
			rv = -ENODEV;
		}
	}
	DPRINTK("%zu bytes sent, rv=%d\n", sent, rv);

	if (talk && (rv == 0)) {
		local_irq_save(flags);
		SET(DATA_OUT);
		RELEASE(ATN_OUT);

		RELEASE(CLK_OUT);
		for (i = 0; (i < 100) && !GET(CLK_IN); i++)
			udelay(10);
		if (!GET(CLK_IN)) {
			printk("cbm_write: device not present\n");
			rv = -ENODEV;
		}

		local_irq_restore(flags);
	} else {
		RELEASE(ATN_OUT);
	}
	udelay(100);

	return (rv < 0) ? rv : (int)sent;
}

static ssize_t cbm_write(struct file *f, const char *buf, size_t cnt,
			 loff_t *ppos)
{
	return cbm_raw_write(buf, cnt, 0, 0);
}

static long cbm_unlocked_ioctl(struct file *f,
		     unsigned int cmd, unsigned long arg)
{
	/* linux parallel burst */
	PARBURST_RW_VALUE *user_val;
	PARBURST_RW_VALUE kernel_val;
	/* linux parallel burst end */

	unsigned char buf[2], c, talk, mask, state, i;
	int rv = 0;

	buf[0] = (arg >> 8) & 0x1f;	/* device */
	buf[1] = arg & 0x0f;	/* secondary address */

	switch (cmd) {
	case CBMCTRL_RESET:
		do_reset();
		return 0;

	case CBMCTRL_TALK:
	case CBMCTRL_LISTEN:
		talk = (cmd == CBMCTRL_TALK);
		buf[0] |= talk ? 0x40 : 0x20;
		buf[1] |= 0x60;
		rv = cbm_raw_write(buf, 2, 1, talk);
		return rv > 0 ? 0 : rv;

	case CBMCTRL_UNTALK:
	case CBMCTRL_UNLISTEN:
		buf[0] = (cmd == CBMCTRL_UNTALK) ? 0x5f : 0x3f;
		rv = cbm_raw_write(buf, 1, 1, 0);
		return rv > 0 ? 0 : rv;

	case CBMCTRL_OPEN:
	case CBMCTRL_CLOSE:
		buf[0] |= 0x20;
		buf[1] |= (cmd == CBMCTRL_OPEN) ? 0xf0 : 0xe0;
		rv = cbm_raw_write(buf, 2, 1, 0);
		if ((cmd == CBMCTRL_CLOSE) && (rv == 0)) {
			/* issue an unlisten */
			buf[0] = 0x3f;
			cbm_raw_write(buf, 1, 1, 0);
		}
		return rv > 0 ? 0 : rv;

	case CBMCTRL_GET_EOI:
		return eoi ? 1 : 0;

	case CBMCTRL_CLEAR_EOI:
		eoi = 0;
		return 0;

	case CBMCTRL_IEC_WAIT:
		switch (arg >> 8) {
		case IEC_DATA:
			mask = DATA_IN;
			break;
		case IEC_CLOCK:
			mask = CLK_IN;
			break;
		case IEC_ATN:
			mask = ATN_IN;
			break;
		default:
			return -EINVAL;
		}
		state = (arg & 0xff) ? mask : 0;
		i = 0;
		while ((POLL() & mask) == state) {
			if (i >= 20) {
				current->state = TASK_INTERRUPTIBLE;
				schedule_timeout(HZ / 50);	/* 20ms */
				if (signal_pending(current))
					return -EINTR;
			} else {
				i++;
				udelay(10);
			}
		}
		/* fall through */

	case CBMCTRL_IEC_POLL:
		c = POLL();
		if ((c & DATA_IN) == 0)
			rv |= IEC_DATA;
		if ((c & CLK_IN) == 0)
			rv |= IEC_CLOCK;
		if ((c & ATN_IN) == 0)
			rv |= IEC_ATN;
		return rv;

	case CBMCTRL_IEC_SET:
		if (arg & ~(IEC_DATA | IEC_CLOCK | IEC_ATN | IEC_RESET)) {
			/*
			 * there was some bit set that is not recognized, return
			 * with an error
			 */
			return -EINVAL;
		} else {
			if (arg & IEC_DATA)
				SET(DATA_OUT);
			if (arg & IEC_CLOCK)
				SET(CLK_OUT);
			if (arg & IEC_ATN)
				SET(ATN_OUT);
			if (arg & IEC_RESET)
				SET(RESET);
		}
		return 0;

	case CBMCTRL_IEC_RELEASE:
		if (arg & ~(IEC_DATA | IEC_CLOCK | IEC_ATN | IEC_RESET)) {
			/*
			 * there was some bit set that is not recognized, return
			 * with an error
			 */
			return -EINVAL;
		} else {
			if (arg & IEC_DATA)
				RELEASE(DATA_OUT);
			if (arg & IEC_CLOCK)
				RELEASE(CLK_OUT);
			if (arg & IEC_ATN)
				RELEASE(ATN_OUT);
			if (arg & IEC_RESET)
				RELEASE(RESET);
		}
		return 0;

	case CBMCTRL_IEC_SETRELEASE:
		{
			unsigned set = arg >> 8;
			unsigned release = arg & 0xFF;
			unsigned set_mask = 0;
			unsigned release_mask = 0;

			if ((set &
			     ~(IEC_DATA | IEC_CLOCK | IEC_ATN | IEC_RESET))
			    || (release &
				~(IEC_DATA | IEC_CLOCK | IEC_ATN | IEC_RESET)))
			{
				/*
				 * there was some bit set that is not recognized, return
				 * with an error
				 */
				return -EINVAL;
			} else {
				if (set & IEC_DATA)
					set_mask |= DATA_OUT;
				if (set & IEC_CLOCK)
					set_mask |= CLK_OUT;
				if (set & IEC_ATN)
					set_mask |= ATN_OUT;
				if (set & IEC_RESET)
					set_mask |= RESET;

				if (release & IEC_DATA)
					release_mask |= DATA_OUT;
				if (release & IEC_CLOCK)
					release_mask |= CLK_OUT;
				if (release & IEC_ATN)
					release_mask |= ATN_OUT;
				if (release & IEC_RESET)
					release_mask |= RESET;

				SET_RELEASE(set_mask, release_mask);
			}
		}
		return 0;

	case CBMCTRL_PP_READ:
		if (!data_reverse) {
			XP_WRITE(0xff);
			set_data_reverse();
		}
		return XP_READ();

	case CBMCTRL_PP_WRITE:
		if (data_reverse)
			set_data_forward();
		XP_WRITE(arg);
		return 0;

/* and now the parallel burst-routines */

	case CBMCTRL_PARBURST_READ:
		return cbm_parallel_burst_read();

	case CBMCTRL_PARBURST_WRITE:
		return cbm_parallel_burst_write(arg);

	case CBMCTRL_PARBURST_READ_TRACK:
		/* cast arg to structure pointer */
		user_val = (PARBURST_RW_VALUE *) arg;
		/* copy the data to the kernel: */
		if (copy_from_user(&kernel_val,	/* kernel buffer */
				   user_val,	/* user buffer */
				   sizeof(PARBURST_RW_VALUE)))
			return -EFAULT;
		/* verify if it's ok to write into the buffer */
		if (access_ok(VERIFY_WRITE, kernel_val.buffer, 0x2000) == 0)
			return -EFAULT;
		/* and do it: */
		return cbm_parallel_burst_read_track(kernel_val.buffer);

	case CBMCTRL_PARBURST_READ_TRACK_VAR:
		/* cast arg to structure pointer */
		user_val = (PARBURST_RW_VALUE *) arg;
		/* copy the data to the kernel: */
		if (copy_from_user(&kernel_val,	/* kernel buffer */
				   user_val,	/* user buffer */
				   sizeof(PARBURST_RW_VALUE)))
			return -EFAULT;
		/* verify if it's ok to write into the buffer */
		if (access_ok(VERIFY_WRITE, kernel_val.buffer, 0x2000) == 0)
			return -EFAULT;
		/* and do it: */
		return cbm_parallel_burst_read_track_var(kernel_val.buffer);

	case CBMCTRL_PARBURST_WRITE_TRACK:
		/* cast arg to structure pointer */
		user_val = (PARBURST_RW_VALUE *) arg;
		/* copy the data to the kernel: */
		if (copy_from_user(&kernel_val,	/* kernel buffer */
				   user_val,	/* user buffer */
				   sizeof(PARBURST_RW_VALUE)))
			return -EFAULT;
		/* verify if it's ok to read from the buffer */
		if (access_ok(VERIFY_READ, (void *)kernel_val.buffer, 0x2000) ==
		    0)
			return -EFAULT;
		/* and do it: */
		return cbm_parallel_burst_write_track(kernel_val.buffer,
						      kernel_val.length);
	}
	return -EINVAL;
}

static int cbm_open(struct inode *inode, struct file *f)
{
	if (busy)
		return -EBUSY;

	init_waitqueue_head(&cbm_wait_q);
	busy = 1;
	if (hold_clk)
		SET(CLK_OUT);

	return 0;
}

static int cbm_release(struct inode *inode, struct file *f)
{
	if (!hold_clk)
		RELEASE(CLK_OUT);
	busy = 0;
	return 0;
}

static irqreturn_t cbm_interrupt(int irq, void *dev_id)
{
	DPRINTK_INT("cbm: cbm_interrupt()\n");
	POLL();			/* acknowledge interrupt */

	if (cbm_irq_count == 0) {
		DPRINTK_INT("cbm: cbm_interrupt(): spurious interrupt\n");
		return IRQ_NONE;
	}
	else if (--cbm_irq_count == 0) {
		DPRINTK_INT("cbm: cbm_interrupt(): can continue\n");
		DPRINTK("cbm: cbm_interrupt(): continue to send (no EOI)\n");
		SET(CLK_OUT);
		wake_up_interruptible(&cbm_wait_q);
	}
	else {
		DPRINTK_INT("cbm: cbm_interrupt(): must still wait\n");
	}
	return IRQ_HANDLED;
}

#ifndef DIRECT_PORT_ACCESS
/* discard return value from cbm_interrupt */
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
static void cbm_interrupt_pp(int irq, void *dev_id, struct pt_regs *regs)
# elif (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
static void cbm_interrupt_pp(int irq, void *dev_id)
# else
static void cbm_interrupt_pp(void *dev_id)
# endif
{
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
	cbm_interrupt(irq, dev_id);
# else
	cbm_interrupt(cbm_device->port->irq, dev_id);
# endif
}
#endif /* DIRECT_PORT_ACCESS */

static const struct file_operations cbm_fops = {
	.owner		= THIS_MODULE,
	.read		= cbm_read,
	.write		= cbm_write,
	.unlocked_ioctl	= cbm_unlocked_ioctl,
	.open		= cbm_open,
	.release	= cbm_release,
};

static struct miscdevice cbm_dev = {
	.minor		= CBM_MINOR,
	.name		= NAME,
	.fops		= &cbm_fops,
};

void cbm_cleanup(void)
{
#ifdef DIRECT_PORT_ACCESS
	free_irq(irq, NULL);
	release_region(port, 3);
#else
	DPRINTK("releasing parallel port\n");
	parport_release(cbm_device);
	parport_unregister_device(cbm_device);
#endif
	misc_deregister(&cbm_dev);
}

int cbm_init(void)
{
	unsigned char in, out;
	char *msg;

#ifdef DIRECT_PORT_ACCESS
	if (check_region(port, 3)) {
		printk("cbm_init: port already in use\n");
		return -EBUSY;
	}
	if (request_irq(irq, cbm_interrupt, SA_INTERRUPT, NAME, NULL)) {
		printk("cbm_init: irq already in use\n");
		return -EBUSY;
	}
	request_region(port, 3, NAME);
#else
	struct parport *pp;
	pp = parport_find_number(lp);

	if (pp == NULL) {
		printk("cbm_init: non-existent port: %d\n", lp);
		return -ENODEV;
	}
	if (pp->irq <= 0) {
		printk("cbm_init: parallel port irq not configured: %d\n", lp);
		return -ENODEV;
	}

	cbm_device = parport_register_device(pp, NAME, NULL, NULL,
					     cbm_interrupt_pp,
					     PARPORT_DEV_EXCL, NULL);
	if (cbm_device == NULL) {
		printk("cbm_init: could not register with parallel port\n");
		return -EBUSY;
	}

	if (parport_claim(cbm_device)) {
		parport_unregister_device(cbm_device);
		printk("cbm_init: could not initialize\n");
		return -EBUSY;
	}
	DPRINTK("parallel port is mine now\n");
#endif
	misc_register(&cbm_dev);

#ifdef DIRECT_PORT_ACCESS
	in_port = port + 1;
	out_port = port + 2;
#endif

	if (cable < 0) {
		in = GET(ATN_IN);
		out = (CTRL_READ() & ATN_OUT) ? 1 : 0;
		cable = (in != out);
		msg = " (auto)";
	} else {
		msg = "";
	}

	out_eor = cable ? 0xcb : 0xc4;

	printk("cbm_init: using %s cable%s, irq %d\n",
	       cable ? "active (XA1541)" : "passive (XM1541)", msg,
#ifdef DIRECT_PORT_ACCESS
	       irq
#else
	       pp->irq
#endif
	    );

	cbm_irq_count = 0;

	out_bits = (CTRL_READ() ^ out_eor) &
	    (DATA_OUT | CLK_OUT | ATN_OUT | RESET);

	if ((reset < 0 && (out_bits & RESET)) || reset > 0)
		do_reset();

	busy = 0;

#ifdef FOUR_BIT_CONTROL
	RELEASE(DATA_OUT | ATN_OUT | CLK_OUT);
	parport_data_forward(pp);
	parport_disable_irq(pp);
#else
	RELEASE(RESET | DATA_OUT | ATN_OUT | LP_BIDIR | LP_IRQ);
#endif
	data_reverse = 0;

	current->state = TASK_INTERRUPTIBLE;
	schedule_timeout(HZ / 20);	/* 50ms */

#ifndef DIRECT_PORT_ACCESS
#endif

	return 0;
}

module_init(cbm_init);
module_exit(cbm_cleanup);

/* 
        And here are the functions, used by parallel burst 
        (they are all called by the ioctl-function)
*/

int cbm_parallel_burst_read_track(unsigned char *buffer)
{
	int i, byte;
	unsigned long flags;

	local_irq_save(flags);

	for (i = 0; i < 0x2000; i += 1) {
		byte = cbm_handshaked_read(i & 1);
		if (byte == -1) {
			local_irq_restore(flags);
			return 0;
		}
		buffer[i] = byte;
	}

	cbm_parallel_burst_read();
	local_irq_restore(flags);
	return 1;
}

int cbm_parallel_burst_read_track_var(unsigned char *buffer)
{
	int i, byte;
	unsigned long flags;

	local_irq_save(flags);

	for (i = 0; i < 0x2000; i += 1) {
		byte = cbm_handshaked_read(i & 1);
		if (byte == -1) {
			local_irq_restore(flags);
			return 0;
		}
		buffer[i] = byte;
		if (byte == 0x55)
			break;
	}

	cbm_parallel_burst_read();
	local_irq_restore(flags);
	return 1;
}

int cbm_parallel_burst_write_track(unsigned char *buffer, int length)
{
	int i;
	unsigned long flags;

	local_irq_save(flags);

	for (i = 0; i < length; i++) {
		if (cbm_handshaked_write(buffer[i], i & 1)) {
			/* timeout */
			local_irq_restore(flags);
			return 0;
		}
	}
	cbm_handshaked_write(0, i & 1);
	cbm_parallel_burst_read();
	local_irq_restore(flags);
	return 1;
}

unsigned char cbm_parallel_burst_read(void)
{
	int rv = 0;

	RELEASE(DATA_OUT | CLK_OUT);
	SET(ATN_OUT);
	udelay(20);		/* 200? */
	while (GET(DATA_IN)) ;
	/* linux rv = inportb(parport); */
	if (!data_reverse) {
		XP_WRITE(0xff);
		set_data_reverse();
	}
	rv = XP_READ();
	udelay(5);
	RELEASE(ATN_OUT);
	udelay(10);
	while (!GET(DATA_IN)) ;
	return rv;
}

int cbm_parallel_burst_write(unsigned char c)
{
	RELEASE(DATA_OUT | CLK_OUT);
	SET(ATN_OUT);
	udelay(20);
	while (GET(DATA_IN)) ;
	/* linux PARWRITE(); */
	if (data_reverse)
		set_data_forward();
	XP_WRITE(c);
	/* linux outportb(parport, arg); */
	udelay(5);
	RELEASE(ATN_OUT);
	udelay(20);
	while (!GET(DATA_IN)) ;
	/* linux PARREAD(); */
	if (!data_reverse) {
		XP_WRITE(0xff);
		set_data_reverse();
	}
	XP_READ();
	return 0;
}

#define TO_HANDSHAKED_READ  3300000
#define TO_HANDSHAKED_WRITE 3300000

int cbm_handshaked_read(int toggle)
{
	static int oldvalue = -1;
	int returnvalue = 0;
	int returnvalue2, returnvalue3, timeoutcount;
	int to = 0;

	RELEASE(DATA_IN);	/* not really needed? */

	/* linux
	   RELEASE(DATA_OUT);
	   udelay(2);
	   GET(DATA_IN); */

	if (!toggle) {
		while (GET(DATA_IN))
			if (to++ > TO_HANDSHAKED_READ)
				return -1;
	} else {
		while (!GET(DATA_IN))
			if (to++ > TO_HANDSHAKED_READ)
				return -1;
	}

	timeoutcount = 0;

	returnvalue3 = XP_READ();
	returnvalue2 = ~returnvalue3;	/* ensure to read once more */

	do {
		if (++timeoutcount >= 8) {
			printk
			    ("Triple-Debounce TIMEOUT: 0x%02x, 0x%02x, 0x%02x (%d, 0x%02x)\n",
			     returnvalue, returnvalue2, returnvalue3,
			     timeoutcount, oldvalue);
			break;
		}
		returnvalue = returnvalue2;
		returnvalue2 = returnvalue3;
		returnvalue3 = XP_READ();
	} while ((returnvalue != returnvalue2)
		 || (returnvalue != returnvalue3));

	oldvalue = returnvalue;

	return returnvalue;
}

int cbm_handshaked_write(char data, int toggle)
{
	int to = 0;

	RELEASE(CLK_IN);

	if (!toggle) {
		while (GET(DATA_IN))
			if (to++ > TO_HANDSHAKED_WRITE)
				return 1;
	} else {
		while (!GET(DATA_IN))
			if (to++ > TO_HANDSHAKED_WRITE)
				return 1;
	}
	/* linux outportb(parport, data); */
	if (data_reverse)
		set_data_forward();
	XP_WRITE(data);
	return 1;
}
