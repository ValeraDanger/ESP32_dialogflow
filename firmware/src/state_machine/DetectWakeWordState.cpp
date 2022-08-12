#include <Arduino.h>
#include "I2SSampler.h"
#include "AudioProcessor.h"
#include "NeuralNetwork.h"
#include "RingBuffer.h"
#include "DetectWakeWordState.h"
#include <GyverOLED.h>

GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled1;

#define WINDOW_SIZE 320
#define STEP_SIZE 160
#define POOLING_SIZE 6
#define AUDIO_LENGTH 16000

TaskHandle_t DisplayWakeWordTask_handler = NULL;
void DisplayWakeWordTask(void *parametrs) {
    oled1.clear();  // очистить дисплей (или буфер)
    oled1.update();

    oled1.home();                  // курсор в 0,0
    oled1.autoPrintln(true);  // печатай что угодно: числа, строки, float, как Serial!
    oled1.print("Я слушаю.");
    oled1.update();
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    oled1.clear();  // очистить дисплей (или буфер)
    oled1.update();

    DisplayWakeWordTask_handler = NULL;
    vTaskDelete(NULL);
}

DetectWakeWordState::DetectWakeWordState(I2SSampler *sample_provider)
{
    // save the sample provider for use later
    m_sample_provider = sample_provider;
    // some stats on performance
    m_average_detect_time = 0;
    m_number_of_runs = 0;
}
void DetectWakeWordState::enterState()
{
    // Create our neural network
    m_nn = new NeuralNetwork();
    Serial.println("Created Neral Net");
    // create our audio processor
    m_audio_processor = new AudioProcessor(AUDIO_LENGTH, WINDOW_SIZE, STEP_SIZE, POOLING_SIZE);
    Serial.println("Created audio processor");

    m_number_of_detections = 0;
}
bool DetectWakeWordState::run()
{
    // time how long this takes for stats
    long start = millis();
    // get access to the samples that have been read in
    RingBufferAccessor *reader = m_sample_provider->getRingBufferReader();
    // rewind by 1 second
    reader->rewind(16000);
    // get hold of the input buffer for the neural network so we can feed it data
    float *input_buffer = m_nn->getInputBuffer();
    // process the samples to get the spectrogram
    m_audio_processor->get_spectrogram(reader, input_buffer);
    // finished with the sample reader
    delete reader;
    // get the prediction for the spectrogram
    float output = m_nn->predict();
    long end = millis();
    // compute the stats
    m_average_detect_time = (end - start) * 0.1 + m_average_detect_time * 0.9;
    m_number_of_runs++;
    // log out some timing info
    if (m_number_of_runs == 100)
    {
        m_number_of_runs = 0;
        Serial.printf("Average detection time %.fms\n", m_average_detect_time);
    }
    // use quite a high threshold to prevent false positives
    if (output > 0.95)
    {
        m_number_of_detections++;
        if (m_number_of_detections > 1)
        {
            m_number_of_detections = 0;
            // detected the wake word in several runs, move to the next state
            Serial.printf("P(%.2f): Я слушаю\n", output);
            if (DisplayWakeWordTask_handler != NULL) {
                vTaskDelete(DisplayWakeWordTask_handler);
            }

            xTaskCreate(
                DisplayWakeWordTask,          //func
                "Print wake word heard",      //descript
                10000,                 //buff size
                NULL,                 //params
                1,                    //priority
                &DisplayWakeWordTask_handler //handle
            );
            return true;
        }
    }
    // nothing detected stay in the current state
    return false;
}
void DetectWakeWordState::exitState()
{
    // Create our neural network
    delete m_nn;
    m_nn = NULL;
    delete m_audio_processor;
    m_audio_processor = NULL;
    uint32_t free_ram = esp_get_free_heap_size();
    Serial.printf("Free ram after DetectWakeWord cleanup %d\n", free_ram);
}