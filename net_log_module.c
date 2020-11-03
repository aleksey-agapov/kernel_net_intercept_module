/*
 ============================================================================
 Name        : net_log_module.c
 Author      : AGAPOV_ALEKSEY
 Version     : 0.01
 Copyright   : Your copyright notice
 Description : A simple example for Group-IB.
 ============================================================================
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>


#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>

#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>

#include <uapi/linux/time.h>



#include "output_stream.h"
#include "reporter.h"

#define MODULE_NAME "CONNECT_INTERCEPT_MODULE"
#define FILE_NAME "group-ib"

static ssize_t proc_file_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos);
static ssize_t proc_file_write(struct file *file, const char __user *ubuf,size_t count, loff_t *ppos);

static struct nf_hook_ops    *net_hook_struct;		/* Struct for registerintercept function for output IP */
static struct proc_dir_entry *proc_ent;				/* Struct proc file entry */
static struct file_operations file_ops =			/* Struct proc file operation */
{	.owner = THIS_MODULE,
	.read  = proc_file_read,
	.write = proc_file_write
};
/*
 * hook_func_intercept_connection - callback for intercept network packet
 * @skb: socket buffer
 * */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0)
unsigned int hook_func_intercept_connection(void *priv,
		struct sk_buff *skb,
		const struct nf_hook_state *state)
#else
unsigned int hook_func_intercept_connection(uint hooknum,
                  struct sk_buff *skb,
                  const struct net_device *in,
                  const struct net_device *out,
                  int (*okfn)(struct sk_buff *)  )
#endif
{

	struct iphdr *iph = ip_hdr(skb);				/* IP header */
	if (iph->protocol == IPPROTO_TCP) {				/* TCP protocol */
		struct tcphdr *tcph = tcp_hdr(skb);			/* TCP header */

		if (iph->version == 4) {					/* TCP V4 */
			if (tcph->syn) {						/* Request to connect */
				char *output_buf = kmalloc(RECORD_SIZE, GFP_KERNEL); /* create output buffer */
				if (output_buf) {
					printk(KERN_DEBUG "interface:%s TCP\n", skb->dev->name);
					if (report_create_msg(output_buf,iph->daddr,ntohs(tcph->dest))) {
						if (!create_message(output_buf)) {
							printk(KERN_ERR "Error add record to stream msg:%s\n", output_buf);
						}
					}
					kfree (output_buf);
				} else {
					printk( KERN_ERR "Error no memory for TCP buffer!\n");
				}
			}
		}
	}

	return NF_ACCEPT;
}

/*
 * proc_file_read - CALLBACK READ REPORT FILE
 * @ubuf: read buffer
 * @count: how many bytes in read buffer.
 * @ppos: position first bytes of memory are required.
 * */
static ssize_t proc_file_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos)
{
	ssize_t len = 0;
	char *output_buf = kmalloc(count, GFP_KERNEL); /* create output buffer */
	if (!output_buf) {
		goto no_memory_for_buffer;
	}
	memset(output_buf, 0, count); /* clear output buffer */

	printk(KERN_DEBUG "read handler buffer:%ld possition:%lld\n", count, *ppos);

	len = read_message_data(output_buf, count, *ppos);

	if (copy_to_user(ubuf, output_buf, len)) {
		goto error_copy_to_user;
	}
	kfree(output_buf);
	*ppos = len;
	return len;

	error_copy_to_user:
		kfree(output_buf);
		printk( KERN_ERR "Error copy from kernel to user memory!\n");
		goto error_func_exit;

	no_memory_for_buffer: /* error no memory */
		printk( KERN_ERR "Error no memory for read_buffer!\n");
		goto error_func_exit;

	error_func_exit:
		return -EFAULT;
}

/*
 * proc_file_write - CALLBACK WRITE REPORT FILE
 * @ubuf: write buffer
 * @count: how many bytes in write buffer.
 * @ppos: position first bytes of memory are required.
 * */
static ssize_t proc_file_write(struct file *file, const char __user *ubuf,size_t count, loff_t *ppos)
{
	printk( KERN_DEBUG "proc write handler\n");
	return -EFAULT;
}


/*
 * clear_operation - CLOSE ALL HOOKS AND PROC FILE
 */
static inline void clear_operation(void) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0)
	struct net *dev_network;
	for_each_net(dev_network)
		nf_unregister_net_hook(dev_network, net_hook_struct);
#else
	nf_unregister_hook(&net_hook_struct);
#endif
	kfree (net_hook_struct);
	proc_remove(proc_ent);
}




static int __init init_hooks_filter(void)
{
	int ret = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0)
	struct net *dev_network;
#endif
	printk(KERN_INFO "Start "MODULE_NAME"\n");

	/* Create proc file */
	proc_ent = proc_create(FILE_NAME,0664,NULL,&file_ops);
	if (!proc_ent) {goto error_create_proc_file;}
	printk(KERN_INFO "Create proc file Success\n");

	/* Create hook struct */
	net_hook_struct = kmalloc(sizeof(struct nf_hook_ops), GFP_KERNEL);
	if (!net_hook_struct) {goto no_memory_net_hook_struct; }
	net_hook_struct->hook = &hook_func_intercept_connection; /* Hook function */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,13,0)
	net_hook_struct.owner = THIS_MODULE;
#endif
	net_hook_struct->pf = PF_INET; /* Net protocol */
	net_hook_struct->hooknum = NF_INET_POST_ROUTING; /* POST_ROUTING */
	net_hook_struct->priority = NF_IP_PRI_FIRST; /* SET HIGH PRIORITY */
	/* Register hook */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0)
	for_each_net(dev_network)
		ret += nf_register_net_hook(dev_network, net_hook_struct);
#else
    ret = nf_register_hook(net_hook_struct);
#endif
	if (ret) {goto error_register_net_hook;}
	printk(KERN_INFO "Register net hook Success\n");
	return 0;


	error_register_net_hook:
		printk(KERN_ERR "Error register 'nf_register_net_hook'\n");
		kfree (net_hook_struct);
		clear_messages_stream();
		goto error_exit;

	no_memory_net_hook_struct:
		printk(KERN_ERR "No memory for 'net_hook_struct'\n");
		proc_remove(proc_ent);
		goto error_exit;

	error_create_proc_file:
		printk(KERN_ERR "Error create proc file\n");
		goto error_exit;

	error_exit:
		return -EFAULT;
}

static void __exit close_hooks_filter(void)
{
	clear_operation();
	clear_messages_stream();
	printk(KERN_INFO "Stop "MODULE_NAME"\n");
}

module_init(init_hooks_filter);
module_exit(close_hooks_filter);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AGAPOV_ALEKSEY");
MODULE_DESCRIPTION("A simple example for Group-IB.");
MODULE_VERSION("0.01");
