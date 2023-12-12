#ifndef _SBC_ERROR_CODE_H_
#define _SBC_ERROR_CODE_H_

typedef enum {
    SBC_OK = 0, /*process success*/
    SBC_FAILED = 1, /*process failed*/
    SBC_INVALID_HANDLE = 2,
    SBC_INVALID_PARAM = 18,
    SBC_CONTINUE = 24,
} sbc_error_code_e;

#endif
