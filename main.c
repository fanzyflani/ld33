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
#include "vec.c"
#include "gpu.c"
#include "joy.c"

int main(void);
void yield(void);

extern volatile uint8_t _BSS_START[];
extern volatile uint8_t _BSS_END[];

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
	0x3800007F,
	0x38007F00,
	0x387F0000,
	0x387F7F00,
	0x387F007F,
	0x38007F7F,
};

fixed player_x = 0;
fixed player_y = 0;
fixed player_ry = 0;

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
	vec4 v[8+8];
	for(z = 0; z < 2; z++)
	for(y = 0; y < 2; y++)
	for(x = 0; x < 2; x++)
	{
		v[z*4+y*2+x][0] = (x*2-1)*0x10000;
		v[z*4+y*2+x][1] = (y*2-1)*0x10000;
		v[z*4+y*2+x][2] = (z*2-1)*0x10000;
		v[z*4+y*2+x][3] = 0x10000;

		v[8+z*4+y*2+x][0] = (x*2-1)*0x10000;
		v[8+z*4+y*2+x][1] = (y*2-1)*0x10000;
		v[8+z*4+y*2+x][2] = (z*2-1)*0x10000;
		v[8+z*4+y*2+x][3] = 0x00000;
		vec4_normalize_3(&v[8+z*4+y*2+x]);
	}

	// Build matrix
	mat4 mat_cam;
	mat4 mat_icam;
	mat4_load_identity(&mat_cam);
	mat4_rotate_y(&mat_cam, rotang*0x30*5);
	mat4_rotate_x(&mat_cam, rotang*0x30*5);
	mat4_translate_imm3(&mat_cam, -player_x, 0x40000, -player_y + 0x40000);
	mat4_rotate_y(&mat_cam, player_ry);

	mat4_load_identity(&mat_icam);
	mat4_rotate_y(&mat_icam, -player_ry);

	// Apply matrix
	for(i = 0; i < 8+8; i++)
		mat4_apply_vec4(&v[i], &mat_cam);

	rotang++;

	// Project cube
	for(i = 0; i < 8; i++)
	{
		v[i][0] = v[i][0]*112/v[i][2];
		v[i][1] = v[i][1]*112/v[i][2];
		//sprintf(update_str_buf, "%08X %08X %08X", v[i][0], v[i][1], v[i][2]);
		//screen_print(16, 16+8*(i+5), 0x00007F, update_str_buf);
	}

	vec4 lite_dst = {0x2000, -0xA000, -0xA000, 0};
	vec4_normalize_3(&lite_dst);

	// Draw cube
	gpu_send_control_gp1(0x01000000);
	for(i = 0; i < 6; i++)
	{
		const uint16_t *l = cube_faces[i];

		// check facing + range
		for(j = 0; j < 4; j++)
		{
			if(v[l[j]][2] <= 0x0080)
				goto skip_vec;
			if(v[l[j]][0] <= -1023)
				goto skip_vec;
			if(v[l[j]][0] >=  1023)
				goto skip_vec;
			if(v[l[j]][1] <= -1023)
				goto skip_vec;
			if(v[l[j]][1] >=  1023)
				goto skip_vec;
		}

		// check direction
		int dx0 = (v[l[1]][0] - v[l[0]][0]);
		int dy0 = (v[l[1]][1] - v[l[0]][1]);
		int dx1 = (v[l[2]][0] - v[l[0]][0]);
		int dy1 = (v[l[2]][1] - v[l[0]][1]);
		if(dx0*dy1 - dy0*dx1 < 0)
			goto skip_vec;

		// draw
		int pr = (cube_cmds[i]>>16)&0xFF;
		int pg = (cube_cmds[i]>> 8)&0xFF;
		int pb = (cube_cmds[i]>> 0)&0xFF;

		for(j = 0; j < 4; j++)
		{
			vec4 *norm = &v[l[j]+8];
			fixed lite = vec4_dot_3(norm, &lite_dst);
			if(lite < 0) lite = 0;
			int r = (pr*lite)>>16;
			int g = (pg*lite)>>16;
			int b = (pb*lite)>>16;

			if(r > 0xFF) r = 0xFF;
			if(g > 0xFF) g = 0xFF;
			if(b > 0xFF) b = 0xFF;
			uint32_t col_merge = (r<<16)|(g<<8)|(b<<0);
			if(j == 0) gpu_send_control_gp0(col_merge | (cube_cmds[i]&0xFF000000));
			else gpu_send_data(col_merge);

			gpu_push_vertex(
				v[l[j]][0],
				v[l[j]][1]);
		}

		// FIXME should actually read status
		for(lag = 0; lag < 0x100; lag++) {}

		skip_vec: (void)0;
	}

	// Draw player
	/*
	gpu_send_control_gp0(0x285F5F3F);
	gpu_push_vertex(fixtoi(player_x)-7,fixtoi(player_y)-7);
	gpu_push_vertex(fixtoi(player_x)+7,fixtoi(player_y)-7);
	gpu_push_vertex(fixtoi(player_x)-7,fixtoi(player_y)+7);
	gpu_push_vertex(fixtoi(player_x)+7,fixtoi(player_y)+7);
	for(lag = 0; lag < 0x100; lag++) {}
	*/

	// Draw strings
	gpu_send_control_gp1(0x01000000);
	sprintf(update_str_buf, "ord=%02i row=%02i speed=%03i/%i"
		, s3mplayer.cord
		, s3mplayer.crow
		, s3mplayer.tempo
		, s3mplayer.speed
		);
	screen_print(16, 16+8*0, 0x7F7F7F, update_str_buf);
	screen_print(16, 16+8*1, 0x7F7F7F, s3mplayer.mod->name);

	/*
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
	*/

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

	vec4 fv = {0, 0, 4<<10, 0<<16};
	vec4 sv = {4<<10, 0, 0, 0<<16};
	mat4_apply_vec4(&fv, &mat_icam);
	mat4_apply_vec4(&sv, &mat_icam);

	// Apply input
	if((pad_data & PAD_L1) != 0)
		player_ry -= 1<<8;
	if((pad_data & PAD_R1) != 0)
		player_ry += 1<<8;
	if((pad_data & PAD_LEFT) != 0)
	{
		player_x -= sv[0];
		player_y -= sv[2];
	}
	if((pad_data & PAD_RIGHT) != 0)
	{
		player_x += sv[0];
		player_y += sv[2];
	}
	if((pad_data & PAD_UP) != 0)
	{
		player_x += fv[0];
		player_y += fv[2];
	}
	if((pad_data & PAD_DOWN) != 0)
	{
		player_x -= fv[0];
		player_y -= fv[2];
	}
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
	volatile int k;

	// Set up GPU
	gpu_init();

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

