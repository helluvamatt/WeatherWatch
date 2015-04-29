#include <pebble.h>

static Window *window;
TextLayer *text_date_layer;
TextLayer *text_time_layer;
TextLayer *text_temp_layer;
TextLayer *text_cond_layer;
TextLayer *text_city_layer;
TextLayer *text_icon_layer;
GFont *font49;
GFont *font39;
GFont *font21;
GFont *icons;

static AppSync s_sync;
static uint8_t *s_sync_buffer;

static char icon_buffer[1];

#define WEATHER_TEMPERATURE 0
#define WEATHER_CONDITIONS 1
#define WEATHER_CITY 2
#define WEATHER_ICON 3
#define STATUS 4

static void sync_changed_handler(const uint32_t key, const Tuple *new_tuple, const Tuple *old_tuple, void *context)
{
	APP_LOG(APP_LOG_LEVEL_DEBUG, "sync_changed_handler() called (%u) = \"%s\"", (unsigned int) key, new_tuple->value->cstring);
	switch (key)
	{
		case WEATHER_TEMPERATURE:
			text_layer_set_text(text_temp_layer, new_tuple->value->cstring);
			break;
		case WEATHER_CONDITIONS:
			text_layer_set_text(text_cond_layer, new_tuple->value->cstring);
			break;
		case WEATHER_CITY:
			text_layer_set_text(text_city_layer, new_tuple->value->cstring);
			break;
		case WEATHER_ICON:
			snprintf(icon_buffer, 1, "%c", new_tuple->value->uint8);
			text_layer_set_text(text_icon_layer, icon_buffer);
			break;
	}
}

static void sync_error_handler(DictionaryResult dict_error, AppMessageResult app_message_error, void *context)
{
	APP_LOG(APP_LOG_LEVEL_ERROR, "sync_error_handler() called: dict_error = %d  app_message_error = %d", dict_error, app_message_error);
	text_layer_set_text(text_cond_layer, "Error");
	text_layer_set_text(text_icon_layer, "b");
}

static void window_load(Window *window)
{
	Layer *window_layer = window_get_root_layer(window);

	font49 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49));
	font39 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_39));
	font21 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21));
	icons = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ICONS_48));

	// create time layer - this is where time goes
	text_time_layer = text_layer_create(GRect(8, 24, 128, 76));
	text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
	text_layer_set_text_color(text_time_layer, GColorWhite);
	text_layer_set_background_color(text_time_layer, GColorClear);
	text_layer_set_font(text_time_layer, font49);
	layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

	// create date layer - this is where the date goes
	text_date_layer = text_layer_create(GRect(8, 0, 128, 24));
	text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);
	text_layer_set_text_color(text_date_layer, GColorWhite);
	text_layer_set_background_color(text_date_layer, GColorClear);
	text_layer_set_font(text_date_layer, font21);
	layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

	// create temperature layer - this is where the temperature goes
	text_temp_layer = text_layer_create(GRect(80, 108, 84, 30));
	text_layer_set_text_color(text_temp_layer, GColorWhite);
	text_layer_set_background_color(text_temp_layer, GColorClear);
	text_layer_set_font(text_temp_layer, font39);
	layer_add_child(window_layer, text_layer_get_layer(text_temp_layer));

	// create conditions layer
	text_cond_layer = text_layer_create(GRect(80, 138, 84, 15));
	text_layer_set_text_color(text_cond_layer, GColorWhite);
	text_layer_set_background_color(text_cond_layer, GColorClear);
	text_layer_set_font(text_cond_layer, font39);
	layer_add_child(window_layer, text_layer_get_layer(text_cond_layer));

	// create city name layer
	text_city_layer = text_layer_create(GRect(80, 153, 84, 15));
	text_layer_set_text_color(text_city_layer, GColorWhite);
	text_layer_set_background_color(text_city_layer, GColorClear);
	text_layer_set_font(text_city_layer, font39);
	layer_add_child(window_layer, text_layer_get_layer(text_city_layer));

	text_icon_layer = text_layer_create(GRect(10, 92, 60, 60));
	text_layer_set_text_color(text_icon_layer, GColorWhite);
	text_layer_set_text_alignment(text_icon_layer, GTextAlignmentCenter);
	text_layer_set_background_color(text_icon_layer, GColorClear);
	text_layer_set_font(text_icon_layer, icons);
	layer_add_child(window_layer, text_layer_get_layer(text_icon_layer));
}

static void window_unload(Window *window)
{
	// destroy the text layers - this is good
	text_layer_destroy(text_date_layer);
	text_layer_destroy(text_time_layer);
	text_layer_destroy(text_temp_layer);
	text_layer_destroy(text_cond_layer);
	text_layer_destroy(text_city_layer);
	text_layer_destroy(text_icon_layer);

	// unload the fonts
	fonts_unload_custom_font(font49);
	fonts_unload_custom_font(font39);
	fonts_unload_custom_font(font21);
	fonts_unload_custom_font(icons);
}

// show the date and time every minute
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed)
{
	APP_LOG(APP_LOG_LEVEL_DEBUG, "handle_minute_tick() called...");

	// Need to be static because they're used by the system later.
	static char time_text[] = "00:00";
	static char date_text[] = "Xxxxxxxxx 00";

	strftime(date_text, sizeof(date_text), "%B %e", tick_time);
	text_layer_set_text(text_date_layer, date_text);

	char *time_format;
	if (clock_is_24h_style()) {
		time_format = "%R";
	}
	else {
		time_format = "%I:%M";
	}

	strftime(time_text, sizeof(time_text), time_format, tick_time);

	if (!clock_is_24h_style() && (time_text[0] == '0')) {
		memmove(time_text, &time_text[1], sizeof(time_text) - 1);
	}

	text_layer_set_text(text_time_layer, time_text);
}

static void init(void)
{
	APP_LOG(APP_LOG_LEVEL_DEBUG, "init() called...");

	// create window
	window = window_create();
	window_set_background_color(window, GColorBlack);
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	window_stack_push(window, true);

	// init app message
	app_message_open(app_message_outbox_size_maximum(), app_message_inbox_size_maximum());

	// Setup initial values
	Tuplet initial_values[] = {
		TupletCString(WEATHER_TEMPERATURE, "N/A"),
		TupletCString(WEATHER_CONDITIONS, "Please wait..."),
		TupletCString(WEATHER_CITY, "N/A"),
		TupletInteger(WEATHER_ICON, (uint8_t) 'a')
	};

	// Allocate sync buffer based on estimate
	size_t bytes = (size_t) dict_calc_buffer_size_from_tuplets(initial_values, 4) * 2;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "malloc() of %d bytes for sync buffer", bytes);
	s_sync_buffer = (uint8_t*) malloc(bytes);

	// Begin using AppSync
	app_sync_init(&s_sync, s_sync_buffer, bytes, initial_values, ARRAY_LENGTH(initial_values), sync_changed_handler, sync_error_handler, NULL);

	// send an initial update message
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	int value = 0;
	dict_write_int(iterator, STATUS, &value, sizeof(int), true);
	app_message_outbox_send();

	// subscribe to update every minute
	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void deinit(void)
{
	tick_timer_service_unsubscribe();
	app_sync_deinit(&s_sync);
	window_stack_remove(window, true);
	window_destroy(window);
}

int main(void)
{
	init();
	app_event_loop();
	deinit();
}
