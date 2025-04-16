// // #include <Object-detection-ESP32_inferencing.h>
// #include "classification.hpp"

// Classification::Classification(Camera *cam, bool debugNn)
//     : BaseModule(
//           "CLASSIFICATION",
//           CLASSIFICATION_TASK_PRIORITY,
//           CLASSIFICATION_TASK_DELAY,
//           CLASSIFICATION_TASK_STACK_DEPTH_LEVEL,
//           CLASSIFICATION_TASK_PINNED_CORE_ID),
//       camera(cam),
//       debugNn(debugNn)
// {
//     bb.xMutex = xSemaphoreCreateMutex();
// }

// Classification::~Classification()
// {
//     free(camera);
// }

// void Classification::taskFn()
// {
//     this->classify();
// }

// void Classification::classify()
// {
//     if (this->camera == nullptr)
//     {
//         ESP_LOGE(this->NAME, "Camera is still null");
//         return;
//     }

//     // if (!this->camera->isTaskRunning())
//     // {
//     //     ESP_LOGE(this->NAME, "Camera 's task is not running");
//     //     return;
//     // }

//     ei::signal_t signal;
//     signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
//     signal.get_data = [&](size_t offset, size_t length, float *out_ptr)
//     {
//         return this->camera->getData(offset, length, out_ptr);
//     };
//     ei_impulse_result_t result = {0};
//     EI_IMPULSE_ERROR err = run_classifier(&signal, &result, this->debugNn);

//     if (err != EI_IMPULSE_OK)
//     {
//         ESP_LOGE(this->NAME, "Failed to run Classifier (%d)", err);
//         return;
//     }

//     ESP_LOGI(this->NAME, "Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.):",
//              result.timing.dsp, result.timing.classification, result.timing.anomaly);

// #if EI_CLASSIFIER_OBJECT_DETECTION == 1
//     if (xSemaphoreTake(this->bb.xMutex, portMAX_DELAY) == pdTRUE)
//     {
//         for (uint32_t i = 0; i < result.bounding_boxes_count; i++)
//         {
//             ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
//             ResultBoundingBox bbCopy = {
//                 .label = bb.label,
//                 .x = bb.x,
//                 .y = bb.y,
//                 .width = bb.width,
//                 .height = bb.height,
//                 .value = bb.value};
//             this->bb.value.push_back(bbCopy);
//             if (bb.value == 0)
//                 continue;
//             ESP_LOGI(this->NAME, "  %s (%f) [ x: %u, y: %u, width: %u, height: %u ]",
//                      bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);
//         }
//         xSemaphoreGive(this->bb.xMutex);
//     }
// #else
//     for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++)
//     {
//         ei_printf("  %s: %.5f\r\n", ei_classifier_inferencing_categories[i], result.classification[i].value);
//     }
// #endif

// #if EI_CLASSIFIER_HAS_ANOMALY
//     ei_printf("Anomaly prediction: %.3f\r\n", result.anomaly);
// #endif
// }

// std::vector<ResultBoundingBox> Classification::getClassifyResult()
// {
//     std::vector<ResultBoundingBox> result;

//     if (xSemaphoreTake(this->bb.xMutex, portMAX_DELAY) == pdTRUE)
//     {
//         result = this->bb.value;
//         this->bb.value.clear();
//         xSemaphoreGive(this->bb.xMutex);
//     }

//     return result;
// }
