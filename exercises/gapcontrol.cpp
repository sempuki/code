#include <iostream>
#include <array>
#include <vector>
#include <cassert>
#include <cstdint>

class GapControl {
 public:
  enum class Direction { incoming, outgoing };

  struct Map {
    uint16_t incoming = 0;
    uint16_t outgoing = 0;
    bool discarded = false;
    bool valid = false;
  };

  GapControl(uint16_t instart = 0, uint16_t outstart = 0)
    : incoming_{instart}, outgoing_{outstart} {}

  uint16_t inject(uint16_t count) {
    uint16_t outgoing = 0;
    while (count--) {
      outgoing = outgoing_++;
      outgoingmap_[outgoing % kMapSize] = {
        0, outgoing, false, true
      };
    }

    state_ = State::inject;
    return outgoing;
  }

  uint16_t translate(uint16_t incoming) {
    if (state_ == State::initial) {
      incoming_ = incoming;
    }

    uint16_t outgoing = outgoing_;
    uint16_t backfill = incoming - incoming_ + 1;
    while (backfill--) {  // translate presumed lost packets
      outgoing = outgoing_++;
      incoming = incoming_++; 
      outgoingmap_[outgoing % kMapSize] =
      incomingmap_[incoming % kMapSize] = {
        incoming, outgoing, false, true
      };
    }

    state_ = State::translate;
    return outgoing;
  }

  void discard(uint16_t incoming) {
    if (state_ == State::translate) {
      translate(incoming-1);  // translate presumed lost packets
    }

    uint16_t backfill = incoming - incoming_ + 1;
    while (backfill--) {  // discard presumed lost packets
      incoming = incoming_++;
      incomingmap_[incoming % kMapSize] = {
        incoming, 0, true, true
      };
    }

    state_ = State::discard;
  }

  bool contains(Direction direction, uint16_t seqnum) {
    size_t index = seqnum % kMapSize;
    switch (direction) {
      case Direction::incoming:
        return incomingmap_[index].valid && incomingmap_[index].incoming == seqnum;
      case Direction::outgoing:
        return outgoingmap_[index].valid && outgoingmap_[index].outgoing == seqnum;
    }
    return false;
  }

  Map lookup(Direction direction, uint16_t seqnum) {
    size_t index = seqnum % kMapSize;
    switch (direction) {
      case Direction::incoming: return incomingmap_[index];
      case Direction::outgoing: return outgoingmap_[index];
    }
    assert(false);
  }

 private:
  uint16_t incoming_ = 0;
  uint16_t outgoing_ = 0;

  enum class State { initial, inject, translate, discard };
  State state_ = State::initial;

  static constexpr size_t kMapSize = 4096;
  std::array<Map, kMapSize> incomingmap_;
  std::array<Map, kMapSize> outgoingmap_;
};

struct GapEvent {
  enum class Type { LOST_PASSED, LOST_DISCARDED, DISCARDED, PASSED, CUSTOM };
  int original;
  int expected;
  Type event;
};

struct GapTest {
  std::vector<GapEvent> events;
  uint16_t init_incoming = 0;
  uint16_t init_outgoing = 0;
};

bool execute(const GapTest& test) {
  GapControl control{test.init_incoming, test.init_outgoing};

  // Run test as simulation
  for (auto &&test : test.events) {
    switch (test.event) {
      case GapEvent::Type::PASSED:
        {
          if (!control.contains(GapControl::Direction::incoming, test.original)) {
            control.translate(test.original);
          }

          bool has_original = control.contains(GapControl::Direction::incoming, test.original);
          bool has_expected = control.contains(GapControl::Direction::outgoing, test.expected);
          GapControl::Map map1 = control.lookup(GapControl::Direction::incoming, test.original);
          GapControl::Map map2 = control.lookup(GapControl::Direction::outgoing, test.expected);

          assert(has_original == true);
          assert(has_expected == true);
          assert(map1.incoming == test.original);
          assert(map1.outgoing == test.expected);
          assert(map1.discarded == false);
          assert(map1.valid == true);
          assert(map2.incoming == map1.incoming);
          assert(map2.outgoing == map1.outgoing);
          assert(map2.discarded == map1.discarded);
        }
        break;

      case GapEvent::Type::DISCARDED:
        {
          control.discard(test.original);
          bool has_original = control.contains(GapControl::Direction::incoming, test.original);
          GapControl::Map map = control.lookup(GapControl::Direction::incoming, test.original);

          assert(has_original == true);
          assert(map.incoming == test.original);
          assert(map.discarded == true);
          assert(map.valid == true);
        }
        break;

      case GapEvent::Type::CUSTOM:
        {
          if (!control.contains(GapControl::Direction::incoming, test.original)) {
            control.inject(1);
          }

          bool has_expected = control.contains(GapControl::Direction::outgoing, test.expected);
          GapControl::Map map = control.lookup(GapControl::Direction::outgoing, test.expected);

          assert(has_expected == true);
          assert(map.outgoing == test.expected);
          assert(map.discarded == false);
          assert(map.valid == true);
        }
        break;

      default:
        break;  // do nothing
    }
  }

  // Ensure incoming -> outgoing queries are recalled correctly post simulation
  for (auto &&test : test.events) {
    switch (test.event) {
      case GapEvent::Type::PASSED:
        {
          bool has_original = control.contains(GapControl::Direction::incoming, test.original);
          GapControl::Map map = control.lookup(GapControl::Direction::incoming, test.original);

          assert(has_original == true);
          assert(map.incoming == test.original);
          assert(map.outgoing == test.expected);
          assert(map.discarded == false);
          assert(map.valid == true);
        }
        break;

      default:
        break;
    }
  }

  // Ensure outgoing -> incoming queries are recalled correctly post simulation
  for (auto &&test : test.events) {
    switch (test.event) {
      case GapEvent::Type::PASSED:
        {
          bool has_expected = control.contains(GapControl::Direction::outgoing, test.expected);
          GapControl::Map map = control.lookup(GapControl::Direction::outgoing, test.expected);

          assert(has_expected == true);
          assert(map.incoming == test.original);
          assert(map.outgoing == test.expected);
          assert(map.discarded == false);
          assert(map.valid == true);
        }
        break;

      default:
        break;
    }
  }

  // Ensure discard queries are recalled correctly post simulation
  for (auto &&test : test.events) {
    switch (test.event) {
      case GapEvent::Type::DISCARDED:
        {
          bool has_original = control.contains(GapControl::Direction::incoming, test.original);
          GapControl::Map map = control.lookup(GapControl::Direction::incoming, test.original);

          assert(has_original == true);
          assert(map.incoming == test.original);
          assert(map.discarded == true);
          assert(map.valid == true);
        }
        break;

      default:
        break;
    }
  }

  return true;
}

/**
 *   5   4
 *   6   5
 *   7 x
 *   8 x
 *   9   6
 *  10   7
 *  11   8
 */
bool packetLossNone() {
  return execute({{
      {5, 4, GapEvent::Type::PASSED},
      {6, 5, GapEvent::Type::PASSED},
      {7, 0, GapEvent::Type::DISCARDED},
      {8, 0, GapEvent::Type::DISCARDED},
      {9, 6, GapEvent::Type::PASSED},
      {10, 7, GapEvent::Type::PASSED},
      {11, 8, GapEvent::Type::PASSED},
    }, 5, 4});
}

/**
 *   5   4
 *   6   5
 * x
 * x
 *   9   8
 *  10   9
 *  11   10
 */
bool packetLossSome() {
  return execute({{
      {5, 4, GapEvent::Type::PASSED},
      {6, 5, GapEvent::Type::PASSED},
      {7, 6, GapEvent::Type::LOST_PASSED},
      {8, 7, GapEvent::Type::LOST_PASSED},
      {9, 8, GapEvent::Type::PASSED},
      {10, 9, GapEvent::Type::PASSED},
    }, 5, 4});
}

/**
 *   7   6
 * x
 * x
 *  10 x
 *  11 x
 *  12   9
 */
bool packetLossBegining() {
  return execute({{
      {7, 6, GapEvent::Type::PASSED},
      {8, 7, GapEvent::Type::LOST_PASSED},
      {9, 8, GapEvent::Type::LOST_PASSED},
      {10, 0, GapEvent::Type::DISCARDED},
      {11, 0, GapEvent::Type::DISCARDED},
      {12, 9, GapEvent::Type::PASSED}
    }, 7, 6});
}

/**
 *   6   5
 *   7 x
 * x
 *   9 x
 *  10   6
 */
bool packetLossMiddle() {
  return execute({{
      {6, 5, GapEvent::Type::PASSED},
      {7, 0, GapEvent::Type::DISCARDED},
      {8, 0, GapEvent::Type::LOST_DISCARDED},
      {9, 0, GapEvent::Type::DISCARDED},
      {10, 6, GapEvent::Type::PASSED}
    }, 6, 5});
}

/**
 *   7   6
 *   8 x
 *   9 x
 * x
 * x
 *  12   9
 */
bool packetLossEnd() {
  return execute({{
    {7, 6, GapEvent::Type::PASSED},
    {8, 0, GapEvent::Type::DISCARDED},
    {9, 0, GapEvent::Type::DISCARDED},
    {10, 7, GapEvent::Type::LOST_PASSED},
    {11, 8, GapEvent::Type::LOST_PASSED},
    {12, 9, GapEvent::Type::PASSED}
  }, 7, 6});
}

/**
 *   7   6
 * x
 * x
 *  10 x
 *  11 x
 * x
 *  13 x
 *  14   9
 *  15  10
 *
 */
bool packetLossBeginningMiddle() {
  return execute({{
    {7, 6, GapEvent::Type::PASSED},
    {8, 7, GapEvent::Type::LOST_PASSED},
    {9, 8, GapEvent::Type::LOST_PASSED},
    {10, 0, GapEvent::Type::DISCARDED},
    {11, 0, GapEvent::Type::DISCARDED},
    {12, 0, GapEvent::Type::LOST_DISCARDED},
    {13, 0, GapEvent::Type::DISCARDED},
    {14, 9, GapEvent::Type::PASSED},
    {15, 10, GapEvent::Type::PASSED}
  }, 7, 6});
}


/**
 *   7   6
 *   8 x
 *   9 x
 * x
 *  11 x
 * x
 * x
 *  14   9
 *  15  10
 *
 */
bool packetLossMiddleEnd() {
  return execute({{
    {7, 6, GapEvent::Type::PASSED},
    {8, 0, GapEvent::Type::DISCARDED},
    {9, 0, GapEvent::Type::DISCARDED},
    {10, 0, GapEvent::Type::LOST_DISCARDED},
    {11, 0, GapEvent::Type::DISCARDED},
    {12, 7, GapEvent::Type::LOST_PASSED},
    {13, 8, GapEvent::Type::LOST_PASSED},
    {14, 9, GapEvent::Type::PASSED},
    {15, 10, GapEvent::Type::PASSED}
  }, 7, 6});
}

/**
 *   7   6
 * x
 * x
 *  10 x
 *  11 x
 *  12 x
 * x
 *  14  10
 */
bool packetLossBeginningEnd() {
  return execute({{
    {7, 6, GapEvent::Type::PASSED},
    {8, 7, GapEvent::Type::LOST_PASSED},
    {9, 8, GapEvent::Type::LOST_PASSED},
    {10, 0, GapEvent::Type::DISCARDED},
    {11, 0, GapEvent::Type::DISCARDED},
    {12, 0, GapEvent::Type::DISCARDED},
    {13, 9, GapEvent::Type::LOST_PASSED},
    {14, 10, GapEvent::Type::PASSED}
  }, 7, 6});
}

/**
 *   7   6
 * x
 * x
 *  10 x
 *  11 x
 * x
 * x
 * x
 *  15 x
 * x
 *  17  10
 *  18  11
 */
bool packetLossBeginningMiddleEnd() {
  return execute({{
    {7, 6, GapEvent::Type::PASSED},
    {8, 7, GapEvent::Type::LOST_PASSED},
    {9, 8, GapEvent::Type::LOST_PASSED},
    {10, 0, GapEvent::Type::DISCARDED},
    {11, 0, GapEvent::Type::DISCARDED},
    {12, 0, GapEvent::Type::LOST_DISCARDED},
    {13, 0, GapEvent::Type::LOST_DISCARDED},
    {14, 0, GapEvent::Type::LOST_DISCARDED},
    {15, 0, GapEvent::Type::DISCARDED},
    {16, 9, GapEvent::Type::LOST_PASSED},
    {17, 10, GapEvent::Type::PASSED},
    {18, 11, GapEvent::Type::PASSED}
  }, 7, 6});
}

/**
 *    65533    65533
 *  x
 *    65535    65535
 *  x
 *        1        1
 */
bool wrapAround1() {
  return execute({{
    {65533, 65533, GapEvent::Type::PASSED},
    {65534, 65534, GapEvent::Type::LOST_PASSED},
    {65535, 65535, GapEvent::Type::PASSED},
    {0, 0, GapEvent::Type::LOST_PASSED},
    {1, 1, GapEvent::Type::PASSED}
  }, 65533, 65533});
}

/**
 *
 *    65535    65535
 *        0  x
 *        1  x
 *        2  x
 *        3       0
 */
bool wrapAround2() {
  return execute({{
    {65535, 65535, GapEvent::Type::PASSED},
    {0, 0, GapEvent::Type::DISCARDED},
    {1, 0, GapEvent::Type::DISCARDED},
    {2, 0, GapEvent::Type::DISCARDED},
    {3, 0, GapEvent::Type::PASSED},
  }, 65535, 65535});
}

/**
 *  1   0
 *  2   1
 *  4   3
 *  5   4
 *  3   2
 */
bool lateArrivalNoLosses() {
  return execute({{
    {1, 0, GapEvent::Type::PASSED},
    {2, 1, GapEvent::Type::PASSED},
    {4, 3, GapEvent::Type::PASSED},
    {5, 4, GapEvent::Type::PASSED},
    {3, 2, GapEvent::Type::PASSED}
  }});
}

/**
 *   1     0
 *   2     1
 * x
 *   4  x
 *   5     3
 *   3     2
 */
bool lateArrivalPacketsDiscarded() {
  return execute({{
    {1, 0, GapEvent::Type::PASSED},
    {2, 1, GapEvent::Type::PASSED},
    {3, 2, GapEvent::Type::LOST_PASSED},
    {4, 0, GapEvent::Type::DISCARDED},
    {5, 3, GapEvent::Type::PASSED},
    {3, 2, GapEvent::Type::PASSED}
  }});
}

/**
 *   5     6
 *   6     7
 *        [8]
 *        [9]
 *   7    10
 */
bool customPacket1() {
  return execute({{
    {5, 6, GapEvent::Type::PASSED},
    {6, 7, GapEvent::Type::PASSED},
    {0, 8, GapEvent::Type::CUSTOM},
    {0, 9, GapEvent::Type::CUSTOM},
    {7, 10, GapEvent::Type::PASSED}
  }, 5, 6});
}

/**
 *   5     6
 *   6  x
 *        [7]
 *        [8]
 *   7     9
 */
bool customPacketDiscardedBefore() {
  return execute({{
    {5, 6, GapEvent::Type::PASSED},
    {6, 0, GapEvent::Type::DISCARDED},
    {0, 7, GapEvent::Type::CUSTOM},
    {0, 8, GapEvent::Type::CUSTOM},
    {7, 9, GapEvent::Type::PASSED}
  }, 5, 6});
}

/**
 *   5     6
 *   6     7
 *        [8]
 *        [9]
 *   7 x
 *   8    10
 */
bool customPacketDiscardedAfter() {
  return execute({{
    {5, 6, GapEvent::Type::PASSED},
    {6, 7, GapEvent::Type::PASSED},
    {0, 8, GapEvent::Type::CUSTOM},
    {0, 9, GapEvent::Type::CUSTOM},
    {7, 0, GapEvent::Type::DISCARDED},
    {8, 10, GapEvent::Type::PASSED}
  }, 5, 6});
}

/**
 *   5     6
 *   6     7
 *   7 x
 *   8 x
 *        [8]
 *        [9]
 *   9 x
 *   10   10
 */
bool customPacketDiscardedBeforeAndAfter() {
  return execute({{
    {5, 6, GapEvent::Type::PASSED},
    {6, 7, GapEvent::Type::PASSED},
    {7, 0, GapEvent::Type::DISCARDED},
    {8, 0, GapEvent::Type::DISCARDED},
    {0, 8, GapEvent::Type::CUSTOM},
    {0, 9, GapEvent::Type::CUSTOM},
    {9, 0, GapEvent::Type::DISCARDED},
    {10, 10, GapEvent::Type::DISCARDED}
  }, 5, 6});
}

/**
 *   5     6
 *   6     7
 *        [8]
 *   7 x
 *   8 x
 *        [9]
 *   9 x
 *        [10]
 *   10   11
 */
bool customPacketDiscardedInterleaved() {
  return execute({{
    {5, 6, GapEvent::Type::PASSED},
    {6, 7, GapEvent::Type::PASSED},
    {0, 8, GapEvent::Type::CUSTOM},
    {7, 0, GapEvent::Type::DISCARDED},
    {8, 0, GapEvent::Type::DISCARDED},
    {0, 9, GapEvent::Type::CUSTOM},
    {9, 0, GapEvent::Type::DISCARDED},
    {0, 10, GapEvent::Type::CUSTOM},
    {10, 11, GapEvent::Type::PASSED}
  }, 5, 6});
}

int main() {
  assert(packetLossNone());
  assert(packetLossSome());
  assert(packetLossBegining());
  assert(packetLossMiddle());
  assert(packetLossEnd());
  assert(packetLossBeginningMiddle());
  assert(packetLossMiddleEnd());
  assert(packetLossBeginningEnd());
  assert(packetLossBeginningMiddleEnd());
  assert(wrapAround1());
  assert(wrapAround2());
  assert(lateArrivalNoLosses());
  assert(lateArrivalPacketsDiscarded());
  assert(customPacket1());
  assert(customPacketDiscardedBefore());
  assert(customPacketDiscardedAfter());
  assert(customPacketDiscardedBeforeAndAfter());
  assert(customPacketDiscardedInterleaved());
}
