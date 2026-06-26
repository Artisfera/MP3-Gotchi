/*
  MP3-Gotchi firmware v0.1.4

  Concept, hardware direction and project decisions: Patryk Ankudowicz (Artisfera).
  Firmware implementation prepared in collaboration with ChatGPT as programming assistant.

  This note is intentional. The project is source-available non-commercial and does not hide that AI was used
  as a coding partner. The device concept, build decisions and creative direction stay with
  the project author.
*/

#include "App.h"

App app;

void setup() {
  app.begin();
}

void loop() {
  app.loop();
}
