We could just use BlazePose GHUM:
https://storage.googleapis.com/mediapipe-assets/Model%20Card%20BlazePose%20GHUM%203D.pdf
https://github.com/google-research/google-research/

BlazePose model files (there's also "full" and "heavy" versions of the model):
[ONNX file](https://storage.googleapis.com/ailia-models/blazepose-fullbody/pose_landmark_lite.onnx)
[Prototxt file](https://storage.googleapis.com/ailia-models/blazepose-fullbody/pose_landmark_lite.onnx.prototxt)
We don't actually need prototxt file, just the onnx file

Example implementation:
https://github.com/axinc-ai/ailia-models/blob/master/pose_estimation_3d/blazepose-fullbody/blazepose-fullbody.py

What if we just used the ONNX runtime for C++ to run inference using the ONNX file???
https://github.com/microsoft/onnxruntime-inference-examples

Perfect, just the the C++ ONNX runtime to run inference on the BlazePoze model
to get 3d pose estimation!