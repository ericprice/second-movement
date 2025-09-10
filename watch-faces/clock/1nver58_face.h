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

#ifndef INVER58_FACE_H_
#define INVER58_FACE_H_

/*
 * 1NVER58 CLOCK FACE
 *
 * Inverted variant of the simple clock face: every segment that would normally
 * be on is turned off, and every segment that would normally be off is turned
 * on. Behavior and interaction are otherwise identical.
 */

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
} inver58_state_t;

void inver58_face_setup(uint8_t watch_face_index, void ** context_ptr);
void inver58_face_activate(void *context);
bool inver58_face_loop(movement_event_t event, void *context);
void inver58_face_resign(void *context);

#define inver58_face ((const watch_face_t){ \
    inver58_face_setup, \
    inver58_face_activate, \
    inver58_face_loop, \
    inver58_face_resign, \
    NULL, \
})

#endif // INVER58_FACE_H_

