#pragma once

#ifdef ESP32
    #ifdef HARDWARE_V3
        #define SDA 14
        #define SCL 13
        #define SERVO_WING 6
        #define SERVO_ROTATE 4
        #define WING_SWITCH 3
        #define PID 11
        #define CENTER_LED 8
        #define GUN_LEDS 10
        #define RING_LEDS 12
        #define BCK 7
        #define WS 9
        #define DOUT 5
    #else
        #define AUDIO_BUSY 5
        #define SERVO_WING 11
        #define SERVO_ROTATE 9
        #define WING_SWITCH 7
        #define PID 2
        #define CENTER_LED 18
        #define GUN_LEDS 16
        #define RING_LEDS 12
        #define AUDIO_RX 37
        #define AUDIO_TX 39
    #endif
#else
    #define RING_LEDS D8
    #ifdef LEGACY
        #define AUDIO_BUSY A0
        #define WING_SWITCH D0
        #define PID D7
        #define CENTER_LED 0
        #define GUN_RIGHT 13
        #define GUN_LEFT 12
        #define SERVO_WING 7
        #define SERVO_ROTATE 8
        #define AUDIO_RX D5
        #define AUDIO_TX D6
    #else
        #define AUDIO_BUSY D0
        #define WING_SWITCH D5
        #define PID A0
        #define GUN_RIGHT 13
        #define GUN_LEFT 12
        #define SERVO_WING 12
        #define SERVO_ROTATE 13        
        #define CENTER_LED D3
        #define GUN_LEDS D4
        #define AUDIO_RX RX
        #define AUDIO_TX TX
    #endif
#endif
