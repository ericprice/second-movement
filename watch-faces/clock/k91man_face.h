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

#ifndef K91MAN_FACE_H_
#define K91MAN_FACE_H_

#include "movement.h"

typedef struct {
    uint8_t previous_minute;
    uint8_t previous_second;
    uint8_t previous_day_date;  // Combined day and hour for change detection
    uint8_t last_battery_check;
    uint8_t watch_face_index;
    bool signal_enabled;
    bool battery_low;
    bool alarm_enabled;
} k91man_state_t;

void k91man_face_setup(uint8_t watch_face_index, void ** context_ptr);
void k91man_face_activate(void *context);
bool k91man_face_loop(movement_event_t event, void *context);
void k91man_face_resign(void *context);

#define k91man_face ((const watch_face_t){ \
    k91man_face_setup, \
    k91man_face_activate, \
    k91man_face_loop, \
    k91man_face_resign, \
    NULL, \
})

#endif // K91MAN_FACE_H_

