# Mocha

Motion capture app. You record a video of someone moving
and that motion is mapped to a 3d animation of a human model
that can be replayed. For example, you could map the choreography
of dancers in kpop dance videos into 3d animations.

Tech Stack:
- SDL3 + Opengl: Engine responsible for the 3d model rendering and their skeletal animation.
- Mediapipe: Get skeletal information from the human pose estimation.
- Custom bridge library: Connect to the mediapipe output to the game engine.

Currently working on the game engine, model loading and skeletal animation