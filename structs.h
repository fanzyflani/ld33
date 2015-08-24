typedef int fixed;
typedef fixed vec3[4];
typedef fixed vec4[4];
typedef vec4 mat4[4];

typedef enum jet_ai
{
	JAI_IDLE,
	JAI_PLAYER,

	JAI_LTURN7,
	JAI_RTURN7,
} jet_ai_e;

typedef struct jet
{
	vec3 pos;
	fixed rx, ry;
	fixed tilt_y;
	fixed tspd;

	jet_ai_e ai_mode;

	int16_t health;
	uint8_t team;
	uint8_t hit_flash;

	uint8_t crashed;
	uint8_t mgun_wait;
	uint8_t mgun_barrel;
	uint8_t mgun_fire;
} jet_s;

typedef struct shot
{
	vec4 pos;
	fixed rx, ry, vel;
	uint8_t team;
	uint8_t lifetime;
	uint8_t _pad0[2];
} shot_s;

