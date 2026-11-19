#include "priority.h"

#include <cmath>

namespace ptm {
namespace {
const int64_t kHour = 3600LL * 1000LL;
const int64_t kDay = 24 * kHour;
double round1(double v) { return std::floor(v * 10.0 + 0.5 + 1e-9) / 10.0; }
}  // namespace

double deadlineScore(int64_t deadline, int64_t now) {
  if (deadline <= 0) return 1.0;
  int64_t r = deadline - now;
  if (r <= kDay) return 5.0;  // termasuk yang sudah terlambat
  if (r <= 3 * kDay) return 4.0;
  if (r <= 7 * kDay) return 3.0;
  if (r <= 14 * kDay) return 2.0;
  return 1.0;
}

double autoScore(const Task& t, int64_t now) {
  return round1(0.4 * t.urgency + 0.3 * t.importance + 0.3 * deadlineScore(t.deadline, now));
}

Level classify(double s) {
  if (s >= 4.0) return Level::High;
  if (s >= 2.5) return Level::Medium;
  return Level::Low;
}

Level autoLevel(const Task& t, int64_t now) {
  if (t.status == Status::Completed) return Level::Low;
  return classify(autoScore(t, now));
}

Level effectiveLevel(const Task& t, int64_t now) {
  return t.mode == PriorityMode::Manual ? t.manualPriority : autoLevel(t, now);
}

bool isOverdue(const Task& t, int64_t now) {
  return t.status != Status::Completed && t.deadline > 0 && t.deadline < now;
}

}  // namespace ptm
