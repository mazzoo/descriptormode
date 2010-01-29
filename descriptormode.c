#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DESCRIPTOR_MODE_MAGIC 0x0ff0a55a

int f;         /* file descriptor to flash file */
int fs;        /* file size */
uint32_t * fm; /* mmap'd file */

struct flash_descriptor {
	uint32_t FLVALSIG; /* 0x00 */
	uint32_t FLMAP0;   /* 0x04 */
	uint32_t FLMAP1;   /* 0x08 */
	uint32_t FLMAP2;   /* 0x0c */

	/* information from above (redundantly kept here) */
	uint8_t  NR;   /* number of regions */
	uint32_t FRBA; /* */
	uint8_t  NC;   /* number of components */
	uint32_t FCBA; /* */

	uint8_t  ISL;
	uint32_t FISBA;
	uint8_t  NM;
	uint32_t FMBA;

	uint8_t  MSL;
	uint32_t FMSBA;
} fd;

void gather_FDBAR(void)
{
	fd.FLVALSIG = fm[0];
	fd.FLMAP0   = fm[1];
	fd.FLMAP1   = fm[2];
	fd.FLMAP2   = fm[3];

	fd.NR    = (fd.FLMAP0 >> 24) & 0x07;
	fd.FRBA  = (fd.FLMAP0 >> 12) & 0x00000ff0;
	fd.NC    = (fd.FLMAP0 >>  8) & 0x03;
	fd.FCBA  = (fd.FLMAP0 <<  4) & 0x00000ff0;

	fd.ISL   =  fd.FLMAP1 >> 24;
	fd.FISBA = (fd.FLMAP1 >> 12) & 0x00000ff0;
	fd.NM    = (fd.FLMAP1 >>  8) & 0x07;
	fd.FMBA  = (fd.FLMAP1 <<  4) & 0x00000ff0;

	fd.MSL   =  fd.FLMAP2 >>  8;
	fd.FMSBA = (fd.FLMAP2 <<  4) & 0x00000ff0;
}

void dump_FDBAR(void)
{
	printf("\n");
	printf("=== FDBAR ===\n");
	printf("FLVALSIG 0x%8.8x\n", fd.FLVALSIG);
	printf("FLMAP0   0x%8.8x\n", fd.FLMAP0  );
	printf("FLMAP1   0x%8.8x\n", fd.FLMAP1  );
	printf("FLMAP2   0x%8.8x\n", fd.FLMAP2  );
	printf("\n");
	printf("--- FDBAR details ---\n");
	printf("0x%2.2x        NR    Number Of Regions\n", fd.NR   );
	printf("0x%8.8x  FRBA  Flash Region Base Address\n", fd.FRBA );
	printf("0x%2.2x        NC    Number Of Components\n", fd.NC   );
	printf("0x%8.8x  FCBA  Flash Component Base Address\n", fd.FCBA );
	printf("\n");
	printf("0x%2.2x        ISL   ICH Strap Length\n", fd.ISL  );
	printf("0x%8.8x  FISBA Flash ICH Strap Base Address\n", fd.FISBA);
	printf("0x%2.2x        NM    Number Of Masters\n", fd.NM   );
	printf("0x%8.8x  FMBA  Flash Master Base Address\n", fd.FMBA );
	printf("\n");
	printf("0x%2.2x        MSL   MCH Strap Length\n", fd.MSL  );
	printf("0x%8.8x  FMSBA Flash MCH Strap Base Address\n", fd.FMSBA);
	printf("\n");
}

void usage(void)
{
	printf("no.\n");
	exit(1);
}

int main(int argc, char ** argv)
{
	if (argc < 2) usage();
	f = open(argv[1], O_RDONLY);
	if (f < 0) usage();

	fs = lseek(f, 0, SEEK_END);
	if (fs < 0) usage();

	fm = mmap(NULL, fs, PROT_READ, MAP_PRIVATE, f, 0);
	if (fm == (void *) -1) usage();

	if (fm[0] != DESCRIPTOR_MODE_MAGIC)
	{
		printf("not in descriptor mode\n");
		exit(0);
	}

	gather_FDBAR();

	dump_FDBAR();
	return 0;
}

