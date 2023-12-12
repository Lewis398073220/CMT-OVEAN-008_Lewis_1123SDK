#ifndef _AAC_ERROR_CODE_H_
#define _AAC_ERROR_CODE_H_

typedef enum {
    AAC_ENC_DISABLED = -2, /*encoder is diabled*/
    AAC_FAILED = -1, /*process failed*/
    AAC_OK = 0, /*process success*/
    AAC_CONCEAL_OUT = 2, /*output buffer is valid and concealed*/
    AAC_NOT_ENOUGH_BITS = 0x1002, /*!< The input buffer ran out of bits. */
} aac_error_code_e;

#endif