#ifndef LAMURE_RENDERER_H
#define LAMURE_RENDERER_H

#include <lamure/vt/common.h>

namespace vt
{
class Renderer
{
  public:
    Renderer() = default;
    ~Renderer() = default;

    void render(GLFWwindow *_window);

  private:
};
}

#endif // LAMURE_RENDERER_H
