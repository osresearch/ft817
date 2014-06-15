/** \file
 * Yaesu FT-817 serial interface.
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <sys/select.h>
#include "util.h"


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
