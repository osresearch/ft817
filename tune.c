/** \file
 * Tune the FT-817 to a specific frequency, mode, offset, CTS, etc.
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include "util.h"
#include "ft817.h"


int
ft817_tune(
	int fd,
	unsigned long freq
)
{
	uint8_t freq_100m	= (freq / 100000000) % 10;
	uint8_t freq_10m	= (freq / 10000000) % 10;
	uint8_t freq_1m		= (freq / 1000000) % 10;
	uint8_t freq_100k	= (freq / 100000) % 10;
	uint8_t freq_10k	= (freq / 10000) % 10;
	uint8_t freq_1k		= (freq / 1000) % 10;
	uint8_t freq_100	= (freq / 100) % 10;
	uint8_t freq_10		= (freq / 10) % 10;

	return ft817_write(
		fd,
		(freq_100m << 4) | (freq_10m << 0),
		(freq_1m << 4) | (freq_100k << 0),
		(freq_10k << 4) | (freq_1k << 0),
		(freq_100 << 4) | (freq_10 << 0),
		FT817_CMD_TUNE
	);
}


int
ft817_ctcss_mode(
	int fd,
	uint8_t mode
)
{
	return ft817_write(
		fd,
		mode,
		0x00,
		0x00,
		0x00,
		FT817_CMD_CTCSS_DCS
	);
}


/** Frequency is in 0.1 Hz */
int
ft817_ctcss(
	int fd,
	unsigned long freq
)
{
	uint8_t freq_0_1 = (freq / 1) % 10;
	freq /= 10;

	uint8_t freq_100	= (freq / 100) % 10;
	uint8_t freq_10		= (freq / 10) % 10;
	uint8_t freq_1		= (freq / 1) % 10;
	
	return ft817_write(
		fd,
		(freq_100 << 4) | (freq_10 << 0),
		(freq_1 << 4) | (freq_0_1 << 0),
		0x00,
		0x00,
		FT817_CMD_CTCSS
	);
}


int
ft817_repeater_dir(
	int fd,
	uint8_t mode
)
{
	return ft817_write(
		fd,
		mode,
		0x00,
		0x00,
		0x00,
		FT817_CMD_REPEATER_DIR
	);
}


int
ft817_repeater_offset(
	int fd,
	unsigned long freq
)
{
	uint8_t freq_10m	= (freq / 10000000) % 10;
	uint8_t freq_1m		= (freq / 1000000) % 10;
	uint8_t freq_100k	= (freq / 100000) % 10;
	uint8_t freq_10k	= (freq / 10000) % 10;

	return ft817_write(
		fd,
		(freq_10m << 4) | (freq_1m << 0),
		(freq_100k << 4) | (freq_10k << 0),
		0x00,
		0x00,
		FT817_CMD_REPEATER_OFFSET
	);
}


int
main(
	int argc,
	char ** argv
)
{
	const char * dev_name = argc > 1 ? argv[1] : "/dev/ttyUSB0";
	int baud = 4800;

	int fd = ft817_open(dev_name, baud);
	if (fd < 0)
		die("%s: Unable to open\n", dev_name);

	unsigned long freq = atof("144.012345") * 1.0e6;
	ft817_tune(fd, freq);

	unsigned ctcss_freq = 141.3 * 10;
	ft817_ctcss_mode(fd, FT817_CTCSS_ENABLE);
	ft817_ctcss(fd, ctcss_freq);
	ft817_repeater_offset(fd, 600e3);
	ft817_repeater_dir(fd, FT817_REPEATER_DIR_PLUS);

	uint8_t buf[128];
	ssize_t rlen = ft817_read(fd, buf, sizeof(buf), sizeof(buf), 500);
	if (rlen > 0)
		hexdump(stderr, buf, rlen);

	return 0;
}
