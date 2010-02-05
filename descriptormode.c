#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

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

struct flash_region {
	uint32_t FLREG0; /* Flash Descriptor */
	uint32_t FLREG1; /* BIOS */
	uint32_t FLREG2; /* ME   */
	uint32_t FLREG3; /* GbE  */

	/* information from above (redundantly kept here) */
	uint32_t reg0_limit;
	uint32_t reg0_base;
	uint32_t reg1_limit;
	uint32_t reg1_base;
	uint32_t reg2_limit;
	uint32_t reg2_base;
	uint32_t reg3_limit;
	uint32_t reg3_base;
} frba;

struct flash_master {
	uint32_t FLMSTR1;
	uint32_t FLMSTR2;
	uint32_t FLMSTR3;

	/* information from above (redundantly kept here) */
	uint8_t  BIOS_GbE_write;
	uint8_t  BIOS_ME_write;
	uint8_t  BIOS_BIOS_write;
	uint8_t  BIOS_descr_write;
	uint8_t  BIOS_GbE_read;
	uint8_t  BIOS_ME_read;
	uint8_t  BIOS_BIOS_read;
	uint8_t  BIOS_descr_read;

	uint8_t  ME_GbE_write;
	uint8_t  ME_ME_write;
	uint8_t  ME_BIOS_write;
	uint8_t  ME_descr_write;
	uint8_t  ME_GbE_read;
	uint8_t  ME_ME_read;
	uint8_t  ME_BIOS_read;
	uint8_t  ME_descr_read;

	uint8_t  GbE_GbE_write;
	uint8_t  GbE_ME_write;
	uint8_t  GbE_BIOS_write;
	uint8_t  GbE_descr_write;
	uint8_t  GbE_GbE_read;
	uint8_t  GbE_ME_read;
	uint8_t  GbE_BIOS_read;
	uint8_t  GbE_descr_read;
} fmba;

struct flash_strap {
	uint32_t STRP0;
	uint32_t STRP1;
	uint32_t FLUMAP1;
	uint32_t JID;
	uint32_t VSCC;

	/* information from above (redundantly kept here) */
	uint8_t  ME_smbus_addr2;
	uint8_t  ME_smbus_2_sel;
	uint8_t  SPI_CS1;
	uint8_t  GPIO12_SEL;
	uint8_t  GLAN_PCIE_SEL;
	uint8_t  BMC_mode;
	uint8_t  ME_smbus_addr1;
	uint8_t  TCO_mode;
	uint8_t  ME_disable_A;
	uint8_t  ME_disable_B;
} fisba;

struct flash_upper_map {
	uint32_t FLUMAP1;
	uint32_t JID[0xff];
	uint32_t VSCC[0xff];

	/* information from above (redundantly kept here) */
	uint32_t VTL;
	uint32_t VTBA;
} flumap;

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
}

void gather_FCBA(void)
{
	fcba.FLCOMP = fm[(fdbar.FCBA >> 2) + 0];
	fcba.FLILL  = fm[(fdbar.FCBA >> 2) + 1];

	fcba.freq_read_id   = (fcba.FLCOMP >> 27) & 0x07;
	fcba.freq_write     = (fcba.FLCOMP >> 24) & 0x07;
	fcba.freq_fastread  = (fcba.FLCOMP >> 21) & 0x07;
	fcba.fastread       = (fcba.FLCOMP >> 20) & 0x01;
	fcba.freq_read      = (fcba.FLCOMP >> 17) & 0x07;
	fcba.comp2_density  = (fcba.FLCOMP >>  3) & 0x07;
	fcba.comp1_density  = (fcba.FLCOMP >>  0) & 0x07;

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
	printf("0x%2.2x        freq_read_id   %s\n",
		fcba.freq_read_id , str_freq[fcba.freq_read_id ]);
	printf("0x%2.2x        freq_write     %s\n",
		fcba.freq_write   , str_freq[fcba.freq_write   ]);
	printf("0x%2.2x        freq_fastread  %s\n",
		fcba.freq_fastread, str_freq[fcba.freq_fastread]);
	printf("0x%2.2x        fastread       %ssupported\n",
		fcba.fastread, fcba.fastread ? "" : "not ");
	printf("0x%2.2x        freq_read      %s\n",
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
}

void gather_FRBA(void)
{
	frba.FLREG0 = fm[(fdbar.FRBA >> 2) + 0];
	frba.FLREG1 = fm[(fdbar.FRBA >> 2) + 1];
	frba.FLREG2 = fm[(fdbar.FRBA >> 2) + 2];
	frba.FLREG3 = fm[(fdbar.FRBA >> 2) + 3];

	frba.reg0_limit = (frba.FLREG0 >>  4) & 0x01fff000;
	frba.reg0_base  = (frba.FLREG0 << 12) & 0x01fff000;
	frba.reg1_limit = (frba.FLREG1 >>  4) & 0x01fff000;
	frba.reg1_base  = (frba.FLREG1 << 12) & 0x01fff000;
	frba.reg2_limit = (frba.FLREG2 >>  4) & 0x01fff000;
	frba.reg2_base  = (frba.FLREG2 << 12) & 0x01fff000;
	frba.reg3_limit = (frba.FLREG3 >>  4) & 0x01fff000;
	frba.reg3_base  = (frba.FLREG3 << 12) & 0x01fff000;
}

void dump_FRBA(void)
{
	printf("\n");
	printf("=== FRBA ===\n");
	printf("FLREG0   0x%8.8x\n", frba.FLREG0);
	printf("FLREG1   0x%8.8x\n", frba.FLREG1);
	printf("FLREG2   0x%8.8x\n", frba.FLREG2);
	printf("FLREG3   0x%8.8x\n", frba.FLREG3);
	printf("\n");
	printf("--- FRBA details ---\n");
	printf("0x%8.8x  region 0 limit (descr)\n", frba.reg0_limit);
	printf("0x%8.8x  region 0 base  (descr)\n", frba.reg0_base );
	printf("0x%8.8x  region 1 limit ( BIOS)\n", frba.reg1_limit);
	printf("0x%8.8x  region 1 base  ( BIOS)\n", frba.reg1_base );
	printf("0x%8.8x  region 2 limit ( ME  )\n", frba.reg2_limit);
	printf("0x%8.8x  region 2 base  ( ME  )\n", frba.reg2_base );
	printf("0x%8.8x  region 3 limit ( GbE )\n", frba.reg3_limit);
	printf("0x%8.8x  region 3 base  ( GbE )\n", frba.reg3_base );
}

void gather_FMBA(void)
{
	fmba.FLMSTR1 = fm[(fdbar.FMBA >> 2) + 0];
	fmba.FLMSTR2 = fm[(fdbar.FMBA >> 2) + 1];
	fmba.FLMSTR3 = fm[(fdbar.FMBA >> 2) + 2];

	fmba.BIOS_GbE_write   = (fmba.FLMSTR1 >> 27) & 0x01;
	fmba.BIOS_ME_write    = (fmba.FLMSTR1 >> 26) & 0x01;
	fmba.BIOS_BIOS_write  = (fmba.FLMSTR1 >> 25) & 0x01;
	fmba.BIOS_descr_write = (fmba.FLMSTR1 >> 24) & 0x01;
	fmba.BIOS_GbE_read    = (fmba.FLMSTR1 >> 19) & 0x01;
	fmba.BIOS_ME_read     = (fmba.FLMSTR1 >> 18) & 0x01;
	fmba.BIOS_BIOS_read   = (fmba.FLMSTR1 >> 17) & 0x01;
	fmba.BIOS_descr_read  = (fmba.FLMSTR1 >> 16) & 0x01;

	fmba.ME_GbE_write     = (fmba.FLMSTR2 >> 27) & 0x01;
	fmba.ME_ME_write      = (fmba.FLMSTR2 >> 26) & 0x01;
	fmba.ME_BIOS_write    = (fmba.FLMSTR2 >> 25) & 0x01;
	fmba.ME_descr_write   = (fmba.FLMSTR2 >> 24) & 0x01;
	fmba.ME_GbE_read      = (fmba.FLMSTR2 >> 19) & 0x01;
	fmba.ME_ME_read       = (fmba.FLMSTR2 >> 18) & 0x01;
	fmba.ME_BIOS_read     = (fmba.FLMSTR2 >> 17) & 0x01;
	fmba.ME_descr_read    = (fmba.FLMSTR2 >> 16) & 0x01;

	fmba.GbE_GbE_write    = (fmba.FLMSTR3 >> 27) & 0x01;
	fmba.GbE_ME_write     = (fmba.FLMSTR3 >> 26) & 0x01;
	fmba.GbE_BIOS_write   = (fmba.FLMSTR3 >> 25) & 0x01;
	fmba.GbE_descr_write  = (fmba.FLMSTR3 >> 24) & 0x01;
	fmba.GbE_GbE_read     = (fmba.FLMSTR3 >> 19) & 0x01;
	fmba.GbE_ME_read      = (fmba.FLMSTR3 >> 18) & 0x01;
	fmba.GbE_BIOS_read    = (fmba.FLMSTR3 >> 17) & 0x01;
	fmba.GbE_descr_read   = (fmba.FLMSTR3 >> 16) & 0x01;
}

void dump_FMBA(void)
{
	printf("\n");
	printf("=== FMBA ===\n");
	printf("FLMSTR1  0x%8.8x\n", fmba.FLMSTR1);
	printf("FLMSTR2  0x%8.8x\n", fmba.FLMSTR2);
	printf("FLMSTR3  0x%8.8x\n", fmba.FLMSTR3);

	printf("\n");
	printf("--- FMBA details ---\n");
	printf("BIOS can %s write GbE\n",   fmba.BIOS_GbE_write   ? "   " : "NOT");
	printf("BIOS can %s write ME\n",    fmba.BIOS_ME_write    ? "   " : "NOT");
	printf("BIOS can %s write BIOS\n",  fmba.BIOS_BIOS_write  ? "   " : "NOT");
	printf("BIOS can %s write descr\n", fmba.BIOS_descr_write ? "   " : "NOT");
	printf("BIOS can %s read  GbE\n",   fmba.BIOS_GbE_read    ? "   " : "NOT");
	printf("BIOS can %s read  ME\n",    fmba.BIOS_ME_read     ? "   " : "NOT");
	printf("BIOS can %s read  BIOS\n",  fmba.BIOS_BIOS_read   ? "   " : "NOT");
	printf("BIOS can %s read  descr\n", fmba.BIOS_descr_read  ? "   " : "NOT");
	printf("ME   can %s write GbE\n",   fmba.ME_GbE_write     ? "   " : "NOT");
	printf("ME   can %s write ME\n",    fmba.ME_ME_write      ? "   " : "NOT");
	printf("ME   can %s write BIOS\n",  fmba.ME_BIOS_write    ? "   " : "NOT");
	printf("ME   can %s write descr\n", fmba.ME_descr_write   ? "   " : "NOT");
	printf("ME   can %s read  GbE\n",   fmba.ME_GbE_read      ? "   " : "NOT");
	printf("ME   can %s read  ME\n",    fmba.ME_ME_read       ? "   " : "NOT");
	printf("ME   can %s read  BIOS\n",  fmba.ME_BIOS_read     ? "   " : "NOT");
	printf("ME   can %s read  descr\n", fmba.ME_descr_read    ? "   " : "NOT");
	printf("GbE  can %s write GbE\n",   fmba.GbE_GbE_write    ? "   " : "NOT");
	printf("GbE  can %s write ME\n",    fmba.GbE_ME_write     ? "   " : "NOT");
	printf("GbE  can %s write BIOS\n",  fmba.GbE_BIOS_write   ? "   " : "NOT");
	printf("GbE  can %s write descr\n", fmba.GbE_descr_write  ? "   " : "NOT");
	printf("GbE  can %s read  GbE\n",   fmba.GbE_GbE_read     ? "   " : "NOT");
	printf("GbE  can %s read  ME\n",    fmba.GbE_ME_read      ? "   " : "NOT");
	printf("GbE  can %s read  BIOS\n",  fmba.GbE_BIOS_read    ? "   " : "NOT");
	printf("GbE  can %s read  descr\n", fmba.GbE_descr_read   ? "   " : "NOT");
}

void gather_FISBA(void)
{
	fisba.STRP0 = fm[(fdbar.FISBA >> 2) + 0];
	fisba.STRP1 = fm[(fdbar.FMSBA >> 2) + 0];

	fisba.ME_smbus_addr2   = (fisba.STRP0 >> 25) & 0x7f;
	fisba.ME_smbus_2_sel   = (fisba.STRP0 >> 23) & 0x01;
	fisba.SPI_CS1          = (fisba.STRP0 >> 22) & 0x01;
	fisba.GPIO12_SEL       = (fisba.STRP0 >> 20) & 0x03;
	fisba.GLAN_PCIE_SEL    = (fisba.STRP0 >> 19) & 0x01;
	fisba.BMC_mode         = (fisba.STRP0 >> 15) & 0x01;
	fisba.ME_smbus_addr1   = (fisba.STRP0 >>  8) & 0x7f;
	fisba.TCO_mode         = (fisba.STRP0 >>  7) & 0x01;
	fisba.ME_disable_A     = (fisba.STRP0 >>  0) & 0x01;

	fisba.ME_disable_B     = (fisba.STRP1 >>  0) & 0x01;
}

void dump_FISBA(void)
{
	char * str_GPIO12[4] = {
		"GPIO12",
		"LAN PHY Power Control Function (Native Output)",
		" GLAN_DOCK# (Native Input)",
		"invalid configuration",
	};

	printf("\n");
	printf("=== FISBA ===\n");
	printf("STRP0    0x%8.8x\n", fisba.STRP0);

	printf("\n");
	printf("--- FISBA details ---\n");
	printf("ME SMBus addr2 0x%2.2x\n", fisba.ME_smbus_addr2);
	printf("ME SMBus addr1 0x%2.2x\n", fisba.ME_smbus_addr1);
	printf("ME SMBus Controller is connected to %s\n", fisba.ME_smbus_2_sel ? "SMLink pins" : "SMBus pins");
	printf("SPI CS1 is used for %s\n", fisba.SPI_CS1 ? "LAN PHY Power Control Function" : "SPI Chip Select");
	printf("GPIO12_SEL is used as %s\n", str_GPIO12[fisba.GPIO12_SEL]);
	printf("PCIe Port 6 is used for %s\n", fisba.GLAN_PCIE_SEL ? "integrated GLAN" : "PCI Express");
	printf("Intel AMT SMBus Controller 1 is connected to %s\n",   fisba.BMC_mode ? "SMLink" : "SMBus");
	printf("TCO slave is on %s. Intel AMT SMBus Controller 1 is %sabled\n",
		fisba.TCO_mode ? "SMBus" : "SMLink", fisba.TCO_mode ? "en" : "dis");
	printf("ME A is %sabled\n", fisba.ME_disable_A ? "dis" : "en");

	printf("\n");
	printf("=== FMSBA ===\n");
	printf("STRP1    0x%8.8x\n", fisba.STRP1);

	printf("\n");
	printf("--- FMSBA details ---\n");
	printf("ME B is %sabled\n", fisba.ME_disable_B ? "dis" : "en");
}

void gather_FLUMAP(void) {
	flumap.FLUMAP1 = fm[(0x0efc >> 2) + 0];

	flumap.VTL  = (flumap.FLUMAP1 >> 8) & 0x00ff;
	flumap.VTBA = (flumap.FLUMAP1 << 4) & 0x0ff0;

	if ((flumap.FLUMAP1 & 0x0000ffff) == 0x0000ffff)
		return;

	int i;
	for (i=0; i < flumap.VTL; i++)
	{
		flumap.JID[i]  = fm[(flumap.VTBA >> 2) + i * 2 + 0];
		flumap.VSCC[i] = fm[(flumap.VTBA >> 2) + i * 2 + 1];
	}
}

void dump_FLUMAP(void)
{
	printf("\n");
	printf("=== FLUMAP ===\n");
	printf("FLUMAP1  0x%8.8x\n", flumap.FLUMAP1);

	printf("\n");
	printf("--- FLUMAP details ---\n");
	printf("VTL   0x%8.8x\n", flumap.VTL);
	printf("VTBA  0x%8.8x\n", flumap.VTBA);
	printf("\n");
	int i;
	for (i=0; i < flumap.VTL; i++)
	{
		printf("  JID%d  = 0x%8.8x\n", i, flumap.JID[i] );
		printf("  VSCC%d = 0x%8.8x\n", i, flumap.VSCC[i]);
	}
}

void dump_file_descriptor(char * fn)
{
	char * n = malloc(strlen(fn) + 11);
	snprintf(n, strlen(fn) + 11, "%s.descr.bin", fn);
	printf("\n");
	printf("+++ dumping %s ... ", n);
	int f = open(n, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (f < 0)
	{
		printf("ERROR: couldn't open(%s): %s\n", n, strerror(errno));
		exit(1);
	}

	int ret;
	ret = write(f, &fm[frba.reg0_base >> 2], frba.reg0_limit);
	if (ret != frba.reg0_limit)
	{
		printf("FAILED.\n");
		exit(1);
	}

	printf("done.\n");

	close(f);
}

void dump_file_BIOS(char * fn)
{
	char * n = malloc(strlen(fn) + 10);
	snprintf(n, strlen(fn) + 10, "%s.BIOS.bin", fn);
	printf("\n");
	printf("+++ dumping %s ... ", n);
	int f = open(n, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (f < 0)
	{
		printf("ERROR: couldn't open(%s): %s\n", n, strerror(errno));
		exit(1);
	}

	int ret;
	ret = write(f, &fm[frba.reg1_base >> 2], frba.reg1_limit);
	if (ret != frba.reg1_limit)
	{
		printf("FAILED.\n");
		exit(1);
	}

	printf("done.\n");

	close(f);
}

void dump_file_ME(char * fn)
{
	char * n = malloc(strlen(fn) + 8);
	snprintf(n, strlen(fn) + 8, "%s.ME.bin", fn);
	printf("\n");
	printf("+++ dumping %s ... ", n);
	int f = open(n, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (f < 0)
	{
		printf("ERROR: couldn't open(%s): %s\n", n, strerror(errno));
		exit(1);
	}

	int ret;
	ret = write(f, &fm[frba.reg2_base >> 2], frba.reg2_limit);
	if (ret != frba.reg2_limit)
	{
		printf("FAILED.\n");
		exit(1);
	}

	printf("done.\n");

	close(f);
}

void dump_file_GbE(char * fn)
{
	char * n = malloc(strlen(fn) + 9);
	snprintf(n, strlen(fn) + 9, "%s.GbE.bin", fn);
	printf("\n");
	printf("+++ dumping %s ... ", n);
	int f = open(n, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (f < 0)
	{
		printf("ERROR: couldn't open(%s): %s\n", n, strerror(errno));
		exit(1);
	}

	int ret;
	ret = write(f, &fm[frba.reg3_base >> 2], frba.reg3_limit);
	if (ret != frba.reg3_limit)
	{
		printf("FAILED.\n");
		exit(1);
	}

	printf("done.\n");
	uint8_t * pMAC = (uint8_t *) &fm[frba.reg3_base >> 2];
	printf("the MAC-address might be: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n",
		pMAC[0],
		pMAC[1],
		pMAC[2],
		pMAC[3],
		pMAC[4],
		pMAC[5]
		);

	close(f);
}

void dump_files(char * f)
{
	printf("=== dumping section files ===\n");
	if (frba.reg0_limit)
		dump_file_descriptor(f);
	if (frba.reg1_limit)
		dump_file_BIOS(f);
	if (frba.reg2_limit)
		dump_file_ME(f);
	if (frba.reg3_limit)
		dump_file_GbE(f);
}

void usage(void)
{
	printf("no.\n");
	printf("\n");
	printf("usage: descriptormode bios_image.bin\n");
	printf("\n");
	printf("\twhere bios_image.bin isn't actually a BIOS image, but the SPI flash\n");
	printf("\tcontents of the SPI connected to your intel ICH southbridge.\n");
	printf("\tin case that image is really in descriptor mode (a.k.a soft-strap)\n");
	printf("\tdescriptormode will dump all ICH8 (and later) relevant information.\n");
	printf("\talso some sections are dumped to files, such as the real BIOS image\n");
	printf("\tas the CPU gets it or e.g. a GbE blob that is required to initialize\n");
	printf("\tyour ICH GbE.\n");
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
	gather_FRBA();
	gather_FMBA();
	gather_FISBA();
	gather_FLUMAP();

	dump_FDBAR();
	dump_FCBA();
	dump_FRBA();
	dump_FMBA();
	dump_FISBA();
	dump_FLUMAP();

	dump_files(argv[1]);

	return 0;
}

