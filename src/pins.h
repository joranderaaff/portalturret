#pragma once

#ifdef ESP32
    #define AUDIO_BUSY 5
    #define SERVO_WING 11
    #define SERVO_ROTATE 9
#else
    #define SERVO_WING 12
    #define SERVO_ROTATE 13
    #ifdef LEGACY
        #define AUDIO_BUSY A0
    #else
        #define AUDIO_BUSY D0
    #endif
#endif