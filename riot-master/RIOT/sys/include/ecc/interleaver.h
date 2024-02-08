/*
 * Copyright (c) 2007 - 2021 Joseph Gaeddert
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// common headers
#include <inttypes.h>

typedef struct interleaver_s * interleaver;

// create interleaver
//   _n     : number of bytes
interleaver interleaver_create(unsigned int _n);

// destroy interleaver object
void interleaver_destroy(interleaver _q);

// print interleaver object internals
void interleaver_print(interleaver _q);

// set depth (number of internal iterations)
//  _q      :   interleaver object
//  _depth  :   depth
void interleaver_set_depth(interleaver _q,
                           unsigned int _depth);

// execute forward interleaver (encoder)
//  _q          :   interleaver object
//  _msg_dec    :   decoded (un-interleaved) message
//  _msg_enc    :   encoded (interleaved) message
void interleaver_encode(interleaver _q,
                        unsigned char * _msg_dec,
                        unsigned char * _msg_enc);

// execute forward interleaver (encoder) on soft bits
//  _q          :   interleaver object
//  _msg_dec    :   decoded (un-interleaved) message
//  _msg_enc    :   encoded (interleaved) message
void interleaver_encode_soft(interleaver _q,
                             unsigned char * _msg_dec,
                             unsigned char * _msg_enc);

// execute reverse interleaver (decoder)
//  _q          :   interleaver object
//  _msg_enc    :   encoded (interleaved) message
//  _msg_dec    :   decoded (un-interleaved) message
void interleaver_decode(interleaver _q,
                        unsigned char * _msg_enc,
                        unsigned char * _msg_dec);

// execute reverse interleaver (decoder) on soft bits
//  _q          :   interleaver object
//  _msg_enc    :   encoded (interleaved) message
//  _msg_dec    :   decoded (un-interleaved) message
void interleaver_decode_soft(interleaver _q,
                             unsigned char * _msg_enc,
                             unsigned char * _msg_dec);

typedef enum {
    // everything ok
    LIQUID_OK=0,

    // internal logic error; this is a bug with liquid and should be reported immediately
    LIQUID_EINT,

    // invalid object, examples:
    //  - destroy() method called on NULL pointer
    LIQUID_EIOBJ,

    // invalid parameter, or configuration; examples:
    //  - setting bandwidth of a filter to a negative number
    //  - setting FFT size to zero
    //  - create a spectral periodogram object with window size greater than nfft
    LIQUID_EICONFIG,

    // input out of range; examples:
    //  - try to take log of -1
    //  - try to create an FFT plan of size zero
    LIQUID_EIVAL,

    // invalid vector length or dimension; examples
    //  - trying to refer to the 17th element of a 2 x 2 matrix
    //  - trying to multiply two matrices of incompatible dimensions
    LIQUID_EIRANGE,

    // invalid mode; examples:
    //  - try to create a modem of type 'LIQUID_MODEM_XXX' which does not exit
    LIQUID_EIMODE,

    // unsupported mode (e.g. LIQUID_FEC_CONV_V27 with 'libfec' not installed)
    LIQUID_EUMODE,

    // object has not been created or properly initialized
    //  - try to run firfilt_crcf_execute(NULL, ...)
    //  - try to modulate using an arbitrary modem without initializing the constellation
    LIQUID_ENOINIT,

    // not enough memory allocated for operation; examples:
    //  - try to factor 100 = 2*2*5*5 but only give 3 spaces for factors
    LIQUID_EIMEM,

    // file input/output; examples:
    //  - could not open a file for writing because of insufficient permissions
    //  - could not open a file for reading because it does not exist
    //  - try to read more data than a file has space for
    //  - could not parse line in file (improper formatting)
    LIQUID_EIO,

} liquid_error_code;