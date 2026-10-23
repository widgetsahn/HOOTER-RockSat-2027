#include <cstdio> //used to print to console
#include "freertos/FreeRTOS.h"
#include "freertos/task.h" //used for task scheduling in freeRTOS
#include "driver/gpio.h" //used for working with inputs and outputs of an ESP32


//add header files here for implementing segments of development
//SD_Card.h, Camera.h, etc....


/*
freeRTOS is a "real-time-operating-system" tool provided that is integrated into Espressif
ESPIDF that lets you run multiple functions at the same time!
you create a task scheduler, set the priorties of these tasks, and which 
core on the MCU certain tasks can run on. Then, freeRTOS does the rest!

to use freeRTOS first you declare your different functions
then in main you declare the task priorities of how important each function is
for example, when running something critical like monitoring the TE-R and TE-1 lines,
we can set those at the highest priority. I think you may get the gist.

Also shoutout chatGPT for teaching me about freeRTOS and how to set up this example
*/

//first, lets write a function for blinking an LED
constexpr gpio_num_t LED_PIN = GPIO_NUM_4; //this is a const that lets us quickly redefine the LED blinking pin

//now lets set up our freeRTOS task
void blinkTask(void *parameter)
{
    //initialize the LED GPIOs
    //set up GPIO pin 4 as a digital output
    gpio_reset_pin(LED_PIN); //reset the status of the GPIO pin
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT); //set GPIO4 as an output
    while (true) //infinite while loop to run the blink task
    {
        gpio_set_level(LED_PIN, 1); //set the led pin to high
        vTaskDelay(pdMS_TO_TICKS(500)); //wait 500ms, tell the task scheduler to convert 500ms to ticks

        gpio_set_level(LED_PIN, 0); //set the led pin to low
        printf("Minimum unused stack for blinkTask: %u bytes\n", (unsigned)uxTaskGetStackHighWaterMark(nullptr));
        vTaskDelay(pdMS_TO_TICKS(500)); //wait 500ms
    }
}


//lets set up an additional freeRTOS task that prints hello world!
void helloTask(void *parameter)
{
    while (true)
    {
        printf("Hello, world! RockSat2027 Rules!\n");
        printf("Salvatore t\n");
        //here is a neat trick to see how much RAM was used
        printf("Minimum unused stack for helloTask: %u bytes\n", (unsigned)uxTaskGetStackHighWaterMark(nullptr));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void)
{
    //now that we wrote our freeRTOS tasks, we can dedicate our tasks! :) pretty cool right?
    
    //first lets define what the blink task needs
    //it will have the following parameters
    //1. it will use the blinkTask function to do its thing
    //2. it will have 2048 bytes of stack allocated to its process
    //3. there is no data to pass TO the blinking task
    //4. this is the highest priority task you can have
    //5. there is nothing refering to or handling the task elsewhere
    xTaskCreate(blinkTask, "Blink", 2048, nullptr, 1, nullptr);

    //you may ask "how tf do we know how much stack we want to allocate?"
    //1. guess
    //2. measure the stack used, a good rule of thumb for most tasks is to have 1000-3000bytes extra in unused stack



    //lets do the samee thing 
    xTaskCreate(helloTask, "Hello", 2048, nullptr, 1, nullptr);
}