
int entry(int argc, char **argv)
{
	window.title = STR("Minimal Game");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720;
	window.x = 200;
	window.y = 120;
	window.clear_color = hex_to_rgba(0x222323ff);

	Gfx_Image *player_sprite = load_image_from_disk(fixed_string("art/player.png"), get_heap_allocator());
	assert(player_sprite, "player sprite not found");

	Vector2 player_pos = v2(0, 0);

	s32 frame_count = 0;
	f64 seconds_counter = 0.0;
	f64 previous_t = os_get_current_time_in_seconds();
	while (!window.should_close)
	{
		reset_temporary_storage();
		os_update();

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

		player_pos = v2_add(player_pos, v2_mulf(input_axis, delta_t));

		Matrix4 xform = m4_scalar(1.0);
		xform = m4_translate(xform, v3(player_pos.x, player_pos.y, 0));
		draw_image_xform(player_sprite, xform, v2(.5f, .5f), COLOR_WHITE);

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