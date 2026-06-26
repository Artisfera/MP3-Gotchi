/*
  MP3-Gotchi firmware v0.1.8

  Concept, hardware direction and project decisions: Patryk Ankudowicz (Artisfera).
  Firmware implementation prepared in collaboration with ChatGPT as programming assistant.

  This note is intentional. The project is shared as source-available community firmware
  and does not hide that AI was used as a coding partner. The device concept, build
  decisions and creative direction stay with the project author.
*/

#include "App.h"

App app;

void setup() {
  app.begin();
}

void loop() {
  app.loop();
}
