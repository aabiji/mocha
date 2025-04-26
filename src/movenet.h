#pragma once

#include <memory>
#include <vector>

#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/model.h>

struct Keypoint
{
    float y, x;  // coordinate, in the range of 0 to 1
    float score; // confidence score
    bool detected() { return score > 0.3; }
};

class MoveNet
{
public:
    // Load the model from the path
    void init(const char* modelPath);

    // Run inference on the MoveNet model with an rgb image as input
    std::vector<Keypoint> runInference(unsigned char* image, int width, int height);

    // Are we ready to process the next frame?
    bool ready() { return readyForNextFrame; }
private:
    bool readyForNextFrame;
    std::unique_ptr<tflite::Interpreter> interpreter;
    std::unique_ptr<tflite::FlatBufferModel> model;
};