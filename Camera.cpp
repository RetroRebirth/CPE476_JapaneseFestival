#include "Camera.h"
using namespace std;

Camera::Camera(
      GLint _h_uP,
      GLint _h_uV,
      GLint _h_uView) {
   // Default attribute valuesf
   pos = glm::vec3(0.0f, 2.0f, 0.0f);
   debug_pos = pos;
   dir = glm::vec3(0.0f, 0.0f, 1.0f);
   theta = -M_PI/2.0f;
   phi = 0.0f;
   debug = false;
   speed = 10.0f;
   blocked = false;
   pov = true;
   playingMinigame = false;
   radius = 3.5f;
   playerYrad = 0.0f;
   withinBounds = false;

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
   glm::vec3 lookAtPt;
   if (playingMinigame) {
      lookAtPt = glm::vec3(cos(phi)*cos(theta), sin(phi), cos(phi)*cos((M_PI/2.0f)-theta));   
   }
   else {
      lookAtPt = glm::vec3(cos(phi)*cos(theta)*radius, sin(phi)*radius, cos(phi)*cos((M_PI/2.0f)-theta)*radius);
   }
   lookAtPt += debug ? debug_pos : player->pos;
   
   return lookAtPt;
}

void Camera::setProjectionMatrix(int g_width, int g_height) {
   Projection = glm::perspective(90.0f, (float)g_width/g_height, 0.1f, 300.f);
   safe_glUniformMatrix4fv(h_uP, glm::value_ptr(Projection));
}

void Camera::setView() {
   glm::vec3 curPos = debug ? debug_pos : player->pos;
   glm::vec3 lookAt = lookAtPt();
   glm::mat4 lookAtMat = glm::lookAt(lookAt, curPos, glm::vec3(0.0f, 1.0f, 0.0f));

   // Extract viewing direction from lookAtPt
   dir = glm::normalize(lookAt - player->pos);
   
   //printf("lookAt: %lf %lf %lf\nlookAt - .2: %lf %lf %lf\n", lookat.x, lookat.y, lookat.z, lookat.x - cos(.2) * .2, lookat.y, lookat.z - cos(M_PI + .2) * .2);

   //mult view by phi rotation matrix
   // TODO what the what is happening here??? it's 5am but still...
   // umm, well I commented it out and nothing changed so I'm gonna keep it commented out...
//   glm::mat4 view_mat = glm::rotate(glm::mat4(1.0f), -1.0f*phi, glm::vec3(4.0f, 0.0f, 0.0f)) * lookAtMat;
   glm::mat4 view_mat = lookAtMat;

   safe_glUniformMatrix4fv(h_uV, glm::value_ptr(view_mat));
   // TODO get the position of the camera itself, not the player's position
//   glm::vec3 camPos = player->pos;// - (glm::vec3(1*dir.x, 1*dir.y, 1*dir.z));
   glUniform3f(h_uView, player->pos.x, player->pos.y, player->pos.z);
//   glUniform3f(h_uView, camPos.x, camPos.y, camPos.z);

   if (!debug) {
      View = view_mat;
   }
}

/*void Camera::updateSDS() {
   player->getBounds(&(player->bounds));
   for (int i=0; i<UNIFORM_GRID_SIZE; ++i) {
      spatialGrid[i].hasPlayer = false;
      for (int j=0; j<spatialGrid[i].members.size(); ++j) {
         if (spatialGrid[i].members[j] == player) {
            spatialGrid[i].hasPlayer = true;
            printf("something has player!\n");
            break;
         }
      }
   }
}*/

void Camera::step(Window* window, int game_state) {

   if (game_state == TITLE_STATE) {
      calcNewPos(window);
   } else if (debug) {
      debug_pos = calcNewPos(window);
   } else {
      if (!playingMinigame && !blocked) {
         pos = calcNewPos(window);
         player->setPos(pos);//calculatePlayerPos());
      }
      blocked = false;
   }
   setProjectionMatrix(window->width, window->height);
   setView();
}

bool Camera::checkStaticObjectCollisions(Object* o, glm::vec3* colPlane) {
   bool retVal = false;
   withinBounds = false;
   
   for (int i=0; i<UNIFORM_GRID_SIZE; ++i) {
      if (spatialGrid[i].hasPlayer) {
         for (int j=0; j<spatialGrid[i].members.size(); ++j) {
            if (spatialGrid[i].members[j] != player) {
               if (((Object*)spatialGrid[i].members[j])->planarCollisionCheck(player, colPlane)) {
                  retVal = true;
               }
            }
         }
      }
   }
   /*for (int i=0; i<structures.size(); ++i) {
      if (structures[i]->planarCollisionCheck(player, colPlane)) {
         return true;
      }
   }*/
   for (int i=0; i<booths.size(); ++i) {
     booths[i]->checkInteract(player->pos);
      
      if (booths[i]->isActive()) {
         withinBounds = true;
         //char ln[60];
         minigame = booths[i]->getMinigameSplash();
         /*fontEngine->useFont("goodDog", 48);
         sprintf(ln, "%s!", minigame.c_str());
         fontEngine->display(glm::vec4(0.99, 0.56, 0.55, 1.0), ln, 0-fontEngine->getTextWidth(ln)/2.0, 0.3);*/
      }
      /*if (booths[i]->booth[1]->planarCollisionCheck(player, colPlane)) {
         return true;
      }*/
   }
   return retVal;
}

string Camera::getMinigameText() {
   return minigame;
}

bool Camera::getWithinBounds() {
   return withinBounds;
}

void Camera::applyProjectionMatrix(MatrixStack* P) {
   P->multMatrix(Projection);
}

void Camera::applyViewMatrix(MatrixStack* MV) {
   MV->multMatrix(View);
}

glm::vec3 Camera::calcNewPos(Window* window) {
   glm::vec3 newPos = debug ? debug_pos : player->pos;
   float moveInc = speed * window->dt;
   
   glm::vec3 viewVector = glm::normalize(newPos - lookAtPt());
   glm::vec3 strafeVector = glm::normalize(glm::cross(viewVector, glm::vec3(0.0f, 1.0f, 0.0f)));
   glm::vec3 crossVector = glm::normalize(glm::cross(viewVector, strafeVector));
   // Scale vectors
   viewVector *= moveInc;
   strafeVector *= moveInc;
   crossVector *= moveInc;

   GLFWwindow* win = window->glfw_window;
   if (!debug) {
      // Normal camera controls
      if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) { // Move forward
         if (abs(playerYrad - theta) > 0.00001) {
            playerYrad = -theta - (M_PI/2);
         }

         newPos.x += moveInc * viewVector.x; //sin(playerYrad)
         newPos.z += moveInc * viewVector.z; //cos(playerYrad) 
      }
      if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) { // Rotate left
         playerYrad += PLAYER_ROT_RAD;
         theta -= PLAYER_ROT_RAD;
      }
      if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) { // Rotate right
         playerYrad -= PLAYER_ROT_RAD;
         theta += PLAYER_ROT_RAD;         
      }
      
      if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) { // Move backward
         if (abs(playerYrad - theta) > 0.00001) {
            playerYrad = -theta + (M_PI/2);
         }
         
         newPos.x -= moveInc * viewVector.x; //sin(playerYrad);
         newPos.z -= moveInc * viewVector.z; //cos(playerYrad);
      }
      
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
  /*if (!debug) {
     float s = SIZE - 0.2f;
     if (newPos.x < -s)
        newPos.x = -s;
     if (newPos.x > s)
        newPos.x = s;
     if (newPos.z < -s)
        newPos.z = -s;
     if (newPos.z > s)
        newPos.z = s;

     newPos.y = radius/2.0f;
  }*/

   if (player != NULL) {
   
      player->checkForPlayer();
   
      // run hit detection on this so called new position!
      glm::vec3 colPlane = glm::vec3(0.0f, 0.0f, 0.0f);
      if (checkStaticObjectCollisions(player, &colPlane)) {
         // there is a hit...
         if (colPlane.x != 0.0f && colPlane.z != 0.0f) {
            newPos = glm::vec3(prevPos.x+(10.0f*PLAYER_OFFSET*colPlane.x), newPos.y, prevPos.z+(10.0f*PLAYER_OFFSET*colPlane.z));
         }
         else if (colPlane.x == -1.0f) { // hit minimum x plane
            //printf("hit minimum x plane\n");
            if (newPos.x > prevPos.x) {
               newPos = glm::vec3(prevPos.x-PLAYER_OFFSET, newPos.y, newPos.z);
            }
         }
         else if (colPlane.x == 1.0f) { // hit maximum x plane
            //printf("hit maximum x plane\n");
            if (newPos.x < prevPos.x) {
               newPos = glm::vec3(prevPos.x+PLAYER_OFFSET, newPos.y, newPos.z);
            }
         }
         else if (colPlane.z == -1.0f) { // hit minimum z plane
            //printf("hit minimum z plane\n");
            if (newPos.z > prevPos.z) {
               newPos = glm::vec3(newPos.x, newPos.y, prevPos.z-PLAYER_OFFSET);
            }
         }
         else if (colPlane.z == 1.0f) { // hit maximum z plane
            //printf("hit maximum z plane\n");
            if (newPos.z < prevPos.z) {
               newPos = glm::vec3(newPos.x, newPos.y, prevPos.z+PLAYER_OFFSET);
            }
         }
         else {
            //printf("Thats not good.... error in planar hit detection...\ncolPlane: ");
            //printVec3(colPlane);
         }
      }
      
      prevPos = newPos;
      //player->setPos(newPos);//calculatePlayerPos());
      //player->scale(glm::vec3(1.0f, 1.0f, 1.0f));
      //player->rotate(0.0, glm::vec3(0.0f, 0.0f, 0.0f));
      player->rotate(radiansToDegrees(playerYrad), glm::vec3(0.0f, 1.0f, 0.0f));
      
      if (pov) {
         player->draw();
      }
      
      /*if (withinBounds) {
         char ln[60];
         //string minigame = booths[i]->getMinigameSplash();
         fontEngine->useFont("goodDog", 48);
         sprintf(ln, "%s!", minigame.c_str());
         fontEngine->display(glm::vec4(0.99, 0.56, 0.55, 1.0), ln, 0-fontEngine->getTextWidth(ln)/2.0, 0.3);
      }*/
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
   if (glm::degrees(newPhi) < 80 && glm::degrees(newPhi) > -30) {
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

   // save location and camera
   prevPos = player->pos;
   prevTheta = theta;
   prevPhi = phi;

   // Send camera to the origin
   this->player->pos = glm::vec3(0, 2, 0);
   this->theta = -M_PI/2.0;
   this->phi = 0.0;
}

void Camera::moveToOverworld() {
   // restore former player position
   player->pos = prevPos;
   theta = prevTheta;
   phi = prevPhi;
   
   // Set the appropriate flags
   playingMinigame = false;
   
   pov = true;
   //playerYrot = 0.0f;
}

void Camera::initPlayer(Object *_player) {
   player = _player;
   player->setPos(pos);
}

