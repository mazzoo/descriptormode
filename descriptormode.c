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
} fdbar;

struct flash_component {
	uint32_t FLCOMP;
	uint32_t FLILL;

	/* information from above (redundantly kept here) */
	uint8_t  freq_read_id;
	uint8_t  freq_write;
	uint8_t  freq_fastread;
	uint8_t  fastread; /* support */
	uint8_t  freq_read;
	uint8_t  comp1_density;
	uint8_t  comp2_density;

	uint8_t  invalid_instr0;
	uint8_t  invalid_instr1;
	uint8_t  invalid_instr2;
	uint8_t  invalid_instr3;
} fcba;

void gather_FDBAR(void)
{
	fdbar.FLVALSIG = fm[0];
	fdbar.FLMAP0   = fm[1];
	fdbar.FLMAP1   = fm[2];
	fdbar.FLMAP2   = fm[3];

	fdbar.NR    = (fdbar.FLMAP0 >> 24) & 0x07;
	fdbar.FRBA  = (fdbar.FLMAP0 >> 12) & 0x00000ff0;
	fdbar.NC    = (fdbar.FLMAP0 >>  8) & 0x03;
	fdbar.FCBA  = (fdbar.FLMAP0 <<  4) & 0x00000ff0;

	fdbar.ISL   =  fdbar.FLMAP1 >> 24;
	fdbar.FISBA = (fdbar.FLMAP1 >> 12) & 0x00000ff0;
	fdbar.NM    = (fdbar.FLMAP1 >>  8) & 0x07;
	fdbar.FMBA  = (fdbar.FLMAP1 <<  4) & 0x00000ff0;

	fdbar.MSL   =  fdbar.FLMAP2 >>  8;
	fdbar.FMSBA = (fdbar.FLMAP2 <<  4) & 0x00000ff0;
}

void dump_FDBAR(void)
{
	printf("\n");
	printf("=== FDBAR ===\n");
	printf("FLVALSIG 0x%8.8x\n", fdbar.FLVALSIG);
	printf("FLMAP0   0x%8.8x\n", fdbar.FLMAP0  );
	printf("FLMAP1   0x%8.8x\n", fdbar.FLMAP1  );
	printf("FLMAP2   0x%8.8x\n", fdbar.FLMAP2  );
	printf("\n");
	printf("--- FDBAR details ---\n");
	printf("0x%2.2x        NR    Number Of Regions\n", fdbar.NR   );
	printf("0x%8.8x  FRBA  Flash Region Base Address\n", fdbar.FRBA );
	printf("0x%2.2x        NC    Number Of Components\n", fdbar.NC   );
	printf("0x%8.8x  FCBA  Flash Component Base Address\n", fdbar.FCBA );
	printf("\n");
	printf("0x%2.2x        ISL   ICH Strap Length\n", fdbar.ISL  );
	printf("0x%8.8x  FISBA Flash ICH Strap Base Address\n", fdbar.FISBA);
	printf("0x%2.2x        NM    Number Of Masters\n", fdbar.NM   );
	printf("0x%8.8x  FMBA  Flash Master Base Address\n", fdbar.FMBA );
	printf("\n");
	printf("0x%2.2x        MSL   MCH Strap Length\n", fdbar.MSL  );
	printf("0x%8.8x  FMSBA Flash MCH Strap Base Address\n", fdbar.FMSBA);
	printf("\n");
}

void gather_FCBA(void)
{
	fcba.FLCOMP = fm[(fdbar.FCBA >> 4) + 0];
	fcba.FLILL  = fm[(fdbar.FCBA >> 4) + 1];

	fcba.freq_read_id   = (fcba.FLCOMP >> 27) & 0x07;
	fcba.freq_write     = (fcba.FLCOMP >> 24) & 0x07;
	fcba.freq_fastread  = (fcba.FLCOMP >> 21) & 0x07;
	fcba.fastread       = (fcba.FLCOMP >> 20) & 0x01;
	fcba.freq_read      = (fcba.FLCOMP >> 17) & 0x07;
	fcba.comp1_density  = (fcba.FLCOMP >>  3) & 0x07;
	fcba.comp2_density  = (fcba.FLCOMP >>  0) & 0x07;
                            
	fcba.invalid_instr0 = (fcba.FLILL  >> 24) & 0xff;
	fcba.invalid_instr1 = (fcba.FLILL  >> 16) & 0xff;
	fcba.invalid_instr2 = (fcba.FLILL  >>  8) & 0xff;
	fcba.invalid_instr3 = (fcba.FLILL  >>  0) & 0xff;
}

void dump_FCBA(void)
{

	char * str_freq[8] = {
		"20 MHz",
		"33 MHz",
		"reserved/illegal",
		"reserved/illegal",
		"reserved/illegal",
		"reserved/illegal",
		"reserved/illegal",
		"reserved/illegal"
	};
	char * str_mem[8] = {
		"512kB",
		"1 MB",
		"2 MB",
		"4 MB",
		"8 MB",
		"16 MB",
		"undocumented/illegal",
		"reserved/illegal"
	};

	printf("\n");
	printf("=== FCBA ===\n");
	printf("FLCOMP   0x%8.8x\n", fcba.FLCOMP);
	printf("FLILL    0x%8.8x\n", fcba.FLILL );
	printf("\n");
	printf("--- FCBA details ---\n");
	printf("0x%2.2x        freq_read_id  %s\n",
		fcba.freq_read_id , str_freq[fcba.freq_read_id ]);
	printf("0x%2.2x        freq_write    %s\n",
		fcba.freq_write   , str_freq[fcba.freq_write   ]);
	printf("0x%2.2x        freq_fastread %s\n",
		fcba.freq_fastread, str_freq[fcba.freq_fastread]);
	printf("0x%2.2x        fastread      %s supported\n",
		fcba.fastread, fcba.fastread ? "" : "not ");
	printf("0x%2.2x        freq_read     %s\n",
		fcba.freq_read, str_freq[fcba.freq_read    ]);
	printf("0x%2.2x        comp 1 density %s\n",
		fcba.comp1_density, str_mem[fcba.comp1_density]);
	printf("0x%2.2x        comp 2 density %s\n",
		fcba.comp2_density, str_mem[fcba.comp2_density]);
	printf("\n");
	printf("0x%2.2x        invalid instr 0\n", fcba.invalid_instr0);
	printf("0x%2.2x        invalid instr 1\n", fcba.invalid_instr1);
	printf("0x%2.2x        invalid instr 2\n", fcba.invalid_instr2);
	printf("0x%2.2x        invalid instr 3\n", fcba.invalid_instr3);
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
	gather_FCBA();

	dump_FDBAR();
	dump_FCBA();
	return 0;
}

