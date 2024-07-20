typedef enum SpriteID
{
	SPRITE_NIL,
	SPRITE_PLAYER,
	SPRITE_TREE0,
	SPRITE_TREE1,
	SPRITE_ROCK0,
	SPRITE_ROCK1,
	SPRITE_MAX,
} SpriteID;

typedef enum EntityArchetype
{
	ARCH_NIL = 0,
	ARCH_ROCK = 1,
	ARCH_TREE = 2,
	ARCH_PLAYER = 3,
} EntityArchetype;

typedef struct Entity
{
	// more or less common
	bool is_valid;
	EntityArchetype arch;
	Vector2 pos;
	// sprites
	bool render_sprite;
	SpriteID sprite_id;
} Entity;

#define MAX_ENTITY_COUNT 128
typedef struct World
{
	Entity entities[MAX_ENTITY_COUNT];
} World;

World *world = null;

Entity *entity_create()
{
	Entity *found_entity = null;
	for (int i = 0; i < MAX_ENTITY_COUNT; i++)
	{
		Entity *current_entity = &world->entities[i];
		if (!current_entity->is_valid)
		{
			found_entity = current_entity;
		}
	}
	assert(found_entity, "no more free entities");
	found_entity->is_valid = true;
	return found_entity;
}

void entity_destroy(Entity *entity)
{
	memset(entity, 0, sizeof(Entity));
}

void setup_player(Entity *entity)
{
	entity->arch = ARCH_PLAYER;
	entity->render_sprite = true;
	entity->sprite_id = SPRITE_PLAYER;
	// ...
}

void setup_rock(Entity *entity)
{
	entity->arch = ARCH_ROCK;
	entity->render_sprite = true;
	entity->sprite_id = SPRITE_ROCK0;
	// ...
}

void setup_tree(Entity *entity)
{
	entity->arch = ARCH_TREE;
	entity->render_sprite = true;
	entity->sprite_id = SPRITE_TREE0;
	// ...
}

typedef struct Sprite
{
	Gfx_Image *image;
	Vector2 size;
} Sprite;

Sprite sprites[SPRITE_MAX];

Sprite *get_sprite(SpriteID id)
{
	if (id >= 0 && id <= SPRITE_MAX)
	{
		return &sprites[id];
	}
	else
	{
		return &sprites[0];
	}
}

bool almost_equal(f32 a, f32 b, f32 epsilon)
{
	return fabs(a - b) <= epsilon;
}

bool animate_f32_to_target(f32 *value, f32 target, f32 delta_t, f32 rate)
{
	*value += (target - *value) * (1.0 - powf(2.0, -rate * delta_t));

	if (almost_equal(*value, target, 0.001f))
	{
		*value = target;
		return true;
	}

	return false;
}

bool animate_v2_to_target(Vector2 *value, Vector2 target, f32 delta_t, f32 rate)
{
	bool x_reached = animate_f32_to_target(&(value->x), target.x, delta_t, rate);
	bool y_reached = animate_f32_to_target(&(value->y), target.y, delta_t, rate);

	return x_reached && y_reached;
}

Vector2 mouse_to_world_pos(f32 mouse_x, f32 mouse_y, Matrix4 proj, Matrix4 view)
{
	f32 x_ndc = (2. * mouse_x / window.width) - 1.;
	f32 y_ndc = (2. * mouse_y / window.height) - 1.;

	Vector4 world_pos = v4(x_ndc, y_ndc, 0., 1.);
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);

	return v2(world_pos.x, world_pos.y);
}

int entry(int argc, char **argv)
{
	window.title = STR("Game Programming in C");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720;
	window.x = 200;
	window.y = 120;
	window.clear_color = hex_to_rgba(0x222323ff);

	world = alloc(get_heap_allocator(), sizeof(World));
	// spawn rocks
	for (int i = 0; i < 10; i++)
	{
		Entity *entity = entity_create();
		setup_rock(entity);
		entity->pos = v2(get_random_float32_in_range(-100, 100), get_random_float32_in_range(-100, 100));
	}
	// spawn trees
	for (int i = 0; i < 10; i++)
	{
		Entity *entity = entity_create();
		setup_tree(entity);
		entity->pos = v2(get_random_float32_in_range(-100, 100), get_random_float32_in_range(-100, 100));
	}

	sprites[SPRITE_PLAYER] = (Sprite){
		.image = load_image_from_disk(fixed_string("art/player.png"), get_heap_allocator()),
		.size = v2(5., 7.)};

	sprites[SPRITE_TREE0] = (Sprite){
		.image = load_image_from_disk(fixed_string("art/tree0.png"), get_heap_allocator()),
		.size = v2(8., 16.)};

	sprites[SPRITE_TREE1] = (Sprite){
		.image = load_image_from_disk(fixed_string("art/tree1.png"), get_heap_allocator()),
		.size = v2(8., 16.)};

	sprites[SPRITE_ROCK0] = (Sprite){
		.image = load_image_from_disk(fixed_string("art/rock0.png"), get_heap_allocator()),
		.size = v2(11., 7.)};

	sprites[SPRITE_ROCK1] = (Sprite){
		.image = load_image_from_disk(fixed_string("art/rock1.png"), get_heap_allocator()),
		.size = v2(11., 7.)};

	Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "could not load arial font");
	const u32 font_height = 48;

	Entity *player_ent = entity_create();
	setup_player(player_ent);

	f32 zoom = 10.;
	Vector2 camera_pos = v2(0., 0.);

	s32 frame_count = 0;
	f64 seconds_counter = 0.0;
	f64 previous_t = os_get_current_time_in_seconds();
	while (!window.should_close)
	{
		reset_temporary_storage();
		os_update();

		f64 now_t = os_get_current_time_in_seconds();
		f64 delta_t = now_t - previous_t;
		previous_t = os_get_current_time_in_seconds();

		// camera stuff
		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 100);
		Vector2 target_pos = player_ent->pos;
		animate_v2_to_target(&camera_pos, target_pos, delta_t, 7.0);
		draw_frame.view = m4_scalar(1.0);
		draw_frame.view = m4_mul(draw_frame.view, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0.)));
		draw_frame.view = m4_mul(draw_frame.view, m4_make_scale(v3(1. / zoom, 1. / zoom, 1.)));

		// input
		if (is_key_just_pressed(KEY_ESCAPE))
			window.should_close = true;

		Vector2 mouse_in_world = mouse_to_world_pos(input_frame.mouse_x, input_frame.mouse_y, draw_frame.projection, draw_frame.view);
		draw_text(font, sprint(temp, STR("%.1f %.1f"), mouse_in_world.x, mouse_in_world.y), font_height, mouse_in_world, v2(0.1, 0.1), COLOR_BLUE);

		Vector2 input_axis = v2(0, 0);
		if (is_key_down('Q'))
		{
			input_axis.x -= 1.0;
		}
		if (is_key_down('D'))
		{
			input_axis.x += 1.0;
		}
		if (is_key_down('S'))
		{
			input_axis.y -= 1.0;
		}
		if (is_key_down('Z'))
		{
			input_axis.y += 1.0;
		}
		input_axis = v2_normalize(input_axis);

		player_ent->pos = v2_add(player_ent->pos, v2_mulf(input_axis, 50.0 * delta_t));

		for (int i = 0; i < MAX_ENTITY_COUNT; i++)
		{
			Entity *entity = &world->entities[i];
			if (entity->is_valid)
			{
				switch (entity->arch)
				{
				default:
				{
					Sprite *entity_sprite = get_sprite(entity->sprite_id);
					Matrix4 xform = m4_scalar(1.0);
					xform = m4_translate(xform, v3(entity_sprite->size.x * -0.5, 0., 0.)); // pivot center bottom
					xform = m4_translate(xform, v3(entity->pos.x, entity->pos.y, 0.));	   // position
					draw_image_xform(entity_sprite->image, xform, entity_sprite->size, COLOR_WHITE);
					draw_text(font, sprint(temp, STR("%.1f %.1f"), entity->pos.x, entity->pos.y), font_height, v2_add(entity->pos, v2(0., -4.)), v2(.1, .1), COLOR_WHITE);
				}
				break;
				}
			}
		}

		gfx_update();

		frame_count += 1;
		seconds_counter += delta_t;
		if (seconds_counter > 1.0)
		{
			log("fps: %i", frame_count);
			frame_count = 0;
			seconds_counter = 0.0;
		}
	}

	return 0;
}
