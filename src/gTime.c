#include <pebble.h>
#include "gTime.h"
static Window *window;
static TextLayer *wday_layer;
static TextLayer *date_layer;
static TextLayer *time_layer;
static TextLayer *week_layer;
static TextLayer *right_info_layer;
static TextLayer *left_info_layer;
static GBitmap *batt_state_unload;
static GBitmap *batt_state_load;
static BitmapLayer *img_layer;
//static InverterLayer *inv_img_layer;
static uint16_t tofd = 25; // tofd als Zahl fuer spaeter

// h = 168 | w = 144

void make_date(char **datum){
  uint16_t wday;
  uint16_t mon;
  
  wday = atoi(datum[1]);
  mon = atoi(datum[2]);

  app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Datum %d %d", wday, mon);

  strcpy(datum[1], weekday[wday]);
  strcpy(datum[2], month[mon - 1]);
   
}

void handle_battery(BatteryChargeState batt) {
  static char batt_text[] = "100% ";

  if(batt.is_charging){
    snprintf(batt_text, sizeof(batt_text), "%d%%", batt.charge_percent);
    bitmap_layer_set_bitmap(img_layer, batt_state_load);
  }
  else {
    snprintf(batt_text, sizeof(batt_text), "%d%%", batt.charge_percent);
    bitmap_layer_set_bitmap(img_layer, batt_state_unload);
  }

  text_layer_set_text(right_info_layer, batt_text);

}

void handle_minute_tick(struct tm *time_tick, TimeUnits units_changed) {
  static char time_text[] = "00:00";
  static char date_text[] = "00. Xxxxxxxxx";
  static char week_text[] = "Woche 00";
  static char h[] = "00";
  static char wday[11];
  static char week_format[] = "Woche %V";
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
  

  strftime(time_text, sizeof(time_text), time_format, time_tick);
  text_layer_set_text(time_layer, time_text);

  strftime(h, sizeof(h), "%H", time_tick);
  t = atoi(h);  

  if(tofd > t) { // Wenn der Tag neu anfaengt, dann Datum lesen...

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
    
    strcpy(date_text, datum[0]);  // Tag im Monat
    strcat(date_text, ". ");
    strcat(date_text, datum[2]);  // Monat in Worten
    strcpy(wday, datum[1]);       // Wochentag
  
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

  time_layer = text_layer_create(GRect(0, 59, bounds.size.w, 50));
  text_layer_set_background_color(time_layer, GColorBlack);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_layer));

  app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Uhrzeit Layer gebaut!");
  
  week_layer = text_layer_create(GRect(0, 110, bounds.size.w, 30));
  text_layer_set_background_color(week_layer, GColorBlack);
  text_layer_set_text_color(week_layer, GColorWhite);
  text_layer_set_font(week_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(week_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(week_layer));
  
  // Infozeile unten links
  left_info_layer = text_layer_create(GRect(0, (bounds.size.h - 20), (bounds.size.w / 2), 20));
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
  
  // Layer für das Bild links unten bauen
  img_layer = bitmap_layer_create(GRect(bounds.size.w - 20, bounds.size.h - 20, 20, 20));
  bitmap_layer_set_bitmap(img_layer, batt_state_load);
  layer_add_child(window_layer, bitmap_layer_get_layer(img_layer));
  
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  battery_state_service_subscribe(&handle_battery);
  handle_minute_tick(NULL, MINUTE_UNIT);
  handle_battery(battery_state_service_peek());
}

void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  text_layer_destroy(date_layer);
  text_layer_destroy(time_layer);
  text_layer_destroy(week_layer);
  text_layer_destroy(left_info_layer);
  gbitmap_destroy(batt_state_load);
  gbitmap_destroy(batt_state_unload);
  bitmap_layer_destroy(img_layer);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}