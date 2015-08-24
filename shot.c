typedef struct shot
{
	vec4 pos;
	fixed rx, ry, vel;
	uint8_t team;
	uint8_t lifetime;
	uint8_t _pad0[2];
} shot_s;

#define SHOT_MAX 128
shot_s shot_list[SHOT_MAX];
int shot_head = 0;
int shot_tail = 0;

static void shot_fire(fixed rx, fixed ry, vec4 *pos, fixed vel, uint8_t team)
{
	// Get new shot
	shot_s *sh = &shot_list[shot_head];

	// Advance shot head pointer
	shot_head++;
	if(shot_head >= SHOT_MAX)
		shot_head = 0;
	
	// Lop off oldest shot if it hasn't disappeared yet
	if(shot_tail == shot_head)
	{
		shot_tail++;
		if(shot_tail >= SHOT_MAX)
			shot_tail = 0;
	}

	// Fill in fields
	vec4_copy(&sh->pos, pos);
	sh->rx = rx;
	sh->ry = ry;
	sh->vel = vel;
	sh->team = team;
	sh->lifetime = 25; // 1/2 second
}

static void shot_update_one(shot_s *sh)
{
	// Skip dead bullets
	if(sh->lifetime == 0)
		return;

	// Update lifetime
	sh->lifetime--;

	// Move forward
	mat4 smat;
	mat4_load_identity(&smat);
	mat4_rotate_x(&smat, -sh->rx);
	mat4_rotate_y(&smat, -sh->ry);
	sh->pos[0] += fixmulf(smat[2][0], sh->vel);
	sh->pos[1] += fixmulf(smat[2][1], sh->vel);
	sh->pos[2] += fixmulf(smat[2][2], sh->vel);

	// Check against heightmap
	if(sh->pos[1] >= hmap_get(sh->pos[0], sh->pos[2]))
		sh->lifetime = 0;

	// Check against objects
	// TODO!
}

static void shot_update(void)
{
	int i = shot_tail;

	while(i != shot_head)
	{
		// Update
		shot_update_one(&shot_list[i]);

		// Advance
		int old_i = i;
		i++;
		if(i >= SHOT_MAX)
			i = 0;

		// Advance tail if necessary
		if(old_i == shot_tail && shot_list[old_i].lifetime == 0)
			shot_tail = i;
	}
}

static void shot_draw_one(shot_s *sh)
{
	// Skip dead bullets
	if(sh->lifetime == 0)
		return;
	
	// Draw
	mat4_load_identity(&mat_obj);
	mat4_rotate_x(&mat_obj, -sh->rx);
	mat4_rotate_y(&mat_obj, -sh->ry);
	mat4_translate_vec4(&mat_obj, &sh->pos);
	mesh_draw(&poly_shot_mgun, MS_NOPRIO);
}

static void shot_draw(void)
{
	int i = shot_tail;

	while(i != shot_head)
	{
		// Update
		shot_draw_one(&shot_list[i]);

		// Advance
		i++;
		if(i >= SHOT_MAX)
			i = 0;
	}

}

