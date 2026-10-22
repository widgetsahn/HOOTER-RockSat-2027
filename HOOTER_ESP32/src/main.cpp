#include <cstdio> //used to 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//add header files here for implementing segments of development
//SD_Card.h, Camera.h, etc....

extern "C" void app_main(void)
{
    //setup GPIO pins and other objects

    // You can create C++ objects and call their methods here.

    //"main program loop"
    while (true)
    {
        printf("Hello from C++!\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}