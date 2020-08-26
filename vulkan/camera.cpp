#include "camera.hpp"

void Camera::init(float fov, float width, float height, float near_plane, float far_plane) {
    pos = glm::vec3(0.0f, 0.0f, 4.0f);
    glm::vec3 camera_front = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);

    view_matrix = glm::mat4(1.0f);
    project_matrix = glm::mat4(1.0f);

    project_matrix = glm::perspective(fov, width / height, near_plane, far_plane);
    view_matrix = glm::lookAt(pos, camera_front, camera_up);
}
