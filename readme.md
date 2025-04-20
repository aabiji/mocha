# Mocha

![Current state of the app](screenshot.png)

Motion capture app. You record a video of someone moving
and that motion is mapped to a 3d animation of a human model
that can be replayed. For example, you could map the choreography
of [dancers](https://www.youtube.com/watch?v=iEyOhEiA5kQ) and
animate a set of 3d models.

This concept could extend into a fully fledged game where
you are rewarded for exercising.

Project outline:
- [x] **Engine**: responsible for rendering and animating the 3d models
- [ ] **Human pose tracker**: responsible for obtaining information about the subject's skeleton in real time
- [ ] **Bridge library**: responsible for connecting the engine and the human pose tracker
