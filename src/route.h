#pragma once

enum RouteStep {
  GO_STRAIGHT,
  TURN_LEFT,
  TURN_RIGHT
};

struct Intersection {
  RouteStep action;    // what to do at this intersection
  bool hasLight;      // capture frames when intersection reached
};

const Intersection ROUTE[] = {
  {TURN_RIGHT,  true},  // intersection 1
  {GO_STRAIGHT, true},   // intersection 2
  {TURN_LEFT,   true},   // intersection 3
  {TURN_LEFT,   true},  // intersection 4
  {GO_STRAIGHT, true},   // intersection 5
  {TURN_RIGHT,  true},   // intersection 6
};

const int ROUTE_LENGTH = sizeof(ROUTE) / sizeof(ROUTE[0]);