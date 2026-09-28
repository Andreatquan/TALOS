#pragma once

// What the bot does at each intersection.
enum RouteStep {
  GO_STRAIGHT,
  TURN_LEFT,
  TURN_RIGHT
};

const RouteStep ROUTE[] = {
  TURN_RIGHT,    // intersection 1
  GO_STRAIGHT,   // intersection 2
  TURN_LEFT,     // intersection 3
  TURN_LEFT,     // intersection 4
  GO_STRAIGHT,   // intersection 5
  TURN_RIGHT,    // intersection 6
};

const int ROUTE_LENGTH = sizeof(ROUTE) / sizeof(ROUTE[0]);