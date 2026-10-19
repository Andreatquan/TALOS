#pragma once

// What the bot does at each intersection.
enum RouteStep {
  GO_STRAIGHT,
  TURN_LEFT,
  TURN_RIGHT
};

// The route: one direction per intersection, in the order the bot reaches them.
// Traffic lights are NOT listed here. The camera finds them at each stop line,
// so the professor can move lights without changing this file.
const RouteStep ROUTE[] = {
  TURN_RIGHT,    // intersection 1
  GO_STRAIGHT,   // intersection 2
  TURN_LEFT,     // intersection 3
  TURN_LEFT,     // intersection 4
  GO_STRAIGHT,   // intersection 5
  TURN_RIGHT,    // intersection 6
};

const int ROUTE_LENGTH = sizeof(ROUTE) / sizeof(ROUTE[0]);