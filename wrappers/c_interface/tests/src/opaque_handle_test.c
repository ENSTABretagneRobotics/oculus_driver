#include <stdio.h>

#include <oculus_driver_c/oculus_driver.h>

void message_callback(OculusMessageHeader header,
                      uint64_t dataSize, const uint8_t* data)
{
    printf("Got message !\n");
}

void ping_callback(OculusSimplePingResult metadata,
                   uint64_t dataSize, const uint8_t* data)
{
    printf("Got data !\n");
}

void status_callback(OculusStatusMsg status)
{
    printf("Got status !\n");
}

int main()
{
    oculus_handle_t* sonar = oculus_handle_create();

    oculus_start(sonar);

    oculus_add_message_callback(sonar, message_callback);
    oculus_add_ping_callback(sonar, ping_callback);
    oculus_add_status_callback(sonar, status_callback);

    getchar();

    oculus_stop(sonar);

    oculus_handle_destroy(sonar);

    return 0;
}


