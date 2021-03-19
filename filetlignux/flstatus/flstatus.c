/* See LICENSE file for copyright and license details.
 *
 * Simple updater for desktop environment status bars.
 *
 * Customised to run for filetlignux.
 * Continuously sends updated status text to stdout.
 * Pipe this to xrootname or something that applies
 * the status update for each new line.
 *
 * "{VPN} (1*2%|2456M) [100] FiletLignux 17:22"
 *
 * {VPN} => whether or not a tun0 network connection is active.
 * (1*2%|2456M) => multiplier*maxcpu|memuse
 *   maxcpu => the percent used of the highest load CPU.
 *   multiplier => how many times higher the total cpu use is over the maxcpu.
 *   memuse => memory use in Megabytes.
 * [100] => battery level.
 *
 * This file adheres to the opinion that editing C code
 * as if it is a config file, is a similar complexity level
 * to alternatives, and is a perfect learning challenge.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define INTERVAL 5
#define FILESCANLENGTH 5000
#define CPUS 64
#define SCANAFT(V, buf) ((scan = strstr(buf, V)) ? scan += strlen(V) : NULL)
#define MEM(V, buf) (SCANAFT(V, buf) ? strtol(scan, &dcp, 10) : 0)

static char filescanbuffer[FILESCANLENGTH + 1];

void
printstatus() {
	int f;

	/* {VPN} string */
	if ((f = open("/proc/net/dev", O_RDONLY)) >= 0
	&& read(f, filescanbuffer, sizeof(filescanbuffer)) >= 0
	&& strstr(filescanbuffer, "tun0")) {
		printf("{VPN} ");
	}
	if (f != -1)
		close(f);

	{ /* resource stats */
		int i, j;
		static long ctimelast[CPUS] = {0}, cidlelast[CPUS] = {0};
		long mem = 0, maxuse = 0;
		long delta, idle, ctime, use, load, totuse = 0;
		char *scan, *dcp;
		/* cpu utilisation */
		if ((f = open("/proc/stat", O_RDONLY)) >= 0
		&& read(f, filescanbuffer, sizeof(filescanbuffer)) >= 0) {
			SCANAFT("cpu", filescanbuffer);
			for (j = 0; SCANAFT("cpu", scan)
			&& (scan = strstr(scan, " ")) && j < CPUS; j++) {
				idle = 0;
				ctime = 0;
				for (i = 0; !strncmp(scan, " ", strlen(" ")); i++) {
					if (i == 3 || i == 4)
						idle += strtol(scan, &dcp, 10);
					ctime += strtol(scan, &scan, 10);
				}
				delta = ctime - ctimelast[j];
				load = delta - idle + cidlelast[j];
				if ((use = load*100/(delta?delta:1)) > maxuse)
					maxuse = use;
				totuse += use;
				ctimelast[j] = ctime;
				cidlelast[j] = idle;
			}
			printf("(%ld*%ld%%", totuse/(maxuse?maxuse:1), maxuse);
		}
		if (f != -1)
			close(f);
		/* memory used */
		if ((f = open("/proc/meminfo", O_RDONLY)) >= 0
		&& read(f, filescanbuffer, sizeof(filescanbuffer)) >= 0) {
			mem = MEM("MemTotal:", filescanbuffer);
			mem -= MEM("MemFree:", filescanbuffer);
			mem -= MEM("Buffers:", filescanbuffer);
			mem -= MEM("Cached:", filescanbuffer);
			mem -= MEM("SReclaimable:", filescanbuffer);
			printf("|%ldM) ", mem / 1024);
		}
		if (f != -1)
			close(f);
	}

	{ /* battery levels */
		char battery[] = "\0\0\0\0";
		if (((f = open("/sys/class/power_supply/BAT0/capacity", O_RDONLY)) >= 0
			|| (f = open("/sys/class/power_supply/BAT1/capacity", O_RDONLY)) >= 0)
		&& read(f, battery, sizeof(battery) - sizeof(char)) >=0 ) {
			if (strstr(battery, "\n"))
				*strstr(battery, "\n") = '\0';
			printf("[%s] ", battery);
		}
	if (f != -1)
		close(f);
	}

	printf("FiletLignux ");

	{ /* time string */
		time_t timenow = time(0);
		struct tm *localtm = localtime(&timenow);
		printf("%02d:%02d", localtm->tm_hour, localtm->tm_min);
	}

	printf("\n");
	fflush(stdout);
}

void
run()
{
	while (1) {
		printstatus();
		sleep(INTERVAL);
	}
}

int
main(int argc, char *argv[])
{
	run();
}
