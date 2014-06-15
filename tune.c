/** \file
 * Tune the FT-817 to a specific frequency, mode, offset, CTS, etc.
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <getopt.h>
#include "util.h"
#include "ft817.h"



static struct option long_options[] =
{
	{"help", no_argument, 0,		'h'},
	{"verbose", no_argument, 0,		'v'},
	{"device", required_argument, 0,	'd' },
	{"baud", required_argument, 0,		'b' },
	{"frequency", required_argument, 0,	'f' },
	{"tone", required_argument, 0, 		't' },
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
		if (ctcss_freq == 0)
		{
			ft817_ctcss_mode(fd, FT817_CTCSS_DCS_DISABLE);
			usleep(500e3);
		} else {
			ft817_ctcss_mode(fd, FT817_CTCSS_DCS_ENABLE);
			usleep(500e3);
			ft817_ctcss(fd, ctcss_freq);
			usleep(500e3);
		}
	}

	if (offset_str)
	{
		int offset = atof(offset_str) * 1.0e6;
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
		printf("repeater offset: %d\n", offset);
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
