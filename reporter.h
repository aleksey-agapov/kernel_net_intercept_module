/*
 * reporter.h
 *
 *  Created on: Nov 2, 2020
 *      Author: AGAPOV_ALEKSEY
 */

#ifndef REPORTER_H_
#define REPORTER_H_
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <uapi/linux/time.h>

union net_tcp { /* Union for convert IP to string format */
	__be32 ip_adr;
	unsigned char adr[sizeof(uint)];
};


/*
 * report_create_msg - CREATE REPORT STRING
 * @output_buf: report buffer
 * @ip_addr: how many bytes of memory in buffer.
 * @port: position first bytes of memory are required.
 * */
static inline int report_create_msg(char *output_buf,__be32 ip_addr,uint port)  {
	int ret;
    struct timeval time_value;
    struct tm date_time;
    union net_tcp ip_converter;

    do_gettimeofday(&time_value);
    time_to_tm(time_value.tv_sec, 0, &date_time);

    ip_converter.ip_adr = ip_addr;

	printk(KERN_DEBUG "%02u-%02u-%lu %02u:%02u:%02u.%lu %u.%u.%u.%u:%u\n",
			date_time.tm_mday,
			date_time.tm_mon+1,			/* Add tm_mon since 1. */
			date_time.tm_year + 1900,	/* Add years since 1900. */
			date_time.tm_hour+3,		/* Add time zone +3. */
			date_time.tm_min,
			date_time.tm_sec,
			time_value.tv_usec,

			ip_converter.adr[0],
			ip_converter.adr[1],
			ip_converter.adr[2],
			ip_converter.adr[3],
			port
	);

	ret = sprintf(output_buf,
			"%02u-%02u-%lu %02u:%02u:%02u.%lu %u.%u.%u.%u:%u",
						date_time.tm_mday,
						date_time.tm_mon+1,			/* Add tm_mon since 1. */
						date_time.tm_year + 1900,	/* Add years since 1900. */
						date_time.tm_hour+3,		/* Add time zone +3. */
						date_time.tm_min,
						date_time.tm_sec,
						time_value.tv_usec,

						ip_converter.adr[0],
						ip_converter.adr[1],
						ip_converter.adr[2],
						ip_converter.adr[3],
						port
	);

return ret;
}






#endif /* REPORTER_H_ */
