/** \file
 * Tune the FT-817 to a specific frequency, mode, offset, CTS, etc.
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <getopt.h>
#include "util.h"
#include "ft817.h"


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


static struct option long_options[] =
{
	{"help", no_argument, 0,		'h'},
	{"verbose", no_argument, 0,		'v'},
	{"device", required_argument, 0,	'd' },
	{"baud", required_argument, 0,		'b' },
	{"frequency", required_argument, 0,	'f' },
	{"ctcss", required_argument, 0, 	't' },
	{"mode", required_argument, 0,		'm' },
	{"offset", required_argument, 0,	'o' },
	{0,0,0,0},
};

static void
usage(FILE * file)
{
	fprintf(file,
"Usage: tune [options]\n"
"Options:\n"
"    -v | --verbose                  Verbose serial port reads/writes\n"
"    -d | --device /dev/...          Serial device to use\n"
"    -b | --baud 4800                CAT baud rate\n"
"    -f | --frequency MHZ            Frequency to tune in MHz\n"
"    -t | --tone HZ                  CTCSS tone frequency in Hz\n"
"    -o | --offset MHz               Repeater offset and dir in MHz\n"
"    -m | --mode MODE                Transmit mode\n"
"\n"
"Modes: AM, FM, CW, CWR, WFM, DIG, PKT, LSB, USB\n"
);
}

int
main(
	int argc,
	char ** argv
)
{
	const char * dev_name = "/dev/ttyUSB0";
	int baud = 4800;

	const char * freq_str = NULL;
	const char * tone_str = NULL;
	const char * mode_str = NULL;
	const char * offset_str = NULL;

	int option_index = 0;

	while (1)
	{
		const int c = getopt_long(
			argc,
			argv,
			"h?vd:b:f:t:m:o:",
			long_options,
			&option_index
		);
		if (c == -1)
			break;
		switch (c)
		{
		case 'v': ft817_debug++; break;
		case 'd': dev_name = optarg; break;
		case 'b': baud = atoi(optarg); break;
		case 'f': freq_str = optarg; break;
		case 't': tone_str = optarg; break;
		case 'm': mode_str = optarg; break;
		case 'o': offset_str = optarg; break;
		case 'h': case '?':
			usage(stdout);
			return 0;
		default:
			usage(stderr);
			return -1;
		}
	}
		

	int fd = ft817_open(dev_name, baud);
	if (fd < 0)
		die("%s: Unable to open\n", dev_name);

	// retrieve the current status
	ft817_write(fd, 0x00, 0x00, 0x00, 0x00, FT817_CMD_READ_MODE);
	usleep(500e3);

	// \todo: Parse the frquency string
	// 144.01234+600@143.3
	// freq([+-][offset]?)?(@tone)?

	if (freq_str)
	{
		unsigned long freq = atof(freq_str) * 1.0e6;
		ft817_tune(fd, freq);
		usleep(500e3);
	}

	if (mode_str)
	{
		// \todo: parse mode
		ft817_set_mode(fd, FT817_MODE_FM);
		usleep(500e3);
	}

	if (tone_str)
	{
		unsigned ctcss_freq = atof(tone_str) * 10;
		ft817_ctcss_mode(fd, FT817_CTCSS_ENABLE);
		usleep(500e3);
		ft817_ctcss(fd, ctcss_freq);
		usleep(500e3);
	} else {
		ft817_ctcss_mode(fd, FT817_CTCSS_DCS_DISABLE);
		usleep(500e3);
	}

	if (offset_str)
	{
		int offset = atof(offset_str) * 1e6;
		if (offset == 0)
		{
			ft817_repeater_dir(fd, FT817_REPEATER_DIR_SIMPLEX);
		} else
		if (offset < 0)
		{
			ft817_repeater_dir(fd, FT817_REPEATER_DIR_MINUS);
			offset = -offset;
		} else {
			ft817_repeater_dir(fd, FT817_REPEATER_DIR_PLUS);
		}
		ft817_repeater_offset(fd, offset);
		usleep(500e3);
	}

	ft817_flush(fd);

	unsigned long freq;
	uint8_t mode;
	ft817_read_freq(fd, &freq, &mode);
	printf("Frequency: %.6f mode 0x%02x\n", freq / 1.0e6, mode);



	return 0;
}
