#include "task.h"

namespace ptm {

const char* toString(Status s) {
  switch (s) {
    case Status::Todo: return "todo";
    case Status::InProgress: return "inprogress";
    case Status::Completed: return "completed";
  }
  return "todo";
}
const char* toString(Level l) {
  switch (l) {
    case Level::Low: return "low";
    case Level::Medium: return "medium";
    case Level::High: return "high";
  }
  return "low";
}
const char* toString(PriorityMode m) { return m == PriorityMode::Manual ? "manual" : "auto"; }

bool parseStatus(const std::string& s, Status& out) {
  if (s == "todo") out = Status::Todo;
  else if (s == "inprogress") out = Status::InProgress;
  else if (s == "completed") out = Status::Completed;
  else return false;
  return true;
}
bool parseLevel(const std::string& s, Level& out) {
  if (s == "low") out = Level::Low;
  else if (s == "medium") out = Level::Medium;
  else if (s == "high") out = Level::High;
  else return false;
  return true;
}
bool parseMode(const std::string& s, PriorityMode& out) {
  if (s == "auto") out = PriorityMode::Auto;
  else if (s == "manual") out = PriorityMode::Manual;
  else return false;
  return true;
}

}  // namespace ptm
