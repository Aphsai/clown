#include "graphics_pipeline.hpp"
#include "object_buffers.hpp"
#include "descriptor.hpp"
#include "camera.hpp"

class Renderer;
class ObjectRenderer {
    public:
        ObjectRenderer(Renderer*);
        ~ObjectRenderer();
        void createObjectRenderer(glm::vec3 position, glm::vec3 scale);
        void updateUniformBuffer(Camera camera);
        void draw();
        void destroy();

        GraphicsPipeline* graphics_pipeline;
        ObjectBuffers* object_buffers;
        Descriptor* descriptor;
        
        glm::vec3 position;
        glm::vec3 scale;

        Renderer* renderer;
};
