#include <pebble.h>

static Window *window;
TextLayer *text_date_layer;
TextLayer *text_time_layer;
TextLayer *text_temp_layer;
TextLayer *text_cond_layer;
TextLayer *text_city_layer;
BitmapLayer *icon_layer;
GFont *font_time;
GFont *font_temp;
GFont *font_large;
GFont *font_normal;
GFont *font_small;
GFont *icons;

static AppSync s_sync;
static uint8_t *s_sync_buffer;

#define WEATHER_TEMPERATURE 0
#define WEATHER_CONDITIONS 1
#define WEATHER_CITY 2
#define WEATHER_ICON 3
#define STATUS 4

static GBitmap *icon_bitmap = NULL;

#define FIRST_ICON_VALUE 'a'
const int ICON_RES_IDS[] = {

		RESOURCE_ID_IC_REFRESH,				// a
		RESOURCE_ID_IC_ERROR,				// b
		RESOURCE_ID_IC_CLEAR_DAY,			// c
		RESOURCE_ID_IC_CLEAR_NIGHT,			// d
		RESOURCE_ID_IC_FOG,					// e
		RESOURCE_ID_IC_WIND,				// f
		RESOURCE_ID_IC_COLD,				// g
		RESOURCE_ID_IC_PARTLY_CLOUDY_DAY,	// h
		RESOURCE_ID_IC_PARTLY_CLOUDY_NIGHT,	// i
		RESOURCE_ID_IC_FOG_ALT,				// j
		RESOURCE_ID_IC_CLOUDY,				// k
		RESOURCE_ID_IC_STORM,				// l
		RESOURCE_ID_IC_LIGHT_RAIN,			// m
		RESOURCE_ID_IC_RAIN,				// n
		RESOURCE_ID_IC_SNOW,				// o
		RESOURCE_ID_IC_LIGHT_SNOW,			// p
		RESOURCE_ID_IC_HEAVY_SNOW,			// q
		RESOURCE_ID_IC_HAIL_SLEET,			// r
		RESOURCE_ID_IC_MOSTLY_CLOUDY,		// s
		RESOURCE_ID_IC_HEAVY_STORM,			// t
		RESOURCE_ID_IC_HOT,					// u
		RESOURCE_ID_IC_NA,					// v

};

// char to icon
static void set_icon(char c)
{
	if (icon_bitmap != NULL)
	{
		gbitmap_destroy(icon_bitmap);
	}

	icon_bitmap = gbitmap_create_with_resource(ICON_RES_IDS[c-FIRST_ICON_VALUE]);
	bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
}

static void sync_changed_handler(const uint32_t key, const Tuple *new_tuple, const Tuple *old_tuple, void *context)
{
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
			set_icon((char) new_tuple->value->uint8);
			break;
	}
}

static void sync_error_handler(DictionaryResult dict_error, AppMessageResult app_message_error, void *context)
{
	APP_LOG(APP_LOG_LEVEL_ERROR, "sync_error_handler() called: dict_error = %d  app_message_error = %d", dict_error, app_message_error);
	text_layer_set_text(text_cond_layer, "Error");
	set_icon('b');
}

static void window_load(Window *window)
{
	Layer *window_layer = window_get_root_layer(window);

	font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49));
	font_temp = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_39));
	font_large = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_24));
	font_normal = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21));
	font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_18));

	// create date layer - this is where the date goes
	text_date_layer = text_layer_create(GRect(0, 0, 144, 24));
	text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);
	text_layer_set_text_color(text_date_layer, GColorWhite);
	text_layer_set_background_color(text_date_layer, GColorClear);
	text_layer_set_font(text_date_layer, font_normal);
	layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

	// create time layer - this is where time goes
	text_time_layer = text_layer_create(GRect(0, 24, 144, 64));
	text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
	text_layer_set_text_color(text_time_layer, GColorWhite);
	text_layer_set_background_color(text_time_layer, GColorClear);
	text_layer_set_font(text_time_layer, font_time);
	layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

	// create temperature layer - this is where the temperature goes
	text_temp_layer = text_layer_create(GRect(60, 88, 84, 40));
	text_layer_set_text_color(text_temp_layer, GColorBlack);
	text_layer_set_background_color(text_temp_layer, GColorWhite);
	text_layer_set_font(text_temp_layer, font_large);
	layer_add_child(window_layer, text_layer_get_layer(text_temp_layer));

	// create conditions layer
	text_cond_layer = text_layer_create(GRect(60, 128, 84, 20));
	text_layer_set_text_color(text_cond_layer, GColorBlack);
	text_layer_set_background_color(text_cond_layer, GColorWhite);
	text_layer_set_font(text_cond_layer, font_small);
	layer_add_child(window_layer, text_layer_get_layer(text_cond_layer));

	// create city name layer
	text_city_layer = text_layer_create(GRect(0, 148, 144, 20));
	text_layer_set_text_color(text_city_layer, GColorBlack);
	text_layer_set_background_color(text_city_layer, GColorWhite);
	text_layer_set_font(text_city_layer, font_small);
	layer_add_child(window_layer, text_layer_get_layer(text_city_layer));

	// create icon layer
	icon_layer = bitmap_layer_create(GRect(0, 88, 60, 60));
	bitmap_layer_set_alignment(icon_layer, GAlignCenter);
	bitmap_layer_set_background_color(icon_layer, GColorWhite);
	layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));
	set_icon('a');
}

static void window_unload(Window *window)
{
	// destroy the text layers - this is good
	text_layer_destroy(text_date_layer);
	text_layer_destroy(text_time_layer);
	text_layer_destroy(text_temp_layer);
	text_layer_destroy(text_cond_layer);
	text_layer_destroy(text_city_layer);
	bitmap_layer_destroy(icon_layer);

	// unload the fonts
	fonts_unload_custom_font(font_time);
	fonts_unload_custom_font(font_temp);
	fonts_unload_custom_font(font_large);
	fonts_unload_custom_font(font_normal);
	fonts_unload_custom_font(font_small);
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
