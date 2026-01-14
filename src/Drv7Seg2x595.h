/*************** FILE DESCRIPTION ***************/

/**
 * Filename: Drv7Seg2x595.h
 * ----------------------------------------------------------------------------|---------------------------------------|
 * Purpose:  A class for driving a multiplexed 7-segment display using
 *           two daisy-chained 74HC595 shift register ICs.
 * ----------------------------------------------------------------------------|---------------------------------------|
 * Notes:    Refer to the README for a general library overview and
 *           a basic API usage description.
 *
 *           Intended for displays with 1 to 4 character positions (digits).
 *
 *           seg_byte means a byte that turns ON and OFF individual segments.
 *
 *           pos_byte means a byte that turns ON and OFF whole character
 *           positions (digits).
 */


/************ PREPROCESSOR DIRECTIVES ***********/

// Include guards.
#ifndef DRV7SEG2X595_H
#define DRV7SEG2X595_H


/*--- Includes ---*/

// Arduino core header file.
#include <Arduino.h>


/*--- Misc ---*/

/* Comment out if SPI.h isn't provided for the Arduino core you're using or
 * for the specific board (device) you're using (a rare case, but not impossible).
 *
 * __has_include() macro is deliberately avoided due to limited portability.
 */
#define DRV7SEG2X595_SPI_PROVIDED_ASSUMED

/* Duration (in microseconds) of a short period during which
 * a currently output glyph is retained on a respective character position.
 */
#define DRV7SEG2X595_ANTI_GHOSTING_DEFAULT_RETENTION_DURATION 600

#define DRV7SEG2X595_POS_MIN 1
#define DRV7SEG2X595_POS_MAX 4

#define DRV7SEG2X595_BITS_IN_BYTE          8
#define DRV7SEG2X595_MSB                   (DRV7SEG2X595_BITS_IN_BYTE - 1)
#define DRV7SEG2X595_LSB                   0
#define DRV7SEG2X595_ONLY_LSB_SET_MASK     0x01u
#define DRV7SEG2X595_ONLY_MSB_SET_MASK     (DRV7SEG2X595_ONLY_LSB_SET_MASK << DRV7SEG2X595_MSB)
#define DRV7SEG2X595_ALL_BITS_SET_MASK     0xFF
#define DRV7SEG2X595_ALL_BITS_CLEARED_MASK 0x00

// Driver configuration status codes. Double as return codes for some methods.
#define DRV7SEG2X595_STATUS_INITIAL                     -1
#define DRV7SEG2X595_STATUS_ERR_VARIANT_NOT_SPECIFIED   -2
#define DRV7SEG2X595_STATUS_ERR_INVALID_BYTE_ORDER      -3
#define DRV7SEG2X595_STATUS_ERR_INVALID_POS_SWITCH_TYPE -4
#define DRV7SEG2X595_STATUS_ERR_INVALID_POS_BIT         -5
#define DRV7SEG2X595_STATUS_ERR_POS_BIT_DUPLICATION     -6
#define DRV7SEG2X595_STATUS_OK                           0

// set_glyph_to_pos() method additional return codes.
#define DRV7SEG2X595_SET_GLYPH_ERR_INVALID_POS                   -7
#define DRV7SEG2X595_SET_GLYPH_ERR_POS_BIT_NOT_SPECIFIED_FOR_POS -8
#define DRV7SEG2X595_SET_GLYPH_OK                                 0

// output() method additional return codes.
#define DRV7SEG2X595_OUTPUT_ERR_INVALID_POS                   -7
#define DRV7SEG2X595_OUTPUT_ERR_POS_BIT_NOT_SPECIFIED_FOR_POS -8
#define DRV7SEG2X595_OUTPUT_NEXT                               0
#define DRV7SEG2X595_OUTPUT_ANTI_GHOSTING_RETENTION_RUNNING    1

#if defined(DRV7SEG2X595_SPI_PROVIDED_ASSUMED) || \
    defined(ARDUINO_ARCH_AVR)                  || \
    defined(ARDUINO_ARCH_MEGAAVR)              || \
    defined(ARDUINO_ARCH_SAM)                  || \
    defined(ARDUINO_ARCH_SAMD)                 || \
    defined(ARDUINO_ARCH_MBED)                 || \
    defined(ARDUINO_ARCH_ESP8266)              || \
    defined(ARDUINO_ARCH_ESP32)                || \
    defined(ARDUINO_ARCH_STM32)                || \
    defined(ARDUINO_ARCH_RP2040)               || \
    defined(ARDUINO_ARCH_RENESAS)              || \
    defined(ARDUINO_ARCH_NRF52)
    #define DRV7SEG2X595_SPI_PROVIDED
#endif

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_STM32)
    #define DRV7SEG2X595_SPI_PROVIDED_CUSTOM_PINS
#endif

// Driver configuration variant codes.
#define DRV7SEG2X595_VARIANT_INITIAL     -1
#define DRV7SEG2X595_VARIANT_BIT_BANGING  0
#ifdef DRV7SEG2X595_SPI_PROVIDED
    #define DRV7SEG2X595_VARIANT_SPI      1
#endif


/****************** DATA TYPES ******************/

class Drv7Seg2x595Class {
    public:
        /*--- Data types ---*/

        enum class ByteOrder {
            PosByteFirst = 0,
            SegByteFirst = 1
        };

        enum class PosSwitchType {
            ActiveLow  = 0,
            ActiveHigh = 1
        };

        enum class PosBit {
            PosBitInitial = -1,
            PosBit0       =  DRV7SEG2X595_LSB,  // 0
            PosBit1       =  1,
            PosBit2       =  2,
            PosBit3       =  3,
            PosBit4       =  4,
            PosBit5       =  5,
            PosBit6       =  6,
            PosBit7       =  DRV7SEG2X595_MSB   // 7
        };

        enum class Pos {
            Pos1 =  DRV7SEG2X595_POS_MIN,  // 1
            Pos2 =  2,
            Pos3 =  3,
            Pos4 =  DRV7SEG2X595_POS_MAX   // 4
        };


        /*--- Methods ---*/

        // Default constructor.
        Drv7Seg2x595Class();

        /* Configure the driver to use bit-banging.
         *
         * Returns: zero if configuration was successful (if all passed parameters are valid),
         * a negative integer otherwise (see the preprocessor macros list for possible values).
         *
         * Parameters:
         * - byte_order                     - within a 16-bit register formed by two ICs either
         *                                    pos_byte is an upper byte and seg_byte is a lower byte or
         *                                    seg_byte is an upper byte and pos_byte is a lower byte.
         * - pos_switch_type                - character positions are turned on either by
         *                                    set (active-high) or cleared (active-low) pos_byte bits.
         * - data_pin, latch_pin, clock_pin - pins used for bit-banging and latching.
         * - pos_N_bit                      - pos_byte bits that control character positions.
         *                                    pos_1_bit is required, other bits are optional
         *                                    (respective parameters can be omitted).
         *
         * Multiple calls to this method are valid, each call leads to a fresh configuration.
         */
        int32_t begin_bb(ByteOrder byte_order,
                         PosSwitchType pos_switch_type,
                         uint32_t data_pin,
                         uint32_t latch_pin,
                         uint32_t clock_pin,
                         PosBit pos_1_bit,
                         PosBit pos_2_bit = PosBit::PosBitInitial,
                         PosBit pos_3_bit = PosBit::PosBitInitial,
                         PosBit pos_4_bit = PosBit::PosBitInitial
                        );

        /* Configure the driver to use SPI with default pins.
         *
         * Returns: equivalent to begin_bb().
         *
         * Parameters: mostly equivalent to begin_bb(), but
         * data_pin and clock_pin aren't specified
         * (default MOSI and SCK pins will be used instead).
         *
         * For many hardware platforms and SPI.h implementations default SPI pins
         * are the only SPI pins available (custom SPI pins cannot be assigned).
         *
         * Safety of multiple calls to this method depends on the SPI.h implementation.
         */
        #ifdef DRV7SEG2X595_SPI_PROVIDED
        int32_t begin_spi(ByteOrder byte_order,
                          PosSwitchType pos_switch_type,
                          uint32_t latch_pin,
                          PosBit pos_1_bit,
                          PosBit pos_2_bit = PosBit::PosBitInitial,
                          PosBit pos_3_bit = PosBit::PosBitInitial,
                          PosBit pos_4_bit = PosBit::PosBitInitial
                         );
        #endif

        /* Configure the driver to use SPI with custom-assigned pins.
         *
         * Returns: equivalent to begin_bb() and begin_spi().
         *
         * Parameters: mostly equivalent to begin_bb(), but
         * data_pin is replaced with mosi_pin and clock_pin is replaced with sck_pin.
         *
         * This method is only available (via conditional compilation) for those hardware platforms that
         * support custom SPI pins assignment and have an SPI.h implementation that reflects such support
         * (as of the last library update those are ESP32 and STM32).
         *
         * Safety of multiple calls to this method depends on the SPI.h implementation.
         */
        #ifdef DRV7SEG2X595_SPI_PROVIDED_CUSTOM_PINS
        int32_t begin_spi_custom_pins(ByteOrder byte_order,
                                      PosSwitchType pos_switch_type,
                                      uint32_t mosi_pin,
                                      uint32_t latch_pin,
                                      uint32_t sck_pin,
                                      PosBit pos_1_bit,
                                      PosBit pos_2_bit = PosBit::PosBitInitial,
                                      PosBit pos_3_bit = PosBit::PosBitInitial,
                                      PosBit pos_4_bit = PosBit::PosBitInitial
                                     );
        #endif

        /* Get the last driver configuration status.
         *
         * Returns: zero if driver configuration was successful, a negative integer otherwise
         * (see the preprocessor macros list for possible values).
         */
        int32_t get_status();

        /* Assign a glyph to be output on a specified position.
         *
         * Returns:
         * - a negative integer if driver configuration had failed or not all passed parameters are valid
         *   (see the preprocessor macros list for possible values).
         * - zero if the glyph was successfully assigned.
         *
         * Parameters:
         * - seg_byte - a byte that corresponds to a glyph to be output.
         * - pos      - a number of the character position (digit) the next glyph
         *              must be output on.
         */
        int32_t set_glyph_to_pos(uint8_t seg_byte, Pos pos);

        /* Output a glyph on a specified character position.
         *
         * Shifts four bytes into two daisy-chained ICs:
         * - two blank bytes for anti-ghosting purposes.
         * - two bytes of payload (seg_byte and pos_byte).
         * After every second byte latches the data into the ICs' outer register.
         *
         * Returns:
         * - a negative integer if driver configuration had failed or not all passed parameters are valid
         *   (see the preprocessor macros list for possible values).
         * - zero if the program execution has reached the next glyph output sequence.
         * - a positive integer if an anti-ghosting retention is running.
         *
         * Parameters:
         * - seg_byte - a byte that corresponds to a glyph to be output.
         * - pos      - a number of the character position (digit) the next glyph
         *              must be output on.
         */
        int32_t output(uint8_t seg_byte,
                       Pos pos
                      );

        /* Output the glyphs assigned to each valid character position in quick succession.
         *
         * Returns: nothing.
         */
        void output_all();

        /* Set new anti-ghosting retention duration.
         *
         * Sets the duration (in microseconds) of a short period during which
         * a currently output glyph is retained on a respective position.
         * Until this method is called, the default value is applied.
         *
         * Returns: nothing.
         */
        void set_anti_ghosting_retention_duration(uint32_t new_val);

    private:
        /*--- Variables ---*/

        int32_t _status  = DRV7SEG2X595_STATUS_INITIAL;
        int32_t _variant = DRV7SEG2X595_VARIANT_INITIAL;

        ByteOrder     _byte_order;
        PosSwitchType _pos_switch_type;

        // Used in all variants.
        uint32_t _latch_pin;

        // Used in the bit-banging variant.
        uint32_t _data_pin;
        uint32_t _clock_pin;

        #ifdef DRV7SEG2X595_SPI_PROVIDED_CUSTOM_PINS
        // Used in the custom SPI pins variant.
        uint32_t _mosi_pin;
        uint32_t _sck_pin;
        #endif

        // pos_byte bits that may correspond to the actual display character positions (digits).
        PosBit _pos_bits[DRV7SEG2X595_POS_MAX] = {PosBit::PosBitInitial,
                                                  PosBit::PosBitInitial,
                                                  PosBit::PosBitInitial,
                                                  PosBit::PosBitInitial
                                                 };

        // Glyphs assigned to be output next.
        uint8_t _pos_glyphs[DRV7SEG2X595_POS_MAX] = {0};

        // Elements of the anti-ghosting logic.
        uint32_t _anti_ghosting_retention_duration = DRV7SEG2X595_ANTI_GHOSTING_DEFAULT_RETENTION_DURATION;
        bool     _anti_ghosting_first_output_call = true;
        Pos      _anti_ghosting_retained_pos;
        uint32_t _anti_ghosting_timer_previous_micros;


        /*--- Methods ---*/

        /* Helper method that does most of the begin_*() methods' job by
         * handling checks and assignments common for all of them.
         *
         * Returns: zero if the respective configuration stage was successful (if all passed parameters are
         * valid), a negative integer otherwise (see the preprocessor macros list for possible values).
         */
        int32_t begin_helper(int32_t config_variant,
                             ByteOrder byte_order,
                             PosSwitchType pos_switch_type,
                             uint32_t latch_pin,

                             PosBit pos_1_bit,
                             /* Following parameters are assigned with values even if those were omitted
                              * in a begin_*() method call (in this case default values are assigned).
                              */
                             PosBit pos_2_bit,
                             PosBit pos_3_bit,
                             PosBit pos_4_bit
                            );

        /* Send a single byte to a shift register.
         *
         * Returns: nothing.
         *
         * A reimplementation of the shiftOut() function commonly included in Arduino cores.
         * The main difference is the added digitalWrite() call that sets the _clock_pin output level to LOW before
         * the actual shifting sequence begins. This is done because the de facto standard shiftOut() implementation
         * doesn't handle the initial clock pin output level, which may result in a lack of low-to-high transition and
         * thus in the first bit not getting shifted in case a clock pin is set to HIGH for some reason before
         * the shifting starts.
         */
        void shift_out(uint8_t byte_to_shift);

        /* Find out which character position (digit) must be turned on
         * next after the current retention period is over.
         *
         * Returns: a value of enum class Pos type in the Pos1..Pos4 range
         * (corresponds to the 1..4 range of the underlying integer type).
         */
        Pos  anti_ghosting_next_pos_to_output();

        /* Find out if the anti-ghosting retention timer has elapsed.
         *
         * Returns: true if the timer has elapsed, false otherwise.
         */
        bool anti_ghosting_timer();
};

// Class-related aliases.
constexpr Drv7Seg2x595Class::ByteOrder Drv7SegPosByteFirst = Drv7Seg2x595Class::ByteOrder::PosByteFirst;
constexpr Drv7Seg2x595Class::ByteOrder Drv7SegSegByteFirst = Drv7Seg2x595Class::ByteOrder::SegByteFirst;

constexpr Drv7Seg2x595Class::PosSwitchType Drv7SegActiveLow  = Drv7Seg2x595Class::PosSwitchType::ActiveLow;
constexpr Drv7Seg2x595Class::PosSwitchType Drv7SegActiveHigh = Drv7Seg2x595Class::PosSwitchType::ActiveHigh;

constexpr Drv7Seg2x595Class::PosBit Drv7SegPosBitInitial = Drv7Seg2x595Class::PosBit::PosBitInitial;
constexpr Drv7Seg2x595Class::PosBit Drv7SegPosBit0       = Drv7Seg2x595Class::PosBit::PosBit0;
constexpr Drv7Seg2x595Class::PosBit Drv7SegPosBit1       = Drv7Seg2x595Class::PosBit::PosBit1;
constexpr Drv7Seg2x595Class::PosBit Drv7SegPosBit2       = Drv7Seg2x595Class::PosBit::PosBit2;
constexpr Drv7Seg2x595Class::PosBit Drv7SegPosBit3       = Drv7Seg2x595Class::PosBit::PosBit3;
constexpr Drv7Seg2x595Class::PosBit Drv7SegPosBit4       = Drv7Seg2x595Class::PosBit::PosBit4;
constexpr Drv7Seg2x595Class::PosBit Drv7SegPosBit5       = Drv7Seg2x595Class::PosBit::PosBit5;
constexpr Drv7Seg2x595Class::PosBit Drv7SegPosBit6       = Drv7Seg2x595Class::PosBit::PosBit6;
constexpr Drv7Seg2x595Class::PosBit Drv7SegPosBit7       = Drv7Seg2x595Class::PosBit::PosBit7;

constexpr Drv7Seg2x595Class::Pos Drv7SegPos1 = Drv7Seg2x595Class::Pos::Pos1;
constexpr Drv7Seg2x595Class::Pos Drv7SegPos2 = Drv7Seg2x595Class::Pos::Pos2;
constexpr Drv7Seg2x595Class::Pos Drv7SegPos3 = Drv7Seg2x595Class::Pos::Pos3;
constexpr Drv7Seg2x595Class::Pos Drv7SegPos4 = Drv7Seg2x595Class::Pos::Pos4;


/*************** GLOBAL VARIABLES ***************/

/* An Arduino-style singleton object.
 * More instances of the same class can be created if necessary.
 */
extern Drv7Seg2x595Class Drv7Seg;


#endif  // Include guards.
