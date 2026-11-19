#pragma once
// Perhitungan prioritas.
//   P = 0,4*U + 0,3*K + 0,3*D   (dibulatkan 1 desimal)
//   High 4,0-5,0 | Medium 2,5-3,9 | Low 1,0-2,4
// Skor kedekatan deadline D (sisa waktu r):
//   tanpa deadline -> 1 | terlambat atau r <= 24 jam -> 5 | <= 3 hari -> 4
//   <= 7 hari -> 3 | <= 14 hari -> 2 | lebih jauh -> 1
// Tugas selesai: prioritas otomatis selalu Low dan tidak pernah dianggap terlambat.
// Mode manual: level manual selalu dipakai; perhitungan otomatis hanya informasi.
#include "task.h"

namespace ptm {

double deadlineScore(int64_t deadline, int64_t now);
double autoScore(const Task& t, int64_t now);
Level classify(double score);
Level autoLevel(const Task& t, int64_t now);
Level effectiveLevel(const Task& t, int64_t now);
bool isOverdue(const Task& t, int64_t now);

}  // namespace ptm
