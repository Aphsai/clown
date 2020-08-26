#pragma once

#define GLM_FORCE_RADIAN
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
    public:
        void init(float fov, float width, float height, float near_plane, float far_plane);

        glm::mat4 project_matrix;
        glm::mat4 view_matrix;
        glm::vec3 pos;
};
