#ifndef _ft817_h_
#define _ft817_h_

#include <inttypes.h>

#define FT817_CMD_TUNE		0x01

#define FT817_CMD_SET_MODE	0x07

#define FT817_CMD_READ_MODE	0x03
#define FT817_MODE_LSB		0x00
#define FT817_MODE_USB		0x01
#define FT817_MODE_CW		0x02
#define FT817_MODE_CWR		0x03
#define FT817_MODE_AM		0x04
#define FT817_MODE_WFM		0x06
#define FT817_MODE_FM		0x08
#define FT817_MODE_DIG		0x0A
#define FT817_MODE_PKT		0x0C

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


extern int ft817_debug;

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
ft817_write_buf(
	int fd,
	const void * buf,
	size_t len
);


extern ssize_t
ft817_read(
	int fd,
	uint8_t * const buf,
	size_t max_len,
	size_t min_len,
	int timeout_ms
);


extern void
ft817_flush(
	int fd
);


extern int
ft817_tune(
	int fd,
	unsigned long freq
);


extern int
ft817_read_freq(
	int fd,
	unsigned long *freq,
	uint8_t * mode
);


extern int
ft817_set_mode(
	int fd,
	uint8_t mode
);


extern int
ft817_ctcss_mode(
	int fd,
	uint8_t mode
);


/** Frequency is in 0.1 Hz */
extern int
ft817_ctcss(
	int fd,
	unsigned long freq
);


extern int
ft817_repeater_dir(
	int fd,
	uint8_t mode
);


extern int
ft817_repeater_offset(
	int fd,
	unsigned long freq
);


#endif
