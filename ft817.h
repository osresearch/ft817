#ifndef _ft817_h_
#define _ft817_h_

#include <inttypes.h>

#define FT817_CMD_TUNE		0x01

#define FT817_CMD_CTCSS		0x0B

#define FT817_CMD_CTCSS_DCS	0x0A
#define FT817_CTCSS_ENABLE	0x2A
#define FT817_CTCSS_DCS_ENABLE	0x4A
#define FT817_DCS_ENABLE	0x0A
#define FT817_CTCSS_DCS_DISABLE	0x8A

#define FT817_CMD_REPEATER_DIR	0x09
#define FT817_REPEATER_DIR_PLUS	0x49
#define FT817_REPEATER_DIR_MINUS	0x09
#define FT817_REPEATER_DIR_SIMPLEX	0x89

#define FT817_CMD_REPEATER_OFFSET	0xF9

extern int
ft817_open(
	const char * dev_name,
	int baud
);


extern ssize_t
ft817_write(
	int fd,
	uint8_t b1,
	uint8_t b2,
	uint8_t b3,
	uint8_t b4,
	uint8_t b5
);


extern ssize_t
ft817_read(
	int fd,
	uint8_t * const buf,
	size_t max_len,
	size_t min_len,
	int timeout_ms
);


#endif
