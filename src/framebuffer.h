#pragma once

#include <glad.h>

class Framebuffer
{
public:
    void init(int width, int height)
    {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Attach the color buffer
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                     GL_FLOAT, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                     GL_TEXTURE_2D, tex, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Attach the depth buffer
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                        width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                        GL_RENDERBUFFER, rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // Check if we've setup everything correctly
        int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            throw "Incomplete frame buffer!";

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void cleanup()
    {
        if (fbo != UINT_MAX) glDeleteFramebuffers(1, &fbo);
        if (tex != UINT_MAX) glDeleteTextures(1, &tex);
        if (rbo != UINT_MAX) glDeleteRenderbuffers(1, &rbo);
    }

    void readPixel(int x, int y, float pixel[4])
    {
        bind();
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, pixel);
        unbind();
    }

    void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
    void bind()
    {
        if (fbo == UINT_MAX)
            throw "Uninitialized frame buffer";
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }
private:
    unsigned int fbo = UINT_MAX; // frame buffer object
    unsigned int rbo = UINT_MAX; // render buffer object
    unsigned int tex = UINT_MAX; // texture object
};