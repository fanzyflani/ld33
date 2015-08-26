// I'd normally do this in Python, but eh, might as well just keep using C
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	int i;

	FILE *tga = fopen("dat/piresy.tga", "rb");
	FILE *dat = fopen("dat/piresy.bin", "wb");

	for(i = 0; i < 18; i++)
		fgetc(tga);

	for(i = 0; i < 320*240; i++)
	{
		int b = (int)(uint8_t)fgetc(tga);
		int g = (int)(uint8_t)fgetc(tga);
		int r = (int)(uint8_t)fgetc(tga);
		int a = (int)(uint8_t)fgetc(tga);

		b >>= 3;
		g >>= 3;
		r >>= 3;

		int c = (b<<10)|(g<<5)|r;
		fputc((c&255), dat);
		fputc((c>>8), dat);
	}

	fclose(dat);
	fclose(tga);

	return 0;
}

