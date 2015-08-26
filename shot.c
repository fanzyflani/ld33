
extern int jet_count;
extern jet_s jet_list[];

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
	int i, j;

	// Skip dead bullets
	if(sh->lifetime == 0)
		return;

	// Update lifetime
	sh->lifetime--;

	// Move forward
	fixed xs = fixsin(sh->rx);
	fixed xc = fixcos(sh->rx);
	fixed ys = fixmulf(fixsin(sh->ry), xc);
	fixed yc = fixmulf(fixcos(sh->ry), xc);
	sh->pos[0] += fixmulf(ys, sh->vel);
	sh->pos[1] += fixmulf(xs, sh->vel);
	sh->pos[2] += fixmulf(yc, sh->vel);

	// Check against heightmap
	if(sh->pos[1] >= hmap_get(sh->pos[0], sh->pos[2]))
		sh->lifetime = 0;

	// Check against jets
	// TODO!
	for(i = 0; i < jet_count; i++)
	{
		jet_s *jet = &jet_list[i];

		// CHEAT: Only do player if team is nonplayer
		if(sh->team == 2)
		{
			jet = player;
			i = jet_count;
		}

		// Skip friendly fire and dead jets
		if(jet->team == sh->team || jet->health <= 0)
			continue;

		// Check if in range
		vec4 cmpvec;
		for(j = 0; j < 3; j++)
			cmpvec[j] = jet->pos[j] - sh->pos[j];

		fixed r2 = vec4_dot_3(&cmpvec, &cmpvec);

		if(r2 <= 0x10000)
		{
			// Register hit!
			jet->health -= 5;
			jet->hit_flash = (jet->health > 0 ? 5 : 50);

			// Play sound
			if(jet->health > 0 || jet == player)
			{
				SPU_n_ADSR(17) = 0x9FC083FF;
				SPU_n_START(17) = s3mplayer.psx_spu_offset[17];
				if(jet == player)
				{
					SPU_n_PITCH(17) = (jet->health > 0 ? 0x0600 : 0x0200);
					SPU_n_MVOL_L(17) = 0x3FFF;
					SPU_n_MVOL_R(17) = 0x3FFF;
				} else {
					SPU_n_PITCH(17) = (0x0800);
					SPU_n_MVOL_L(17) = 0x1FFF;
					SPU_n_MVOL_R(17) = 0x1FFF;
				}
				SPU_KON = (1<<17);
			} else {
				// Play crash sound
				SPU_n_ADSR(16) = 0x9FC083FF;
				SPU_n_START(16) = s3mplayer.psx_spu_offset[16];
				SPU_n_PITCH(16) = 0x0800;
				SPU_n_MVOL_L(16) = 0x1FFF;
				SPU_n_MVOL_R(16) = 0x1FFF;
				SPU_KON = (1<<16);
			}

		}
	}
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
	
	// Check if in view dist
	int hx = (sh->pos[0]>>18);
	int hz = (sh->pos[2]>>18);
	hx -= hmap_visx;
	hz -= hmap_visz;
	if(hx < 0) hx = -hx;
	if(hz < 0) hz = -hz;

	if(hx > VISRANGE || hz > VISRANGE) return;

	// Draw
	mat4_load_identity(&mat_obj);
	mat4_rotate_x(&mat_obj, -sh->rx);
	mat4_rotate_y(&mat_obj, -sh->ry);
	mat4_translate_vec4(&mat_obj, &sh->pos);
	mesh_draw(&poly_shot_mgun, MS_NOPRIO);
}

static void shot_draw(void)
{
	int i = shot_head-1;
	int j;
	if(i < 0) i = SHOT_MAX-1;

	for(j = 0; j < 5 && i != shot_tail; j++)
	{
		// Update
		shot_draw_one(&shot_list[i]);

		// Advance
		i--;
		if(i < 0)
			i = SHOT_MAX-1;
	}

}

