/*
 * output_stream.h
 *
 *  Created on: Nov 2, 2020
 *      Author: AGAPOV_ALEKSEY
 */

#ifndef OUTPUT_STREAM_H_
#define OUTPUT_STREAM_H_


#include <linux/slab.h>
#include <linux/types.h>



#define RECORD_SIZE 64


struct output_stream_record {
//	struct output_stream_record * first;
	struct output_stream_record * next;
	struct output_stream_record * before;
	char buffer[RECORD_SIZE];
};

/*
 * prt_message_stream - pointer on message list
 */
static struct output_stream_record * prt_message_stream_first = 0;
static struct output_stream_record * prt_message_stream_last  = 0;

static rwlock_t message_lock = __RW_LOCK_UNLOCKED (message_lock);
static unsigned long flags;

/*
 * create_message - CREATE NEW OUTPUT MESSAGE
 * @msg: how many bytes of memory are required.
 * */
static inline int create_message(const char * msg) {
	ssize_t len_msg;
	/* Prepare new record */
	struct output_stream_record *new_msg = kmalloc(sizeof(struct output_stream_record), GFP_KERNEL);	/* create new record */
	if (!new_msg) goto error_msg;																		/* error no memory */
	memset (new_msg, 0, sizeof(struct output_stream_record));											/* clear new record */

	write_lock_irqsave(&message_lock, flags);

	/* Add new record to stream */
	if (!prt_message_stream_first) {
		prt_message_stream_first  = new_msg;
		prt_message_stream_last = new_msg;
	} else {
//		new_msg->first = (last_msg->first)?last_msg->first:last_msg;									/* set poiner on fist element */
		new_msg->before= prt_message_stream_last;														/* set poiner to before */
		prt_message_stream_last->next = new_msg;														/* set poiner to next */
		prt_message_stream_last = new_msg;																/* set poiner to last */
	}

	/* Get message size */
	len_msg = strlen(msg);
	if (len_msg >= RECORD_SIZE) {len_msg = RECORD_SIZE-1;}

	/* Copy msg to stream record */
	strncpy(new_msg->buffer, msg, len_msg);
	new_msg->buffer[len_msg] = '\n';

	write_unlock_irqrestore(&message_lock, flags);

	printk(KERN_DEBUG "Add string:%s", new_msg->buffer );


	return 1;

error_msg:
	printk(KERN_ERR "Error get memory from string:%s\n", msg);
	return 0;

}



/*
 * read_message_data - READ MESSAGE DATA
 * @msg_buff: input buffer
 * @buffer_size: how many bytes of memory in buffer.
 * @read_pos: position first bytes of memory are required.
 * */
static inline ssize_t read_message_data(char *msg_buff, ssize_t buffer_size, loff_t read_pos) {
	struct output_stream_record *prt_message_stream_next = prt_message_stream_first;		/* Take first element message data*/
	ssize_t read_index = 0;																	/* Index first byte */
	ssize_t write_index = 0;																/* Index write position in output buffer */
	int is_found = 0;																		/* Flag found first byte */

	read_lock_irqsave (&message_lock, flags);

	while (prt_message_stream_next && write_index< buffer_size) {							/* Loop while not end record or output buffer is full */
		ssize_t record_size = strlen(prt_message_stream_next->buffer);						/* Get size data in record */
		if (is_found) {																		/* Read index found */
			/* Copy message to buffer */
			ssize_t copy_size = 															/* Calculate size copy data */
					(record_size < (buffer_size - write_index)) ? record_size : (buffer_size - write_index);
			memcpy(msg_buff + write_index, prt_message_stream_next->buffer, copy_size);
			write_index += copy_size;														/* increment write index for input buffer */
		} else {																			/* Found first byte in message */
			ssize_t offset = read_pos - read_index;
			if (offset < record_size) {
				ssize_t copy_size =															/* Calculate size copy data */
						((record_size - offset) < buffer_size) ? (record_size - offset) : buffer_size ;
				memcpy(msg_buff + write_index, prt_message_stream_next->buffer + offset, copy_size);
				write_index += copy_size;													/* increment write index for input buffer */
				read_index = 0;
				is_found = 1;																/* First byte find */
			} else {
				read_index += record_size;													/* increment read index for message data */
			}
		}
		printk(KERN_DEBUG "Read Message:%s", prt_message_stream_next->buffer);
		prt_message_stream_next = prt_message_stream_next->next;						/* Next record */
	}

	read_unlock_irqrestore(&message_lock, flags);

	return write_index;
}




/*
 * clear_messages_stream - CLEAR ALL MESSAGE DATA
 * */
static inline void clear_messages_stream(void) {
	while (prt_message_stream_last) {
		struct output_stream_record * before_element = prt_message_stream_last->before;
		printk(KERN_DEBUG "Clear Message:%s", prt_message_stream_last->buffer);
		kfree(prt_message_stream_last);
		prt_message_stream_last = before_element;
	}

	prt_message_stream_first = 0;
}


#endif /* OUTPUT_STREAM_H_ */
