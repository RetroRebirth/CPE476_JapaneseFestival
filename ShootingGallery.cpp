#include "ShootingGallery.h"

ShootingGallery::ShootingGallery(GLuint _ShadeProg, Clicks* _clicks) {
   ShadeProg = _ShadeProg;
   clicks = _clicks;

   // Initialize a sphere to test clicking on TODO remove
   Object* object = new Object(shapes, materials, ShadeProg);
   object->load("sphere.obj");
   object->setPos(glm::vec3(0.0, 2.0, 10.0));
   object->setTexture(MISC_TYPE);
    object->setShadows(false);
   objects.push_back(object);
    
    // Create a wall in the back
    wall = new Object(shapes, materials, ShadeProg);
    wall->load("cube.obj");
    wall->setPos(glm::vec3(0.0, 0.0, 15.0));
    wall->scale(glm::vec3(100.0f, 100.0f, 1.0f));
    wall->setTexture(TEX_WOOD_WALL);
    wall->setShadows(false);
    
    //score = 0;
   clicks->setObjects(&objects);
}

void ShootingGallery::newTarget(){
    // Initialize a sphere to test clicking on TODO remove
    Object* object = new Object(shapes, materials, ShadeProg);
    object->load("sphere.obj");
    object->setPos(glm::vec3(Util::randF() * 10, Util::randF() * 10, 10.0));
    object->setTexture(MISC_TYPE);
    objects.push_back(object);
}

ShootingGallery::~ShootingGallery() {
   for (int i = 0; i < objects.size(); ++i) {
      delete objects[i];
   }
    for(int i = 0; i < bullets.size(); ++i){
        delete bullets[i];
    }
}

void ShootingGallery::makeBullets(){
    Object* bullet = new Object(shapes, materials, ShadeProg);
    bullet->load("sphere.obj");
    bullet->setPos(glm::vec3(0, 0, 0));
    bullet->setDir(clicks->getDirection());
    bullet->setSpeed(1.0f);
    bullet->setTexture(TEX_WOOD_WALL);
    bullets.push_back(bullet);
}

void ShootingGallery::step(Window* window) {
   vector<Object*> clickedObjects = clicks->getClickedObjects();
    makeBullets();
    for (int i = 0; i < clickedObjects.size(); ++i) {
      //clickedObjects[i]->remove();
       clickedObjects[i]->setTexture(WALL_TYPE);
        newTarget();
       score++;
       //printf("Hit a target! Score: %d\n", score);
   }
   for (int i = 0; i < objects.size(); ++i) {
      objects[i]->draw();
   }
    for(int i = 0; i< bullets.size(); ++i){
        if (glm::length(bullets[i]->getPos()) < 50.0f){
            if(bullets[i] != NULL){
                bullets[i]->setPos(bullets[i]->calculateNewPos(1.0f));
                bullets[i]->draw();
            }
        }else{
            delete bullets[i];
        }
    }
    wall->draw();
}
