uint16_t pad_old_data = 0xFFFF;
uint16_t pad_data = 0x0000;
uint16_t pad_id = 0x0000;

static void joy_readpad(void)
{
	volatile int lag;

	JOY_CTRL = 0x0003;
	for(lag = 0; lag < 300; lag++) {}
	uint8_t v0 = joy_swap(0x01);
	uint8_t v1 = joy_swap(0x42);
	uint8_t v2 = joy_swap(0x00);
	uint8_t v3 = joy_swap(0x00);
	uint8_t v4 = joy_swap(0x00);
	JOY_CTRL = 0x0000;

	pad_id   = (((uint16_t)v2)<<8)|v1;
	pad_data = (((uint16_t)v4)<<8)|v3;
	pad_data = ~pad_data;
}
