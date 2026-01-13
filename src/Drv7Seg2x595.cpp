/*************** FILE DESCRIPTION ***************/

/**
 * Filename: Drv7Seg2x595.cpp
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

/*--- Includes ---*/

// This source file's own header file.
#include "Drv7Seg2x595.h"

// Additional Arduino libraries.
#ifdef DRV7SEG2X595_SPI_PROVIDED
    #include <SPI.h>
#endif


/*************** GLOBAL VARIABLES ***************/

Drv7Seg2x595Class Drv7Seg;


/******************* FUNCTIONS ******************/

/*--- Constructors ---*/

Drv7Seg2x595Class::Drv7Seg2x595Class() {}


/*--- Public methods ---*/

int32_t Drv7Seg2x595Class::begin_bb(ByteOrder byte_order,
                                    PosSwitchType pos_switch_type,
                                    uint32_t data_pin,
                                    uint32_t latch_pin,
                                    uint32_t clock_pin,
                                    PosBit pos_1_bit,
                                    PosBit pos_2_bit,
                                    PosBit pos_3_bit,
                                    PosBit pos_4_bit
                                   )
{
    _status = begin_helper(DRV7SEG2X595_VARIANT_BIT_BANGING,
                           byte_order,
                           pos_switch_type,
                           latch_pin,
                           pos_1_bit,
                           pos_2_bit,
                           pos_3_bit,
                           pos_4_bit
                          );

    if (_status < 0) {
        return _status;
    }

    _data_pin  = data_pin;
    _clock_pin = clock_pin;
    pinMode(_data_pin,  OUTPUT);
    pinMode(_clock_pin, OUTPUT);

    return _status;
}

#ifdef DRV7SEG2X595_SPI_PROVIDED
int32_t Drv7Seg2x595Class::begin_spi(ByteOrder byte_order,
                                     PosSwitchType pos_switch_type,
                                     uint32_t latch_pin,
                                     PosBit pos_1_bit,
                                     PosBit pos_2_bit,
                                     PosBit pos_3_bit,
                                     PosBit pos_4_bit
                                    )
{
    _status = begin_helper(DRV7SEG2X595_VARIANT_SPI,
                           byte_order,
                           pos_switch_type,
                           latch_pin,
                           pos_1_bit,
                           pos_2_bit,
                           pos_3_bit,
                           pos_4_bit
                          );

    if (_status < 0) {
        return _status;
    }

    SPI.begin();

    return _status;
}
#endif

#ifdef DRV7SEG2X595_SPI_PROVIDED_CUSTOM_PINS
int32_t Drv7Seg2x595Class::begin_spi_custom_pins(ByteOrder byte_order,
                                                 PosSwitchType pos_switch_type,
                                                 uint32_t mosi_pin,
                                                 uint32_t latch_pin,
                                                 uint32_t sck_pin,
                                                 PosBit pos_1_bit,
                                                 PosBit pos_2_bit,
                                                 PosBit pos_3_bit,
                                                 PosBit pos_4_bit
                                                )
{
    _status = begin_helper(DRV7SEG2X595_VARIANT_SPI,
                           byte_order,
                           pos_switch_type,
                           latch_pin,
                           pos_1_bit,
                           pos_2_bit,
                           pos_3_bit,
                           pos_4_bit
                          );

    if (_status < 0) {
        return _status;
    }

    _mosi_pin = mosi_pin;
    _sck_pin  = sck_pin;

    #if defined(ARDUINO_ARCH_ESP32)
        SPI.begin(_sck_pin, -1, _mosi_pin, -1);
    #elif defined(ARDUINO_ARCH_STM32)
        SPI.setMOSI(_mosi_pin);
        SPI.setSCLK(_sck_pin);
        SPI.begin();
    #endif

    return _status;
}
#endif

int32_t Drv7Seg2x595Class::get_status()
{
    return _status;
}

int32_t Drv7Seg2x595Class::set_glyph_to_pos(uint8_t seg_byte, Pos pos)
{
    /*--- Configuration status check ---*/

    if (_status < 0) {
        return _status;
    }


    /*--- Protection from unexpected casts ---*/

    if (pos < Drv7SegPos1 || pos > Drv7SegPos4) {
        return DRV7SEG2X595_SET_GLYPH_ERR_INVALID_POS;
    }


    /*--- Assign a glyph to a position ---*/

    size_t pos_as_index = static_cast<size_t>(pos) - 1;
    if (_pos_bits[pos_as_index] == Drv7SegPosBitInitial) {
        return DRV7SEG2X595_SET_GLYPH_ERR_POS_BIT_NOT_SPECIFIED_FOR_POS;
    } else {
        _pos_glyphs[pos_as_index] = seg_byte;
        return DRV7SEG2X595_SET_GLYPH_OK;
    }
}

int32_t Drv7Seg2x595Class::output(uint8_t seg_byte,
                                  Pos pos
                                 )
{
    /*--- Configuration status check ---*/

    if (_status < 0) {
        return _status;
    }


    /*--- Protection from unexpected casts ---*/

    if (pos < Drv7SegPos1 || pos > Drv7SegPos4) {
        return DRV7SEG2X595_OUTPUT_ERR_INVALID_POS;
    }


    /*--- Anti-ghosting retention ---*/

    if (_anti_ghosting_retention_duration > 0 && _anti_ghosting_first_output_call == false) {
        /* If this method has been called not for the character position
         * that must be turned on next, return and continue the retention.
         */
        if (pos != anti_ghosting_next_pos_to_output()) {
            return DRV7SEG2X595_OUTPUT_ANTI_GHOSTING_RETENTION_RUNNING;
        }

        // If the retention timer hasn't elapsed, return and continue the retention.
        if (anti_ghosting_timer() == false) {
            return DRV7SEG2X595_OUTPUT_ANTI_GHOSTING_RETENTION_RUNNING;
        }
    } else {
        _anti_ghosting_first_output_call = false;
    }


    /*--- Composing pos_byte ---*/

    uint8_t pos_byte = DRV7SEG2X595_ALL_BITS_CLEARED_MASK;
    size_t  pos_as_index = static_cast<size_t>(pos) - 1;
    if (_pos_bits[pos_as_index] == Drv7SegPosBitInitial) {
        return DRV7SEG2X595_OUTPUT_ERR_POS_BIT_NOT_SPECIFIED_FOR_POS;
    } else {
        pos_byte |= 1u << static_cast<uint8_t>(_pos_bits[pos_as_index]);
    }


    /*--- Account for a character position switch type ---*/

    if (_pos_switch_type == Drv7SegActiveLow) {
        pos_byte ^= static_cast<uint8_t>(DRV7SEG2X595_ALL_BITS_SET_MASK);
    }


    /*--- Account for a byte order ---*/

    uint8_t upper_byte;
    uint8_t lower_byte;

    if (_byte_order == Drv7SegPosByteFirst) {
        upper_byte = pos_byte;
        lower_byte = seg_byte;
    } else {
        upper_byte = seg_byte;
        lower_byte = pos_byte;
    }


    /*--- Shift data ---*/

    switch (_variant) {
        case DRV7SEG2X595_VARIANT_BIT_BANGING:
            digitalWrite(_latch_pin, LOW);
            /* In theory, shifting a single zeroed byte, whether it is seg_byte or pos_byte,
             * is enough to produce a blank output, because both seg_byte and pos_byte, being
             * zeroed, guarantee that either all segments will be turned off individually or
             * the whole character position will be turned off.
             *
             * However, in practice it can lead to artifacts due to imperfectness of
             * shift register ICs and switching devices. Therefore two bytes are shifted.
             *
             * The same is applicable to the SPI variant.
             */
            shift_out(DRV7SEG2X595_ALL_BITS_CLEARED_MASK);
            shift_out(DRV7SEG2X595_ALL_BITS_CLEARED_MASK);
            digitalWrite(_latch_pin, HIGH);

            digitalWrite(_latch_pin, LOW);
            shift_out(upper_byte);
            shift_out(lower_byte);
            digitalWrite(_latch_pin, HIGH);
            break;

        #ifdef DRV7SEG2X595_SPI_PROVIDED
        case DRV7SEG2X595_VARIANT_SPI:
            digitalWrite(_latch_pin, LOW);
            SPI.transfer(DRV7SEG2X595_ALL_BITS_CLEARED_MASK);
            SPI.transfer(DRV7SEG2X595_ALL_BITS_CLEARED_MASK);
            digitalWrite(_latch_pin, HIGH);

            digitalWrite(_latch_pin, LOW);
            SPI.transfer(upper_byte);
            SPI.transfer(lower_byte);
            digitalWrite(_latch_pin, HIGH);
            break;
        #endif

        default:
            break;  // Do nothing and hail MISRA.
    }

    // Update the values related to the anti-ghosting logic.
    _anti_ghosting_retained_pos = pos;
    _anti_ghosting_timer_previous_micros = micros();

    return DRV7SEG2X595_OUTPUT_NEXT;
}

void Drv7Seg2x595Class::output_all()
{
    /*--- Configuration status check ---*/

    if (_status < 0) {
        return;
    }


    /*--- Output ---*/

    for (size_t i = 0; i < DRV7SEG2X595_POS_MAX; ++i) {
         output(_pos_glyphs[i], static_cast<Pos>(i + 1));
    }
}

void Drv7Seg2x595Class::set_anti_ghosting_retention_duration(uint32_t new_val)
{
    /*--- Configuration status check ---*/

    if (_status < 0) {
        return;
    }


    /*--- Retention duration control ---*/

    _anti_ghosting_retention_duration = new_val;
}


/*--- Private methods ---*/

int32_t Drv7Seg2x595Class::begin_helper(int32_t variant,
                                        ByteOrder byte_order,
                                        PosSwitchType pos_switch_type,
                                        uint32_t latch_pin,
                                        PosBit pos_1_bit,
                                        PosBit pos_2_bit,
                                        PosBit pos_3_bit,
                                        PosBit pos_4_bit
                                       )
{
    /* Highly unlikely to occur without messing with the code,
     * but preserved as a redundant safety measure.
     */
    if (variant < 0) {
        return DRV7SEG2X595_STATUS_ERR_VARIANT_NOT_SPECIFIED;
    }

    // Byte order validity check.
    if (byte_order != Drv7SegPosByteFirst && byte_order != Drv7SegSegByteFirst) {
        return DRV7SEG2X595_STATUS_ERR_INVALID_BYTE_ORDER;
    }

    // Position switch type validity check.
    if (pos_switch_type != Drv7SegActiveLow && pos_switch_type != Drv7SegActiveHigh) {
        return DRV7SEG2X595_STATUS_ERR_INVALID_POS_SWITCH_TYPE;
    }

    // Position bits validity check.
    PosBit pos_bits[DRV7SEG2X595_POS_MAX] = {pos_1_bit, pos_2_bit, pos_3_bit, pos_4_bit};
    for (uint32_t i = 0; i < DRV7SEG2X595_POS_MAX; ++i) {

        // First position bit (array index zero) must belong to the 0..7 range (from LSB to MSB).
        if (i == 0) {
            if (pos_bits[i] < Drv7SegPosBit0 || pos_bits[i] > Drv7SegPosBit7) {
                return DRV7SEG2X595_STATUS_ERR_INVALID_POS_BIT;
            }
        }

        // Other position bits must belong to the -1..7 range (from the initial value to MSB).
        if (i > 0) {
            if (pos_bits[i] < Drv7SegPosBitInitial || pos_bits[i] > Drv7SegPosBit7) {
                return DRV7SEG2X595_STATUS_ERR_INVALID_POS_BIT;
            }
        }

        // Position bits duplication check.
        for (uint32_t j = i + 1; j < DRV7SEG2X595_POS_MAX; ++j) {
            if (pos_bits[i] != Drv7SegPosBitInitial &&
                pos_bits[j] != Drv7SegPosBitInitial &&
                pos_bits[i] == pos_bits[j]) {
                return DRV7SEG2X595_STATUS_ERR_POS_BIT_DUPLICATION;
            }
        }
    }

    _variant         = variant;
    _byte_order      = byte_order;
    _pos_switch_type = pos_switch_type;
    _latch_pin       = latch_pin;
    _pos_bits[0]     = pos_1_bit;
    _pos_bits[1]     = pos_2_bit;
    _pos_bits[2]     = pos_3_bit;
    _pos_bits[3]     = pos_4_bit;

    pinMode(_latch_pin, OUTPUT);

    /* Reset the variables related to the anti-ghosting logic
     * (useful if the driver gets reconfigured mid-use).
     */
    _anti_ghosting_first_output_call     = true;
    _anti_ghosting_retained_pos          = Drv7SegPos1;  /* Safe because position 1 is guaranteed to be valid
                                                          * for output. Not strictly necessary, but preserved
                                                          * as a redundant safety measure.
                                                          */
    _anti_ghosting_timer_previous_micros = 0;            /* Could call micros() instead, either is fine.
                                                          * Not strictly necessary, but preserved
                                                          * as a redundant safety measure.
                                                          */

    return DRV7SEG2X595_STATUS_OK;
}

void Drv7Seg2x595Class::shift_out(uint8_t byte_to_shift)
{
    digitalWrite(_clock_pin, LOW);
    for (uint32_t i = 0; i < DRV7SEG2X595_BITS_IN_BYTE; i++) {
        digitalWrite(_data_pin, (byte_to_shift << i) & DRV7SEG2X595_ONLY_MSB_SET_MASK);
        digitalWrite(_clock_pin, HIGH);
        digitalWrite(_clock_pin, LOW);
    }
}

bool Drv7Seg2x595Class::anti_ghosting_timer()
{
    if (_anti_ghosting_retention_duration == 0) {
        return true;  /* If the applicable retention duration is zero, the timer elapses
                       * without further calculations and the function returns early.
                       */
    }

    uint32_t current_micros = micros();

    if (current_micros - _anti_ghosting_timer_previous_micros >= _anti_ghosting_retention_duration) {
        return true;   // The timer has elapsed.
    } else {
        return false;  // The timer hasn't elapsed yet.
    }
}

Drv7Seg2x595Class::Pos Drv7Seg2x595Class::anti_ghosting_next_pos_to_output()
{
    // Subtract 1 because positions are 1-indexed while array members are 0-indexed.
    size_t pos_as_index = static_cast<size_t>(_anti_ghosting_retained_pos) - 1;

    // Add 1 because we're checking the next position, not the current one.
    for (size_t i = pos_as_index + 1; i < DRV7SEG2X595_POS_MAX; ++i) {
        /* Search for a position that is valid for output (that was
         * assigned a position bit that belongs to the 0..7 range).
         */
        if (_pos_bits[i] != Drv7SegPosBitInitial) {
            // Add 1 because we're hopping back from 0-indexed to 1-indexed.
            return static_cast<Pos>(i + 1);
        }
    }

    // Character position 1 is guaranteed to be valid for output.
    return Drv7SegPos1;
}
