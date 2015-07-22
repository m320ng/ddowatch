#include <pebble.h>

enum {
  KEY_ACTION = 0,
  KEY_WEATHER_TEMP = 1,
  KEY_WEATHER_COND = 2,
  KEY_WEATHER_POS = 3,
  KEY_WEATHER_OVER = 4,
  KEY_CUSTOM_HOME = 5,
  KEY_CUSTOM_MSG = 6,
  KEY_NAVI_LAT = 11,
  KEY_NAVI_LNG = 12
};

static uint16_t accel_tap_lasttime = 0;
static int accel_tap_check = 0;
static int accel_tap_tick = 0;
  
static Layer *s_overlay_layer;
static bool s_overlay_running = true;
static int s_overlay_index = 0;

static bool s_night_mode = false;

static char s_time_text[] = "00";
static uint32_t const segments[] = { 200, 100, 400 };

static GBitmap *s_ddochi_body_bitmap;
static GBitmap *s_ddochi_head1_bitmap;
static GBitmap *s_ddochi_eye_bitmap;
static GBitmap *s_time_bg_bitmap;
static GBitmap *s_sec_bg_bitmap;
static GBitmap *s_custom_msg_bg_bitmap;

static GBitmap *s_bluetooth_bitmap;
static GBitmap *s_batt_full_bitmap;
static GBitmap *s_batt_empty_bitmap;
static GBitmap *s_batt_25_bitmap;
static GBitmap *s_batt_50_bitmap;
static GBitmap *s_batt_75_bitmap;
static GBitmap *s_batt_charge_bitmap;

static bool s_is_rainy = false;
static int s_js_ready =0;

static Window *s_main_window;

static InverterLayer *s_inverter_layer;

static BitmapLayer *s_ddochi_body_layer;
static BitmapLayer *s_time_bg_layer;
static BitmapLayer *s_sec_bg_layer;
static BitmapLayer *s_custom_msg_bg_layer;

static BitmapLayer *s_bluetooth_layer;
static BitmapLayer *s_batt_layer;

static TextLayer *s_date_layer;
static TextLayer *s_time_layer;
static TextLayer *s_time_sec_layer;
//static TextLayer *s_battery_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_custom_layer;
static TextLayer *s_custom_msg_layer;

static bool g_custom_visible = false; 
static int g_custom_tick_count = 0; 
static int g_custom_timeout = 0;

static char g_pos_lat[18];
static char g_pos_lng[18];

AppTimer *g_eye_timer;
static int g_eye_tick_count = 0; 
static int g_eye_tick_delta = 250; 

static bool s_ddochi_tick = false;

static int g_min_tick_count = 0; 

AppTimer *g_js_ready_check_timer;

static void put_weather_service() {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    dict_write_cstring(iter, KEY_ACTION, "weather");
    dict_write_cstring(iter, KEY_NAVI_LAT, g_pos_lat);
    dict_write_cstring(iter, KEY_NAVI_LNG, g_pos_lng);

    app_message_outbox_send();
}

static void put_custom_service() {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    dict_write_cstring(iter, KEY_ACTION, "custom");
    dict_write_cstring(iter, KEY_NAVI_LAT, g_pos_lat);
    dict_write_cstring(iter, KEY_NAVI_LNG, g_pos_lng);

    app_message_outbox_send();
}

static void put_all_service() {
  APP_LOG(APP_LOG_LEVEL_INFO, "put_all_service");
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  dict_write_cstring(iter, KEY_ACTION, "all");
  dict_write_cstring(iter, KEY_NAVI_LAT, g_pos_lat);
  dict_write_cstring(iter, KEY_NAVI_LNG, g_pos_lng);

  app_message_outbox_send();
}

static void put_ready_service() {
  APP_LOG(APP_LOG_LEVEL_INFO, "put_ready_service");
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  dict_write_cstring(iter, KEY_ACTION, "ready");
  
  app_message_outbox_send();
}

void put_all_service_callback(void *data) {
  put_all_service();
}

static void overlay_update_proc(Layer *layer, GContext *ctx) {
  
  graphics_context_set_stroke_color(ctx, GColorBlack);
  
  // 초
  //s_sec_bg_layer = bitmap_layer_create(GRect(83, 82, 17, 14));
  //bitmap_layer_set_bitmap(s_sec_bg_layer, s_sec_bg_bitmap);
  graphics_draw_bitmap_in_rect(ctx, s_sec_bg_bitmap, GRect(83, 82, 17, 14));
  graphics_draw_text(ctx, s_time_text, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(86, 80, 14, 14), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  
  // 또치머리
  if (s_ddochi_tick) {
  } else {
    GSize size =  s_ddochi_head1_bitmap->bounds.size;
    graphics_draw_bitmap_in_rect(ctx, s_ddochi_head1_bitmap, GRect(14, 24, size.w, size.h));
  }
  
  // 또치눈
  if (g_eye_tick_count > 0 && g_eye_tick_count--) {
    if (g_eye_tick_count % 2 == 0) {
      GSize size =  s_ddochi_eye_bitmap->bounds.size;
      if (s_ddochi_tick) {
        graphics_draw_bitmap_in_rect(ctx, s_ddochi_eye_bitmap, GRect(28, 57, size.w, size.h));
      } else {
        graphics_draw_bitmap_in_rect(ctx, s_ddochi_eye_bitmap, GRect(26, 57, size.w, size.h));
      }
    }
  }
  
  // 기상
  graphics_context_set_stroke_color(ctx, GColorBlack);
  static int rainy1[12] = {17,33, 64,15, 25,98, 74,85, 141,86, 106,115};
  static int rainy2[12] = {27,12, 22,94, 66,80, 141,61, 43,116, 136,116};
  s_overlay_index++;
  if (s_is_rainy) {
    GPoint point = {
      .x = 12,
      .y = 0,
    };
    GPoint line = {
      .x = 0,
      .y = 40,
    };
    if (s_overlay_index % 2 == 0) {
      for (int i=0; i<6; i++) {
        int x = rainy1[i*2];
        int y = rainy1[i*2+1];
        graphics_draw_line(ctx, GPoint(point.x + x, point.y + y), GPoint(line.x + x, line.y + y));
      }
    } else {
      for (int i=0; i<6; i++) {
        int x = rainy2[i*2];
        int y = rainy2[i*2+1];
        graphics_draw_line(ctx, GPoint(point.x + x, point.y + y), GPoint(line.x + x, line.y + y));
      }
    }
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  static char action_buff[10];
  static char weather_temp_buff[8];
  static char weather_cond_buff[32];
  static char weather_pos_buff[32];
  static char weather_layer_buff[128];
  static char custom_home_buff[8] = {0,};
  static char custom_msg_buff[256] = {0,};
  static int weather_over = 0;
    
  Tuple *t = dict_read_first(iterator);

  while(t != NULL) {
    switch(t->key) {
      case KEY_ACTION:
        snprintf(action_buff, sizeof(action_buff), "%s", t->value->cstring);
        break;
      case KEY_WEATHER_TEMP:
        snprintf(weather_temp_buff, sizeof(weather_temp_buff), "%sC", t->value->cstring);
        break;
      case KEY_WEATHER_COND:
        snprintf(weather_cond_buff, sizeof(weather_cond_buff), "%s", t->value->cstring);
        break;
      case KEY_WEATHER_OVER:
        weather_over = t->value->int32;
        break;
      case KEY_WEATHER_POS:
        snprintf(weather_pos_buff, sizeof(weather_pos_buff), "%s", t->value->cstring);
        break;
      case KEY_CUSTOM_HOME:
        snprintf(custom_home_buff, sizeof(custom_home_buff), "%s", t->value->cstring);
        break;
      case KEY_CUSTOM_MSG:
        snprintf(custom_msg_buff, sizeof(custom_msg_buff), "%s", t->value->cstring);
        break;
      case KEY_NAVI_LAT:
        snprintf(g_pos_lat, sizeof(g_pos_lat), "%s", t->value->cstring);
        persist_write_string(KEY_NAVI_LAT, t->value->cstring);
        break;
      case KEY_NAVI_LNG:
        snprintf(g_pos_lng, sizeof(g_pos_lng), "%s", t->value->cstring);
        persist_write_string(KEY_NAVI_LNG, t->value->cstring);
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
        break;
    }

    t = dict_read_next(iterator);
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "action_buff:%s", action_buff);
  
  if (strcmp(action_buff, "weather")==0) {
    snprintf(weather_layer_buff, sizeof(weather_layer_buff), "%s %s %s", weather_temp_buff, weather_cond_buff, weather_pos_buff);
    text_layer_set_text(s_weather_layer, weather_layer_buff);
    if (weather_over > 0) {
      s_is_rainy = true;
    } else {
      s_is_rainy = false;
    }
  } else if (strcmp(action_buff, "custom")==0) {
    APP_LOG(APP_LOG_LEVEL_INFO, "custom_home_buff:%s", custom_home_buff);
    
    text_layer_set_text(s_custom_msg_layer, custom_msg_buff);
    text_layer_set_text(s_custom_layer, custom_home_buff);
  } else if (strcmp(action_buff, "ready")==0) {
    s_js_ready = 1;
    put_weather_service();
    //app_timer_register(5000, (AppTimerCallback) put_all_service_callback, NULL);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed! %d", reason);
  APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessageResult1 %d", APP_MSG_SEND_TIMEOUT);
  APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessageResult2 %d", APP_MSG_SEND_REJECTED);
  Tuple *t = dict_read_first(iterator);
  APP_LOG(APP_LOG_LEVEL_INFO, "failed-value: %s", t->value->cstring);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void handle_battery(BatteryChargeState charge_state) {
  //static char battery_text[] = "100% charged";

  if (charge_state.is_charging) {
    //snprintf(battery_text, sizeof(battery_text), "charging");
    bitmap_layer_set_bitmap(s_batt_layer, s_batt_charge_bitmap);
  } else {
    //snprintf(battery_text, sizeof(battery_text), "%d%% charged", charge_state.charge_percent);
    if (charge_state.charge_percent > 90) {
      bitmap_layer_set_bitmap(s_batt_layer, s_batt_full_bitmap);
    } else if (charge_state.charge_percent > 60) {
      bitmap_layer_set_bitmap(s_batt_layer, s_batt_75_bitmap);
    } else if (charge_state.charge_percent > 30) {
      bitmap_layer_set_bitmap(s_batt_layer, s_batt_50_bitmap);
    } else if (charge_state.charge_percent > 10) {
      bitmap_layer_set_bitmap(s_batt_layer, s_batt_25_bitmap);
    } else {
      bitmap_layer_set_bitmap(s_batt_layer, s_batt_empty_bitmap);
    }
    
    if (!s_night_mode) {
      if (charge_state.charge_percent > 10) {
        if (s_overlay_running==false) {
          s_overlay_running = true;
          layer_set_update_proc(s_overlay_layer, overlay_update_proc);
        }
      } else {
        if (s_overlay_running==true) {
          s_overlay_running = false;
          layer_set_update_proc(s_overlay_layer, NULL);
        }
      }
    }
  }
  //snprintf(battery_text, sizeof(battery_text), "%d", charge_state.charge_percent);
  //text_layer_set_text(s_battery_layer, battery_text);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";
  static char date_buff[] = "00/00";

  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }
  text_layer_set_text(s_time_layer, buffer);
  
  strftime(date_buff, sizeof(date_buff), "%m/%d", tick_time);
  text_layer_set_text(s_date_layer, date_buff);
  
  if (tick_time->tm_hour >= 1 && tick_time->tm_hour <= 7) {
    s_night_mode = true;
    if (s_overlay_running) {
      s_overlay_running = false;
      layer_set_update_proc(s_overlay_layer, NULL);
    }
  } else {
    s_night_mode = false;
    if (!s_overlay_running) {
      s_overlay_running = true;
      layer_set_update_proc(s_overlay_layer, overlay_update_proc);
    }
  }
}

void eye_timer_callback(void *data) {
  //if (g_eye_tick_count > 0 && g_eye_tick_count--) {
    //layer_set_hidden((Layer*)s_ddochi_eye_layer, g_eye_tick_count % 2 == 0);
  //}
  //if (s_is_rainy) {
    layer_mark_dirty(s_overlay_layer);
  //}
  g_eye_timer = app_timer_register(g_eye_tick_delta, (AppTimerCallback) eye_timer_callback, NULL);
}

static void handle_timer_tick(struct tm* tick_time, TimeUnits units_changed) {
  if( (units_changed & SECOND_UNIT) != 0 ) {
    // sec
    strftime(s_time_text, sizeof(s_time_text), "%S", tick_time);
    //text_layer_set_text(s_time_sec_layer, s_time_text);
  
    // 까닥까딱
    s_ddochi_tick = tick_time->tm_sec % 2 == 0;
  
    // 껌뻑껌뻑
    if (g_eye_tick_count <= 0 && tick_time->tm_sec % 5 == 0) {
      g_eye_tick_count = 4;
    }
    
    // custom 메세지
    if (g_custom_tick_count > 0 && g_custom_tick_count--) {
      if (g_custom_tick_count==0) {
        layer_set_hidden((Layer*)s_custom_msg_layer, true);
        layer_set_hidden((Layer*)s_custom_msg_bg_layer, true);
        g_custom_visible = false;
        g_custom_timeout = 3;
        accel_tap_check = 0;
      }
    }
    if (g_custom_timeout > 0) {
      g_custom_timeout--;
    }
    if (accel_tap_check==1) {
      accel_tap_tick++;
      if (accel_tap_tick>=3) {
        accel_tap_check = 0;
        accel_tap_tick = 0;
      }
    } else {
      accel_tap_tick = 0;
    }
    
    //layer_mark_dirty(s_overlay_layer);
  }  
  
  if( (units_changed & MINUTE_UNIT) != 0 ) {
    g_min_tick_count++;
    
    update_time();
    
    handle_battery(battery_state_service_peek());
    
    // 30 min
    if(g_min_tick_count % 30 == 0) {
      if (s_night_mode) {
        put_weather_service();
      }
    }
    
    // 10 min
    if(g_min_tick_count % 10 == 0) {
      if (!s_night_mode) {
        put_weather_service();
      }
    }
  }
}

static void handle_bluetooth(bool connected) {
  if (connected) {
    layer_set_hidden((Layer *)s_bluetooth_layer, false);
    VibePattern pat = {
      .durations = segments,
      .num_segments = ARRAY_LENGTH(segments),
    };
    vibes_enqueue_custom_pattern(pat);    
    //put_all_service();
  } else {
    layer_set_hidden((Layer *)s_bluetooth_layer, true);
    VibePattern pat = {
      .durations = segments,
      .num_segments = ARRAY_LENGTH(segments),
    };
    vibes_enqueue_custom_pattern(pat);    
    //vibes_short_pulse();
  }
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction)
{
  //if (s_night_mode) return;

  if (axis == ACCEL_AXIS_X) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "axis-x : %d", (int)direction);
  } else if (axis == ACCEL_AXIS_Y) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "axis-y : %d", (int)direction);
  } else if (axis == ACCEL_AXIS_Z) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "axis-z : %d", (int)direction);
  }
  
  /*
  if (direction==accel_tap_direction) {
    accel_tap_lasttime = 0;
    accel_tap_check = 0;
    accel_tap_direction = 0;
    return;
  }
  
  accel_tap_direction = direction;
  accel_tap_check++;
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "accel_tap_check : %d", (int)accel_tap_check);
  
  //accel_tap_lasttime = curtime;
  if (accel_tap_check < 2) {
    return;
  }
  
  accel_tap_lasttime = 0;
  accel_tap_check = 0;
  accel_tap_direction = 0;
  */
  
  /*
  static char date_buff[] = "0000";
  snprintf(date_buff, sizeof("0000"), "%d", (int)direction);
  text_layer_set_text(s_date_layer, date_buff);
  */
  
  if (g_custom_visible) {
    layer_set_hidden((Layer*)s_custom_msg_layer, true);
    layer_set_hidden((Layer*)s_custom_msg_bg_layer, true);
    g_custom_visible = false;
    g_custom_tick_count = 0;
    g_custom_timeout = 3;
    accel_tap_check = 0;
  } else {
    /*
    const char* home = text_layer_get_text(s_custom_layer);
    if (strcmp(home, "HOME")==0) {
    } else if (strcmp(home, "WORK")==0) {
    } else {
    }
    */
    APP_LOG(APP_LOG_LEVEL_DEBUG, "accel_tap_check : %d", (int)accel_tap_check);
    
    if (g_custom_timeout > 0) {
      layer_set_hidden((Layer*)s_custom_msg_layer, false);
      layer_set_hidden((Layer*)s_custom_msg_bg_layer, false);
      g_custom_visible = true;
      g_custom_tick_count = 10;
      return;
    }
    
    accel_tap_check++;
    if (accel_tap_check<2) {
      return;
    }  
    
    //if (strcmp(text_layer_get_text(s_custom_msg_layer), "")!=0) {
      text_layer_set_text(s_custom_msg_layer, "Loading..");
      layer_set_hidden((Layer*)s_custom_msg_layer, false);
      layer_set_hidden((Layer*)s_custom_msg_bg_layer, false);
      g_custom_visible = true;
      g_custom_tick_count = 10;
      accel_tap_check = 0;
      put_custom_service();
    //}
  }
}

static void main_window_load(Window *window) {
  Layer *root_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root_layer);
  
  int time_top = 20;
  
  s_inverter_layer = inverter_layer_create(GRect(0, 0, 144, 168));
  
  // Create GFont
  //s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_20));
  //s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_12));
                                                                    
  // Create GBitmap
  s_ddochi_body_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DDOCHI_BODY1);
  s_ddochi_head1_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DDOCHI_HEAD1);
  s_ddochi_eye_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DDOCHI_EYE);
  
  s_time_bg_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TALKING_TIME);
  s_sec_bg_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SEC_BG);
  s_custom_msg_bg_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CUSTOM_MSG_BG);
  
  s_bluetooth_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_ICON);
  s_batt_full_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_FULL_ICON);
  s_batt_empty_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_EMPTY_ICON);
  s_batt_charge_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_CHG_ICON);
  s_batt_25_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_25_ICON);
  s_batt_50_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_50_ICON);
  s_batt_75_bitmap =  gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_75_ICON);
 
  //s_rain1_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_OVER_RAIN1);
  //s_rain2_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_OVER_RAIN2);
  
  //then set to created BitmapLayer
  
  // 또치
  s_ddochi_body_layer = bitmap_layer_create(GRect(14, 24, 109, 143));
  bitmap_layer_set_bitmap(s_ddochi_body_layer, s_ddochi_body_bitmap);
  layer_add_child(root_layer, bitmap_layer_get_layer(s_ddochi_body_layer));
  /*

  s_ddochi_body2_layer = bitmap_layer_create(GRect(14, 24, 109, 143));
  bitmap_layer_set_bitmap(s_ddochi_body2_layer, s_ddochi_body2_bitmap);
  layer_set_hidden((Layer*)s_ddochi_body2_layer, true);
  
  s_ddochi_eye_layer = bitmap_layer_create(GRect(26, 57, 43, 7));
  bitmap_layer_set_bitmap(s_ddochi_eye_layer, s_ddochi_eye_bitmap);
  layer_set_hidden((Layer*)s_ddochi_eye_layer, true);
  */
  
  //layer_add_child(s_ddochi_layer, bitmap_layer_get_layer(s_ddochi_body2_layer));
  //layer_add_child(s_ddochi_layer, bitmap_layer_get_layer(s_ddochi_eye_layer));
  //layer_add_child(root_layer, s_ddochi_layer);
  
  // 오버레이
  s_overlay_layer = layer_create(bounds);
  layer_set_update_proc(s_overlay_layer, overlay_update_proc);
  layer_add_child(root_layer, s_overlay_layer);
  
  // 상단상태바
  s_bluetooth_layer = bitmap_layer_create(GRect(132, 3, 9, 12));
  bitmap_layer_set_bitmap(s_bluetooth_layer, s_bluetooth_bitmap);
  s_batt_layer = bitmap_layer_create(GRect(121, 3, 9, 12));
  bitmap_layer_set_bitmap(s_batt_layer, s_batt_full_bitmap);
  layer_add_child(root_layer, bitmap_layer_get_layer(s_bluetooth_layer));
  layer_add_child(root_layer, bitmap_layer_get_layer(s_batt_layer));
  
  s_weather_layer = text_layer_create(GRect(2, 0, 125, 16));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorBlack);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentLeft);
  text_layer_set_text(s_weather_layer, "Loading..");
  //text_layer_set_font(s_weather_layer, s_weather_font);
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(root_layer, text_layer_get_layer(s_weather_layer));
  
  // 하단상태바
  s_custom_layer = text_layer_create(GRect(92, 152, 50, 14));
  text_layer_set_background_color(s_custom_layer, GColorClear);
  text_layer_set_text_color(s_custom_layer, GColorBlack);
  text_layer_set_font(s_custom_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_custom_layer, GTextAlignmentRight);
  layer_add_child(root_layer, text_layer_get_layer(s_custom_layer));
  
  // 시간
  s_time_bg_layer = bitmap_layer_create(GRect(86, time_top + 3, 55, 30));
  bitmap_layer_set_bitmap(s_time_bg_layer, s_time_bg_bitmap);
  //s_sec_bg_layer = bitmap_layer_create(GRect(83, 82, 17, 14));
  //bitmap_layer_set_bitmap(s_sec_bg_layer, s_sec_bg_bitmap);
  
  s_date_layer = text_layer_create(GRect(84, time_top + 27, 55, 14));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_text(s_date_layer, "00/00");
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  
  s_time_layer = text_layer_create(GRect(86, time_top, 55, 30));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  //text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  /*
  s_time_sec_layer = text_layer_create(GRect(86, 80, 14, 14));
  text_layer_set_background_color(s_time_sec_layer, GColorClear);
  text_layer_set_text_color(s_time_sec_layer, GColorWhite);
  text_layer_set_font(s_time_sec_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_time_sec_layer, GTextAlignmentCenter);
  */
  layer_add_child(root_layer, bitmap_layer_get_layer(s_time_bg_layer));
  //layer_add_child(root_layer, bitmap_layer_get_layer(s_sec_bg_layer));
  layer_add_child(root_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(root_layer, text_layer_get_layer(s_time_layer));
  //layer_add_child(root_layer, text_layer_get_layer(s_time_sec_layer));
  
  // 팝업
  s_custom_msg_bg_layer = bitmap_layer_create(GRect(0, 52, 144, 116));
  bitmap_layer_set_bitmap(s_custom_msg_bg_layer, s_custom_msg_bg_bitmap);
  layer_set_hidden((Layer*)s_custom_msg_bg_layer, true);
  
  s_custom_msg_layer = text_layer_create(GRect(2, 54, 140, 114));
  text_layer_set_background_color(s_custom_msg_layer, GColorClear);
  text_layer_set_text_color(s_custom_msg_layer, GColorBlack);
  text_layer_set_font(s_custom_msg_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_custom_msg_layer, GTextAlignmentLeft);
  text_layer_set_text(s_custom_msg_layer, "");
  layer_set_hidden((Layer*)s_custom_msg_layer, true);
  
  layer_add_child(root_layer, bitmap_layer_get_layer(s_custom_msg_bg_layer));
  layer_add_child(root_layer, text_layer_get_layer(s_custom_msg_layer));
  
  /*
  s_rain1_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_rain1_layer, s_rain1_bitmap);
  bitmap_layer_set_compositing_mode(s_rain1_layer, GCompOpAnd);

  s_rain2_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_rain2_layer, s_rain2_bitmap);
  bitmap_layer_set_compositing_mode(s_rain2_layer, GCompOpAnd);
  */
  //layer_set_hidden((Layer*)s_rain2_layer, true);
  
  
  // Create battery TextLayer
  //s_battery_layer = text_layer_create(GRect(80, 0, 50, 14));
  //text_layer_set_background_color(s_battery_layer, GColorClear);
  //text_layer_set_text_color(s_battery_layer, GColorBlack);
  //text_layer_set_text(s_battery_layer, "");
  //text_layer_set_font(s_weather_layer, s_weather_font);
  //text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  //text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
  
  // Create temperature Layer
  
  //layer_set_update_proc((Layer*)s_custom_msg_layer, text_update_proc);
  
  //layer_add_child(root_layer, text_layer_get_layer(s_battery_layer));
  //layer_add_child(root_layer, bitmap_layer_get_layer(s_rain1_layer));
  //layer_add_child(root_layer, bitmap_layer_get_layer(s_rain2_layer));
  
  
  // 인버터
  layer_add_child(root_layer, inverter_layer_get_layer(s_inverter_layer));
  
  tick_timer_service_subscribe(SECOND_UNIT | MINUTE_UNIT, handle_timer_tick);
  battery_state_service_subscribe(handle_battery);
  bluetooth_connection_service_subscribe(handle_bluetooth);
  accel_tap_service_subscribe(accel_tap_handler);  
    
  handle_battery(battery_state_service_peek());
  layer_set_hidden((Layer*)s_bluetooth_layer, !bluetooth_connection_service_peek());
  
  g_eye_timer = app_timer_register(g_eye_tick_delta, (AppTimerCallback) eye_timer_callback, NULL);
    //put_weather_service();
}

static void main_window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  accel_tap_service_unsubscribe();
  
  // Destroy GBitmap
  gbitmap_destroy(s_ddochi_body_bitmap);
  gbitmap_destroy(s_ddochi_head1_bitmap);
  gbitmap_destroy(s_ddochi_eye_bitmap);
  gbitmap_destroy(s_time_bg_bitmap);
  gbitmap_destroy(s_sec_bg_bitmap);
  gbitmap_destroy(s_custom_msg_bg_bitmap);
  
  gbitmap_destroy(s_bluetooth_bitmap);
  gbitmap_destroy(s_batt_full_bitmap);
  gbitmap_destroy(s_batt_empty_bitmap);
  gbitmap_destroy(s_batt_charge_bitmap);
  gbitmap_destroy(s_batt_25_bitmap);
  gbitmap_destroy(s_batt_50_bitmap);
  gbitmap_destroy(s_batt_75_bitmap);
  
  //gbitmap_destroy(s_rain1_bitmap);
  //gbitmap_destroy(s_rain2_bitmap);
  //layer_destroy(s_ddochi_layer);
  layer_destroy(s_overlay_layer);
  
  inverter_layer_destroy(s_inverter_layer);

  // Destroy BitmapLayer
  bitmap_layer_destroy(s_ddochi_body_layer);
  //bitmap_layer_destroy(s_ddochi_body2_layer);
  //bitmap_layer_destroy(s_ddochi_eye_layer);
  
  bitmap_layer_destroy(s_time_bg_layer);
  //bitmap_layer_destroy(s_sec_bg_layer);
  bitmap_layer_destroy(s_custom_msg_bg_layer);
  
  bitmap_layer_destroy(s_bluetooth_layer);
  bitmap_layer_destroy(s_batt_layer);
  //bitmap_layer_destroy(s_rain1_layer);
  //bitmap_layer_destroy(s_rain2_layer);
    
  // Destroy TextLayer
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_layer);
  //text_layer_destroy(s_time_sec_layer);
  //text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_custom_layer);
  text_layer_destroy(s_custom_msg_layer);
  
  // Destroy Font
  //fonts_unload_custom_font(s_time_font);
  //fonts_unload_custom_font(s_weather_font);
}

void js_ready_check_callback(void *data) {
  if (s_js_ready==0) {
    g_js_ready_check_timer = app_timer_register(5000, (AppTimerCallback) js_ready_check_callback, NULL);
    put_ready_service();
  }
}

static void init() {
  // psersist
  if (persist_exists(KEY_NAVI_LAT)) {
    persist_read_string(KEY_NAVI_LAT, g_pos_lat, sizeof(g_pos_lat));
  }
  if (persist_exists(KEY_NAVI_LNG)) {
    persist_read_string(KEY_NAVI_LNG, g_pos_lng, sizeof(g_pos_lng));
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "persist g_pos_lat:%s", g_pos_lat);
  APP_LOG(APP_LOG_LEVEL_INFO, "persist g_pos_lng:%s", g_pos_lng);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
    
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  update_time();
  
  g_js_ready_check_timer = app_timer_register(5000, (AppTimerCallback) js_ready_check_callback, NULL);
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
