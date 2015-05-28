#include "WatermelonSmash.h"

bool removeExplosions(vector<Particle*> p) {
   if (p[0]->cycles > 0) {
      return true;
   }
   return false;
}

WatermelonSmash::WatermelonSmash(GLuint _ShadeProg, Program* _particleProg, Camera* _camera, Sound* _sound) {

    // Inititalize the game
    ShadeProg = _ShadeProg;
    particleProg = _particleProg;
    camera = _camera;
    sound = _sound;
    score = numMelons = 0;
    timeStart = timer = timeLeft = timeRight = timeSwing = 0.0;
    spawnLeft = spawnRight = false;
    gameStart = gameOver = false;

    // Set up the booth
    setUp();
}

WatermelonSmash::~WatermelonSmash() {
    for (int i = 0; i < bullets.size(); ++i) {
        delete bullets[i];
    }
    for (int i = 0; i < melons.size(); i++) {
        delete melons[i];
    }
    for (int i = 0; i < misc_objects.size(); i++) {
        delete misc_objects[i];
    }
    delete hammer;
}

void WatermelonSmash::setUp() {
    // Create a wall in the back
    Object *wall = new Object(shapes, materials, ShadeProg);
    wall->load("cube.obj");
    wall->setPos(glm::vec3(0.0, 0.0, 30.0));
    wall->scale(glm::vec3(100.0f, 100.0f, 1.0f));
    wall->setTexture(TEX_WOOD_LIGHT);
    wall->setShadows(false);
    misc_objects.push_back(wall);
    
    // Create a table for the watermelons
    Object *table = new Object(shapes, materials, ShadeProg);
    table->load("cube.obj");
    table->setPos(glm::vec3(0.0, 0.0, MELON_DEPTH));
    table->scale(glm::vec3(10.0f, 1.0f, 1.0f));
    table->setTexture(TEX_WOOD_DARK);
    table->setShadows(false);
    misc_objects.push_back(table);
    
    // Create the hammer
    hammer = new Object(shapes, materials, ShadeProg);
    hammer->load("objs/hammer.obj");
    hammer->setPos(glm::vec3(0.0, 3.5, MELON_DEPTH + .2));
    hammer->scale(glm::vec3(3.0, 3.0, 3.0));
    hammer->updateRadius();
    hammer->setTexture(TEX_HAMMER);
    hammer->setShadows(false);
    
    // Add watermelons
    newMelon(MELON_LEFT);
    newMelon(MELON_RIGHT);
}

void WatermelonSmash::newMelon(float xPos) {
    // Create a watermelon object
    Object *newObj = new Object(shapes, materials, ShadeProg);
    newObj->load((char *)"objs/watermelon.obj");
    newObj->setTexture(TEX_MELON_OUT);
    newObj->setShadows(false);
    
    // Add the watermelons to the game
    Watermelon *newMelon = new Watermelon(particleProg, camera, newObj, xPos);
    melons.push_back(newMelon);
}

void WatermelonSmash::checkTime(Window *window) {
    // Initialize the time if not done so already
    if (timeStart == 0.0) {
        timeStart = window->time;
        timer = window->time;
        srand(timeStart);
    }
    else {
        // Increment the timer every second
        if (window->time - timer >= 1.0) {
            timer = window->time;
        }
        // Check whether the game has ended
        if (window->time - timeStart >= MELON_TIME) {
            gameOver = true;
        }
        // Spawn a watermelon
        if (window->time - timeLeft >= MELON_SPAWN && spawnLeft) {
            newMelon(MELON_LEFT);
            ageLeft = window->time;
            spawnLeft = false;
        }
        else if (window->time - timeRight >= MELON_SPAWN && spawnRight) {
            newMelon(MELON_RIGHT);
            ageRight = window->time;
            spawnRight = false;
        }
        // Swing the hammer
        if (timeSwing - window->time > 0) {
            float angle = -360 * (timeSwing - window->time);
            hammer->rotate(angle, glm::vec3(1.0, 0.0, 0.0));
        }
    }
}

void WatermelonSmash::printInstructions() {
   ifstream instrFile;
   instrFile.open("wminstr.txt");
   string line;
   float yPos = .8;
   float yInc;
   
   fontEngine->useFont("amatic", 48);
   yInc = fontEngine->getTextHeight("blank") * 1.3;
   
   if (instrFile.is_open()) {
      while (getline(instrFile, line)) {
         if (line[0] == '\n') {
            yPos -= yInc;
         }
         
         yPos -= yInc;
         fontEngine->display(glm::vec4(0.98, 0.5, 0.48, 1.0), line.c_str(), 0-fontEngine->getTextWidth(line.c_str())/2.0, yPos);
      }
   }
   else {
      printf("file 'wminstr.txt' was not available or could not be opened\n");
   }
}

void WatermelonSmash::step(Window* window) {
    // Draw the booth
    for (int i = 0; i < misc_objects.size(); i++) 
        misc_objects[i]->draw();
    
    // Check whether game is playing
    if (!gameStart) {
        // PRINT INSTRUCTIONS HERE //
        printInstructions();        
        ageRight = ageLeft = window->time;
        return;
    }
    if (gameOver) {
        // TIME'S UP //
        // Score = x //
        // Watermelons Destroyed: x //
        return;
    }
    
    checkTime(window);
    
    // Draw the watermelons and hammer
    for (int i = 0; i < melons.size(); i++)
        melons[i]->object->draw();
    hammer->draw();
    
    // Check if the watermelons wilted
    for (int i = 0; i < melons.size(); i++) {
        if (melons[i]->xPos == MELON_LEFT) {
            if (window->time - ageLeft >= melons[i]->lifeSpan) {
                spawnLeft = true;
                timeLeft = window->time;
                sound->playBuzzerSound();
                melons.erase(melons.begin() + i--);
            }
        }
        else if (melons[i]->xPos == MELON_RIGHT) {
            if (window->time - ageRight >= melons[i]->lifeSpan) {
                spawnRight = true;
                timeRight = window->time;
                sound->playBuzzerSound();
                melons.erase(melons.begin() + i--);
            }
        }
    }
    
    // Fire the bullets
    for (int i = 0; i < bullets.size(); i++){
        if (bullets[i]->getPos().z <= MELON_DEPTH && bullets[i]->getPos().z >= -MELON_DEPTH) {
            if (bullets[i] != NULL) {
                bullets[i]->setPos(bullets[i]->calculateNewPos(window->dt));
                
                for (int j = 0; j < melons.size(); ++j) {
                    // Player hit a watermelon
                    if (bullets[i]->collidedWithObj(*melons[j]->object, window->dt)) {
                        // Hit the melon
                        timeSwing = window->time + MELON_SWING;
                        hammer->setPos(glm::vec3(melons[j]->xPos, melons[j]->yPos + melons[j]->size * 1.0, MELON_DEPTH + .2));
                        int pointsEarned = melons[j]->hit();
                        score += pointsEarned;
                        
                        // Remove the melon if it was destroyed
                        if (pointsEarned >= 5) {
                            if (melons[j]->xPos == MELON_LEFT) {
                                spawnLeft = true;
                                timeLeft = window->time;
                            }
                            if (melons[j]->xPos == MELON_RIGHT) {
                                spawnRight = true;
                                timeRight = window->time;
                            }
                            numMelons++;
                            melons.erase(melons.begin() + j--);
                            sound->playSplatSound();
                        }
                        else
                            sound->playThwackSound();
                    }
                }
            }
        }
        // Remove the bullet if it has gone past the target
        else
            bullets.erase(bullets.begin() + i--);
    }
    
    // Draw the HUD
    char scrStr[15];
    sprintf(scrStr, "Score: %d", score);
    
    // Draw the watermelons exploding
    for (int i = 0; i < melons.size(); i++) {
        melons[i]->explosionsStarted.erase(std::remove_if(melons[i]->explosionsStarted.begin(),
                                           melons[i]->explosionsStarted.end(),
                                           &removeExplosions),
                                           melons[i]->explosionsStarted.end());
        melons[i]->particleStep();
    }
    fontEngine->useFont("seaside", 30);
    fontEngine->display(glm::vec4(0.98, 0.5, 0.48, 1.0), scrStr, 0.55, 0.85);

}

void WatermelonSmash::mouseClick(glm::vec3 direction, glm::vec4 point) {
    // Shoot a "bullet"
    Object* bullet = new Object(shapes, materials, ShadeProg);
    bullet->load("sphere.obj");
    bullet->setPos(glm::vec3(point.x, point.y - 7.5, 0));
    bullet->setDir(direction);
    bullet->setSpeed(1.0f);
    bullet->setTexture(TEX_WOOD_WALL);
    bullet->setShadows(false);
    bullet->setSpeed(BULLET_SPD);
    bullet->scale(glm::vec3(0.2, 0.2, 0.2));
    bullets.push_back(bullet);
}
