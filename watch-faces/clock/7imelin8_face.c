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
#include "7imelin8_face.h"
#include "watch.h"
#include "watch_common_display.h"

// Timeline face: Shows daily progress across the 6 time digit positions
// Moves through 12 positions (6 digits × 2 sides) over 24 hours
// Each position represents 2 hours of the day
// We use positions 4-9 (HH:MM:SS) and the left/right vertical segments:
// F (top-left) and E (bottom-left), B (top-right) and C (bottom-right).
// From Character_Set mapping semantics: A=0, B=1, C=2, D=3, E=4, F=5, G=6.

static void clear_all_lr_segments(void) {
    // Clear F,E,B,C on positions 4..9
    for (uint8_t pos = 4; pos <= 9; pos++) {
        const digit_mapping_t map = (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM)
            ? Custom_LCD_Display_Mapping[pos]
            : Classic_LCD_Display_Mapping[pos];
        for (uint8_t seg_code = 0; seg_code < 8; seg_code++) {
            if (map.segment[seg_code].value == segment_does_not_exist) continue;
            uint8_t com = map.segment[seg_code].address.com;
            uint8_t seg = map.segment[seg_code].address.seg;
            if (seg_code == 1 || seg_code == 2 || seg_code == 4 || seg_code == 5) {
                watch_clear_pixel(com, seg);
            }
        }
    }
}

static void set_lr_segments(uint8_t position, bool left_side) {
    // Light F&E for left side; B&C for right side, on given position 4..9
    if (position < 4 || position > 9) return;
    const digit_mapping_t map = (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM)
        ? Custom_LCD_Display_Mapping[position]
        : Classic_LCD_Display_Mapping[position];
    for (uint8_t seg_code = 0; seg_code < 8; seg_code++) {
        if (map.segment[seg_code].value == segment_does_not_exist) continue;
        uint8_t com = map.segment[seg_code].address.com;
        uint8_t seg = map.segment[seg_code].address.seg;
        if (left_side) {
            if (seg_code == 5 || seg_code == 4) watch_set_pixel(com, seg); // F, E
        } else {
            if (seg_code == 1 || seg_code == 2) watch_set_pixel(com, seg); // B, C
        }
    }
}

void timelin8_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        timelin8_state_t *state = (timelin8_state_t *)malloc(sizeof(timelin8_state_t));
        state->last_bucket = -1;
        state->last_position = 255; // Invalid position
        state->last_minute = -1;
        *context_ptr = state;
    }
}

void timelin8_face_activate(void *context) {
    timelin8_state_t *state = (timelin8_state_t *)context;
    if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();
    // Since we only update every 2 hours, we can use 1Hz and check less frequently
    // We'll wake up once per minute to ensure we catch the hour change reliably
    movement_request_tick_frequency(1);
    state->last_bucket = -1; // Force update
    state->last_minute = -1;
    clear_all_lr_segments();
}

bool timelin8_face_loop(movement_event_t event, void *context) {
    timelin8_state_t *state = (timelin8_state_t *)context;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            state->last_bucket = -1; // Force update on activate
            // Fall through
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE: {
            // Skip most ticks - only check once per minute for efficiency
            // Since we update every 2 hours, checking once per minute is plenty
            watch_date_time_t now = movement_get_local_date_time();
            
            if (event.event_type == EVENT_TICK && now.unit.minute == state->last_minute) {
                break; // Skip if minute hasn't changed
            }
            state->last_minute = now.unit.minute;
            
            // Divide 24 hours into 12 buckets (2 hours each)
            // 00:00-01:59 = bucket 0, 02:00-03:59 = bucket 1, ... 22:00-23:59 = bucket 11
            uint8_t bucket = now.unit.hour / 2; // 0..11
            
            // Only update when bucket changes (every 2 hours)
            if (bucket != state->last_bucket) {
                state->last_bucket = bucket;
                
                // Clear only the previous segment position for efficiency
                if (state->last_position < 10) {
                    // Clear the last lit segment
                    const digit_mapping_t map = (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM)
                        ? Custom_LCD_Display_Mapping[state->last_position]
                        : Classic_LCD_Display_Mapping[state->last_position];
                    for (uint8_t i = 0; i < 8; i++) {
                        if (map.segment[i].value == segment_does_not_exist) continue;
                        uint8_t com = map.segment[i].address.com;
                        uint8_t seg = map.segment[i].address.seg;
                        if (i == 1 || i == 2 || i == 4 || i == 5) watch_clear_pixel(com, seg);
                    }
                }
                
                // Set new segment
                uint8_t digit_index = bucket / 2;              // 0..5 => positions 4..9
                bool right_side = (bucket % 2) == 1;           // odd => right, even => left
                uint8_t position = 4 + digit_index;            // 4..9
                state->last_position = position;
                set_lr_segments(position, !right_side);
            }
            break;
        }
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void timelin8_face_resign(void *context) {
    (void) context;
    movement_request_tick_frequency(1);
}
