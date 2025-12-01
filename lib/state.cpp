#include "state.hpp"

MALState::MALState() : state(new MALState::State) {}

MALState::~MALState() { delete state; }

std::string MALState::print_str(void) const { return state->value; }
