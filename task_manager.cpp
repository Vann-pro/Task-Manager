#include "task_manager.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <set>

#include "json.h"
#include "priority.h"

namespace ptm {
namespace {

using json::Value;
const size_t kMaxTitle = 200, kMaxDesc = 2000, kMaxTasks = 10000;
const int64_t kDay = 24LL * 3600LL * 1000LL;

Result fail(const std::string& msg) { Result r; r.ok = false; r.error = msg; return r; }

std::string trim(const std::string& s) {
  size_t a = 0, b = s.size();
  while (a < b && std::isspace((unsigned char)s[a])) a++;
  while (b > a && std::isspace((unsigned char)s[b - 1])) b--;
  return s.substr(a, b - a);
}
size_t cpCount(const std::string& s) {
  size_t n = 0;
  for (unsigned char c : s) if ((c & 0xC0) != 0x80) n++;
  return n;
}
std::string lower(std::string s) {
  for (auto& c : s) c = (char)std::tolower((unsigned char)c);
  return s;
}

bool getInt(const Value& o, const char* key, int lo, int hi, int& out, std::string& err, const std::string& ctx) {
  const Value* v = o.get(key);
  if (!v) return true;
  if (v->type != Value::Number || v->n != std::floor(v->n) || v->n < lo || v->n > hi) {
    err = ctx + ": '" + key + "' harus bilangan bulat " + std::to_string(lo) + "-" + std::to_string(hi);
    return false;
  }
  out = (int)v->n;
  return true;
}

bool getTime(const Value& o, const char* key, int64_t& out, bool& present, std::string& err, const std::string& ctx) {
  const Value* v = o.get(key);
  present = false;
  if (!v || v->type == Value::Null) return true;
  if (v->type != Value::Number || v->n < 0 || v->n > 8.64e15) {
    err = ctx + ": '" + key + "' harus berupa waktu (milidetik) yang valid atau null";
    return false;
  }
  out = (int64_t)v->n;
  present = true;
  return true;
}

bool getEnumStr(const Value& o, const char* key, std::string& out, bool& present, std::string& err, const std::string& ctx) {
  const Value* v = o.get(key);
  present = false;
  if (!v) return true;
  if (v->type != Value::String) { err = ctx + ": '" + key + "' harus berupa teks"; return false; }
  out = v->s;
  present = true;
  return true;
}

bool parseTask(const Value& o, int64_t now, Task& t, std::string& err, const std::string& ctx) {
  if (!o.isObj()) { err = ctx + ": harus berupa objek"; return false; }
  const Value* v = o.get("title");
  if (!v || v->type != Value::String) { err = ctx + ": 'title' wajib berupa teks"; return false; }
  t.title = trim(v->s);
  if (t.title.empty()) { err = ctx + ": judul tidak boleh kosong"; return false; }
  if (cpCount(t.title) > kMaxTitle) { err = ctx + ": judul maksimal 200 karakter"; return false; }

  t.description.clear();
  if ((v = o.get("description")) && v->type != Value::Null) {
    if (v->type != Value::String) { err = ctx + ": 'description' harus berupa teks"; return false; }
    t.description = trim(v->s);
    if (cpCount(t.description) > kMaxDesc) { err = ctx + ": deskripsi maksimal 2000 karakter"; return false; }
  }
  bool present;
  int64_t tm = 0;
  if (!getTime(o, "createdAt", tm, present, err, ctx)) return false;
  t.createdAt = present ? tm : now;
  tm = 0;
  if (!getTime(o, "deadline", tm, present, err, ctx)) return false;
  t.deadline = present ? tm : 0;

  if (!getInt(o, "urgency", 1, 5, t.urgency, err, ctx)) return false;
  if (!getInt(o, "importance", 1, 5, t.importance, err, ctx)) return false;

  std::string s;
  if (!getEnumStr(o, "status", s, present, err, ctx)) return false;
  if (present && !parseStatus(s, t.status)) { err = ctx + ": status '" + s + "' tidak dikenal"; return false; }
  if (!getEnumStr(o, "priorityMode", s, present, err, ctx)) return false;
  if (present && !parseMode(s, t.mode)) { err = ctx + ": priorityMode '" + s + "' tidak dikenal"; return false; }
  if (!getEnumStr(o, "manualPriority", s, present, err, ctx)) return false;
  if (present && !parseLevel(s, t.manualPriority)) { err = ctx + ": manualPriority '" + s + "' tidak dikenal"; return false; }
  return true;
}

// Membaca dokumen {tasks:[...], nextId} atau array langsung. Semua-atau-tidak-sama-sekali.
bool parseDocument(const std::string& text, int64_t now, std::vector<Task>& out, int& nextId, std::string& err) {
  Value root;
  std::string perr;
  if (!json::parse(text, root, perr)) { err = "JSON tidak valid: " + perr; return false; }
  const Value* arr = nullptr;
  int declaredNext = 1;
  if (root.type == Value::Array) {
    arr = &root;
  } else if (root.isObj()) {
    arr = root.get("tasks");
    if (!arr || arr->type != Value::Array) { err = "properti 'tasks' (array) tidak ditemukan"; return false; }
    const Value* n = root.get("nextId");
    if (n && n->type == Value::Number && n->n >= 1 && n->n < 1e9 && n->n == std::floor(n->n)) declaredNext = (int)n->n;
  } else {
    err = "format berkas tidak dikenali";
    return false;
  }
  if (arr->a.size() > kMaxTasks) { err = "jumlah tugas melebihi batas 10000"; return false; }

  std::vector<Task> tmp;
  std::set<int> used;
  int maxId = 0;
  for (size_t i = 0; i < arr->a.size(); i++) {
    const Value& o = arr->a[i];
    std::string ctx = "Tugas #" + std::to_string(i + 1);
    Task t;
    if (!parseTask(o, now, t, err, ctx)) return false;
    const Value* idv = o.isObj() ? o.get("id") : nullptr;
    if (idv) {
      if (idv->type != Value::Number || idv->n < 1 || idv->n >= 1e9 || idv->n != std::floor(idv->n)) {
        err = ctx + ": 'id' harus bilangan bulat positif";
        return false;
      }
      t.id = (int)idv->n;
      if (!used.insert(t.id).second) { err = ctx + ": id " + std::to_string(t.id) + " ganda"; return false; }
      maxId = std::max(maxId, t.id);
    }
    tmp.push_back(std::move(t));
  }
  int next = std::max(declaredNext, maxId + 1);
  for (auto& t : tmp) if (t.id == 0) t.id = next++;
  out = std::move(tmp);
  nextId = next;
  return true;
}

std::string numStr(double v) {
  char buf[40];
  std::snprintf(buf, sizeof buf, "%.0f", v);
  return buf;
}

std::string taskJson(const Task& t, int64_t now, bool computed) {
  std::string s = "{";
  s += "\"id\":" + std::to_string(t.id);
  s += ",\"title\":\"" + json::escape(t.title) + "\"";
  s += ",\"description\":\"" + json::escape(t.description) + "\"";
  s += ",\"createdAt\":" + numStr((double)t.createdAt);
  s += ",\"deadline\":" + (t.deadline > 0 ? numStr((double)t.deadline) : std::string("null"));
  s += ",\"urgency\":" + std::to_string(t.urgency);
  s += ",\"importance\":" + std::to_string(t.importance);
  s += std::string(",\"status\":\"") + toString(t.status) + "\"";
  s += std::string(",\"priorityMode\":\"") + toString(t.mode) + "\"";
  s += std::string(",\"manualPriority\":\"") + toString(t.manualPriority) + "\"";
  if (computed) {
    char b[16];
    std::snprintf(b, sizeof b, "%.1f", autoScore(t, now));
    s += std::string(",\"autoScore\":") + b;
    s += std::string(",\"autoPriority\":\"") + toString(autoLevel(t, now)) + "\"";
    s += std::string(",\"priority\":\"") + toString(effectiveLevel(t, now)) + "\"";
    s += std::string(",\"overdue\":") + (isOverdue(t, now) ? "true" : "false");
  }
  return s + "}";
}

}  // namespace

std::string resultToJson(const Result& r) {
  if (!r.ok) return "{\"ok\":false,\"error\":\"" + json::escape(r.error) + "\"}";
  return "{\"ok\":true,\"id\":" + std::to_string(r.id) + ",\"count\":" + std::to_string(r.count) + "}";
}

Result TaskManager::load(const std::string& text, int64_t now) {
  Result r;
  if (trim(text).empty()) { tasks_.clear(); nextId_ = 1; return r; }
  std::vector<Task> tmp;
  int next = 1;
  std::string err;
  if (!parseDocument(text, now, tmp, next, err)) return fail(err);
  tasks_ = std::move(tmp);
  nextId_ = next;
  r.count = (int)tasks_.size();
  return r;
}

Result TaskManager::importData(const std::string& text, int mode, int64_t now) {
  std::vector<Task> tmp;
  int next = 1;
  std::string err;
  if (!parseDocument(text, now, tmp, next, err)) return fail(err);
  Result r;
  r.count = (int)tmp.size();
  if (mode == 1) {
    tasks_ = std::move(tmp);
    nextId_ = next;
  } else if (mode == 2) {
    if (tasks_.size() + tmp.size() > kMaxTasks) return fail("jumlah tugas melebihi batas 10000");
    for (auto& t : tmp) { t.id = nextId_++; tasks_.push_back(std::move(t)); }
  }
  return r;
}

Result TaskManager::add(const std::string& text, int64_t now) {
  if (tasks_.size() >= kMaxTasks) return fail("jumlah tugas melebihi batas 10000");
  Value root;
  std::string err;
  if (!json::parse(text, root, err)) return fail("JSON tidak valid: " + err);
  Task t;
  if (!parseTask(root, now, t, err, "Tugas")) return fail(err);
  t.id = nextId_++;
  t.createdAt = now;
  tasks_.push_back(t);
  Result r;
  r.id = t.id;
  r.count = (int)tasks_.size();
  return r;
}

Result TaskManager::update(const std::string& text, int64_t now) {
  Value root;
  std::string err;
  if (!json::parse(text, root, err)) return fail("JSON tidak valid: " + err);
  const Value* idv = root.isObj() ? root.get("id") : nullptr;
  if (!idv || idv->type != Value::Number) return fail("'id' wajib ada");
  int id = (int)idv->n;
  auto it = std::find_if(tasks_.begin(), tasks_.end(), [&](const Task& t) { return t.id == id; });
  if (it == tasks_.end()) return fail("tugas tidak ditemukan");
  Task t = *it;
  if (!parseTask(root, now, t, err, "Tugas")) return fail(err);
  t.id = it->id;
  t.createdAt = it->createdAt;
  *it = t;
  Result r;
  r.id = id;
  r.count = (int)tasks_.size();
  return r;
}

Result TaskManager::remove(int id) {
  auto it = std::find_if(tasks_.begin(), tasks_.end(), [&](const Task& t) { return t.id == id; });
  if (it == tasks_.end()) return fail("tugas tidak ditemukan");
  tasks_.erase(it);
  Result r;
  r.id = id;
  r.count = (int)tasks_.size();
  return r;
}

std::string TaskManager::get(int id, int64_t now) const {
  for (const auto& t : tasks_) if (t.id == id) return "{\"ok\":true,\"task\":" + taskJson(t, now, true) + "}";
  return "{\"ok\":false,\"error\":\"tugas tidak ditemukan\"}";
}

std::string TaskManager::serialize(int64_t now) const {
  std::string s = "{\"app\":\"personal-task-manager\",\"version\":1,\"exportedAt\":" + numStr((double)now);
  s += ",\"nextId\":" + std::to_string(nextId_) + ",\"tasks\":[";
  for (size_t i = 0; i < tasks_.size(); i++) {
    if (i) s += ",";
    s += taskJson(tasks_[i], now, false);
  }
  return s + "]}";
}

std::string TaskManager::query(const std::string& filterJson, int64_t now) const {
  Value f;
  if (!trim(filterJson).empty()) {
    std::string err;
    if (!json::parse(filterJson, f, err) || !f.isObj()) return "{\"ok\":false,\"error\":\"filter tidak valid\"}";
  }
  auto str = [&](const char* k, const char* def) {
    const Value* v = f.get(k);
    return (v && v->type == Value::String) ? v->s : std::string(def);
  };
  auto num = [&](const char* k, double def) {
    const Value* v = f.get(k);
    return (v && v->type == Value::Number) ? v->n : def;
  };
  const std::string fStatus = str("status", "all"), fPrio = str("priority", "all"), fDead = str("deadline", "all");
  const std::string search = lower(trim(str("search", "")));
  const std::string sort = str("sort", "created"), dir = str("dir", "desc");
  const int64_t dayStart = (int64_t)num("dayStart", (double)(now - now % kDay));
  const int64_t dayEnd = (int64_t)num("dayEnd", (double)(dayStart + kDay - 1));
  const int64_t weekEnd = dayStart + 7 * kDay - 1;

  struct Row { const Task* t; double score; Level lvl; };
  std::vector<Row> rows;
  for (const auto& t : tasks_) {
    if (fStatus != "all") { Status st; if (parseStatus(fStatus, st) && t.status != st) continue; }
    Level lvl = effectiveLevel(t, now);
    if (fPrio != "all") { Level pl; if (parseLevel(fPrio, pl) && lvl != pl) continue; }
    if (fDead == "overdue" && !isOverdue(t, now)) continue;
    if (fDead == "today" && !(t.deadline > 0 && t.deadline >= dayStart && t.deadline <= dayEnd)) continue;
    if (fDead == "week" && !(t.deadline > 0 && t.deadline >= dayStart && t.deadline <= weekEnd)) continue;
    if (fDead == "none" && t.deadline != 0) continue;
    if (!search.empty() && lower(t.title).find(search) == std::string::npos) continue;
    rows.push_back({&t, autoScore(t, now), lvl});
  }
  const bool asc = (dir == "asc");
  std::stable_sort(rows.begin(), rows.end(), [&](const Row& a, const Row& b) {
    int c = 0;
    if (sort == "deadline") {
      bool an = a.t->deadline == 0, bn = b.t->deadline == 0;
      if (an != bn) return bn;  // tanpa deadline selalu di akhir
      c = a.t->deadline < b.t->deadline ? -1 : (a.t->deadline > b.t->deadline ? 1 : 0);
    } else if (sort == "priority") {
      if (a.lvl != b.lvl) c = (int)a.lvl < (int)b.lvl ? -1 : 1;
      else c = a.score < b.score ? -1 : (a.score > b.score ? 1 : 0);
    } else {
      c = a.t->createdAt < b.t->createdAt ? -1 : (a.t->createdAt > b.t->createdAt ? 1 : 0);
    }
    if (c == 0) return a.t->id < b.t->id;
    return asc ? c < 0 : c > 0;
  });
  std::string s = "{\"ok\":true,\"total\":" + std::to_string(tasks_.size()) + ",\"count\":" + std::to_string(rows.size()) + ",\"tasks\":[";
  for (size_t i = 0; i < rows.size(); i++) {
    if (i) s += ",";
    s += taskJson(*rows[i].t, now, true);
  }
  return s + "]}";
}

std::string TaskManager::stats(int64_t now) const {
  int total = 0, todo = 0, prog = 0, done = 0, overdue = 0, hi = 0, me = 0, lo = 0, today = 0, week = 0;
  const int64_t dayStart = now - now % kDay;
  for (const auto& t : tasks_) {
    total++;
    if (t.status == Status::Todo) todo++;
    else if (t.status == Status::InProgress) prog++;
    else done++;
    if (isOverdue(t, now)) overdue++;
    if (t.status != Status::Completed) {
      Level l = effectiveLevel(t, now);
      if (l == Level::High) hi++; else if (l == Level::Medium) me++; else lo++;
      if (t.deadline > 0 && t.deadline >= now && t.deadline < dayStart + kDay) today++;
      if (t.deadline > 0 && t.deadline >= now && t.deadline < now + 7 * kDay) week++;
    }
  }
  int rate = total ? (int)std::floor(100.0 * done / total + 0.5) : 0;
  char b[400];
  std::snprintf(b, sizeof b,
                "{\"total\":%d,\"todo\":%d,\"inprogress\":%d,\"completed\":%d,\"overdue\":%d,"
                "\"high\":%d,\"medium\":%d,\"low\":%d,\"dueToday\":%d,\"dueWeek\":%d,\"completionRate\":%d}",
                total, todo, prog, done, overdue, hi, me, lo, today, week, rate);
  return b;
}

}  // namespace ptm
