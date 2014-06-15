/** \file
 * Yaesu FT-817 serial interface.
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <sys/select.h>
#include "util.h"
#include "ft817.h"


int ft817_debug = 1;


ssize_t
ft817_write_buf(
	int fd,
	const void * buf,
	size_t len
)
{
	if (ft817_debug)
	{
		fprintf(stderr, "Sending: ");
		hexdump(stderr, buf, len);
	}

	return write_all(fd, buf, len);
}


ssize_t
ft817_write(
	int fd,
	uint8_t b1,
	uint8_t b2,
	uint8_t b3,
	uint8_t b4,
	uint8_t b5
)
{
	uint8_t buf[] = { b1, b2, b3, b4, b5 };
	return ft817_write_buf(fd, buf, sizeof(buf));
}


ssize_t
ft817_read(
	int fd,
	uint8_t * const buf,
	size_t max_len,
	size_t min_len,
	int timeout_ms
)
{
	size_t offset = 0;

	while (offset < min_len)
	{
		fd_set rd_fds;

		FD_ZERO(&rd_fds);
		FD_SET(fd, &rd_fds);

		struct timeval tv = {
			timeout_ms / 1000,
			(timeout_ms % 1000) * 1000, // scale to usec
		};

		int rc = select(fd+1, &rd_fds, NULL, NULL, &tv);
		if (rc < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return -1;
		}

		// a timeout occured
		if (rc == 0)
			break;

		if (!FD_ISSET(fd, &rd_fds))
		{
			// ???
			continue;
		}

		ssize_t rlen = read(fd, &buf[offset], max_len - offset);

		if (rlen < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return -1;
		}

		if (rlen == 0)
			break;

		if (ft817_debug)
		{
			fprintf(stderr, "Read %zu:", rlen);
			hexdump(stderr, &buf[offset], rlen);
		}

		offset += rlen;
	}

	return offset;
}

	
int
ft817_open(
	const char * const dev_name,
	int baud
)
{
	ssize_t rc;
	uint8_t buf[128];

	int fd = serial_open(dev_name, baud);
	// send a status read so that we know we are talking to the radio
	rc = ft817_write(fd, 0x00, 0x00, 0x00, 0x00, 0xa7);
	if (rc < 0)
		die("%s: write failed\n", dev_name);

	rc = ft817_read(fd, buf, sizeof(buf), 9, 100);
	if (rc < 0)
		die("%s: read failed\n", dev_name);

	if (rc < 9)
		die("%s; short read? %zu bytes < 9 expected\n", dev_name, rc);

	if (rc > 9)
		warn("%s: ignoring stale data: read %zu bytes\n", dev_name, rc);

	fprintf(stderr, "%s: ", dev_name);
	hexdump(stderr, buf, rc);

	return fd;
}


void
bcd_pack(
	uint8_t * buf,
	unsigned long val,
	int digits
)
{
	for (int i = 0 ; i < digits ; i+=2)
	{
		uint8_t byte = val % 100;
		val /= 100;
		uint8_t tens = byte / 10;
		uint8_t ones = byte % 10;

		buf[(digits - i)/ 2 - 1] = (tens << 4) | ones;
	}
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
ft817_set_mode(
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
		FT817_CMD_SET_MODE
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

