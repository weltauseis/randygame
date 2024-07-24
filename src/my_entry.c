typedef enum SpriteID
{
	SPRITE_NIL,
	SPRITE_PLAYER,
	SPRITE_PINE_TREE,
	SPRITE_OAK_TREE,
	SPRITE_ROCK0,
	SPRITE_ROCK1,
	SPRITE_ITEM_WOOD,
	SPRITE_ITEM_STONE,
	SPRITE_MAX,
} SpriteID;

typedef enum ItemID
{
	ITEM_NIL,
	ITEM_TREE0,
	ITEM_ROCK0,
	ITEM_ROCK1,
	ITEM_MAX,
} ItemID;

typedef enum EntityArchetype
{
	ARCH_NIL = 0,
	ARCH_ROCK = 1,
	ARCH_TREE = 2,
	ARCH_PLAYER = 3,
	ARCH_ITEM_WOOD = 4,
	ARCH_ITEM_STONE = 5,
} EntityArchetype;

typedef struct Entity
{
	// more or less common
	bool is_valid;
	EntityArchetype arch;
	Vector2 pos;
	s32 health;
	bool destroyable_world_entity;
	// sprites
	bool render_sprite;
	SpriteID sprite_id;
	// items
	s32 item_count; // unused
	bool is_item;
} Entity;

#define MAX_ENTITY_COUNT 128
typedef struct World
{
	Entity entities[MAX_ENTITY_COUNT];
} World;

World *world = null;

typedef struct WorldFrame
{
	Entity *selected_entity;
} WorldFrame;

WorldFrame world_frame;

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
const s32 ROCK_HEALTH = 3;
void setup_rock(Entity *entity)
{
	entity->arch = ARCH_ROCK;
	entity->render_sprite = true;
	entity->sprite_id = SPRITE_ROCK0;
	entity->health = ROCK_HEALTH;
	entity->destroyable_world_entity = true;
	// ...
}

const s32 TREE_HEALTH = 2;
void setup_tree(Entity *entity)
{
	entity->arch = ARCH_TREE;
	entity->render_sprite = true;
	entity->sprite_id = SPRITE_PINE_TREE;
	entity->health = TREE_HEALTH;
	entity->destroyable_world_entity = true;
	// ...
}

void setup_item_wood(Entity *entity)
{
	entity->arch = ARCH_ITEM_WOOD;
	entity->render_sprite = true;
	entity->is_item = true;
	entity->sprite_id = SPRITE_ITEM_WOOD;
	// ...
}

void setup_item_stone(Entity *entity)
{
	entity->arch = ARCH_ITEM_STONE;
	entity->render_sprite = true;
	entity->is_item = true;
	entity->sprite_id = SPRITE_ITEM_STONE;
	// ...
}

typedef struct Sprite
{
	Gfx_Image *image;
} Sprite;

Sprite sprites[SPRITE_MAX];

Sprite *get_sprite(SpriteID id)
{
	if (id >= 0 && id <= SPRITE_MAX)
	{
		return &sprites[id];
	}

	return &sprites[0];
}

Vector2 get_sprite_size(Sprite *sprite)
{
	return v2(sprite->image->width, sprite->image->height);
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

f32 sin_breathe(f32 time, f32 rate)
{
	return sin(time * rate) * 0.5 + 1.0;
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

const s32 TILE_WIDTH = 8;

s32 world_pos_to_tile_pos(f32 world_pos)
{
	return roundf(world_pos / (float)TILE_WIDTH);
}

f32 tile_pos_to_world_pos(s32 tile_pos)
{
	return (f32)tile_pos * (f32)TILE_WIDTH;
}

Vector2 v2_tilemap_round_world_pos(Vector2 world_pos)
{
	return v2(world_pos_to_tile_pos(world_pos.x) * TILE_WIDTH, world_pos_to_tile_pos(world_pos.y) * TILE_WIDTH);
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
		entity->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
		entity->pos = v2_tilemap_round_world_pos(entity->pos);
		// entity->pos = v2_sub(entity->pos, v2(0., 0.5 * TILE_WIDTH));
	}
	// spawn trees
	for (int i = 0; i < 10; i++)
	{
		Entity *entity = entity_create();
		setup_tree(entity);
		entity->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
		entity->pos = v2_tilemap_round_world_pos(entity->pos);
		// entity->pos = v2_sub(entity->pos, v2(0., 0.5 * TILE_WIDTH));
	}

	sprites[SPRITE_PLAYER] = (Sprite){
		.image = load_image_from_disk(fixed_string("res/sprites/player.png"), get_heap_allocator())};

	sprites[SPRITE_PINE_TREE] = (Sprite){
		.image = load_image_from_disk(fixed_string("res/sprites/pine_tree.png"), get_heap_allocator())};

	sprites[SPRITE_OAK_TREE] = (Sprite){
		.image = load_image_from_disk(fixed_string("res/sprites/oak_tree.png"), get_heap_allocator())};

	sprites[SPRITE_ROCK0] = (Sprite){
		.image = load_image_from_disk(fixed_string("res/sprites/rock0.png"), get_heap_allocator())};

	sprites[SPRITE_ROCK1] = (Sprite){
		.image = load_image_from_disk(fixed_string("res/sprites/rock1.png"), get_heap_allocator())};

	sprites[SPRITE_ITEM_WOOD] = (Sprite){
		.image = load_image_from_disk(fixed_string("res/sprites/item_wood.png"), get_heap_allocator())};

	sprites[SPRITE_ITEM_STONE] = (Sprite){
		.image = load_image_from_disk(fixed_string("res/sprites/item_stone.png"), get_heap_allocator())};

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

		world_frame = (WorldFrame){0};

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

		// mouse hover and entity selection
		Vector2 mouse_in_world = mouse_to_world_pos(input_frame.mouse_x, input_frame.mouse_y, draw_frame.projection, draw_frame.view);
		s32 mouse_tile_pos_x = world_pos_to_tile_pos(mouse_in_world.x);
		s32 mouse_tile_pos_y = world_pos_to_tile_pos(mouse_in_world.y);
		f32 entity_selection_radius = 8.;

		f32 closest_distance = 0;
		for (int i = 0; i < MAX_ENTITY_COUNT; i++)
		{
			Entity *entity = &world->entities[i];
			if (entity->is_valid && entity->destroyable_world_entity)
			{
				f32 dist = v2_length(v2_sub(mouse_in_world, entity->pos));
				if (dist < entity_selection_radius)
				{
					if ((world_frame.selected_entity == null) || (dist < closest_distance))
					{
						world_frame.selected_entity = entity;
						closest_distance = dist;
					}
				}
			}
		}

		// draw a sample tile map
		s32 tilemap_radius_x = 16;
		s32 tilemap_radius_y = 8;
		s32 player_tile_x = world_pos_to_tile_pos(player_ent->pos.x);
		s32 player_tile_y = world_pos_to_tile_pos(player_ent->pos.y);
		for (s32 x = player_tile_x - tilemap_radius_x; x < player_tile_x + tilemap_radius_x; x++)
		{
			for (s32 y = player_tile_y - tilemap_radius_y; y < player_tile_y + tilemap_radius_y; y++)
			{
				Vector4 color = v4(0., 0., 0., 0.);
				// color only even tiles
				if ((x + (y % 2 == 0)) % 2 == 0)
				{
					color = v4(0., 0., 1., 0.3);
				}

				f32 tile_pos_in_world_x = tile_pos_to_world_pos(x) - TILE_WIDTH * 0.5;
				f32 tile_pos_in_world_y = tile_pos_to_world_pos(y) - TILE_WIDTH * 0.5;
				draw_rect(v2(tile_pos_in_world_x, tile_pos_in_world_y), v2(TILE_WIDTH, TILE_WIDTH), color);

				// color the hovered tile
				/* if (x == mouse_tile_pos_x && y == mouse_tile_pos_y)
				{
					draw_rect(v2_sub(tile_pos, v2(0.5 * TILE_WIDTH, 0.5 * TILE_WIDTH)), v2(TILE_WIDTH, TILE_WIDTH), v4(1., 0.9, 0., 1.));
				} */
			}
		}

		// click handling
		{
			Entity *selected_entity = world_frame.selected_entity;
			if (is_key_just_pressed(MOUSE_BUTTON_LEFT))
			{
				consume_key_just_pressed(MOUSE_BUTTON_LEFT);

				if (selected_entity)
				{
					selected_entity->health -= 1;

					if (selected_entity->health <= 0)
					{
						switch (selected_entity->arch)
						{
						case ARCH_TREE:
							/* spawn item */
							{
								Entity *item_entity = entity_create();
								setup_item_wood(item_entity);
								item_entity->pos = selected_entity->pos;
							}
							break;

						case ARCH_ROCK:
							/* spawn item */
							{
								Entity *item_entity = entity_create();
								setup_item_stone(item_entity);
								item_entity->pos = selected_entity->pos;
							}
							break;

						default:
							break;
						}
						entity_destroy(selected_entity);
					}
				}
			}
		}

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

		// draw entities
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

					if (entity->is_item)
						xform = m4_translate(xform, v3(0., sin_breathe(now_t, 2.5), 0.));

					// xform = m4_translate(xform, v3(entity_sprite->size.x * -0.5, 0., 0.)); // pivot center bottom
					xform = m4_translate(xform, v3(entity_sprite->image->width * -0.5, -0.5 * TILE_WIDTH, 0.)); // pivot center and a little up to account for the tile
					xform = m4_translate(xform, v3(entity->pos.x, entity->pos.y, 0.));							// position

					Vector4 color = COLOR_WHITE;
					if (world_frame.selected_entity == entity)
						color = COLOR_RED;

					draw_image_xform(entity_sprite->image, xform, get_sprite_size(entity_sprite), color);
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
