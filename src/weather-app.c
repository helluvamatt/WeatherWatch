#include <pebble.h>

static Window *window;
TextLayer *text_date_layer;
TextLayer *text_time_layer;
TextLayer *text_temp_layer;
TextLayer *text_cond_layer;
TextLayer *text_city_layer;
GFont *font49;
GFont *font39;
GFont *font21;

static AppSync s_sync;
static uint8_t *s_sync_buffer;

#define NUMBER_OF_IMAGES 11
static GBitmap *image = NULL;
static BitmapLayer *image_layer;

const int IMAGE_RESOURCE_IDS[NUMBER_OF_IMAGES] = {
	RESOURCE_ID_ERROR,
	RESOURCE_ID_CLEAR_DAY,
	RESOURCE_ID_CLEAR_NIGHT,
	RESOURCE_ID_CLOUDY,
	RESOURCE_ID_FOG,
	RESOURCE_ID_PARTLY_CLOUDY_DAY,
	RESOURCE_ID_PARTLY_CLOUDY_NIGHT,
	RESOURCE_ID_RAIN,
	RESOURCE_ID_SLEET,
	RESOURCE_ID_SNOW,
	RESOURCE_ID_WIND
};

#define WEATHER_TEMPERATURE 0
#define WEATHER_CONDITIONS 1
#define WEATHER_CITY 2
#define WEATHER_ICON 3

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
		// figure out which resource to use
		int8_t id = new_tuple->value->int8;
		if (image != NULL) {
			gbitmap_destroy(image);
			layer_remove_from_parent(bitmap_layer_get_layer(image_layer));
			bitmap_layer_destroy(image_layer);
		}
		Layer *window_layer = window_get_root_layer(window);
		image = gbitmap_create_with_resource(IMAGE_RESOURCE_IDS[id]);
		image_layer = bitmap_layer_create(GRect(10, 92, 60, 60));
		bitmap_layer_set_bitmap(image_layer, image);
		layer_add_child(window_layer, bitmap_layer_get_layer(image_layer));
		break;
	}
}

static void sync_error_handler(DictionaryResult dict_error, AppMessageResult app_message_error, void *context)
{
	// TODO Need error handling logic
}

static void window_load(Window *window)
{
	Layer *window_layer = window_get_root_layer(window);

	font49 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49));
	font39 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_39));
	font21 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21));

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
}

static void window_unload(Window *window)
{
	// destroy the text layers - this is good
	text_layer_destroy(text_date_layer);
	text_layer_destroy(text_time_layer);
	text_layer_destroy(text_temp_layer);
	text_layer_destroy(text_cond_layer);
	text_layer_destroy(text_city_layer);

	// destroy the image layers
	gbitmap_destroy(image);
	layer_remove_from_parent(bitmap_layer_get_layer(image_layer));
	bitmap_layer_destroy(image_layer);

	// unload the fonts
	fonts_unload_custom_font(font49);
	fonts_unload_custom_font(font39);
	fonts_unload_custom_font(font21);
}

// show the date and time every minute
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed)
{
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
	// init app message
	app_message_open(app_message_outbox_size_maximum(), app_message_inbox_size_maximum());

	// Setup initial values
	Tuplet initial_values[] = {
		TupletCString(WEATHER_TEMPERATURE, "N/A"),
		TupletCString(WEATHER_CONDITIONS, "Please wait..."),
		TupletCString(WEATHER_CITY, "N/A"),
		TupletInteger(WEATHER_ICON, 0)
	};

	// Allocate sync buffer based on estimate
	s_sync_buffer = (uint8_t*) malloc(dict_calc_buffer_size_from_tuplets(initial_values, 4));

	// Begin using AppSync
	app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer), initial_values, ARRAY_LENGTH(initial_values), sync_changed_handler, sync_error_handler, NULL);

	// create window
	window = window_create();
	window_set_background_color(window, GColorBlack);
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	window_stack_push(window, true);

	// subscribe to update every minute
	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void deinit(void)
{
	tick_timer_service_unsubscribe();
	app_message_deregister_callbacks();
	window_stack_remove(window, true);
	window_destroy(window);
}

int main(void)
{
	init();
	app_event_loop();
	deinit();
}
