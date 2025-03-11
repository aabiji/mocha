#pragma once

class Shader {
public:
  void use();
  void assemble();
  void load(int type, const char *path);
  void setInt(const char *name, int value);
  void setFloat(const char *name, float value);
  void setMatrix(const char *name, float *matrix);
  void setVector(const char *name, const float *vector);

private:
  int vertexShader = -1;
  int fragmentShader = -1;
  int geometryShader = -1;
  int program = -1;
};