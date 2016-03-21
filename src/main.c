// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_LIGHT_COLOR_SCHEME 1
#define KEY_DEGREE_CELSIUS 2

static Window *s_main_window;

static GFont s_time_font;
static GFont s_others_font;

static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_bt_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_temperature_layer;

static bool is_light_color_scheme() {
    bool light_color_scheme = true; // default
    if (persist_exists(KEY_LIGHT_COLOR_SCHEME)) {
       light_color_scheme = persist_read_bool(KEY_LIGHT_COLOR_SCHEME);
    }
    return light_color_scheme;
}

static bool is_degree_celsius() {
    bool degree_celsius = true; // default
    if (persist_exists(KEY_DEGREE_CELSIUS)) {
       degree_celsius = persist_read_bool(KEY_DEGREE_CELSIUS);
    }
    return degree_celsius;
}

static void update_time() {
    
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    
    static char s_buffer_time[8];
    static char s_buffer_date[8];
    strftime(s_buffer_time, sizeof(s_buffer_time), clock_is_24h_style() ? "%k:%M" : "%l:%M", tick_time);
    strftime(s_buffer_date, sizeof(s_buffer_date), "%a %e", tick_time);
    
    text_layer_set_text(s_time_layer, s_buffer_time);
    text_layer_set_text(s_date_layer, s_buffer_date);
}

static void update_bt(bool bt_connected) {
    
    if (bt_connected) {
        text_layer_set_text(s_bt_layer, "bt");
    } else {
        text_layer_set_text(s_bt_layer, "!bt");
        vibes_long_pulse();
    }
}

static void update_battery(int battery_level) {
    
    static char s_battery_buffer[5];
    strcpy(s_battery_buffer, "|");
    for (int i = 0; i < battery_level / 20; i++) {
        strcat(s_battery_buffer, "|");
    }
    text_layer_set_text(s_battery_layer, s_battery_buffer);
}

static void update_temperature(bool temperature_given, int temperature, bool degree_celsius) {
    
    if (!degree_celsius) {
       temperature = temperature * 9/5 + 32;
    }
    static char temperature_buffer[8];
    if (temperature_given) {
        snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°", temperature);
    } else {
        strcpy(temperature_buffer, "...");
    }
    text_layer_set_text(s_temperature_layer, temperature_buffer);
}

static void update_colors(bool light_color_scheme) {

    GColor bg_color = GColorWhite;
    GColor text_color = GColorBlack;

    if (!light_color_scheme) {
        bg_color = GColorBlack;
        text_color = GColorWhite;
    }

    window_set_background_color(s_main_window, bg_color);
    text_layer_set_text_color(s_time_layer, text_color);
    text_layer_set_background_color(s_time_layer, bg_color);
    text_layer_set_text_color(s_date_layer, text_color);
    text_layer_set_background_color(s_date_layer, bg_color);
    text_layer_set_text_color(s_bt_layer, text_color);
    text_layer_set_background_color(s_bt_layer, bg_color);
    text_layer_set_text_color(s_battery_layer, text_color);
    text_layer_set_background_color(s_battery_layer, bg_color);
    text_layer_set_text_color(s_temperature_layer, text_color);
    text_layer_set_background_color(s_temperature_layer, bg_color);
}

static void on_ticked(struct tm *tick_time, TimeUnits units_changed) {
    
    update_time();
    
    // request weather update every 10 minutes
    if(tick_time->tm_min % 10 == 0) {
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);
        dict_write_uint8(iter, 0, 0);
        app_message_outbox_send();
    }
}

static void on_bt_connection_changed(bool bt_connected) {
    update_bt(bt_connected);
}

static void on_battery_level_changed(BatteryChargeState battery_level) {
    update_battery(battery_level.charge_percent);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    
    Tuple *temperature_tuple = dict_find(iterator, KEY_TEMPERATURE);
    Tuple *light_color_scheme_tuple = dict_find(iterator, KEY_LIGHT_COLOR_SCHEME);
    Tuple *degree_celsius_tuple = dict_find(iterator, KEY_DEGREE_CELSIUS);
    if (temperature_tuple) {
        int temperature = (int) temperature_tuple->value->int32;
        update_temperature(true, temperature, is_degree_celsius());
        persist_write_int(KEY_TEMPERATURE, temperature);
    }
    if (light_color_scheme_tuple) {
        bool light_color_scheme = light_color_scheme_tuple->value->int8;
        persist_write_bool(KEY_LIGHT_COLOR_SCHEME, light_color_scheme);
        update_colors(light_color_scheme);
    }
    if (degree_celsius_tuple) {
        bool degree_celsius = degree_celsius_tuple->value->int8;
        persist_write_bool(KEY_DEGREE_CELSIUS, degree_celsius);
        if (persist_exists(KEY_TEMPERATURE)) {
           update_temperature(true, persist_read_int(KEY_TEMPERATURE), degree_celsius);
        } else {
           update_temperature(false, 0, degree_celsius);
        }
        
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void main_window_load(Window *window) {
    
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_NEUROPOL_X_32));
    s_others_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_NEUROPOL_X_14));
    
    // TIME
    s_time_layer = text_layer_create(GRect(0, 57, bounds.size.w, 50));
    text_layer_set_font(s_time_layer, s_time_font);
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
    
    // DATE
    s_date_layer = text_layer_create(GRect(0, 44, bounds.size.w, 20));
    text_layer_set_font(s_date_layer, s_others_font);
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
    
    // BT
    s_bt_layer = text_layer_create(GRect(0, 110, bounds.size.w/3 , 25));
    text_layer_set_font(s_bt_layer, s_others_font);
    text_layer_set_text_alignment(s_bt_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_bt_layer));
    
    // BATTERY
    s_battery_layer = text_layer_create(GRect(bounds.size.w/3, 110, bounds.size.w/3, 25));
    text_layer_set_font(s_battery_layer, s_others_font);
    text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
    
    // TEMPERATURE
    s_temperature_layer = text_layer_create(GRect(2 * bounds.size.w/3, 110, bounds.size.w/3, 25));
    text_layer_set_font(s_temperature_layer, s_others_font);
    text_layer_set_text_alignment(s_temperature_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_temperature_layer));

    bool light_color_scheme = is_light_color_scheme();
    if (light_color_scheme) {
        update_colors(light_color_scheme);
    }
    
}

static void main_window_unload(Window *window) {
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_bt_layer);
    text_layer_destroy(s_date_layer);
    text_layer_destroy(s_temperature_layer);
    text_layer_destroy(s_battery_layer);
    
    fonts_unload_custom_font(s_time_font);
    fonts_unload_custom_font(s_others_font);
}

static void init() {
    
    s_main_window = window_create();
    
    // set handlers to manage the elements inside the Window
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
    
    // show the Window on the watch, with animated=true
    window_stack_push(s_main_window, true);
    
    setlocale(LC_ALL, "");
    
    // make sure the time, etc. is displayed from the start
    update_time();
    update_bt(connection_service_peek_pebble_app_connection());
    BatteryChargeState charge_state = battery_state_service_peek();
    update_battery(charge_state.charge_percent);
    update_temperature(false, 0, is_degree_celsius());
    
    // subsribe to services
    tick_timer_service_subscribe(MINUTE_UNIT, on_ticked);
    battery_state_service_subscribe(on_battery_level_changed);
    connection_service_subscribe((ConnectionHandlers) {
        .pebble_app_connection_handler = on_bt_connection_changed
    });
    
    // register callbacks for weather responses
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
