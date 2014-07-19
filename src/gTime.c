#include <pebble.h>
#include "gTime.h"

static Window *window;
static TextLayer *wday_layer;
static TextLayer *date_layer;
static TextLayer *time_layer;
static TextLayer *sec_layer;
static TextLayer *week_layer;
static TextLayer *right_info_layer;
static TextLayer *left_info_layer;
static GBitmap *batt_state_unload;
static GBitmap *batt_state_load;
static GBitmap *bt_state_con;
static GBitmap *bt_state_dis;
static BitmapLayer *img_right_info_layer;
static BitmapLayer *img_left_info_layer;
static int8_t tofd = -1; // tofd als Zahl fuer spaeter
static bool first_run = true; // Zur Initialisierung, am Ende von init auf false setzen

// h = 168 | w = 144

void make_date(char **datum){
  uint16_t wday;
  uint16_t mon;
  
  wday = atoi(datum[1]);
  mon = atoi(datum[2]);

  app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Datum %d %d", wday, mon);

  // Hier wird mit Absicht kein strncpy benutzt, da datum[] an der Stelle
  // zu wenig Zeichen enthaelt. Spaeter werden 11 Zeichen benoetigt, wie reserviert wurden!
  strcpy(datum[1], weekday[wday]);
  strcpy(datum[2], month[mon - 1]);
   
}

void handle_battery(BatteryChargeState batt) {
  static char batt_text[] = "100% ";

  if(batt.is_charging){
    snprintf(batt_text, sizeof(batt_text), "%d%%", batt.charge_percent);
    bitmap_layer_set_bitmap(img_right_info_layer, batt_state_load);
  }
  else {
    snprintf(batt_text, sizeof(batt_text), "%d%%", batt.charge_percent);
    bitmap_layer_set_bitmap(img_right_info_layer, batt_state_unload);
  }

  text_layer_set_text(right_info_layer, batt_text);

}

void handle_connection(bool con) {
  
  if(con) {
    bitmap_layer_set_bitmap(img_left_info_layer, bt_state_con);
    if(!first_run) {
      vibes_double_pulse();
    }
  }
  else {
    bitmap_layer_set_bitmap(img_left_info_layer, bt_state_dis);
    if(!first_run) {
      vibes_long_pulse();
    }
  }
}

void handle_second_tick(struct tm *time_tick, TimeUnits units_changed) {
  static char sec_text[] = "00";
  if(!time_tick) {
    time_t now = time(NULL);
    time_tick = localtime(&now);
  }
  
  strftime(sec_text, sizeof(sec_text), "%S", time_tick);
  text_layer_set_text(sec_layer, sec_text);
}

void handle_minute_tick(struct tm *time_tick, TimeUnits units_changed) {
  static char time_text[] = "00:00";
  static char sec_text[] = "00";
  static char date_text[] = "00. Xxxxxxxxx";
  static char week_text[] = "KW 00";
  static char d[] = "00";
  static char wday[11];
  static char week_format[] = "KW %V";
  char *time_format;
  char **datum; 
  uint16_t i = 0;
  uint16_t t = 0;

  if(!time_tick) {
    time_t now = time(NULL);
    time_tick = localtime(&now);
  }
 
  if(clock_is_24h_style()) {
    time_format = "%H:%M";
  }
  else {
    time_format = "%I:%M";
  }
  
  strftime(sec_text, sizeof(sec_text), "%S", time_tick);
  text_layer_set_text(sec_layer, sec_text);

  strftime(time_text, sizeof(time_text), time_format, time_tick);
  text_layer_set_text(time_layer, time_text);

  strftime(d, sizeof(d), "%d", time_tick);
  t = atoi(d);  

  if(tofd != t) { // only if the day changes, then we re-read the date

    tofd = t;
  
    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Speicher holen!");

    datum = (char **)malloc(3 * sizeof(char *));
    for(i = 0; i < 3; i++){
      datum[i] = (char *)malloc(11 * sizeof(char));
    }

    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Datum auslesen!");

    // Datum auslesen
    strftime(date_text, sizeof(date_text), "%d%w%m", time_tick); // Tag im Monat (01-31) + Tag in der Woche (00-06) So-Sa / Monat (01-12)
    
    // Woche auslesen
    strftime(week_text, sizeof(week_text), week_format, time_tick); // Woche nach ISO8601 
    text_layer_set_text(week_layer, week_text);
  
    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Datum gelesen! %s", date_text); //ddWmm

    memcpy(datum[0], date_text, 2);
    memcpy(datum[1], date_text+2, 1);
    memcpy(datum[2], date_text+3, 2);
  
    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Datum bauen! %s / %s / %s", datum[0], datum[1], datum[2]);
  
    make_date(datum);
  
    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Datum gebaut %s %s %s!", datum[0], datum[1], datum[2]);
    
    strncpy(date_text, datum[0], sizeof(date_text));  // Tag im Monat
    strncat(date_text, ". ", sizeof(date_text));
    strncat(date_text, datum[2], sizeof(date_text));  // Monat in Worten
    strncpy(wday, datum[1], sizeof(wday));       // Wochentag
  
    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Datum: %s, %s!", wday, date_text); 
  
    text_layer_set_text(wday_layer, wday);
    text_layer_set_text(date_layer, date_text);

    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Speicher freigeben "); 
    
    free(datum);
  }
}

void init(void) {
  window = window_create();
  window_stack_push(window, true);
  window_set_background_color(window, GColorBlack);

  app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Fenster gebaut!");

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer); // Groesse des obersten Layers holen

  app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Groesse geholt!");

  wday_layer = text_layer_create(GRect(0, 0, bounds.size.w, 30));
  text_layer_set_background_color(wday_layer, GColorBlack);
  text_layer_set_text_color(wday_layer, GColorWhite);
  text_layer_set_font(wday_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(wday_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(wday_layer));
  
  app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Wochentag Layer gebaut!");

  date_layer = text_layer_create(GRect(0, 31, bounds.size.w, 30));
  text_layer_set_background_color(date_layer, GColorBlack);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(date_layer));

  app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Datum Layer gebaut! %d %d", bounds.size.h, bounds.size.w);

  time_layer = text_layer_create(GRect(0, 55, bounds.size.w, 50));
  text_layer_set_background_color(time_layer, GColorBlack);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_layer));

  sec_layer = text_layer_create(GRect(0, 100, bounds.size.w, 20));
  text_layer_set_background_color(sec_layer, GColorBlack);
  text_layer_set_text_color(sec_layer, GColorWhite);
  text_layer_set_font(sec_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(sec_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(sec_layer));
  
  app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Uhrzeit Layer gebaut!");
  
  week_layer = text_layer_create(GRect(0, 120, bounds.size.w, 30));
  text_layer_set_background_color(week_layer, GColorBlack);
  text_layer_set_text_color(week_layer, GColorWhite);
  text_layer_set_font(week_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(week_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(week_layer));
  
  // Infozeile unten links
  left_info_layer = text_layer_create(GRect(21, (bounds.size.h - 20), (bounds.size.w / 2), 20));
  text_layer_set_background_color(left_info_layer, GColorBlack);
  text_layer_set_font(left_info_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_color(left_info_layer, GColorWhite);
  text_layer_set_text_alignment(left_info_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(left_info_layer));
  
  // Infozeile unten rechts
  right_info_layer = text_layer_create(GRect(bounds.size.w - 45, (bounds.size.h - 20), bounds.size.w - 21, 20));
  text_layer_set_background_color(right_info_layer, GColorBlack);
  text_layer_set_font(right_info_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_color(right_info_layer, GColorWhite);
  text_layer_set_text_alignment(right_info_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(right_info_layer));
  
  // Icon fuer den Akkuzustand laden
  batt_state_load = gbitmap_create_with_resource(RESOURCE_ID_IMG_ARROW_UP);
  batt_state_unload = gbitmap_create_with_resource(RESOURCE_ID_IMG_ARROW_DOWN);
  
  // Icon fuer den Bluetooth Verbindungszustand
  bt_state_con = gbitmap_create_with_resource(RESOURCE_ID_IMG_BT_CON);
  bt_state_dis = gbitmap_create_with_resource(RESOURCE_ID_IMG_BT_DIS);
  
  // Layer für das Bild links unten bauen
  img_left_info_layer = bitmap_layer_create(GRect(0, bounds.size.h - 20, 20, 20));
  bitmap_layer_set_bitmap(img_left_info_layer, bt_state_con);
  layer_add_child(window_layer, bitmap_layer_get_layer(img_left_info_layer));
  
  
  // Layer für das Bild rechts unten bauen
  img_right_info_layer = bitmap_layer_create(GRect(bounds.size.w - 20, bounds.size.h - 20, 20, 20));
  bitmap_layer_set_bitmap(img_right_info_layer, batt_state_load);
  layer_add_child(window_layer, bitmap_layer_get_layer(img_right_info_layer));
  
  tick_timer_service_subscribe(SECOND_UNIT, handle_minute_tick);
  //tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(&handle_connection);
  handle_minute_tick(NULL, MINUTE_UNIT);
  //handle_second_tick(NULL, SECOND_UNIT);
  handle_battery(battery_state_service_peek());
  handle_connection(bluetooth_connection_service_peek());
  
  first_run = false;
}

void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  text_layer_destroy(date_layer);
  text_layer_destroy(time_layer);
  text_layer_destroy(sec_layer);
  text_layer_destroy(week_layer);
  text_layer_destroy(left_info_layer);
  gbitmap_destroy(batt_state_load);
  gbitmap_destroy(batt_state_unload);
  gbitmap_destroy(bt_state_con);
  gbitmap_destroy(bt_state_dis);
  bitmap_layer_destroy(img_right_info_layer);
  bitmap_layer_destroy(img_left_info_layer);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}