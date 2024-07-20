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

int entry(int argc, char **argv)
{
	window.title = STR("Cool Game");
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

	Entity *player_ent = entity_create();
	setup_player(player_ent);

	s32 frame_count = 0;
	f64 seconds_counter = 0.0;
	f64 previous_t = os_get_current_time_in_seconds();
	while (!window.should_close)
	{
		reset_temporary_storage();
		os_update();
		draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 100);
		f32 zoom = 10.;
		draw_frame.view = m4_make_scale(v3(1. / zoom, 1. / zoom, 1.));

		if (is_key_just_pressed(KEY_ESCAPE))
			window.should_close = true;

		f64 now_t = os_get_current_time_in_seconds();
		f64 delta_t = now_t - previous_t;
		previous_t = os_get_current_time_in_seconds();

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
