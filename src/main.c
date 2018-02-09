/*
    Quake Watchface for Pebble Time
    Version 1.1
    By: Justin Thompson / Antillian
    Twitter: @jthomp
    
    Quake® is a registered trademark of id Software, Inc.
    Quake is Copyright © 1996-1997 id Software, Inc.    
  
    DPQuake TrueType font license included in this project.
*/

#include <pebble.h>
#include <string.h>

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer, *s_battery_layer;
static BitmapLayer *s_background_layer, *s_health_icon_layer;
static GBitmap *battery_images[7];
static GBitmap *s_background_bitmap, *s_health_icon_bitmap;
static GFont s_time_font, s_date_font, s_battery_font;
static int s_battery_level;
static bool s_charging, s_connected;

static void alloc_battery_images() {
  battery_images[0] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEALTH_20_ICON);
  battery_images[1] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEALTH_40_ICON);
  battery_images[2] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEALTH_60_ICON);
  battery_images[3] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEALTH_80_ICON);
  battery_images[4] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEALTH_100_ICON);
  battery_images[5] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGING_ICON);
  battery_images[6] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NOT_CONNECTED_ICON);
}

// Helper to change the Ranger face based on battery level.
// The lower the level, the more "damage" the face takes.
static void handle_battery_level() {

  if (s_connected) {

    if (s_battery_level >= 80) {
      bitmap_layer_set_bitmap(s_health_icon_layer, battery_images[4]);   
    } else if (s_battery_level < 80 && s_battery_level >= 60) {
      bitmap_layer_set_bitmap(s_health_icon_layer, battery_images[3]);
    } else if (s_battery_level < 60 && s_battery_level >= 40) {
      bitmap_layer_set_bitmap(s_health_icon_layer, battery_images[2]);     
    } else if (s_battery_level < 40 && s_battery_level >= 20 ) {
      bitmap_layer_set_bitmap(s_health_icon_layer, battery_images[1]);      
    } else {
      bitmap_layer_set_bitmap(s_health_icon_layer, battery_images[0]);
    }
    
  } else {
    bitmap_layer_set_bitmap(s_health_icon_layer, battery_images[6]);
  }
  
  if (s_charging) {
    bitmap_layer_set_bitmap(s_health_icon_layer, battery_images[5]);  
  }
  
  if (s_battery_level < 30) {
    text_layer_set_text_color(s_battery_layer, GColorRed);
  } else {
    text_layer_set_text_color(s_battery_layer, GColorWhite);
  }
}

static void bluetooth_callback(bool connected) {
  s_connected = connected;
  
  if (!connected) {    
    // Alert the user.
    vibes_double_pulse();
  }
  
  // Manually update the Ranger face here to make sure
  // if we're connected, we update the face immediately.
  handle_battery_level();
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  
  // Handle any changes to the battery level color and face
  s_charging = state.is_charging;
  handle_battery_level();
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
  
  // Copy date into the buffer from tm structure
  static char d_buffer[16];
  strftime(d_buffer, sizeof(d_buffer), "%m/%d", tick_time);
  
  // Display the date on the Date TextLayer
  text_layer_set_text(s_date_layer, d_buffer);
  
  static char b_buffer[4];
  // Write the current battery percentage into a buffer
  snprintf(b_buffer, sizeof(b_buffer), "%d", s_battery_level);
  
  handle_battery_level();
  
  // Display the battery percentage on the Battery TextLayer
  text_layer_set_text(s_battery_layer, b_buffer);  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  // Get information about the window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create GBitmap
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_QUAKE_LOGO_2);  
  
  // Create BitmapLayer to display the GBitmap
  s_background_layer = bitmap_layer_create(bounds);
  
  // Set the bitmap onto the layer and add to the window
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
  
  // Create the time TextLayer with specific bounds
  s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(64, 56), bounds.size.w, 50));  
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Create GFont for Time
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DP_QUAKE_42));
  
  // Add time TextLayer layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer)); 
  
  // Create date TextLayer
  s_date_layer = text_layer_create(GRect(-6, 150, 144, 30));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  
  // Create GFont for Date
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DP_QUAKE_16));  
  
  // Add date TextLayer to Window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  
  // Create the battery TextLayer
  s_battery_layer = text_layer_create(GRect(25, 150, 144, 30));
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentLeft);
  
  // Add battery TextLayer to Window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_layer));
  
  // Create GFont for Battery
  s_battery_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DP_QUAKE_16));
  
  // Apply GFont for Time to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);
   
  // Apply GFont for Date to TextLayer
  text_layer_set_font(s_date_layer, s_date_font);
  
  // Apply GFont for Battery to TextLayer
  text_layer_set_font(s_battery_layer, s_battery_font);
  
  // Show the correct state of the BT connection from the start
  bluetooth_callback(connection_service_peek_pebble_app_connection());
  
  // Create the Health icon GBitmap
  s_health_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEALTH_100_ICON);
  
  // Create the BitmapLayer to display the GBitmap for Health
  s_health_icon_layer = bitmap_layer_create(GRect(3, 151, 18, 16));
  bitmap_layer_set_bitmap(s_health_icon_layer, s_health_icon_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_health_icon_layer));
}

static void main_window_unload(Window *window) {
  
  // Unsubscribe from our services.
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  
  // Destroy TextLayer for Time
  text_layer_destroy(s_time_layer);
  
  // Destroy TextLayer for Date
  text_layer_destroy(s_date_layer);
  
  // Destroy TextLayer for Battery
  text_layer_destroy(s_battery_layer);
  
  // Unload GFont for Time
  fonts_unload_custom_font(s_time_font);
  
  // Unload GFont for Date
  fonts_unload_custom_font(s_date_font);
  
  // Unload GFont for Battery
  fonts_unload_custom_font(s_battery_font);
  
  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);
  
  // Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  
  // Destroy BitmapLayer for the Health icon
  gbitmap_destroy(s_health_icon_bitmap);
  bitmap_layer_destroy(s_health_icon_layer);
}

static void init() {
  // Create main window element and assign to pointer
  s_main_window = window_create();
  
  // Set the background color
  window_set_background_color(s_main_window, GColorBlack);
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true); 
  
  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  
  // Ensure battery level is displayed from the start.
  battery_callback(battery_state_service_peek());
  
  // Register for the Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
  
  alloc_battery_images();
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}