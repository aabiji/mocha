#pragma once

#include <glad.h>

class Framebuffer
{
public:
    void cleanup() { glDeleteFramebuffers(1, &id); }

    void init(int width, int height)
    {
        id = 0;
        glGenFramebuffers(1, &id);
        glBindFramebuffer(GL_FRAMEBUFFER, id);

        // Add a color buffer (texture) to the framebuffer
        unsigned int colorId;
        glGenTextures(1, &colorId);
        glBindTexture(GL_TEXTURE_2D, colorId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, colorId, 0);

        // Add a depth buffer (render buffer) to the framebuffer
        unsigned int depthId;
        glGenRenderbuffers(1, &depthId);
        glBindRenderbuffer(GL_RENDERBUFFER, depthId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width,
                              height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, depthId);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw "Incomplete framebuffer!";

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    void bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, id);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
    }

    unsigned int readPixel(int x, int y)
    {
        unsigned char pixels[4];
        glBindFramebuffer(GL_FRAMEBUFFER, id);
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return pixels[0];
    }

private:
    unsigned int id;
};