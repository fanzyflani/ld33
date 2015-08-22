#include <string.h>
#include <stdint.h>
#include <math.h>

#include "psx.h"

void update_music_status(int ins, int ins_num);

#define TARGET_PSX
#define F3M_FREQ 44100
// PAL clock
#define F3M_BUFLEN 882
#define F3M_CHNS 2
#include "f3m.c"
player_s s3mplayer;

extern uint8_t fsys_rawcga[];
//extern mod_s fsys_s3m_test[];

#include "fix.c"
#include "gpu.c"
#include "joy.c"

int main(void);
void yield(void);

extern volatile uint8_t _BSS_START[];
extern volatile uint8_t _BSS_END[];

volatile int screen_buffer = 0;

volatile int vblank_triggered = 0;

void isr_handler(uint32_t cause, uint32_t sr, uint32_t epc)
{
	if((I_STAT & 0x0001) != 0)
	{
		vblank_triggered = 1;
		I_STAT = ~0x0001;
	}
}

extern const uint8_t int_handler_stub_start[];
extern const uint8_t int_handler_stub_end[];
void aaa_start(void)
{
	int i;

	// FIXME: make interrupts work
	I_MASK = 0x0000;
	/*
	I_MASK = 0x0000;
	memcpy((void *)0x80000080,
		int_handler_stub_start,
		int_handler_stub_end - int_handler_stub_start);
	
	I_MASK = 0x0001;
	I_STAT = ~0x0001;

	// Enable interrupts
	asm volatile (
		"\tmfc0 $2, $12\n"
		"\tori $2, $2, 0x0201\n"
		"\tmtc0 $2, $12\n"
	);
	*/

	//memset(_BSS_START, 0, _BSS_END - _BSS_START);
	//for(i = 0; i < _BSS_END - _BSS_START; i++) _BSS_START[i] = 0;

	//InitHeap(0x100000, 0x0F0000);
	/*
	// this code does work, no guarantees that it won't clobber things though
	asm volatile (
		"\tlui $4, 0x0010\n"
		"\tlui $5, 0x000F\n"
		"\tori $9, $zero, 0x39\n"
		"\taddiu $sp, $sp, 0x10\n"
		"\tori $2, $zero, 0x00A0\n"
		"\tjalr $2\n"
		"\tnop\n"
		"\taddiu $sp, $sp, -0x10\n"
	);
	*/

	main();

	for(;;)
		yield();
}

void yield(void)
{
	// TODO: halt 
}

const uint16_t cube_faces[6][4] = {
	{0, 1, 2, 3},
	{0, 4, 1, 5},
	{0, 2, 4, 6},
	{7, 3, 5, 1},
	{7, 6, 3, 2},
	{7, 5, 6, 4},
};
const uint32_t cube_cmds[6] = {
	0x2800007F,
	0x28007F00,
	0x287F0000,
	0x287F7F00,
	0x287F007F,
	0x28007F7F,
};

int rotang = 0;
static void update_frame(void)
{
	volatile int lag;
	int x, y, z, i, j;
	uint8_t update_str_buf[64];

	// Flip pages
	gpu_display_start(0, screen_buffer + 8);
	screen_buffer = (screen_buffer == 0 ? 240 : 0);
	gpu_draw_range(0, screen_buffer, 320, 240 + screen_buffer);
	gpu_draw_offset(0 + 160, screen_buffer + 120);

	// Clear screen
	//gpu_send_control_gp0((screen_buffer == 0 ? 0x027D7D7D : 0x024D4D4D));
	gpu_send_control_gp0(0x02001D00);
	gpu_send_data(0x00000000 + (screen_buffer<<16));
	gpu_send_data((320) | ((240)<<16));

	// Draw spinny cube
	// TODO: spin cube
	fixed v[8][3];
	for(z = 0; z < 2; z++)
	for(y = 0; y < 2; y++)
	for(x = 0; x < 2; x++)
	{
		v[z*4+y*2+x][0] = (x*2-1)*0x10000;
		v[z*4+y*2+x][1] = (y*2-1)*0x10000;
		v[z*4+y*2+x][2] = (z*2-1)*0x10000;
	}

	// Y-rotate cube
	for(i = 0; i < 8; i++)
	{
		fixed tx = v[i][0];
		fixed tz = v[i][2];

		fixed vs = ((fixed)(sintab[(rotang*3+0x00)&0xFF]))<<2;
		fixed vc = ((fixed)(sintab[(rotang*3+0x40)&0xFF]))<<2;

		v[i][0] = fixmul(tx, vc) - fixmul(tz, vs);
		v[i][2] = fixmul(tx, vs) + fixmul(tz, vc);
	}

	// X-rotate cube
	for(i = 0; i < 8; i++)
	{
		fixed ty = v[i][1];
		fixed tz = v[i][2];

		fixed vs = ((fixed)(sintab[(rotang*2+0x00)&0xFF]))<<2;
		fixed vc = ((fixed)(sintab[(rotang*2+0x40)&0xFF]))<<2;

		v[i][1] = fixmul(ty, vc) - fixmul(tz, vs);
		v[i][2] = fixmul(ty, vs) + fixmul(tz, vc);
	}

	rotang++;

	// Project cube
	for(i = 0; i < 8; i++)
	{
		v[i][2] += 0x30000;
		v[i][0] = (fixdiv(v[i][0], v[i][2])*200/2)>>16;
		v[i][1] = (fixdiv(v[i][1], v[i][2])*200/2)>>16;
		//sprintf(update_str_buf, "%08X %08X %08X", v[i][0], v[i][1], v[i][2]);
		//screen_print(16, 16+8*(i+5), 0x00007F, update_str_buf);
	}

	// Draw cube
	for(i = 0; i < 6; i++)
	{
		const uint16_t *l = cube_faces[i];

		// check facing
		for(j = 0; j < 4; j++)
		if(v[l[j]][2] <= 0x0080)
			goto skip_vec;

		// check direction
		int dx0 = (v[l[1]][0] - v[l[0]][0]);
		int dy0 = (v[l[1]][1] - v[l[0]][1]);
		int dx1 = (v[l[2]][0] - v[l[0]][0]);
		int dy1 = (v[l[2]][1] - v[l[0]][1]);
		if(dx0*dy1 - dy0*dx1 < 0)
			goto skip_vec;

		// draw
		gpu_send_control_gp1(0x01000000);
		gpu_send_control_gp0(cube_cmds[i]);
		for(j = 0; j < 4; j++)
		{
			gpu_push_vertex(
				v[l[j]][0],
				v[l[j]][1]);
		}
		for(lag = 0; lag < 0x300; lag++) {}

		skip_vec: (void)0;
	}

	/*
	gpu_send_control_gp1(0x01000000);
	gpu_send_control_gp0(0x2000007F);
	gpu_push_vertex(-50, -50);
	gpu_push_vertex(50, -50);
	gpu_push_vertex(0, 50);
	*/
	for(lag = 0; lag < 0x300; lag++) {}

	// Draw string
	gpu_send_control_gp1(0x01000000);
	sprintf(update_str_buf, "ord=%02i row=%02i speed=%03i/%i"
		, s3mplayer.cord
		, s3mplayer.crow
		, s3mplayer.tempo
		, s3mplayer.speed
		);
	screen_print(16, 16+8*0, 0x7F7F7F, update_str_buf);
	screen_print(16, 16+8*1, 0x7F7F7F, s3mplayer.mod->name);

	if(((pad_data&~pad_old_data) & PAD_RIGHT) != 0)
	{
		if(s3mplayer.cord > s3mplayer.mod->ord_num-2)
			s3mplayer.cord = s3mplayer.mod->ord_num-2;
		s3mplayer.crow=64;
		s3mplayer.ctick = s3mplayer.speed;
	}

	if(((pad_data&~pad_old_data) & PAD_LEFT) != 0)
	{
		s3mplayer.cord -= 2;
		if(s3mplayer.cord < -1)
			s3mplayer.cord = -1;
		s3mplayer.crow=64;
		s3mplayer.ctick = s3mplayer.speed;
	}

	const char *pad_id_str = "Unknown";
	switch(pad_id)
	{
		case 0x5A12:
			// I've heard this one's rather buggy
			pad_id_str = "Mouse - FUCK YOU";
			break;
		case 0x5A41:
			pad_id_str = "Digital Pad";
			break;
		case 0x5A53:
			pad_id_str = "Analogue Stick";
			break;
		case 0x5A73:
			pad_id_str = "Analogue Pad";
			break;
		case 0xFFFF:
			pad_id_str = "NOTHING";
			break;
	}

	sprintf(update_str_buf, "joypad=%04X (%04X: %s)", pad_data, pad_id, pad_id_str);
	screen_print(16, 16+8*3, 0x007F7F, update_str_buf);

	while((GP1 & 0x10000000) == 0)
		{}

	// Read joypad
	pad_old_data = pad_data;
	joy_readpad();
}

void update_music_status(int ins, int ins_num)
{
	int i;
	int scroll_len = ((320-20*2)*ins)/ins_num;

	// Clear screen
	gpu_send_control_gp0(0x024D0000);
	gpu_send_data(0x00000000 + (screen_buffer<<16));
	gpu_send_data((320) | ((240)<<16));

	// Draw status bar
	if(scroll_len < 320)
	{
		gpu_send_control_gp1(0x01000000);
		gpu_send_control_gp0(0x02000000);
		gpu_send_data(0x00000000 + 20 + scroll_len + ((screen_buffer+120-5)<<16));
		gpu_send_data((320-20*2-scroll_len) | ((10)<<16));
	}

	if(scroll_len > 0)
	{
		gpu_send_control_gp1(0x01000000);
		gpu_send_control_gp0(0x02007F00);
		gpu_send_data(0x00000000 + 20 + ((screen_buffer+120-5)<<16));
		gpu_send_data((scroll_len) | ((10)<<16));
	}

	// Draw string
	gpu_send_control_gp1(0x01000000);
	const static uint8_t test_str[] = "Converting samples...";
	for(i = 0; test_str[i] != '\x00'; i++)
	{
		uint32_t test_char = test_str[i];
		gpu_send_control_gp0(0xE1080208 | ((test_char>>5)));
		gpu_draw_texmask(8, 8, (test_char&31)<<3, 0);
		gpu_send_control_gp0(0x757F7F7F);
		gpu_push_vertex(i*8+40-160, 120-5-8-4-120);
		gpu_send_data(0x001C0000);
	}

	while((GP1 & 0x10000000) == 0)
		{}

	// Flip pages
	gpu_display_start(0, screen_buffer + 8);
	screen_buffer = (screen_buffer == 0 ? 240 : 0);
	gpu_draw_range(0, screen_buffer, 320, 240 + screen_buffer);
	gpu_draw_offset(0 + 160, screen_buffer + 120);
}

int main(void)
{
	int i;
	volatile int k = 0;
	int x, y, xc, yc;

	// Reset GPU 
	gpu_send_control_gp1(0x00000000);

	// Fix up DMA 
	gpu_send_control_gp1(0x04000001);
	gpu_send_control_gp1(0x01000000);

	// Set display area 
	//gpu_crtc_range(0x260, 0x88-(224/2), 320*8, 224); // NTSC
	gpu_crtc_range(0x260, 0xA3-(224/2), 320*8, 224); // PAL
	gpu_display_start(0, 8);

	// Set display mode 
	//gpu_send_control_gp1(0x08000001); // NTSC
	gpu_send_control_gp1(0x08000009); // PAL

	// Set draw mode 
	gpu_send_control_gp0(0xE6000000); gpu_send_control_gp0(0xE1000618); // Texpage
	gpu_draw_texmask(32, 32, 0, 0);
	gpu_draw_range(0, 0, 320, 240);

	// Copy CLUT to GPU
	gpu_send_control_gp1(0x01000000);
	gpu_send_control_gp0(0xA0000000);
	//gpu_send_data(0x01F70000);
	gpu_send_data(0x000001C0);
	gpu_send_data(0x00010002);
	//gpu_send_data(0x7FFF0001);
	gpu_send_data(0x7FFF0000);

	// Copy font to GPU
	gpu_send_control_gp1(0x01000000);
	gpu_send_control_gp0(0xA0000000);
	gpu_send_data(0x00000200);
	gpu_send_data(0x00080200);

	for(y = 0; y < 8; y++)
	for(x = 0; x < 256; x++)
	{
		uint32_t wdata = 0;
		for(i = 0; i < 8; i++, wdata <<= 4)
		if((fsys_rawcga[y+x*8]&(1<<i)) != 0)
			wdata |= 0x1;

		gpu_send_data(wdata);
	}

	// Enable display 
	gpu_send_control_gp1(0x03000000);

	// Clear screen 
	gpu_send_control_gp0(0x027D7D7D);
	gpu_send_data(0x00000000);
	gpu_send_data((320) | ((240)<<16));
	screen_buffer = 0;
	gpu_display_start(0, screen_buffer + 8);

	// Set up joypad
	JOY_CTRL = 0x0000;
	JOY_MODE = 0x000D;
	JOY_BAUD = 0x0088;

	// Prep module
	f3m_player_init(&s3mplayer, fsys_s3m_test);

	SPU_CNT = 0xC000;
	SPU_MVOL_L = 0x3FFF;
	SPU_MVOL_R = 0x3FFF;

	// Set up timer
	//TMR_n_TARGET(2) = 42300;//42336;
	// pcsxr tends to get the timing wrong
	/*
	TMR_n_TARGET(2) = 42336;
	TMR_n_COUNT(2) = 0x0000;
	TMR_n_MODE(2) = 0x0608;
	k = TMR_n_MODE(2);
	*/

	TMR_n_COUNT(1) = 0x0000;
	TMR_n_MODE(1) = 0x0703;
	k = TMR_n_MODE(1);

	for(;;)
	{
		//while(vblank_triggered == 0) {}
		while(TMR_n_COUNT(1) < 0x80) {}
		while(TMR_n_COUNT(1) >= 0x80) {}
		/*
		while((TMR_n_MODE(2) & 0x0800) == 0) {}
		while((TMR_n_MODE(2) & 0x0800) != 0) {}
		while((TMR_n_MODE(2) & 0x0800) == 0) {}
		while((TMR_n_MODE(2) & 0x0800) != 0) {}
		*/
		f3m_player_play(&s3mplayer, NULL, NULL);
		update_frame();
	}

	for(;;)
		yield();

	return 0;
}

