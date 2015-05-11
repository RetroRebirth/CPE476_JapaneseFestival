#include "Camera.h"

using namespace std;

Camera::Camera(
      GLint _h_uP,
      GLint _h_uV,
      GLint _h_uView) {
   // Default attribute valuesf
   pos = glm::vec3(0.0f, 1.0f, 0.0f);
   debug_pos = pos;
   theta = -M_PI/2.0f;
   phi = 0.0f;
   debug = false;
   speed = INITIAL_SPEED;
   blocked = false;
   pov = true;
   playingMinigame = false;
   radius = 1.0f;
   playerYrot = 0.0f;

   // Defined attribute values
   h_uP = _h_uP;
   h_uV = _h_uV;
   h_uView = _h_uView;
}

Camera::~Camera() {}

inline void Camera::safe_glUniformMatrix4fv(const GLint handle, const GLfloat data[]) {
   if (handle >= 0) {
      glUniformMatrix4fv(handle, 1, GL_FALSE, data);
   }
}

float Camera::getYRot() {
   return Util::radiansToDegrees(theta);
}

float Camera::getXRot() {
   return Util::radiansToDegrees(phi);
}

glm::vec3 Camera::lookAtPt() {
   glm::vec3 lookAtPt = glm::vec3(cos(phi)*cos(theta), sin(phi), cos(phi)*cos((M_PI/2)-theta));
   lookAtPt += debug ? debug_pos : pos;
   return lookAtPt;
}

void Camera::setProjectionMatrix(int g_width, int g_height) {
   Projection = glm::perspective(90.0f, (float)g_width/g_height, 0.1f, 300.f);
   safe_glUniformMatrix4fv(h_uP, glm::value_ptr(Projection));
}

void Camera::setView() {
   glm::vec3 curPos = debug ? debug_pos : pos;
   glm::mat4 lookAtMat = glm::lookAt(lookAtPt(), curPos, glm::vec3(0, 1, 0));

   //mult view by phi rotation matrix
   glm::mat4 view_mat = glm::rotate(glm::mat4(1.0f), phi, glm::vec3(1, 0, 0)) * lookAtMat;

   safe_glUniformMatrix4fv(h_uV, glm::value_ptr(view_mat));
   glUniform3f(h_uView, pos.x, pos.y, pos.z);

   if (!debug) {
      View = view_mat;
   }
}

void Camera::step(Window* window, bool playerHit) {
   setProjectionMatrix(window->width, window->height);
   setView();

   if (debug) {
      debug_pos = calcNewPos(window, playerHit);
   } else {
      if (!playingMinigame && !blocked) {
         pos = calcNewPos(window, playerHit);
      }
      blocked = false;
   }
}

glm::vec3 Camera::calcNewPos(Window* window, bool playerHit) {
   glm::vec3 newPos = debug ? debug_pos : pos;
   float moveInc = speed * window->dt;
   float playerYrad = Util::degreesToRadians(playerYrot);

   glm::vec3 viewVector = glm::normalize(newPos - lookAtPt());
   glm::vec3 strafeVector = glm::normalize(glm::cross(viewVector, glm::vec3(0, 1, 0)));
   glm::vec3 crossVector = glm::normalize(glm::cross(viewVector, strafeVector));
   // Scale vectors
   viewVector *= moveInc;
   strafeVector *= moveInc;
   crossVector *= moveInc;

   GLFWwindow* win = window->glfw_window;
   if (!debug) {
      // Normal camera controls
      if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) { // Move forward
         newPos.x += moveInc * sin(playerYrad);
         newPos.z += moveInc * cos(playerYrad);
      }
      if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) { // Rotate left
         playerYrot += PLAYER_ROT_DEG;
      }
      if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) { // Rotate right
         playerYrot -= PLAYER_ROT_DEG;
      }
      /*
      if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) { // Move backward
         newPos.x -= moveInc * sin(playerYrad);
         newPos.z -= moveInc * cos(playerYrad);
      }
      */
   } else {
      // Debug free-flying camera
      if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) { // Move forward
         newPos += viewVector;
      }
      if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) { // Move backward
         newPos -= viewVector;
      }
      if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) { // Strafe left
         newPos -= strafeVector;
      }
      if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) { // Strafe right
         newPos += strafeVector;
      }
      if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) { // Move up
         newPos += crossVector;
      }
      if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) { // Move down
         newPos -= crossVector;
      }
   }

   // Bounding
  if (!debug) {
     float s = SIZE - 0.2f;
     if (newPos.x < -s)
        newPos.x = -s;
     if (newPos.x > s)
        newPos.x = s;
     if (newPos.z < -s)
        newPos.z = -s;
     if (newPos.z > s)
        newPos.z = s;

     newPos.y = 1;
  }

   if (player != NULL) {
      player->setPos(calculatePlayerPos());
      player->scale(glm::vec3(1.0f, 2.0f, 1.0f));
      player->rotate(playerYrot, glm::vec3(0.0f, 1.0f, 0.0f));
      
      if (pov && !playerHit)
         player->draw();
   }

   return newPos;
}

void Camera::mouse_callback(GLFWwindow* window, double xpos, double ypos, int g_width, int g_height) {
   // Don't do anything if we are playing the minigame
   if (playingMinigame) {
      return;
   }

   // Update theta (x angle) and phi (y angle)
   float half_width = g_width / 2.0f;
   float half_height = g_height / 2.0f;
   float xPosFromCenter = xpos - half_width;
   float yPosFromCenter = ypos - half_height;
   float xMag = xPosFromCenter / half_width;
   float yMag = yPosFromCenter / half_height;

   theta += MOUSE_SPEED*M_PI*xMag;
   // Bound phi to 80 degrees
   float newPhi = phi + MOUSE_SPEED*M_PI*yMag/2.0;
   //bounded between 80 and -40 to keep from going into the char
   if (glm::degrees(newPhi) < 80 && glm::degrees(newPhi) > -40) {
      phi = newPhi;
   }

   // Keep mouse in center
   glfwSetCursorPos(window, g_width/2, g_height/2);
}

void Camera::enter_callback(GLFWwindow* window, int entered, int g_width, int g_height) {
   // Don't do anything if we are playing the minigame
   if (playingMinigame) {
      return;
   }

   // Position mouse at center if enter screen
   glfwSetCursorPos(window, g_width/2, g_height/2);
}

void Camera::moveToMinigame() {
   // Set the appropriate flags
   this->playingMinigame = true;
   this->pov = false;

   // Send camera to the origin
   this->pos = glm::vec3(0, 2, 0);
   this->theta = -M_PI/2.0;
   this->phi = 0.0;
}

void Camera::moveToOverworld() {
   // Set the appropriate flags
   this->playingMinigame = false;
   this->pov = true;
}

void Camera::initPlayer(Object *_player) {
   player = _player;
}

glm::vec3 Camera::calculatePlayerPos() {
   glm::vec3 temp;

   temp.x = pos.x;
   temp.y = 1;
   temp.z = pos.z;

   return temp;
}
