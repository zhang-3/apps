#ifndef __BT_TEST_BT_LOG_H__
#define __BT_TEST_BT_LOG_H__

#define BT_LOG(fmt, ...) 							\
	do {										\		
		printk(fmt, ##__VA_ARGS__); 		\
	}while(0)

#endif
