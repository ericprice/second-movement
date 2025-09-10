/*
 * MIT License
 *
 * Copyright (c) 2025
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include "1nver58_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_private_display.h"

static void inver58_display_character(uint8_t character, uint8_t position) {
    // Same character normalization logic as the standard renderer
    if (position == 4 || position == 6) {
        if (character == '7') character = '&';
        else if (character == 'A') character = 'a';
        else if (character == 'o') character = 'O';
        else if (character == 'L') character = '!';
        else if (character == 'M' || character == 'm' || character == 'N') character = 'n';
        else if (character == 'c') character = 'C';
        else if (character == 'J') character = 'j';
        else if (character == 't' || character == 'T') character = '+';
        else if (character == 'y' || character == 'Y') character = '4';
        else if (character == 'v' || character == 'V' || character == 'U' || character == 'W' || character == 'w') character = 'u';
    } else {
        if (character == 'u') character = 'v';
        else if (character == 'j') character = 'J';
    }
    if (position > 1) {
        if (character == 'T') character = 't';
    }
    if (position == 1) {
        if (character == 'a') character = 'A';
        else if (character == 'o') character = 'O';
        else if (character == 'i') character = 'l';
        else if (character == 'n') character = 'N';
        else if (character == 'r') character = 'R';
        else if (character == 'd') character = 'D';
        else if (character == 'v' || character == 'V' || character == 'u') character = 'U';
        else if (character == 'b') character = 'B';
        else if (character == 'c') character = 'C';
    } else {
        if (character == 'R') character = 'r';
    }
    if (position == 0) {
        // normal renderer clears this; for inversion, set it
        watch_set_pixel(0, 15);
    } else {
        if (character == 'I') character = 'l';
    }

    uint64_t segmap = Segment_Map[position];
    uint64_t segdata = Character_Set[character - 0x20];

    for (int i = 0; i < 8; i++) {
        uint8_t com = (segmap & 0xFF) >> 6;
        if (com > 2) {
            segmap = segmap >> 8;
            segdata = segdata >> 1;
            continue;
        }
        uint8_t seg = segmap & 0x3F;

        if (segdata & 1)
            watch_clear_pixel(com, seg);
        else
            watch_set_pixel(com, seg);

        segmap = segmap >> 8;
        segdata = segdata >> 1;
    }

    // Invert the special-case extra segments from the normal renderer
    if (character == 'T' && position == 1) watch_clear_pixel(1, 12);
    else if (position == 0 && (character == 'B' || character == 'D' || character == '@')) watch_clear_pixel(0, 15);
    else if (position == 1 && (character == 'B' || character == 'D' || character == '@')) watch_clear_pixel(0, 12);
}

static void inver58_display_character_lp_seconds(uint8_t character, uint8_t position) {
    uint64_t segmap = Segment_Map[position];
    uint64_t segdata = Character_Set[character - 0x20];

    for (int i = 0; i < 8; i++) {
        uint8_t com = (segmap & 0xFF) >> 6;
        if (com > 2) {
            segmap = segmap >> 8;
            segdata = segdata >> 1;
            continue;
        }
        uint8_t seg = segmap & 0x3F;

        if (segdata & 1)
            watch_clear_pixel(com, seg);
        else
            watch_set_pixel(com, seg);

        segmap = segmap >> 8;
        segdata = segdata >> 1;
    }
}

static void inver58_display_string(char *string, uint8_t position) {
    size_t i = 0;
    while (string[i] != 0) {
        inver58_display_character(string[i], position + i);
        i++;
        if (position + i >= 10) break;
    }
}

static void _update_alarm_indicator(bool settings_alarm_enabled, inver58_state_t *state) {
    state->alarm_enabled = settings_alarm_enabled;
    if (state->alarm_enabled) watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    else watch_set_indicator(WATCH_INDICATOR_SIGNAL);
}

void inver58_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(inver58_state_t));
        inver58_state_t *state = (inver58_state_t *)*context_ptr;
        state->signal_enabled = false;
        state->watch_face_index = watch_face_index;
    }
}

void inver58_face_activate(movement_settings_t *settings, void *context) {
    inver58_state_t *state = (inver58_state_t *)context;

    if (watch_tick_animation_is_running()) watch_stop_tick_animation();

#ifdef CLOCK_FACE_24H_ONLY
    watch_clear_indicator(WATCH_INDICATOR_24H);
#else
    if (settings->bit.clock_mode_24h) watch_clear_indicator(WATCH_INDICATOR_24H);
    else watch_set_indicator(WATCH_INDICATOR_24H);
#endif

    // handle chime indicator (inverted)
    if (state->signal_enabled) watch_clear_indicator(WATCH_INDICATOR_BELL);
    else watch_set_indicator(WATCH_INDICATOR_BELL);

    // show alarm indicator if there is an active alarm (inverted)
    _update_alarm_indicator(settings->bit.alarm_enabled, state);

    // colon inverted
    watch_clear_colon();

    // force full re-render
    state->previous_minute = 0xFF;
    state->previous_second = 0xFF;
    state->previous_day_date = 0xFF;
}

bool inver58_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    inver58_state_t *state = (inver58_state_t *)context;
    char buf[11];
    uint8_t pos;

    watch_date_time date_time;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
            date_time = watch_rtc_get_date_time();
            uint8_t prev_min = state->previous_minute;
            uint8_t prev_sec = state->previous_second;
            uint8_t prev_day_date = state->previous_day_date;
            state->previous_minute = date_time.unit.minute;
            state->previous_second = date_time.unit.second;
            state->previous_day_date = (date_time.unit.hour << 5) | date_time.unit.day;

            // Blink colon at 1 Hz in active mode
            if (event.event_type != EVENT_LOW_ENERGY_UPDATE) {
                if ((date_time.unit.second & 1) == 0) watch_set_colon();
                else watch_clear_colon();
            }

            // check the battery voltage once a week (on day change when day % 7 == 0)
            if (date_time.unit.day != state->last_battery_check && (date_time.unit.day % 7) == 0) {
                state->last_battery_check = date_time.unit.day;
                watch_enable_adc();
                uint16_t voltage = watch_get_vcc_voltage();
                watch_disable_adc();
                state->battery_low = (voltage < 2200);
            }

            // set the LAP indicator inverted: on when battery is NOT low
            if (state->battery_low) watch_clear_indicator(WATCH_INDICATOR_LAP);
            else watch_set_indicator(WATCH_INDICATOR_LAP);

            bool set_leading_zero = false;
            bool seconds_only = (date_time.unit.minute == prev_min && prev_sec != date_time.unit.second && 
                                 state->previous_day_date == ((date_time.unit.hour << 5) | date_time.unit.day) &&
                                 event.event_type != EVENT_LOW_ENERGY_UPDATE);
            bool minutes_only = (date_time.unit.minute != prev_min && 
                                 state->previous_day_date == ((date_time.unit.hour << 5) | date_time.unit.day) &&
                                 event.event_type != EVENT_LOW_ENERGY_UPDATE);
            
            if (seconds_only) {
                // seconds changed only
                inver58_display_character_lp_seconds('0' + date_time.unit.second / 10, 8);
                inver58_display_character_lp_seconds('0' + date_time.unit.second % 10, 9);
                break;
            } else if (minutes_only) {
                // minutes changed only
                pos = 6;
                sprintf(buf, "%02d%02d", date_time.unit.minute, date_time.unit.second);
            } else {
                // full refresh
#ifndef CLOCK_FACE_24H_ONLY
                if (!settings->bit.clock_mode_24h) {
                    if (date_time.unit.hour < 12) {
                        watch_set_indicator(WATCH_INDICATOR_PM);
                    } else {
                        watch_clear_indicator(WATCH_INDICATOR_PM);
                    }
                    date_time.unit.hour %= 12;
                    if (date_time.unit.hour == 0) date_time.unit.hour = 12;
                }
#endif

                if (settings->bit.clock_mode_24h && settings->bit.clock_24h_leading_zero && date_time.unit.hour < 10) {
                    set_leading_zero = true;
                }

                pos = 0;
                if (event.event_type == EVENT_LOW_ENERGY_UPDATE) {
                    if (!watch_tick_animation_is_running()) watch_start_tick_animation(500);
                    snprintf(buf, 11, "%s%2d%2d%02d  ", watch_utility_get_weekday(date_time), date_time.unit.day, date_time.unit.hour, date_time.unit.minute);
                } else {
                    snprintf(buf, 11, "%s%2d%2d%02d%02d", watch_utility_get_weekday(date_time), date_time.unit.day, date_time.unit.hour, date_time.unit.minute, date_time.unit.second);
                }
            }
            inver58_display_string(buf, pos);

            if (set_leading_zero) inver58_display_string("0", 4);

            // handle alarm indicator
            if (state->alarm_enabled != settings->bit.alarm_enabled) _update_alarm_indicator(settings->bit.alarm_enabled, state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->signal_enabled = !state->signal_enabled;
            if (state->signal_enabled) watch_clear_indicator(WATCH_INDICATOR_BELL);
            else watch_set_indicator(WATCH_INDICATOR_BELL);
            break;
        case EVENT_BACKGROUND_TASK:
            // movement_move_to_face(state->watch_face_index);
            movement_play_signal();
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }

    return true;
}

void inver58_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}

bool inver58_face_wants_background_task(movement_settings_t *settings, void *context) {
    (void) settings;
    inver58_state_t *state = (inver58_state_t *)context;
    if (!state->signal_enabled) return false;

    watch_date_time date_time = watch_rtc_get_date_time();

    return date_time.unit.minute == 0;
}

