What if we mapped human movement to a 3d model? So we input video or live webcam and
do pose estimation and map that to skeletal animation? Then perhaps "replay" the movement
with a 3d model?

We'd need:
- OpenCV for webcam and general computer vision
- OpenGL to render our 3d models and our scenes
- OpenPose to get pose estimation

- Actually learn OpenGL
    - Go through tutorials and build small demos
      - Implement math for camera in OpenGL [FROM SCRATCH](http://www.songho.ca/opengl/gl_projectionmatrix.html)
    - Questions:
        - How to render textures using the correct aspect ratio. I'm assuming it has something to do with the vertex positions.
          To support multiple textures it would be best to implement that in the shader. How to do that?
- Go through OpenPose to understand how it works
- Understand how skeletal animations should work
- Connect OpenPose with our OpenGL project

The goal would be to understand fully how everything works --
not just glue things together for the hell of it.
