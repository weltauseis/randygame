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
	SPRITE_FURNACE,
	SPRITE_WORKBENCH,
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
	ARCH_FURNACE = 6,
	ARCH_WORKBENCH = 7,
	ARCH_MAX,
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

typedef struct ItemData
{
	u32 amount;
} ItemData;

typedef enum UXState
{
	UX_NIL,
	UX_INVENTORY,
	UX_BUILDING,
	UX_PLACE_MODE,
} UXState;

// buiding resources
// a "resource" is a thing we setup during startup and is constant
typedef enum BuildingID
{
	BUILDING_NIL,
	BUILDING_FURNACE,
	BUILDING_WORKBENCH,
	BUILDING_MAX,
} BuildingID;

typedef struct BuildingData
{
	EntityArchetype to_build;
	SpriteID icon;
	// display name
	// cost
} BuildingData;

BuildingData buildings[BUILDING_MAX];

BuildingData get_building_data(BuildingID id)
{
	return buildings[id];
}

#define MAX_ENTITY_COUNT 128
typedef struct World
{
	Entity entities[MAX_ENTITY_COUNT];
	ItemData inventory_items[ARCH_MAX];
	UXState ux_state;
	BuildingID building_to_place;
} World;

World *world = null;

typedef struct WorldFrame
{
	Entity *selected_entity;
	Matrix4 world_proj;
	Matrix4 camera_view;
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

void setup_furnace(Entity *entity)
{
	entity->arch = ARCH_FURNACE;
	entity->render_sprite = true;
	entity->sprite_id = SPRITE_FURNACE;
	// ...
}

void setup_workbench(Entity *entity)
{
	entity->arch = ARCH_WORKBENCH;
	entity->render_sprite = true;
	entity->sprite_id = SPRITE_WORKBENCH;
	// ...
}

void entity_setup(Entity *entity, EntityArchetype archetype)
{
	switch (archetype)
	{
	case ARCH_ROCK:
		setup_rock(entity);
		break;
	case ARCH_TREE:
		setup_tree(entity);
		break;
	case ARCH_FURNACE:
		setup_furnace(entity);
		break;

	default:
		log_error("entity_setup: archetype is missing");
	}
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
		Sprite *sprite = &sprites[id];
		// the image might be null even if the ID is valid
		// if the loading failed or never happened
		if (sprite->image)
		{
			return sprite;
		}
	}

	return &sprites[0];
}

SpriteID get_sprite_id_from_archetype(EntityArchetype archetype)
{
	switch (archetype)
	{
	case ARCH_ITEM_STONE:
		return SPRITE_ITEM_STONE;

	case ARCH_ITEM_WOOD:
		return SPRITE_ITEM_WOOD;

	default:
		return SPRITE_NIL;
	}
}

Vector2 get_sprite_size(Sprite *sprite)
{
	return v2(sprite->image->width, sprite->image->height);
}

string get_archetype_pretty_name(EntityArchetype archetype)
{
	switch (archetype)
	{
	case ARCH_ITEM_WOOD:
		return STR("Wood");

	case ARCH_ITEM_STONE:
		return STR("Stone");

	default:
		return STR("NIL");
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

f32 sin_breathe(f32 time, f32 rate)
{
	return sin(time * rate) * 0.5 + 0.5;
}

Vector2 get_mouse_pos_in_world(Matrix4 proj, Matrix4 view)
{
	f32 x_ndc = (2. * input_frame.mouse_x / window.width) - 1.;
	f32 y_ndc = (2. * input_frame.mouse_y / window.height) - 1.;

	Vector4 world_pos = v4(x_ndc, y_ndc, 0., 1.);
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);

	return v2(world_pos.x, world_pos.y);
}

Vector2 get_mouse_pos_in_ndc()
{
	f32 x_ndc = (2. * input_frame.mouse_x / window.width) - 1.;
	f32 y_ndc = (2. * input_frame.mouse_y / window.height) - 1.;

	return v2(x_ndc, y_ndc);
}

Range2f quad_to_range(Draw_Quad *quad)
{
	return (Range2f){quad->bottom_left, quad->top_right};
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

#define VUI_W 240.0
#define VUI_H 135.0
void set_virtual_screen_space()
{
	draw_frame.view = m4_scalar(1.0);
	draw_frame.projection = m4_make_orthographic_projection(0.0, VUI_W, 0, VUI_H, -1., 10.);
}

void set_world_space()
{
	draw_frame.projection = world_frame.world_proj;
	draw_frame.view = world_frame.camera_view;
}

f64 delta_t;
Gfx_Font *font;
u32 font_height;
const s32 z_layer_ui = 15;

void render_ui()
{
	set_virtual_screen_space();
	push_z_layer(z_layer_ui);

	Vector4 bg_box_color = v4(0., 0., 0., 0.5);
	Vector2 mouse_in_world = get_mouse_pos_in_world(world_frame.world_proj, world_frame.camera_view);

	// inventory
	bool is_inventory_enabled = (world->ux_state == UX_INVENTORY);
	if (is_inventory_enabled)
	{
		f32 item_width = 8.0;
		f32 padding = 2.0;

		s32 icon_row_count = 8;
		f32 inventory_width = icon_row_count * (item_width + padding);

		f32 x_pos = (VUI_W - inventory_width) * 0.5;
		f32 y_pos = 70.;

		// draw the background box
		{
			Matrix4 xform = m4_scalar(1.0);
			xform = m4_translate(xform, v3(x_pos, y_pos, 0.));
			draw_rect_xform(xform, v2(inventory_width, item_width), bg_box_color);
		}

		// draw the items
		for (int archetype_i = 0; archetype_i < ARCH_MAX; archetype_i++)
		{
			ItemData *item = &world->inventory_items[archetype_i];
			if (item->amount > 0)
			{
				Matrix4 xform = m4_scalar(1.0);
				xform = m4_translate(xform, v3(x_pos, y_pos, 0.));

				// draw lighter square & store its range
				Range2f slot_range = quad_to_range(draw_rect_xform(xform, v2(item_width, item_width), v4(1., 1., 1., .3)));

				Sprite *item_sprite = get_sprite(get_sprite_id_from_archetype(archetype_i));
				xform = m4_translate(xform, v3(item_width * 0.5, item_width * 0.5, 0.));
				Vector2 slot_center = m4_transform(xform, v4(0., 0., 0., 1.)).xy;

				bool is_hovered = range2f_contains(slot_range, get_mouse_pos_in_ndc());
				if (is_hovered)
				{
					f32 icon_scale = 1.3 /* + 0.2 * sin_breathe(now_t, 3.0) */;
					xform = m4_scale(xform, v3(icon_scale, icon_scale, 1.));
				}
				xform = m4_translate(xform, v3(item_sprite->image->width * -0.5, item_sprite->image->height * -0.5, 0.));
				draw_image_xform(item_sprite->image, xform, get_sprite_size(item_sprite), COLOR_WHITE);

				// tooltip
				if (is_hovered)
				{
					// background box
					Matrix4 xform = m4_scalar(1.0);
					Vector2 box_size = v2(40, 14);

					xform = m4_translate(xform, v3(slot_center.x, slot_center.y, 0.0)); // put it where we render the square
					xform = m4_translate(xform, v3(box_size.x * -0.5, box_size.y * -1. - item_width * 0.5, 0.));

					draw_rect_xform(xform, box_size, bg_box_color);

					// text
					f32 current_text_y = -10.;
					{
						string text_string = get_archetype_pretty_name(archetype_i);
						Gfx_Text_Metrics metrics = measure_text(font, text_string, font_height, v2(0.1, 0.1));

						Matrix4 text_xform = m4_scalar(1.0);
						text_xform = m4_translate(text_xform, v3(slot_center.x, slot_center.y, 0.0));		 // put it in the center of the slot
						text_xform = m4_translate(text_xform, v3(-metrics.functional_size.x * 0.5, 0., 0.)); // pivot is now top center
						text_xform = m4_translate(text_xform, v3(0., current_text_y, 0.));					 // pad it down
						draw_text_xform(font, text_string, font_height, text_xform, v2(0.1, 0.1), COLOR_WHITE);
						current_text_y += -5.;
					}

					{
						string text_string = sprintf(temp_allocator, "x%d", item->amount);
						Gfx_Text_Metrics metrics = measure_text(font, text_string, font_height, v2(0.1, 0.1));

						Matrix4 text_xform = m4_scalar(1.0);
						text_xform = m4_translate(text_xform, v3(slot_center.x, slot_center.y, 0.0));		 // put it in the center of the slot
						text_xform = m4_translate(text_xform, v3(-metrics.functional_size.x * 0.5, 0., 0.)); // pivot is now top center
						text_xform = m4_translate(text_xform, v3(0., current_text_y, 0.));					 // pad it down
						draw_text_xform(font, text_string, font_height, text_xform, v2(0.1, 0.1), COLOR_WHITE);
						current_text_y += -5.;
					}
				}

				x_pos += item_width + padding;
			}
		}
	}

	// building UI
	bool is_building_enabled = (world->ux_state == UX_BUILDING);
	if (is_building_enabled)
	{
		f32 y_pos = 20.0;
		f32 slot_width = 12.;
		f32 slot_padding = 6.;

		s32 building_count = BUILDING_MAX - 1;
		f32 box_width = building_count * slot_width + (building_count + 1) * slot_padding;
		f32 x_pos = VUI_W * 0.5 - box_width * 0.5;
		// draw the background box
		{
			Matrix4 xform = m4_scalar(1.0);
			xform = m4_translate(xform, v3(x_pos, y_pos - slot_padding, 0.));
			draw_rect_xform(xform, v2(box_width, slot_padding * 2 + slot_width), bg_box_color);
		}

		for (BuildingID id = 1; id < BUILDING_MAX; id++)
		{
			BuildingData *building = &buildings[id];
			Sprite *icon = get_sprite(building->icon);

			Matrix4 xform = m4_scalar(1.0);

			x_pos += slot_padding;

			xform = m4_translate(xform, v3(x_pos, y_pos, 0.));
			// draw_rect_xform(xform, v2(slot_width, slot_width), v4(1., 1., 1., 0.3));

			Range2f slot_range = quad_to_range(draw_image_xform(icon->image, xform, v2(slot_width, slot_width), COLOR_WHITE));
			bool is_hovered = range2f_contains(slot_range, get_mouse_pos_in_ndc());

			// tooltip
			if (is_hovered)
			{
				if (is_key_just_pressed(MOUSE_BUTTON_LEFT))
				{
					consume_key_just_pressed(MOUSE_BUTTON_LEFT);

					world->building_to_place = id;
					world->ux_state = UX_PLACE_MODE;
				}
			}

			x_pos += slot_width;
		}
	}

	// placement mode UI
	bool is_place_mode_enabled = (world->ux_state == UX_PLACE_MODE);
	if (is_place_mode_enabled)
	{
		// the proj and view is currently in virtual screen coordinates
		Matrix4 prev_proj = draw_frame.projection;
		Matrix4 prev_view = draw_frame.view;

		draw_frame.projection = world_frame.world_proj;
		draw_frame.view = world_frame.camera_view;

		Vector2 pos = v2_tilemap_round_world_pos(mouse_in_world);
		Matrix4 building_xform = m4_scalar(1.0);
		building_xform = m4_translate(building_xform, v3(pos.x, pos.y, 0.));

		BuildingData building = get_building_data(world->building_to_place);
		Sprite *sprite = get_sprite(building.icon);

		Vector2 sprite_size = get_sprite_size(sprite);
		building_xform = m4_translate(building_xform, v3(-sprite_size.x * 0.5, -TILE_WIDTH * 0.5, 0.));

		draw_image_xform(sprite->image, building_xform, sprite_size, v4(1., 1., 1., 0.4));

		if (is_key_just_pressed(MOUSE_BUTTON_LEFT))
		{
			consume_key_just_pressed(MOUSE_BUTTON_LEFT);

			Entity *entity = entity_create();
			entity_setup(entity, building.to_build);
			entity->pos = pos;
			world->ux_state = UX_NIL;
		}

		// restore the old proj and view
		draw_frame.projection = prev_proj;
		draw_frame.view = prev_view;
	}

	// restore the world proj and view
	set_world_space();
	pop_z_layer();
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
	for (int i = 0; i < 30; i++)
	{
		Entity *entity = entity_create();
		setup_rock(entity);
		entity->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
		entity->pos = v2_tilemap_round_world_pos(entity->pos);
		// entity->pos = v2_sub(entity->pos, v2(0., 0.5 * TILE_WIDTH));
	}
	// spawn trees
	for (int i = 0; i < 30; i++)
	{
		Entity *entity = entity_create();
		setup_tree(entity);
		entity->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
		entity->pos = v2_tilemap_round_world_pos(entity->pos);
		// entity->pos = v2_sub(entity->pos, v2(0., 0.5 * TILE_WIDTH));
	}
	// add test items @ship remove this
	{
		world->inventory_items[ARCH_ITEM_WOOD].amount = 5;
		// world->inventory_items[ARCH_ITEM_STONE].amount = 3;
		// Entity *furnace_entity = entity_create();
		// setup_furnace(furnace_entity);
		// furnace_entity->pos.x = 16.;
	}

	// resources setup

	sprites[SPRITE_NIL] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/missing_texture.png"), get_heap_allocator())};
	sprites[SPRITE_PLAYER] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/player.png"), get_heap_allocator())};
	sprites[SPRITE_PINE_TREE] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/pine_tree.png"), get_heap_allocator())};
	sprites[SPRITE_OAK_TREE] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/oak_tree.png"), get_heap_allocator())};
	sprites[SPRITE_ROCK0] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/rock0.png"), get_heap_allocator())};
	sprites[SPRITE_ROCK1] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/rock1.png"), get_heap_allocator())};
	sprites[SPRITE_ITEM_WOOD] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/item_wood.png"), get_heap_allocator())};
	sprites[SPRITE_ITEM_STONE] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/item_stone.png"), get_heap_allocator())};
	sprites[SPRITE_FURNACE] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/furnace.png"), get_heap_allocator())};
	sprites[SPRITE_WORKBENCH] = (Sprite){.image = load_image_from_disk(fixed_string("res/sprites/workbench.png"), get_heap_allocator())};

	//@ship remove this
	for (int i = 0; i < SPRITE_MAX; i++)
		assert(sprites[i].image, "Sprite was not setup properly");

	buildings[BUILDING_NIL] = (BuildingData){.to_build = ARCH_NIL, .icon = SPRITE_NIL};
	buildings[BUILDING_FURNACE] = (BuildingData){.to_build = ARCH_FURNACE, .icon = SPRITE_FURNACE};
	buildings[BUILDING_WORKBENCH] = (BuildingData){.to_build = ARCH_WORKBENCH, .icon = SPRITE_WORKBENCH};

	font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "could not load arial font");
	font_height = 48;

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
		delta_t = now_t - previous_t;
		previous_t = os_get_current_time_in_seconds();

		draw_frame.enable_z_sorting = true;

		world_frame = (WorldFrame){0};

		// camera stuff
		world_frame.world_proj = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 100);
		Vector2 target_pos = player_ent->pos;
		animate_v2_to_target(&camera_pos, target_pos, delta_t, 7.0);
		world_frame.camera_view = m4_scalar(1.0);
		world_frame.camera_view = m4_mul(world_frame.camera_view, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0.)));
		world_frame.camera_view = m4_mul(world_frame.camera_view, m4_make_scale(v3(1. / zoom, 1. / zoom, 1.)));
		set_world_space();

		// input
		// @ship don't do this
		if (is_key_just_pressed(KEY_ESCAPE))
			window.should_close = true;

		// UI rendering
		render_ui();

		// mouse hover and entity selection
		if (world->ux_state == UX_NIL)
		{
			Vector2 mouse_in_world = get_mouse_pos_in_world(draw_frame.projection, draw_frame.view);
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
		}

		// update entities
		f32 player_pickup_radius = 8.0;
		for (int i = 0; i < MAX_ENTITY_COUNT; i++)
		{
			Entity *entity = &world->entities[i];
			if (entity->is_valid)
			{
				// pick up nearby items
				if (entity->is_item)
				{
					// TODO - add physics to item pickup

					f32 dist_to_player = v2_length(v2_sub(entity->pos, player_ent->pos));
					if (dist_to_player < player_pickup_radius)
					{
						world->inventory_items[entity->arch].amount += 1;
						entity_destroy(entity);
					}
				}
			}
		}

		// draw the tile map
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
					color = v4(1., 1., 1., 0.1);
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

		// entities rendering
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

		// UI state updating
		if (is_key_just_pressed(KEY_TAB))
		{
			consume_key_just_pressed(KEY_TAB);
			world->ux_state = (world->ux_state == UX_INVENTORY) ? UX_NIL : UX_INVENTORY;
		}

		if (is_key_just_pressed('C'))
		{
			consume_key_just_pressed('C');
			world->ux_state = (world->ux_state == UX_BUILDING) ? UX_NIL : UX_BUILDING;
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
