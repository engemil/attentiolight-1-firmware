/*
MIT License

Copyright (c) 2026 EngEmil

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/**
 * @file    animation_commands.h
 * @brief   Animation command types for the animation thread.
 */

#ifndef ANIMATION_COMMANDS_H
#define ANIMATION_COMMANDS_H

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================*/
/* Animation Command Types                                                   */
/*===========================================================================*/


/* TO DO: Add ANIM_CMD_CUSTOM for user-defined animations via external control. */ 
/**
 * @brief   Animation command types.
 */
typedef enum {
    ANIM_CMD_STOP = 0,          /**< Stop animation, turn LEDs off          */
    ANIM_CMD_SOLID,             /**< Set solid color (no animation)         */
    ANIM_CMD_BLINK,             /**< Blink on/off pattern                   */
    ANIM_CMD_PULSE,             /**< Breathing/pulse effect                 */
    ANIM_CMD_RAINBOW,           /**< Rainbow color cycle                    */
    ANIM_CMD_COLOR_CYCLE,       /**< Cycle through color palette            */
    ANIM_CMD_TRAFFIC_LIGHT,     /**< Traffic light R-Y-G sequence           */
    ANIM_CMD_FLASH_FEEDBACK,    /**< Quick flash for button feedback        */
    ANIM_CMD_POWERUP_SEQUENCE,  /**< powerup animation                      */
    ANIM_CMD_POWERDOWN_SEQUENCE,/**< Powerdown animation                    */
    /* Effects mode animations */
    ANIM_CMD_BREATHING,         /**< Asymmetric breathing (slow in, slower out) */
    ANIM_CMD_CANDLE,            /**< Candle flicker effect                  */
    ANIM_CMD_FIRE,              /**< Fire flickering effect                 */
    ANIM_CMD_LAVA_LAMP,         /**< Organic slow color morphing            */
    ANIM_CMD_DAY_NIGHT,         /**< Day/night cycle (sunrise/sunset loop)  */
    ANIM_CMD_OCEAN,             /**< Ocean blue/cyan gentle waves           */
    ANIM_CMD_NORTHERN_LIGHTS,   /**< Aurora-like green/purple/blue shifting */
    ANIM_CMD_THUNDER_STORM,     /**< Dark blue with random lightning        */
    ANIM_CMD_POLICE,            /**< Red/blue with smooth transitions       */
    ANIM_CMD_HEALTH_PULSE,      /**< Red heartbeat double-pulse             */
    ANIM_CMD_MEMORY             /**< Random warm soft glows, irregular timing */
} anim_cmd_type_t;

/**
 * @brief   Animation command structure.
 * @details Sent to animation thread via mailbox.
 */
typedef struct {
    anim_cmd_type_t type;       /**< Animation type                         */
    uint8_t r;                  /**< Target red component                   */
    uint8_t g;                  /**< Target green component                 */
    uint8_t b;                  /**< Target blue component                  */
    uint8_t brightness;         /**< Brightness level (0-255)               */
    uint16_t period_ms;         /**< Animation period/speed in ms           */
    uint8_t param;              /**< Mode-specific parameter                */
} anim_command_t;

/**
 * @brief   Animation state for tracking progress.
 */
typedef struct {
    anim_cmd_type_t current_type;   /**< Current animation type             */
    uint8_t target_r;               /**< Target red                         */
    uint8_t target_g;               /**< Target green                       */
    uint8_t target_b;               /**< Target blue                        */
    uint8_t current_r;              /**< Current red                        */
    uint8_t current_g;              /**< Current green                      */
    uint8_t current_b;              /**< Current blue                       */
    uint8_t brightness;             /**< Current brightness                 */
    uint16_t period_ms;             /**< Animation period                   */
    uint32_t start_time;            /**< Animation start time               */
    uint32_t elapsed_ms;            /**< Elapsed time in current cycle      */
    uint8_t phase;                  /**< Animation phase (for multi-phase)  */
    bool active;                    /**< Animation is active                */
} anim_state_t;

#endif /* ANIMATION_COMMANDS_H */
