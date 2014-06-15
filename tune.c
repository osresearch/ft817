/** \file
 * Tune the FT-817 to a specific frequency, mode, offset, CTS, etc.
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include "util.h"
#include "ft817.h"


void
bcd_pack(
	uint8_t * buf,
	unsigned long val,
	int digits
)
{
	printf("pack %lu:", val);

	for (int i = 0 ; i < digits ; i+=2)
	{
		uint8_t byte = val % 100;
		val /= 100;
		uint8_t tens = byte / 10;
		uint8_t ones = byte % 10;

		buf[(digits - i)/ 2 - 1] = (tens << 4) | ones;
	}

	hexdump(stdout, buf, digits/2);

}


unsigned long
bcd_unpack(
	const uint8_t * buf,
	int digits
)
{
	unsigned long val = 0;
	for (int i = 0 ; i < digits ; i += 2)
	{
		uint8_t byte = buf[i/2];
		val = (val * 100) + (byte >> 4) * 10 + (byte & 0xF);
	}

	return val;
}


int
ft817_tune(
	int fd,
	unsigned long freq
)
{
	uint8_t buf[] = { 0, 0, 0, 0, FT817_CMD_TUNE };;
	bcd_pack(buf, freq/10, 8);

	return ft817_write_buf(fd, buf, sizeof(buf));
}


int
ft817_read_freq(
	int fd,
	unsigned long *freq,
	uint8_t * mode
)
{
	ft817_write(fd, 0x00, 0x00, 0x00, 0x00, FT817_CMD_READ_MODE);
	uint8_t buf[16];
	ssize_t rlen = ft817_read(fd, buf, sizeof(buf), 5, 1000);
	if (rlen != 5)
		warn("bad read %zd != 5\n", rlen);

	*freq = bcd_unpack(buf, 8) * 10;
	*mode = buf[4];

	return 0;
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
	uint8_t buf[] = { 0, 0, 0, 0, FT817_CMD_CTCSS };
	bcd_pack(buf, freq, 4);
	
	return ft817_write_buf(fd, buf, sizeof(buf));
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
	uint8_t buf[] = { 0, 0, 0, 0, FT817_CMD_REPEATER_OFFSET };
	bcd_pack(buf, freq, 4);

	return ft817_write_buf(fd, buf, sizeof(buf));
}


void
ft817_flush(
	int fd
)
{
	uint8_t buf[128];
	ssize_t rlen = ft817_read(fd, buf, sizeof(buf), sizeof(buf), 500);
/*
	if (rlen > 0)
		hexdump(stderr, buf, rlen);
*/
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

	// retrieve the current status
	ft817_write(fd, 0x00, 0x00, 0x00, 0x00, FT817_CMD_READ_MODE);

	unsigned long freq = atof("144.012345") * 1.0e6;
	ft817_tune(fd, freq);

	unsigned ctcss_freq = 141.3 * 10;
	ft817_ctcss_mode(fd, FT817_CTCSS_ENABLE);
	ft817_ctcss(fd, ctcss_freq);
	ft817_repeater_offset(fd, 5e6);
	ft817_repeater_dir(fd, FT817_REPEATER_DIR_MINUS);

	ft817_flush(fd);

	uint8_t mode;
	ft817_read_freq(fd, &freq, &mode);
	printf("Frequency: %.6f mode 0x%02x\n", freq / 1.0e6, mode);



	return 0;
}
