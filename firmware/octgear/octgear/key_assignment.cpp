#include "key_assignment.h"

KeyAssignment blankAssignment() {
  return {
    AssignmentKind::None,
    0,
    { 0, 0, 0, 0, 0, 0 },
    0,
    0,
  };
}

KeyAssignment keyboardAssignment(uint8_t keycode, uint8_t modifier) {
  return {
    AssignmentKind::Keyboard,
    modifier,
    { keycode, 0, 0, 0, 0, 0 },
    0,
    0,
  };
}

KeyAssignment consumerAssignment(uint16_t usage) {
  return {
    AssignmentKind::Consumer,
    0,
    { 0, 0, 0, 0, 0, 0 },
    usage,
    0,
  };
}

KeyAssignment layerCycleAssignment() {
  return {
    AssignmentKind::LayerCycle,
    0,
    { 0, 0, 0, 0, 0, 0 },
    0,
    0,
  };
}

KeyAssignment momentaryLayerAssignment(uint8_t layer) {
  return {
    AssignmentKind::MomentaryLayer,
    0,
    { 0, 0, 0, 0, 0, 0 },
    0,
    layer,
  };
}
