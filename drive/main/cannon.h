// Photon cannon: flashes the LED on GPIO18 without blocking the main loop.
// Sound (DFPlayer) joins it in milestone 7.
#pragma once

namespace cannon {

void begin();

// Queue `shots` flashes. Adds to any volley already in progress.
void fire(int shots);

// Call every loop.
void update();

bool busy();

// Drop any queued shots and turn the LED off.
void cancel();

}  // namespace cannon
