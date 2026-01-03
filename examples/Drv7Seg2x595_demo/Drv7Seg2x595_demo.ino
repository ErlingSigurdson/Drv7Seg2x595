/*************** FILE DESCRIPTION ***************/

/**
 * Filename: Drv7Seg2x595_demo.ino
 * ----------------------------------------------------------------------------|---------------------------------------|
 * Purpose:  An example sketch demonstrating basic usage of the Drv7Seg2x595
 *           library.
 *
 *           Runs a simple electronic clock: counts seconds and minutes
 *           and outputs current values on a 7-segment 4-digit display.
 *           Additionally, prints the values and diagnostic information
 *           via UART.
 *
 *           The dot segment (decimal point, DP) of the second character
 *           position (second digit, where the minutes' ones are output)
 *           blinks (toggles ON or OFF) once per second (at 0.5 Hz).
 * ----------------------------------------------------------------------------|---------------------------------------|
 * Notes:    Refer to the README for a general library overview and
 *           a basic API usage description.
 *
 *           Refer to Drv7Seg2x595.h for more API details.
 *
 *           Requires installation of SegMap595 library
 *           (available from Arduino Library Manager).
 */


/************ PREPROCESSOR DIRECTIVES ***********/

/*--- Includes ---*/

#include <Drv7Seg2x595.h>

/* Helper library.
 * Provides a user-friendly API for mapping the parallel outputs of the 74HC595 shift register IC
 * (the one that corresponds to seg_byte) to the segment control pins of the 7-segment display.
 */
#include <SegMap595.h>


/*--- Drv7Seg2x595 library API parameters ---*/

/* Specify the data transfer approach. Use one variant, comment out or delete the others.
 *
 * Bit-banging uses regular digital output pins.
 *
 * SPI variant assumes an SPI.h implementation is provided for your board (device).
 * All major Arduino cores have it, although it's not guaranteed that every single
 * Arduino core in the world will have it as well.
 *
 * SPI variant with custom-assigned pins is only available for those hardware platforms
 * that support custom SPI pins assignment and have an SPI.h implementation that reflects
 * such support (as of the last library update those are ESP32 and STM32).
 */
#define USE_BIT_BANGING
//#define USE_SPI_DEFAULT_PINS
//#define USE_SPI_CUSTOM_PINS

/* Specify the order in which seg_byte and pos_byte are placed within
 * the shift register. Use one variant, comment out or delete the other.
 */
#define BYTE_SHIFT_ORDER Drv7SegPosByteFirst
//#define BYTE_SHIFT_ORDER Drv7SegSegByteFirst

/* Specify the signal level that turns on the character positions of your display.
 * Use one variant, comment out or delete the other.
 */
//#define POS_SWITCH_TYPE Drv7SegActiveLow
#define POS_SWITCH_TYPE Drv7SegActiveHigh

// Specify appropriately based on your wiring. Variant for bit-banging.
#ifdef USE_BIT_BANGING
    #define DATA_PIN  16
    #define LATCH_PIN 17
    #define CLOCK_PIN 18
#endif

// Specify appropriately based on your wiring. Variant for SPI with default pins.
#ifdef USE_SPI_DEFAULT_PINS
    #define LATCH_PIN 17
#endif

// Specify appropriately based on your wiring. Variant for SPI with custom-assigned pins.
#ifdef USE_SPI_CUSTOM_PINS
    #define MOSI_PIN  16
    #define LATCH_PIN 17
    #define SCK_PIN   18
#endif

/* Specify appropriately based on which pos_byte bits control
 * the character positions of your 7-segment display.
 *
 * POS_1_BIT means the leftmost character position (often referred to as D1 in 7-segment display pinout diagrams).
 * POS_4_BIT means the rightmost character position (often referred to as D4 in 7-segment display pinout diagrams).
 *
 * Valid arguments are Drv7SegPosBitN, where N is in the 0..7 range (MSB to LSB of pos_byte).
 */
#define POS_1_BIT Drv7SegPosBit7  // Assumes that D1 is connected to Q7 (Q is for a 595's parallel output number).
#define POS_2_BIT Drv7SegPosBit5  // Assumes that D2 is connected to Q5.
#define POS_3_BIT Drv7SegPosBit3  // Assumes that D3 is connected to Q3.
#define POS_4_BIT Drv7SegPosBit1  // Assumes that D4 is connected to Q1.

// (optional) Set a non-default anti-ghosting retention duration value (in microseconds).
#define ANTI_GHOSTING_RETENTION_DURATION 1000


/*--- SegMap595 library API parameters ---*/

/* Map string.
 *
 * This string must reflect the actual (physical) order of connections made between
 * the parallel outputs of your 74HC595 and the segment control pins of your 7-segment display.
 *
 * The map string must consist of exactly 8 ASCII characters: @, A, B, C, D, E, F and G.
 * Every character corresponds to a single segment (@ stands for a dot, also known as a decimal point or DP).
 *
 * The first (leftmost) character in the map string corresponds to the 7th bit (most significant bit, MSB) of a byte
 * stored in the shift register, connected to the Q7 parallel output.
 * The last (rightmost) character in the map string corresponds to the 0th bit (least significant bit, LSB) of a byte
 * stored in the shift register, connected to the Q0 parallel output.
 *
 * Uppercase characters may be replaced with their lowercase counterparts. Any other characters are invalid.
 * Duplicating characters is invalid as well.
 */
#define MAP_STR "ED@CGAFB"

// Specify your display type based on its common pin. Use one variant, comment out or delete the other.
#define DISPLAY_COMMON_PIN SegMap595CommonCathode
//#define DISPLAY_COMMON_PIN SegMap595CommonAnode

// Select a glyph set. Use one variant, comment out or delete the other.
#define GLYPH_SET_ID SegMap595GlyphSet1
//#define GLYPH_SET_ID SegMap595GlyphSet2


/*--- Misc ---*/

#define MAX_COUNT_MINUTES 60
#define MAX_COUNT_SECONDS 60

// Comment out or delete to suppress the output of current timer values via UART.
#define SERIAL_OUTPUT_TIMER_VALUES

// Set appropriately based on the baud rate you use.
#define BAUD_RATE 115200

// Output interval ("once every X milliseconds").
#define INTERVAL 1000


/******************* FUNCTIONS ******************/

void setup()
{
    #ifdef SERIAL_OUTPUT_TIMER_VALUES
    Serial.begin(BAUD_RATE);
    #endif


    /*--- Byte mapping ---*/

    SegMap595.init(MAP_STR, DISPLAY_COMMON_PIN, GLYPH_SET_ID);

    /* Mapping status check.
     * You can also check the value returned by init() instead of calling get_status().
     */
    int32_t mapping_status = SegMap595.get_status();

    // Loop the error output if the mapping was unsuccessful.
    if (mapping_status < 0) {  // If an error is detected.
        while(true) {
            Serial.print("Error: mapping failed, error code ");
            Serial.println(mapping_status);
            delay(INTERVAL);
        }
    }


    /*--- Driver object configuration ---*/

    #ifdef USE_BIT_BANGING
    Drv7Seg.begin_bb(BYTE_SHIFT_ORDER, POS_SWITCH_TYPE,
                     DATA_PIN, LATCH_PIN, CLOCK_PIN,
                     POS_1_BIT,
                     POS_2_BIT,
                     POS_3_BIT,
                     POS_4_BIT
                    );
    #endif

    #ifdef USE_SPI_DEFAULT_PINS
    Drv7Seg.begin_spi(BYTE_SHIFT_ORDER, POS_SWITCH_TYPE,
                      LATCH_PIN,
                      POS_1_BIT,
                      POS_2_BIT,
                      POS_3_BIT,
                      POS_4_BIT
                     );
    #endif

    #ifdef USE_SPI_CUSTOM_PINS
    Drv7Seg.begin_spi_custom_pins(BYTE_SHIFT_ORDER, POS_SWITCH_TYPE,
                                  MOSI_PIN, LATCH_PIN, SCK_PIN,
                                  POS_1_BIT,
                                  POS_2_BIT,
                                  POS_3_BIT,
                                  POS_4_BIT
                                 );
    #endif

    /* Driver configuration status check.
     * You can also check the value returned by begin_*() instead of calling get_status().
     */
    int32_t drv_config_status = Drv7Seg.get_status();
    // Loop the error output if the driver configuration was unsuccessful.
    if (drv_config_status < 0) {  // If an error is detected.
        while(true) {
            Serial.print("Error: driver configuration failed, error code ");
            Serial.println(drv_config_status);
            delay(INTERVAL);
        }
    }

    // (optional) Override the default anti-ghosting retention duration value.
    #ifdef ANTI_GHOSTING_RETENTION_DURATION
    Drv7Seg.set_anti_ghosting_retention_duration(ANTI_GHOSTING_RETENTION_DURATION);
    #endif
}

void loop()
{
    /*--- Counter and value update trigger ---*/

    // Counter.
    uint32_t current_millis = millis();
    static uint32_t previous_millis = current_millis;

    static size_t counter_seconds = 0;
    static size_t counter_minutes = 0;

    if (counter_seconds >= MAX_COUNT_SECONDS) {
        ++counter_minutes;
        counter_seconds = 0;
    }

    if (counter_minutes >= MAX_COUNT_MINUTES) {
        counter_minutes = 0;
    }

    // Value update trigger.
    static bool update_due = true;


    /*--- Demo output ---*/

    if (update_due) {
        uint8_t seg_byte_minutes_tens = SegMap595.get_mapped_byte(counter_minutes / 10);
        uint8_t seg_byte_minutes_ones = SegMap595.get_mapped_byte(counter_minutes % 10);
        uint8_t seg_byte_seconds_tens = SegMap595.get_mapped_byte(counter_seconds / 10);
        uint8_t seg_byte_seconds_ones = SegMap595.get_mapped_byte(counter_seconds % 10);

        // Dot-segment blink.
        if (counter_seconds % 2) {
            seg_byte_minutes_ones = SegMap595.toggle_dot(seg_byte_minutes_ones);
        }

        Drv7Seg.set_glyph_to_pos(seg_byte_minutes_tens, Drv7SegPos1);
        Drv7Seg.set_glyph_to_pos(seg_byte_minutes_ones, Drv7SegPos2);
        Drv7Seg.set_glyph_to_pos(seg_byte_seconds_tens, Drv7SegPos3);
        Drv7Seg.set_glyph_to_pos(seg_byte_seconds_ones, Drv7SegPos4);

        #ifdef SERIAL_OUTPUT_TIMER_VALUES
            Serial.print("Timer values (minutes and seconds): ");
            Serial.print(counter_minutes / 10);
            Serial.print(counter_minutes % 10);
            Serial.print(":");
            Serial.print(counter_seconds / 10);
            Serial.println(counter_seconds % 10);
        #endif

        update_due = false;
    }

    // Commence output.
    Drv7Seg.output_all();


    /*--- Counter and value update trigger, continued ---*/

    if (current_millis - previous_millis >= INTERVAL) {
        ++counter_seconds;
        update_due = true;
        previous_millis = current_millis;
    }
}
