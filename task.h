#pragma once
// Struktur data tugas (tanpa logika bisnis).
#include <cstdint>
#include <string>

namespace ptm {

enum class Status { Todo, InProgress, Completed };
enum class Level { Low = 0, Medium = 1, High = 2 };
enum class PriorityMode { Auto, Manual };

struct Task {
  int id = 0;
  std::string title;
  std::string description;
  int64_t createdAt = 0;  // epoch milidetik
  int64_t deadline = 0;   // epoch milidetik; 0 = tanpa deadline
  int urgency = 3;        // 1..5
  int importance = 3;     // 1..5
  Status status = Status::Todo;
  PriorityMode mode = PriorityMode::Auto;
  Level manualPriority = Level::Medium;  // hanya dipakai jika mode == Manual
};

const char* toString(Status s);
const char* toString(Level l);
const char* toString(PriorityMode m);
bool parseStatus(const std::string& s, Status& out);
bool parseLevel(const std::string& s, Level& out);
bool parseMode(const std::string& s, PriorityMode& out);

}  // namespace ptm
