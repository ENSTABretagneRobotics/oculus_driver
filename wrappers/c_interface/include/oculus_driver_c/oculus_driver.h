#ifndef _DEF_OCULUS_DRIVER_C_OCULUS_DRIVER_H_
#define _DEF_OCULUS_DRIVER_C_OCULUS_DRIVER_H_

#include <oculus_driver/Oculus.h>

#ifdef __cplusplus
extern "C" {
#endif

struct oculus_handle;
typedef struct oculus_handle oculus_handle_t;

oculus_handle_t* oculus_handle_create();
void oculus_handle_destroy(oculus_handle_t* handle);

void oculus_start(oculus_handle_t* handle);
void oculus_stop(oculus_handle_t* handle);

void oculus_add_message_callback(oculus_handle_t* handle,
    void (*callback)(OculusMessageHeader, uint64_t, const uint8_t*));
void oculus_add_ping_callback(oculus_handle_t* handle,
    void (*callback)(OculusSimplePingResult, uint64_t, const uint8_t*));
void oculus_add_status_callback(oculus_handle_t* handle,
    void (*callback)(OculusStatusMsg));

#ifdef __cplusplus
} // extern "C"
#endif 


#endif //_DEF_OCULUS_DRIVER_C_OCULUS_DRIVER_H_
