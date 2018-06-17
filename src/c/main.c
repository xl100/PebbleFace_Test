#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer, *s_weather_layer;
static Layer *s_watch_battery_layer, *s_phone_battery_layer;
static BitmapLayer *s_background_layer, *s_bt_icon_layer;
static GBitmap *s_background_bitmap, *s_bt_icon_bitmap;
static GFont s_time_font, s_weather_font;
static int s_watch_battery_level, s_phone_battery_level;

static void bluetooth_callback(bool connected) {
    // Show icon if disconnected
    layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);
    
    if(!connected) {
        // Issure a vibrating alert
        vibes_double_pulse();
    }
}

static void battery_proc(int width, GRect bounds, GContext *ctx) {
    // Draw the background
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    
    // Draw the bar
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void watch_battery_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    
    // Find the width of the bar (total width = 114)
    int width = (s_watch_battery_level * 115) / 100;
    
    battery_proc(width, bounds, ctx);
}

static void phone_battery_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    
    // Find the width of the bar (total width = 114)
    int width = (s_phone_battery_level * 115) / 100;
    
    battery_proc(width, bounds, ctx);
}

static void battery_callback(BatteryChargeState state) {
    // Record the new battery level
    s_watch_battery_level = state.charge_percent;
    
    // Update meter
    layer_mark_dirty(s_watch_battery_layer);
}

static void update_time() {
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    
    // Write the current hours and minutes into a buffer
    static char s_buffer[8];
    strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    
    // Display this time on the TextLayer
    text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
    
    // Get weather update every 30 minutes
    if(tick_time->tm_min % 30 == 0) {
        // Begin dictionary
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);
        
        // Add a key-value pair
        dict_write_uint8(iter, 0, 0);
        
        // Send the message!
        app_message_outbox_send();
    }
}

static void main_window_load(Window *window) {
    // Get information about the window
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    
    // Create GFont
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_48));
    
    // Create the TextLayer with specific bounds
    s_time_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(2, 0), PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
    
    // Create temperature Layer
    s_weather_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(125, 120), bounds.size.w, 25));
    
    // Import the layout to be more like a watchface
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorBlack);
    text_layer_set_font(s_time_layer, s_time_font);
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    
    // Style the text
    text_layer_set_background_color(s_weather_layer, GColorClear);
    text_layer_set_text_color(s_weather_layer, GColorWhite);
    text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
    text_layer_set_text(s_weather_layer, "Loading...");
    
    // Create second custom font, apply it and add to window
    s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_20));
    text_layer_set_font(s_weather_layer, s_weather_font);
    layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
    
    // Create GBitmap
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
    
    // Create BitmapLayer to display the gBitmap
    s_background_layer = bitmap_layer_create(bounds);
    
    // Set the bitmap onto the layer and add it to the window
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
    
    // Add it as a child layer to the Window's root layer
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
    
    // Create battery meter Layer
    s_watch_battery_layer = layer_create(GRect(PBL_IF_ROUND_ELSE(33, 14), PBL_IF_ROUND_ELSE(59, 53), 115, 2));
    layer_set_update_proc(s_watch_battery_layer, watch_battery_update_proc);
    
    s_phone_battery_layer = layer_create(GRect(PBL_IF_ROUND_ELSE(33, 14), PBL_IF_ROUND_ELSE(118, 112), 115, 2));
    layer_set_update_proc(s_phone_battery_layer, phone_battery_update_proc);
    
    // Add to Window
    layer_add_child(window_layer, s_watch_battery_layer);
    layer_add_child(window_layer, s_phone_battery_layer);
    
    // Create the bluetooth icon GBitmap
    s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);
    
    // Create the Bitmap to display the GBitmap
    s_bt_icon_layer = bitmap_layer_create(GRect(59, 12, 30, 30));
    bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_bt_icon_layer));
    
    // Show the correct state of the BT connection from the start
    bluetooth_callback(connection_service_peek_pebble_app_connection());
}

static void main_window_unload(Window *window) {
    // Destroy TextLayer
    text_layer_destroy(s_time_layer);
    
    // Unload GFont
    fonts_unload_custom_font(s_time_font);
    
    // Destroy Gbitmap
    gbitmap_destroy(s_background_bitmap);
    
    // Destroy BitmapLayer
    bitmap_layer_destroy(s_background_layer);
    
    // Destroy weather elements
    text_layer_destroy(s_weather_layer);
    fonts_unload_custom_font(s_weather_font);
    
    layer_destroy(s_watch_battery_layer);
    layer_destroy(s_phone_battery_layer);
    
    gbitmap_destroy(s_bt_icon_bitmap);
    bitmap_layer_destroy(s_bt_icon_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    // Store incoming information
    static char temperature_buffer[8];
    static char conditions_buffer[32];
    static char weather_layer_buffer[32];
    
    // Read tuples for data
    Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
    Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
    Tuple *battery_tuple = dict_find(iterator, MESSAGE_KEY_BATTERY);
    
    // If all data is available, use it
    if(temp_tuple && conditions_tuple) {
        snprintf(temperature_buffer, sizeof(temperature_buffer), "%dF", (int)temp_tuple->value->int32);
        snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
        
        // Assemble full string and display
        snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
        text_layer_set_text(s_weather_layer, weather_layer_buffer);
    }
    
    // Update Phone battery gauge
    if(battery_tuple) {
        s_phone_battery_level = (int)battery_tuple->value->int32;
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send success!");
}

static void init() {
    // Create main Window element and assign to pointer
    s_main_window = window_create();
    
    // Set handlers to manage the elements inside the Window
    window_set_background_color(s_main_window, GColorBlack);
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
    
    // Show the Window on the watch, with anaimated=true
    window_stack_push(s_main_window, true);
    
    // Register with TickTimerService
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    
    // Make sure the time is displayed from the start
    update_time();
    
    // Registor callback
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    
    // Open AppMessage
    const int inbox_size = 128;
    const int outbox_size = 128;
    app_message_open(inbox_size, outbox_size);
    
    // Registor for battery level updates
    battery_state_service_subscribe(battery_callback);
    
    // Ensure battery level is displayed from the start
    battery_callback(battery_state_service_peek());
    
    // Register for Bluetooth connection update
    connection_service_subscribe((ConnectionHandlers) {
        .pebble_app_connection_handler = bluetooth_callback
    });
}

static void deinit() {
    // Destroy Window
    window_destroy(s_main_window);
}

int main() {
    init();
    app_event_loop();
    deinit();
}