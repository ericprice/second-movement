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
#include <string.h>
#include "an91og_face.h"
#include "watch.h"
#include "watch_private_display.h"

static const int32_t clock_mapping_ep[6][7][2] = {
    // hour 1
    {{1,18}, {2,19}, {0,19}, {1,18}, {0,18}, {2,18}, {1,19}},
    // hour 2
    {{2,20}, {2,21}, {1,21}, {0,21}, {0,20}, {1,17}, {1,20}},
    // minute 1
    {{0,22}, {2,23}, {0,23}, {0,22}, {1,22}, {2,22}, {1,23}},
    // minute 2
    {{2,1}, {2,10}, {0,1}, {0,0}, {1,0}, {2,0}, {1,1}},
    // second 1 (unused)
    {{2,2}, {2,3}, {0,4}, {0,3}, {0,2}, {1,2}, {1,3}},
    // second 2 (unused)
    {{2,4}, {2,5}, {1,6}, {0,6}, {0,5}, {1,4}, {1,5}},
};

// 12-segment ring order around all four HH:MM digits, clockwise, starting at
// the top of the 3rd digit (position 2). Each entry is { position, segment }.
// Segments: 0..5 => A..F (skip G)
static const uint8_t ring_order[12][2] = {
    {2, 0}, // step  1: digit3 (pos2) top (A) — START
    {3, 0}, // step  2: digit4 (pos3) top (A)
    {3, 1}, // step  3: digit4 (pos3) top-right (B)
    {3, 2}, // step  4: digit4 (pos3) bottom-right (C)
    {3, 3}, // step  5: digit4 (pos3) bottom (D)
    {2, 3}, // step  6: digit3 (pos2) bottom (D)
    {1, 3}, // step  7: digit2 (pos1) bottom (D)
    {0, 3}, // step  8: digit1 (pos0) bottom (D)
    {0, 4}, // step  9: digit1 (pos0) bottom-left (E)
    {0, 5}, // step 10: digit1 (pos0) top-left (F)
    {0, 0}, // step 11: digit1 (pos0) top (A)
    {1, 0}, // step 12: digit2 (pos1) top (A)
};

// Minute indicator: 12-step inner "ring" using inner segments across HH:MM.
// Each entry is { position, segment } where position 0..3 => H1,H2,M1,M2 and
// segment 0..6 => A..G. We only use inner segments (B,C,E,F,G) to avoid the perimeter.
// Bucket mapping: 0=00..04, 1=05..09, ..., 11=55..59
static const uint8_t minute_indicator_order[12][2] = {
    {2, 5}, // 00-04:  digit2 top-left (F)
    {2, 1}, // 05-09:  digit2 top-right (B)
    {3, 5}, // 10-14:  digit3 top-left (F)
    {3, 6}, // 15-19:  digit3 center (G)
    {3, 4}, // 20-24:  digit3 bottom-left (E)
    {2, 2}, // 25-29:  digit2 bottom-right (C)
    {1, 2}, // 30-34:  digit1 bottom-right (C)
    {1, 4}, // 35-39:  digit1 bottom-left (E)
    {0, 2}, // 40-44:  digit0 bottom-right (C)
    {0, 6}, // 45-49:  digit0 center (G)
    {0, 1}, // 50-54:  digit0 top-right (B)
    {0, 6}, // 55-59:  digit0 center (G) — pairs with H2 center via extra rule
};

static inline void render_minute_indicator(uint8_t minute, bool visible) {
    uint8_t bucket = minute / 5; // 0..11
    uint8_t pos = minute_indicator_order[bucket][0];
    uint8_t seg = minute_indicator_order[bucket][1];
    if (visible) {
        watch_set_pixel(clock_mapping_ep[pos][seg][0], clock_mapping_ep[pos][seg][1]);
    } else {
        watch_clear_pixel(clock_mapping_ep[pos][seg][0], clock_mapping_ep[pos][seg][1]);
    }
    // Additional segments per spec for select buckets
    switch (bucket) {
        case 1: // 05-09: also center middle of digit 2
            pos = 2; seg = 6; break;
        case 3: // 15-19: also center middle of digit 2
            pos = 2; seg = 6; break;
        case 5: // 25-29: also center middle of digit 2
            pos = 2; seg = 6; break;
        case 7: // 35-39: also center middle of digit 1
            pos = 1; seg = 6; break;
        case 9: // 45-49: also center middle of digit 1
            pos = 1; seg = 6; break;
        case 11: // 55-59: also center middle of digit 1
            pos = 1; seg = 6; break;
        default:
            return;
    }
    if (visible) watch_set_pixel(clock_mapping_ep[pos][seg][0], clock_mapping_ep[pos][seg][1]);
    else watch_clear_pixel(clock_mapping_ep[pos][seg][0], clock_mapping_ep[pos][seg][1]);
}

static inline void clear_minute_indicator(uint8_t minute) {
    render_minute_indicator(minute, false);
}

static inline void clear_all_centers(void) {
    // Proactively clear center segment (G) on all four HH:MM digits to avoid any "stuck" middles
    for (uint8_t pos = 0; pos <= 3; pos++) {
        watch_clear_pixel(clock_mapping_ep[pos][6][0], clock_mapping_ep[pos][6][1]);
    }
}

// Blink helper: return true if this ring segment should blink at the given hour.
// Ambiguous tied segments (A/D) are:
//  - pos2:A  (top of digit 3)
//  - pos0:D  (bottom of digit 1)
// Blink these at: 1:xx (pos2:A), 8:xx (pos0:D), 10:xx (both pos2:A and pos0:D)
static bool _segment_should_blink(uint8_t hour_12, uint8_t pos, uint8_t seg) {
    // pos2:A (digit 2 top) blinks at 1..5 and 10
    if (pos == 2 && seg == 0) {
        return (hour_12 == 1 || hour_12 == 2 || hour_12 == 3 || hour_12 == 4 || hour_12 == 5 || hour_12 == 10);
    }
    // pos0:D (digit 0 bottom) blinks at 8, 9 and 10
    if (pos == 0 && seg == 3) {
        return (hour_12 == 8 || hour_12 == 9 || hour_12 == 10);
    }
    return false;
}

static void set_segment_pixel(uint8_t pos, uint8_t seg) {
    if (seg > 5) return; // only A..F
    watch_set_pixel(clock_mapping_ep[pos][seg][0], clock_mapping_ep[pos][seg][1]);
}

static void clear_outline_all_digits(void) {
    // Clear A..F on positions 0..3
    for (uint8_t pos = 0; pos <= 3; pos++) {
        for (uint8_t seg = 0; seg < 6; seg++) {
            watch_clear_pixel(clock_mapping_ep[pos][seg][0], clock_mapping_ep[pos][seg][1]);
        }
    }
}

static void render_hour_ring(uint8_t hour_12, uint8_t subsecond, bool enable_blink) {
    // Map hour to 1..12 segments; 0 means 12 (full ring)
    uint8_t segments_to_light = (hour_12 == 0) ? 12 : hour_12;

    // Clear existing outline first
    clear_outline_all_digits();

    for (uint8_t i = 0; i < segments_to_light; i++) {
        uint8_t pos = ring_order[i][0];
        uint8_t seg = ring_order[i][1];
        bool blink_this = enable_blink && _segment_should_blink(hour_12, pos, seg);
        // Match set_time_face: request 4 Hz and toggle visibility every other tick (~2 Hz blink)
        bool visible = !blink_this || ((subsecond % 2) == 0);
        if (visible) set_segment_pixel(pos, seg);
    }
}

void an91og_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings; (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(ep_analog_state_t));
    }
}

void an91og_face_activate(movement_settings_t *settings, void *context) {
    ep_analog_state_t *state = (ep_analog_state_t *)context;
    if (watch_tick_animation_is_running()) watch_stop_tick_animation();
    movement_request_tick_frequency(1);
    state->last_minute = 0xFF;
    state->needs_high_freq = false;
}

bool an91og_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    ep_analog_state_t *state = (ep_analog_state_t *)context;
    
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK: {
            watch_date_time now = watch_rtc_get_date_time();
            uint8_t hour_now = now.unit.hour % 12;
            
            // Check if we need high frequency for blinking using bit mask for efficiency
            // Hours that need blinking: 1,2,3,4,5,8,9,10
            uint16_t blink_mask = 0x073E; // Binary: 0000 0111 0011 1110 (bits 1-5, 8-10)
            bool should_blink = (blink_mask & (1 << hour_now)) != 0;
            
            // Switch frequency if needed
            if (should_blink != state->needs_high_freq) {
                movement_request_tick_frequency(should_blink ? 4 : 1);
                state->needs_high_freq = should_blink;
            }
            
            // Only clear ring when minute changes (minute indicator moves)
            if (now.unit.minute != state->last_minute) {
                // Clear prior minute indicator fully (including center G if used)
                if (state->last_minute != 0xFF) clear_minute_indicator(state->last_minute);
                state->last_minute = now.unit.minute;
                clear_outline_all_digits();  // Only clear the ring (A..F)
            }
            
            // Handle colon blink and derive a shared 1 Hz boolean for minute-hand blink
            bool colon_on;
            if (state->needs_high_freq) {
                // 4 Hz tick: show colon on the first sub-tick each second (1 Hz overall, ~25% duty)
                colon_on = (event.subsecond % 4 == 0);
            } else {
                // 1 Hz tick: toggle every second
                colon_on = ((now.unit.second & 1) == 0);
            }
            if (colon_on) watch_set_colon(); else watch_clear_colon();
            
            // Render hour ring with blinking support
            render_hour_ring(hour_now, state->needs_high_freq ? event.subsecond : 0, state->needs_high_freq);
            
            // Minute-hand blink: match the colon's 1 Hz cadence regardless of ring blink
            bool minute_visible = colon_on;
            // Clear current bucket completely, then draw on visible phase to ensure all segments (incl. G) blink
            clear_minute_indicator(now.unit.minute);
            if (minute_visible) render_minute_indicator(now.unit.minute, true);
            
            break;
        }

        case EVENT_LOW_ENERGY_UPDATE: {
            if (watch_tick_animation_is_running()) watch_stop_tick_animation();
            movement_request_tick_frequency(1);
            watch_clear_display();
            watch_set_colon();
            
            watch_date_time now = watch_rtc_get_date_time();
            uint8_t hour_now = now.unit.hour % 12;
            // Low energy: draw static, no blinking
            render_hour_ring(hour_now, 0, false);
            // Low energy: ensure only current minute indicator is lit
            clear_all_centers();
            render_minute_indicator(now.unit.minute, true);
            
            break;
        }
        default:
            return movement_default_loop_handler(event, settings);
    }
    return true;
}

void an91og_face_resign(movement_settings_t *settings, void *context) {
    (void) settings; (void) context;
    movement_request_tick_frequency(1);
    if (watch_tick_animation_is_running()) watch_stop_tick_animation();
}
