# Application State Machine

The `app_state_machine` module implements the main state machine of the application. It manages system states, operational modes, and handles user input.

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          THREAD ARCHITECTURE                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐               │
│  │   Button     │    │ State Machine│    │  Animation   │               │
│  │   Thread     │───►│    Thread    │───►│   Thread     │               │
│  │  (existing)  │    │ (NORMALPRIO) │    │(NORMALPRIO+1)│               │
│  └──────────────┘    └──────────────┘    └──────────────┘               │
│        │                    │                   │                       │
│        │ button_event       │ anim_command      │ led_render            │
│        ▼                    ▼                   ▼                       │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                      LED Driver (existing)                      │    │
│  └─────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────────┘
```

## System States

```
    ┌──────────┐   auto    ┌──────────┐   auto    ┌──────────┐
    │   BOOT   │ ───────►  │ STARTUP  │ ───────►  │  ACTIVE  │◄───┐
    │  (init)  │           │(fade-in) │           │ (normal) │    │
    └──────────┘           └──────────┘           └────┬─────┘    │
                                                       │          │
                                LONGEST_PRESS ────────►│          │
                                                       ▼          │
                                                  ┌──────────┐    │
                                                  │ SHUTDOWN │────┘
                                                  │(fade-out)│  (btn)
                                                  └────┬─────┘
                                                       │ auto
                                                       ▼
                                                  ┌──────────┐
                                                  │   OFF    │
                                                  │(low pwr) │
                                                  └──────────┘
```

| State | Description |
|-------|-------------|
| **BOOT** | System initialization, transient state |
| **STARTUP** | Fade-in animation |
| **ACTIVE** | Normal operation, user can interact with modes |
| **SHUTDOWN** | Fade-out animation |
| **OFF** | LEDs off, waiting for button press to wake |

## Operational Modes

Within the **ACTIVE** state, the following modes are available:

| Mode | Description | Short Press Action |
|------|-------------|-------------------|
| **Solid Color** | Static color display | Cycle through 12 colors |
| **Strength** | Brightness control | Cycle through 8 brightness levels |
| **Blinking** | Blink on/off | Cycle through 5 blink speeds |
| **Pulsation** | Breathing effect | Cycle through 5 pulse speeds |
| **Animation** | Dynamic animations | Cycle through sub-modes (Rainbow, Strobe, Color Cycle) |
| **Traffic Light** | Red-Yellow-Green sequence | Toggle auto/manual mode |
| **Night Light** | Dim warm light | Cycle through 4 dim levels |

## Button Controls

| Button Event | Action |
|--------------|--------|
| **Short Press** | Mode-specific action (see table above) |
| **Long Press Release** | Go to next mode |
| **Longest Press Release** | Turn off (enter shutdown state) |


## API Usage

### Basic Initialization

```c
#include "app_state_machine.h"

/* Initialize and start the state machine */
app_sm_init();
app_sm_start();

/* Route button events to state machine */
void on_button_event(button_event_t event) {
    switch (event) {
        case BTN_EVT_SHORT_PRESS:
            chprintf((BaseSequentialStream*)&PORTAB_SDU1, "Button registered also in app state machine section!\r\n");
            app_sm_process_input(APP_SM_INPUT_BTN_SHORT);
            break;
        case BTN_EVT_LONG_PRESS_START:
            app_sm_process_input(APP_SM_INPUT_BTN_LONG_START);
            break;
        case BTN_EVT_LONG_PRESS_RELEASE:
            app_sm_process_input(APP_SM_INPUT_BTN_LONG_RELEASE);
            break;
        case BTN_EVT_LONGEST_PRESS_START:
            app_sm_process_input(APP_SM_INPUT_BTN_LONGEST_START);
            break;
        case BTN_EVT_LONGEST_PRESS_RELEASE:
            app_sm_process_input(APP_SM_INPUT_BTN_LONGEST_RELEASE);
            break;
    }
}
```

### Query State

```c
/* Get current system state */
app_sm_system_state_t state = app_sm_get_system_state();

/* Get current mode (only valid in ACTIVE state) */
app_sm_mode_t mode = app_sm_get_mode();

/* Get state/mode names for debugging */
const char* state_name = app_sm_system_state_name(state);
const char* mode_name = app_sm_mode_name(mode);
```

### External Control (Future)

```c
/* Enter external control mode (e.g., from WiFi/BLE) */
app_sm_external_control_enter();

/* Exit external control mode */
app_sm_external_control_exit();

/* Check if external control is active */
bool ext_active = app_sm_is_external_control_active();
```

## Adding New Modes

To add a new mode:

1. Create `mode_yourmode.c` in the `modes/` directory
2. Implement the mode operations structure:
   ```c
   const app_sm_mode_ops_t mode_yourmode_ops = {
       .name = "Your Mode",
       .enter = yourmode_enter,
       .exit = yourmode_exit,
       .on_short_press = yourmode_on_short_press,
       .on_long_start = yourmode_on_long_start
   };
   ```
3. Add the mode to `app_sm_mode_t` enum in `app_state_machine.h`
4. Register the mode in `modes.c` mode_registry array
5. Add the source file to the Makefile

## Animation Thread

The animation thread provides smooth, jitter-free LED animations:

```c
/* Set solid color */
anim_thread_set_solid(255, 0, 0, 128);  /* Red at 50% brightness */

/* Fade in */
anim_thread_fade_in(0, 255, 0, 255, 1000);  /* Green, 1 second */

/* Start blinking */
anim_thread_blink(255, 255, 255, 128, 500);  /* White, 500ms interval */

/* Rainbow animation */
anim_thread_rainbow(128, 5000);  /* 5 second cycle */

/* Turn off */
anim_thread_off();
```

## Future Enhancements

- [ ] Flash persistence for mode/settings
- [ ] Low power mode in OFF state
- [ ] External control via WiFi/BLE
- [ ] More animation patterns
- [ ] Custom color selection
