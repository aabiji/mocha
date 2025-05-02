This is the branch of the project where I figure out how to do pose estimation.

We could just use BlazePose GHUM: https://storage.googleapis.com/mediapipe-assets/Model%20Card%20BlazePose%20GHUM%203D.pdf

BlazePose model files (there's also "full" and "heavy" versions of the model): ONNX file Prototxt file We don't actually need prototxt file, just the onnx file

Example implementation: https://github.com/axinc-ai/ailia-models/blob/master/pose_estimation_3d/blazepose-fullbody/blazepose-fullbody.py

Link to model: https://storage.googleapis.com/ailia-models/blazepose-fullbody/pose_landmark_lite.onnx

We can just the ONNX runtime for C++ to run inference on the model.

Install dependencies on arch linux
```bash
yay -S ffmpeg opencv hdf5 vtk onnxruntime openmpi
```
