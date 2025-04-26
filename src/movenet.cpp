#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

#include <tensorflow/lite/c/c_api_types.h>
#include <tensorflow/lite/core/interpreter_builder.h>
#include <tensorflow/lite/core/kernels/register.h>
#include <tensorflow/lite/core/model_builder.h>
#include <tensorflow/lite/optional_debug_tools.h>

#include "movenet.h"

void MoveNet::init(const char* modelPath)
{
    model = tflite::FlatBufferModel::BuildFromFile(modelPath);
    if (model == nullptr)
        throw "Couldn't load " + std::string(modelPath);

    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder builder(*model, resolver);
    std::unique_ptr<tflite::Interpreter> interpreter;
    builder(&interpreter);
    if (interpreter == nullptr)
        throw "Failed to create the model interpreter";

    if (interpreter->AllocateTensors() != kTfLiteOk)
        throw "Failed to allocate the input tensors";

    readyForNextFrame = true;
}

std::vector<Keypoint> MoveNet::runInference(unsigned char* image, int width, int height)
{
    readyForNextFrame = false;

    int targetSize = 192; // MoveNet requires the input tensor to be 192x192x3
    int allocationSize = targetSize * targetSize * 3 * sizeof(unsigned char);
    unsigned char* scaledImage = (unsigned char*)malloc(allocationSize);

    // scaled down to the required size
    int res = stbir_resize_uint8(
        image, width, height, 0,
        scaledImage, targetSize, targetSize, 0, 3);
    if (res == 0)
        throw "Couldn't resize the image down to the required size";

    assert(interpreter != nullptr); // TODO: this fails?? ooh...plot twist!

    // pass in the input tensor and run inference
    memcpy(
        interpreter->typed_input_tensor<unsigned char>(0),
        scaledImage, allocationSize
    );
    free(scaledImage);
    if (interpreter->Invoke() != kTfLiteOk)
        throw "Failed to run inference with MoveNet";

    // parse the model output
    std::vector<Keypoint> points;
    float* output = interpreter->typed_output_tensor<float>(0);
    for (int i = 0; i < 17; i++) {
        points.push_back({output[i * 3], output[i * 3 + 1], output[i * 3 + 2]});
    }

    readyForNextFrame = true;
    return points;
}