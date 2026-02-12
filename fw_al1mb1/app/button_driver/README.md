# Button Driver

A button state detection driver based on ChibiOS/RT that detects short, long, and longest button presses using interrupt-based edge detection with a dedicated thread for timing and callback management.

## Features

- **Interrupt-driven**: Uses PAL callbacks for efficient edge detection (no polling when idle)
- **RTOS-friendly**: Dedicated low-priority thread handles timing logic
- **Software debouncing**: Configurable debounce time (default 20ms)
- **Three press types**: Short (<1s), Long (2-5s), Longest (>=5s)
- **Dual callbacks**: Events fire at threshold crossings AND on release
- **Thread-safe**: Callbacks invoked from thread context, safe to use RTOS functions
- **Modular**: GPIO line and active state passed at runtime for easy reuse
- **Configurable**: Timing parameters adjustable via compile-time defines

## Event Types

| Event | When Fired | Duration |
|-------|-----------|----------|
| `BTN_EVT_SHORT_PRESS` | On release | < 1 second |
| `BTN_EVT_LONG_PRESS_START` | At threshold | >= 2 seconds (while held) |
| `BTN_EVT_LONG_PRESS_RELEASE` | On release | 2-5 seconds |
| `BTN_EVT_LONGEST_PRESS_START` | At threshold | >= 5 seconds (while held) |
| `BTN_EVT_LONGEST_PRESS_RELEASE` | On release | >= 5 seconds |

**Note**: Releases between 1-2 seconds produce no event (considered "cancelled" press).

## Usage

### Basic Example

```c
#include "button_driver.h"

/* Button event handler */
static void on_button_event(button_event_t event) {
    switch (event) {
    case BTN_EVT_SHORT_PRESS:
        /* Handle short press - e.g., toggle mode */
        break;
    case BTN_EVT_LONG_PRESS_START:
        /* Handle long press start - e.g., start brightness adjustment */
        break;
    case BTN_EVT_LONG_PRESS_RELEASE:
        /* Handle long press release - e.g., stop brightness adjustment */
        break;
    case BTN_EVT_LONGEST_PRESS_START:
        /* Handle longest press - e.g., enter factory reset mode */
        break;
    case BTN_EVT_LONGEST_PRESS_RELEASE:
        /* Handle longest press release - e.g., confirm factory reset */
        break;
    default:
        break;
    }
}

int main(void) {
    halInit();
    chSysInit();
    
    /* Initialize and start button driver
     * Parameters:
     *   - LINE_USER_BUTTON: GPIO line for the button
     *   - PAL_LOW: Button is active-low (pressed = LOW, externally pulled up)
     */
    button_init(LINE_USER_BUTTON, PAL_LOW);
    button_register_callback(on_button_event);
    button_start();
    
    while (true) {
        /* Main loop - button handling is in separate thread */
        chThdSleepMilliseconds(500);
    }
}
```

### Checking Button State

```c
if (button_is_pressed()) {
    /* Button is currently held down */
}
```

### Stopping/Starting

```c
/* Temporarily disable button monitoring */
button_stop();

/* Re-enable button monitoring */
button_start();
```

## Configuration

### Runtime Parameters (passed to `button_init()`)

| Parameter | Description |
|-----------|-------------|
| `button_line` | GPIO line for the button (e.g., `LINE_USER_BUTTON`) |
| `active_state` | `PAL_LOW` for active-low (external pull-up), `PAL_HIGH` for active-high |

### Compile-time Configuration

Configuration macros are defined in `button_driver_config.h`. Modify this file directly for project-wide changes.

### Configuration Options

| Macro | Default | Description |
|-------|---------|-------------|
| `BTN_DEBOUNCE_MS` | 20 | Debounce time (milliseconds) |
| `BTN_SHORT_MAX_MS` | 1000 | Max duration for short press (ms) |
| `BTN_LONG_MIN_MS` | 2000 | Min duration for long press (ms) |
| `BTN_LONGEST_MIN_MS` | 5000 | Min duration for longest press (ms) |
| `BTN_POLL_INTERVAL_MS` | 50 | Polling interval while pressed (ms) |
| `BTN_THREAD_WA_SIZE` | 256 | Thread stack size (bytes) |
| `BTN_THREAD_PRIORITY` | LOWPRIO+1 | Thread priority |

## File Structure

```
button_driver/
├── button_driver.h          # Main API header
├── button_driver.c          # Implementation
├── button_driver_config.h   # Configuration macros
└── README.md                # This file
```

## Requirements

- ChibiOS/RT with HAL
- PAL driver with callbacks enabled (`PAL_USE_CALLBACKS TRUE` in halconf.h)
- PAL wait support (`PAL_USE_WAIT TRUE` in halconf.h)

## Resource Usage

- **RAM**: ~256 bytes stack + ~50 bytes static variables
- **Synchronization**: 1 binary semaphore, 1 mutex
- **PAL callbacks**: 1 callback slot for button GPIO

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    PAL Interrupt (EXTI)                         │
│   - Triggers on both edges                                      │
│   - Records timestamp, signals semaphore                        │
└─────────────────────────┬───────────────────────────────────────┘
                          │ chBSemSignalI()
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Button Thread                                │
│   - State machine: IDLE -> DEBOUNCE -> PRESSED -> DETECTED     │
│   - Sleeps on semaphore (efficient)                            │
│   - Polls only while button is held (threshold checking)        │
│   - Invokes callbacks from thread context                       │
└─────────────────────────────────────────────────────────────────┘
```

## Thread Safety Notes

- Callback is invoked from thread context, not ISR - safe to use RTOS functions
- Keep callback execution short to maintain responsiveness
- `button_is_pressed()` returns volatile state, no locking needed for single read
- Callback registration is protected by mutex

## License

Copyright (c) 2024
