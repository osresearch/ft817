/** \file
 * Read the FT-817 eeprom and write it to stdout.
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include "util.h"
#include "ft817.h"


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

		ft817_write(fd, hi, lo, 0x00, 0x00, 0xBB);

		uint8_t buf[16];
		ssize_t rlen = ft817_read(fd, buf, sizeof(buf), 2, 100);
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
