#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include "util.h"

int debug = 0;


static ssize_t
write_command(
	int fd,
	uint8_t b1,
	uint8_t b2,
	uint8_t b3,
	uint8_t b4,
	uint8_t b5
)
{
	uint8_t buf[] = { b1, b2, b3, b4, b5 };
	if (debug)
	{
		fprintf(stderr, "Sending: ");
		hexdump(stderr, buf, sizeof(buf));
	}

	return write_all(fd, buf, sizeof(buf));
}


static ssize_t
read_response(
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

		if (debug)
		{
			fprintf(stderr, "Read %zu:", rlen);
			hexdump(stderr, &buf[offset], rlen);
		}

		offset += rlen;
	}

	return offset;
}

	


int
main(
	int argc,
	char ** argv
)
{
	const char * dev_name = argc > 1 ? argv[1] : "/dev/ttyUSB0";
	int baud = 4800;

	int fd = serial_open(dev_name, baud);
	if (fd < 0)
		die("%s: Unable to open\n", dev_name);

	ssize_t rlen;
	uint8_t buf[128];

	// send a status read so that we know we are talking to the radio
	write_command(fd, 0x00, 0x00, 0x00, 0x00, 0xa7);
	rlen = read_response(fd, buf, sizeof(buf), 9, 100);
	if (rlen < 0)
		die("%s: read failed\n", dev_name);
	if (rlen < 9)
		die("%s; short read? %zu bytes < 9 expected\n", dev_name, rlen);

	if (rlen > 9)
	{
		warn("%s: ignoring stale data: read %zu bytes\n", dev_name, rlen);
	}

	fprintf(stderr, "ID: ");
	hexdump(stderr, buf, rlen);

	// iterate through the read eeprom commands; this returns two bytes
	// at a time.  Experimentally, the eeprom only goes up to 0x1925;
	// there will be a duplicate byte for 0x1926.
	unsigned max_addr = 0x1926;
	uint8_t * eeprom = calloc(1, max_addr);
	
	for (unsigned addr = 0 ; addr < max_addr ; addr += 2)
	{

		uint8_t hi = (addr >> 8) & 0xFF;
		uint8_t lo = (addr >> 0) & 0xFF;

		if (lo == 0)
			fprintf(stderr, "reading 0x%04x\n", addr);

		write_command(fd, hi, lo, 0x00, 0x00, 0xBB);
		rlen = read_response(fd, buf, sizeof(buf), 2, 100);
		if (rlen <= 0)
		{
			warn("%s: Only read up to 0x%04x\n", dev_name, addr);
			break;
		}
		if (rlen != 2)
		{
			warn("%s: short read at 0x%04x\n", dev_name, addr);
		}

		eeprom[addr] = buf[0];
		eeprom[addr] = buf[1];
	}

	write_all(1, eeprom, max_addr);

	return 0;
}
