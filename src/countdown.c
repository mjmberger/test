#include <pebble.h>
static Window *window;
static TextLayer *tl_status;
static TextLayer *tl_zeit;
static TextLayer *tl_down;
static TextLayer *tl_time;
static AppTimer *timer;

static const uint16_t timer_interval_ms = 1000;
// static const uint32_t HOURS_KEY = 1 << 1;
static const uint32_t MINUTES_KEY = 1 << 2;
static const uint32_t SECONDS_KEY = 1 << 3;
static const uint32_t LONG_VIBES_KEY = 1 << 4;
static const uint32_t SINGLE_VIBES_KEY = 1 << 5;
static const uint32_t DOUBLE_VIBES_KEY = 1 << 6;

int t_sec;
int t_min;
int t_std;


int hours;
int minutes;
int seconds;
int sec;
char *single_vibes;
char *double_vibes;
char *long_vibes;
// int default_hours;
int default_minutes;
int default_seconds;
int default_sec;
char *default_long_vibes;
char *default_single_vibes;
char *default_double_vibes;
char *time_key;
char *long_key;
char *single_key;
char *double_key;
int running;
int running_sec;

void update_time()
{
  time_t  locTime;
  struct tm* timeInfo;
  static char time_text[9];
  
  time(&locTime);
  timeInfo = localtime(&locTime);
  
  t_std = timeInfo->tm_hour;
  t_min = timeInfo->tm_min;
  t_sec = timeInfo->tm_sec;
  snprintf(time_text,sizeof(time_text),"%02d:%02d:%02d",t_std,t_min,t_sec);
  text_layer_set_text(tl_time,time_text);
  // APP_LOG(APP_LOG_LEVEL_DEBUG,"zeit %s",time_text);
  
}

static void show_time(void) 
{
  static char body_text[6];
  static char sec_text[4];
  static char time_test[9];
  update_time();
 
    snprintf(body_text, sizeof(body_text), "%d:%02d", minutes, seconds);
    snprintf(sec_text,sizeof(sec_text),"%02d",sec);
    snprintf(time_test, sizeof(body_text), "%d:%02d", minutes, seconds);
   // APP_LOG(APP_LOG_LEVEL_DEBUG, "show_time %02d:%02d", minutes, seconds);

  text_layer_set_text(tl_zeit, body_text);
  if (minutes < 10 )
  {
       text_layer_set_text_color(tl_zeit, GColorRed);
  }
  text_layer_set_text(tl_status,"run");
  text_layer_set_text(tl_down,sec_text);
  if (strstr(long_vibes, time_test)) 
  {
    vibes_long_pulse();
  }
  if (strstr(single_vibes, time_test)) 
  {
    vibes_short_pulse();
  }
  if (strstr(double_vibes, time_test)) 
  {
    vibes_double_pulse();
  }
  // update_time();
}

static void timer_callback(void *data) {

  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Timer: %02d:%02d", minutes, seconds);

  seconds--;
  if (running_sec == 1)
  {
  //  APP_LOG(APP_LOG_LEVEL_DEBUG, "%02d drin", running_sec);
    if (sec > 0)
    {
      sec--;
    }
    else
    {
      sec = 25;  
      running_sec = 0;
    }
  }
  if (seconds < 0) {
    minutes--;
    seconds = 59;
  }
  if (minutes < 0) 
  {
    text_layer_set_text(tl_status,"end");

    vibes_double_pulse();
    app_timer_cancel(timer);
  }
  else
  {
    timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
    show_time();
  }
 
}

static void reset(void) {
  running = 0;
  minutes = 3;   //default_minutes;
  seconds = default_seconds;
  sec = 25;
  long_vibes = default_long_vibes;
  single_vibes = default_single_vibes;
  double_vibes = default_double_vibes;
  show_time();
}

void in_received_handler(DictionaryIterator *received, void *context)
{

  Tuple *msg_type = dict_read_first(received);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got message from phone: %s", msg_type->value->cstring);
  if (strcmp(msg_type->value->cstring, time_key) == 0) 
  {
    Tuple *mins = dict_find(received, 2);
    default_minutes = mins->value->int8;
    Tuple *secs = dict_find(received, 3);
    default_seconds = secs->value->int8;
	reset();
	APP_LOG(APP_LOG_LEVEL_DEBUG, "New config: %02d:%02d",  minutes, seconds);
  }
  else 
  {
    Tuple *val = dict_read_next(received);
    if (strcmp(msg_type->value->cstring, long_key) == 0) 
    {
      strcpy(default_long_vibes, val->value->cstring);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Set long vibes to: %s", default_long_vibes);
    }
    else if (strcmp(msg_type->value->cstring, single_key) == 0) 
    {
      strcpy(default_single_vibes, val->value->cstring);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Set single vibes to: %s", default_single_vibes);
    }
    else if (strcmp(msg_type->value->cstring, double_key) == 0) 
    {
      strcpy(default_double_vibes, val->value->cstring);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Set double vibes to: %s", default_double_vibes);
    }
  }
}

void in_dropped_handler(AppMessageResult reason, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message from phone dropped: %d", reason);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context)
{
  if ((running) || ((minutes == 0) && (seconds == 0))) 
  {
   APP_LOG(APP_LOG_LEVEL_DEBUG, "Stop: %02d:%02d",  minutes, seconds);
   app_timer_cancel(timer);
   if (!running) 
   {
     reset();
   }
   text_layer_set_text(tl_status,"pause");

   running = 0;
   vibes_double_pulse();
   return;
  }
  else 
  {
   APP_LOG(APP_LOG_LEVEL_DEBUG, "Start: %02d:%02d", minutes, seconds);
   running = 1;
   running_sec = 1;
  }
  text_layer_set_text(tl_status,"run");

  timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
  show_time();
  vibes_short_pulse();
  //seconds--;
  //sec--;
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sync up: %02d:%02d", minutes, seconds);
  app_timer_cancel(timer);
  timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
  running = 1;
  //minutes++;
 // if (minutes > 59) 
//  {
    // hours++;
 //   minutes = default_minutes;
 // }
  //seconds = default_seconds;
  reset();
  sec = 25;
  show_time();
  vibes_short_pulse();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sync down: %02d:%02d", minutes, seconds);
  app_timer_cancel(timer);
  timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
  running = 1;
  seconds = 0;
  show_time();
  // update_time();
  vibes_short_pulse();
}

/*
static void long_click_handler()
{
  APP_LOG(APP_LOG_LEVEL_DEBUG,"long");
  text_layer_set_text_color(tl_down,GColorWhite);
}
*/
static void click_config_provider(void *context) 
{
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
 // window_long_click_subscribe(BUTTON_ID_UP,1000,NULL,long_click_handler);
}

static void tick_handler(struct tm *timeInfo, TimeUnits units_changed)
{
  update_time();
}

static void window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
 // GRect bounds = layer_get_frame(window_layer);

  tl_status = text_layer_create(GRect(0,0, 144, 20));
  text_layer_set_font(tl_status, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(tl_status, GTextAlignmentLeft);
  text_layer_set_background_color(tl_status, GColorBlack);
  text_layer_set_text_color(tl_status, GColorWhite);

  
  tl_zeit = text_layer_create(GRect(0,20, 144, 55));  
  text_layer_set_font(tl_zeit, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(tl_zeit, GTextAlignmentCenter);
  text_layer_set_background_color(tl_zeit, GColorBlack);
  text_layer_set_text_color(tl_zeit, GColorWhite);
 
  tl_down = text_layer_create(GRect(0,75, 144, 55));  
  text_layer_set_font(tl_down, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(tl_down, GTextAlignmentCenter);
  text_layer_set_background_color(tl_down, GColorBlack);
  text_layer_set_text_color(tl_down, GColorRed);

  tl_time = text_layer_create(GRect(0,130,144,50));
  text_layer_set_font(tl_time,fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(tl_time,GTextAlignmentCenter);
  text_layer_set_background_color(tl_time,GColorBlack);
  text_layer_set_text_color(tl_time,GColorWhite);
  
  
  // Layer einfügen
  layer_add_child(window_layer, text_layer_get_layer(tl_status));  
  layer_add_child(window_layer, text_layer_get_layer(tl_zeit));
  layer_add_child(window_layer, text_layer_get_layer(tl_down));
  layer_add_child(window_layer,text_layer_get_layer(tl_time));
  
  tick_timer_service_subscribe(SECOND_UNIT,tick_handler);
  reset();
  
  
}

static void window_unload(Window *window) 
{
  text_layer_destroy(tl_zeit);
}

static void init(void)
{
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  const uint32_t inbound_size = 128;
  const uint32_t outbound_size = 128;
  app_message_open(inbound_size, outbound_size);

  // default_hours = persist_exists(HOURS_KEY) ? persist_read_int(HOURS_KEY) : 0;
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised hours to: %d", default_hours);
  default_minutes = persist_exists(MINUTES_KEY) ? persist_read_int(MINUTES_KEY) : 15;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised minutes to: %d", default_minutes);
  default_seconds = persist_exists(SECONDS_KEY) ? persist_read_int(SECONDS_KEY) : 0;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised seconds to: %d", default_seconds);
  default_long_vibes = "|4:00|3:00|2:00|1:00|0:00";
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised long vibes to: %s", default_long_vibes);
  if (persist_exists(LONG_VIBES_KEY))
  {
    persist_read_string(LONG_VIBES_KEY, default_long_vibes, 256);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised persisted long vibes to: %s", default_long_vibes);
  }
  default_single_vibes = "|4:30|3:30|2:30|1:30";
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised single vibes to: %s", default_single_vibes);
  if (persist_exists(SINGLE_VIBES_KEY)) 
  {
    persist_read_string(SINGLE_VIBES_KEY, default_single_vibes, 256);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised persisted single vibes to: %s", default_single_vibes);
  }
  default_double_vibes = "|0:50|0:40|0:30|0:25|0:20|0:15|0:10|0:09|0:08|0:07|0:06|0:05|0:04|0:03|0:02|0:01";
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised double vibes to: %s", default_double_vibes);
  if (persist_exists(DOUBLE_VIBES_KEY)) 
  {
    persist_read_string(DOUBLE_VIBES_KEY, default_double_vibes, 256);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Initialised persisted double vibes to: %s", default_double_vibes);
  }
  time_key = "time";
  long_key = "long";
  single_key = "single";
  double_key = "double";

  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers)
  {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  update_time();
}

static void deinit(void) 
{
  //  persist_write_int(HOURS_KEY, default_hours);
  persist_write_int(MINUTES_KEY, default_minutes);
  persist_write_int(SECONDS_KEY, default_seconds);
  persist_write_string(LONG_VIBES_KEY, default_long_vibes);
  persist_write_string(SINGLE_VIBES_KEY, default_single_vibes);
  persist_write_string(DOUBLE_VIBES_KEY, default_double_vibes);
  window_destroy(window);
}

int main(void) 
{
  init();
  app_event_loop();
  deinit();
}
