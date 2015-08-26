#include <string.h>
#include <stdint.h>
#include <math.h>

// TODO: NTSC support (would be useful for PC and Windows)

#ifdef TARGET_PC
// TODO: make this work on a PC or a Windows machine

#else

#include "psx.h"
// PCSXR tends to be a bit buggy with things even if you use FVP
int pcsxr_detected = 0;
void update_music_status(int ins, int ins_num);
#define TARGET_PSX
#endif

#define F3M_FREQ 44100
// PAL clock
#define F3M_BUFLEN 882
#define F3M_CHNS 2
#include "f3m.c"
player_s s3mplayer;

#include "structs.h"

jet_s *player;
extern uint8_t fsys_rawcga[];
extern uint32_t fsys_piresy[];
//extern mod_s fsys_s3m_test[];

extern uint8_t end[];
volatile void *_current_brk = (volatile void *)end;

#include "fix.c"
#include "vec.c"

#include "gpu.c"
#include "joy.c"

#include "hmap.c"
#include "mesh.c"
#include "dat/mesh.c"

#include "bldg.c"
#include "shot.c"
#include "jet.c"

#include "game.c"

void *sbrk(intptr_t diff)
{
	int i;

	volatile void *proposed_brk = _current_brk + diff + 0x20000; // extra alloc makes newlib behave
	if(proposed_brk < (void *)end)
		return NULL;
	if(proposed_brk >= (void *)0x801E0000)
		return NULL;
	
	if(_current_brk < proposed_brk)
		memset((void *)_current_brk, 0, proposed_brk - _current_brk);
	if(_current_brk > proposed_brk)
		memset((void *)proposed_brk, 0, _current_brk - proposed_brk);
	
	_current_brk = proposed_brk;
	return (void *)_current_brk;
}

int main(void);
void yield(void);

extern volatile uint8_t _BSS_START[];
extern volatile uint8_t _BSS_END[];

void isr_handler(uint32_t cause, uint32_t sr, uint32_t epc)
{
	(void)cause;
	(void)sr;
	(void)epc;
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

	memset((uint8_t *)_BSS_START, 0, _BSS_END - _BSS_START);
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

void update_music_status(int ins, int ins_num)
{
	volatile int lag;
	int i;
	const int scroll_blen = 320-24*2;
	int scroll_len = ((scroll_blen*(ins+1))/ins_num)&~0x0F;

	// Clear screen
	gpu_send_control_gp0(0x024D0000);
	gpu_send_data(0x00000000 + (screen_buffer<<16));
	gpu_send_data((320) | ((240)<<16));

	// Draw status bar
	if(scroll_len > 0)
	{
		gpu_send_control_gp1(0x01000000);
		gpu_send_control_gp0(0x02007F00);
		gpu_send_data(24 + ((screen_buffer+120-5)<<16));
		gpu_send_data((scroll_len) | ((10)<<16));
		for(lag = 0; lag < 0x100; lag++) {}
	}

	if(scroll_len < scroll_blen)
	{
		gpu_send_control_gp1(0x01000000);
		gpu_send_control_gp0(0x02000000);
		gpu_send_data(24 + scroll_len + ((screen_buffer+120-5)<<16));
		gpu_send_data((scroll_blen - scroll_len) | ((10)<<16));
		for(lag = 0; lag < 0x100; lag++) {}
	}

	// Draw string
	gpu_send_control_gp1(0x01000000);
	uint8_t buf[64];
	sprintf(buf, "Converting samples (%i/%i)", ins, ins_num);
	screen_print(40, 120-5-8-4, 0x00FFFFFF, buf);

	while((GP1 & 0x10000000) == 0)
		{}

	// Flip pages
	gpu_display_start(0, screen_buffer + 1);
	screen_buffer = (screen_buffer == 0 ? 240 : 0);
	gpu_draw_range(0, screen_buffer, 320, 240 + screen_buffer);
	gpu_draw_offset(0 + 160, screen_buffer + 120);
}

int main(void)
{
	volatile int k;
	int x, y, i, j;

	// Set up GPU
	gpu_init();

	// Set up joypad
	JOY_CTRL = 0x0000;
	JOY_MODE = 0x000D;
	JOY_BAUD = 0x0088;

	// Set up timer
	TMR_n_COUNT(1) = 0x0000;
	TMR_n_TARGET(1) = 400;
	TMR_n_MODE(1) = 0x0703;

	// PIRACY harms consumers
	gpu_send_control_gp1(0x01000000);
	gpu_send_control_gp0(0x02000000);
	gpu_push_vertex(0, 0);
	gpu_push_vertex(320, 240);
	gpu_send_control_gp0(0xA0000000);
	gpu_push_vertex(0, 8);
	gpu_push_vertex(320, 240);

	uint32_t colacc = 0;
	for(i = 0; i < 320*240/2; i++)
		gpu_send_data(fsys_piresy[i]);

	// Kill 6 seconds while we wait for the PS1 chime to stop
	for(i = 0; i < 50*1; i++) // PAL, fast test
	//for(i = 0; i < 50*6; i++) // PAL
	//for(i = 0; i < 60*6; i++) // NTSC
	{
		while(TMR_n_COUNT(1) < 0x80) {}
		while(TMR_n_COUNT(1) >= 0x80)
		{
			// work around a bug in PCSXR
			if(TMR_n_COUNT(1) >= 900)
			{
				TMR_n_COUNT(1) = 0;
				// considering that PCSXR doesn't even show this damn thing,
				// we should just skip it
				if(i >= 3)
				{
					i = 100000;
					pcsxr_detected = 1;
					break;
				}
			}
		}
	}

	// Prep game seeds
	jet_level = 1;
	randseed = 12342135; // keyboard mash
	for(i = 1; i < JET_MAX; i++)
	{
		jet_seeds[i] = randseed;
		for(j = 0; j < 10000; j++)
		{
			randseed *= 1103515245U;
			randseed += 12345U;
			randseed &= 0x7FFFFFFFU;
		}
	}

	// Prep module
	f3m_player_init(&s3mplayer, fsys_s3m_test);

	SPU_CNT = 0xC000;
	SPU_MVOL_L = 0x3FFF;
	SPU_MVOL_R = 0x3FFF;
	SPU_KOFF = 0x00FFFFFF;
	for(i = 0; i < 24; i++)
	{
		SPU_n_MVOL_L(i) = 0;
		SPU_n_MVOL_R(i) = 0;
		SPU_n_ADSR(i) = 0x9FC083FF;
	}
	SPU_KOFF = 0x00FFFFFF;

	for(;;)
	{
		/*
		SPU_CNT = 0xC000;
		SPU_MVOL_L = 0x3FFF;
		SPU_MVOL_R = 0x3FFF;
		SPU_KOFF = 0x00FFFFFF;
		for(i = 0; i < 24; i++)
		{
			SPU_n_MVOL_L(i) = 0;
			SPU_n_MVOL_R(i) = 0;
			SPU_n_ADSR(i) = 0x9FC083FF;
		}
		SPU_KOFF = 0x00FFFFFF;
		s3mplayer.cord = -1;
		s3mplayer.crow = 64;
		s3mplayer.ctick = s3mplayer.speed;
		*/

		// Set up timer
		//TMR_n_TARGET(2) = 42300;//42336;
		// pcsxr tends to get the timing wrong
		/*
		TMR_n_TARGET(2) = 42336;
		TMR_n_COUNT(2) = 0x0000;
		TMR_n_MODE(2) = 0x0608;
		k = TMR_n_MODE(2);
		*/

		game_init();

		TMR_n_COUNT(1) = 0;
		for(;;)
		{
			const uint32_t tmr_gap = 14;
			//while(vblank_triggered == 0) {}
			while(TMR_n_COUNT(1) < tmr_gap) {}
			while(TMR_n_COUNT(1) >= tmr_gap)
			{
				// work around a bug in PCSXR
				if(TMR_n_COUNT(1) >= (pcsxr_detected ? 314 : 350))
					TMR_n_COUNT(1) = 0;
			}
			/*
			while((TMR_n_MODE(2) & 0x0800) == 0) {}
			while((TMR_n_MODE(2) & 0x0800) != 0) {}
			while((TMR_n_MODE(2) & 0x0800) == 0) {}
			while((TMR_n_MODE(2) & 0x0800) != 0) {}
			*/
			game_update_frame();
			if(((pad_old_data & ~pad_data) & PAD_START) != 0)
				break;
			f3m_player_play(&s3mplayer, NULL, NULL);
		}
	}

	for(;;)
		yield();

	return 0;
}

