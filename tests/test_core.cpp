// Uji logika inti C++ secara native (tanpa browser). Jalankan: scripts/test.sh atau scripts/test.bat
#include <cstdio>
#include <cstring>
#include <string>

#include "../src/json.h"
#include "../src/priority.h"
#include "../src/task_manager.h"

extern "C" {
const char* ptm_load(const char*, double);
const char* ptm_add(const char*, double);
const char* ptm_query(const char*, double);
const char* ptm_state(double);
const char* ptm_import(const char*, int, double);
}

static int g_fail = 0, g_pass = 0;
#define CHECK(c) do { if (c) g_pass++; else { g_fail++; std::printf("GAGAL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
static bool has(const std::string& s, const char* sub) { return s.find(sub) != std::string::npos; }

static const int64_t H = 3600000LL, D = 24 * H;
static const int64_t NOW = 1800000000000LL;

int main() {
  using namespace ptm;
  // --- Prioritas ---
  Task t; t.urgency = 5; t.importance = 5; t.deadline = NOW + 2 * H;
  CHECK(autoScore(t, NOW) == 5.0 && autoLevel(t, NOW) == Level::High);
  t.deadline = 0;  // tanpa deadline: D=1 -> 2+1.5+0.3 = 3.8
  CHECK(autoScore(t, NOW) == 3.8 && autoLevel(t, NOW) == Level::Medium);
  t.urgency = 1; t.importance = 1;  // 0.4+0.3+0.3 = 1.0
  CHECK(autoScore(t, NOW) == 1.0 && autoLevel(t, NOW) == Level::Low);
  t.urgency = 3; t.importance = 3; t.deadline = NOW - D;  // terlambat D=5: 1.2+0.9+1.5=3.6
  CHECK(isOverdue(t, NOW) && autoScore(t, NOW) == 3.6);
  t.status = Status::Completed;  // selesai: tidak terlambat, auto Low
  CHECK(!isOverdue(t, NOW) && autoLevel(t, NOW) == Level::Low);
  t.status = Status::Todo; t.urgency = 5; t.importance = 5; t.deadline = NOW + 20 * D;  // 2+1.5+0.3
  CHECK(autoScore(t, NOW) == 3.8);
  CHECK(classify(4.0) == Level::High && classify(3.9) == Level::Medium && classify(2.5) == Level::Medium && classify(2.4) == Level::Low);
  CHECK(deadlineScore(NOW + 3 * D, NOW) == 4.0 && deadlineScore(NOW + 7 * D, NOW) == 3.0 && deadlineScore(NOW + 14 * D, NOW) == 2.0 && deadlineScore(NOW + 15 * D, NOW) == 1.0);
  // Mode manual tidak ditimpa
  Task m; m.urgency = 5; m.importance = 5; m.deadline = NOW + H; m.mode = PriorityMode::Manual; m.manualPriority = Level::Low;
  CHECK(effectiveLevel(m, NOW) == Level::Low && autoLevel(m, NOW) == Level::High);

  // --- CRUD ---
  TaskManager tm;
  Result r = tm.add("{\"title\":\"  Tulis laporan \",\"urgency\":5,\"importance\":4,\"deadline\":" + std::to_string(NOW + D) + "}", NOW);
  CHECK(r.ok && r.id == 1);
  r = tm.add("{\"title\":\"Belanja \\u00e9 \\ud83d\\ude00\",\"description\":\"susu\\ntelur\",\"status\":\"inprogress\"}", NOW + 1000);
  CHECK(r.ok && r.id == 2);
  r = tm.add("{\"title\":\"Rapikan arsip\",\"urgency\":1,\"importance\":1,\"priorityMode\":\"manual\",\"manualPriority\":\"high\"}", NOW + 2000);
  CHECK(r.ok && r.id == 3);
  CHECK(!tm.add("{\"title\":\"\"}", NOW).ok);
  CHECK(!tm.add("{\"title\":\"x\",\"urgency\":9}", NOW).ok);
  CHECK(!tm.add("{\"title\":\"x\",\"status\":\"bogus\"}", NOW).ok);
  CHECK(!tm.add("bukan json", NOW).ok);
  CHECK(tm.size() == 3);
  r = tm.update("{\"id\":2,\"title\":\"Belanja bulanan\",\"status\":\"completed\"}", NOW + 5000);
  CHECK(r.ok);
  std::string g = tm.get(2, NOW);
  CHECK(has(g, "Belanja bulanan") && has(g, "\"status\":\"completed\"") && has(g, "\"createdAt\":1800000001000"));
  CHECK(!tm.update("{\"id\":99,\"title\":\"x\"}", NOW).ok);
  CHECK(!tm.remove(99).ok);

  // --- Filter, cari, urut ---
  std::string q = tm.query("{\"status\":\"completed\"}", NOW);
  CHECK(has(q, "\"count\":1") && has(q, "Belanja bulanan"));
  q = tm.query("{\"priority\":\"high\"}", NOW);  // tugas 1 (auto High) + tugas 3 (manual High)
  CHECK(has(q, "\"count\":2"));
  q = tm.query("{\"search\":\"LAPORAN\"}", NOW);
  CHECK(has(q, "\"count\":1") && has(q, "Tulis laporan"));
  q = tm.query("{\"deadline\":\"none\"}", NOW);
  CHECK(has(q, "\"count\":2"));
  q = tm.query("{\"sort\":\"deadline\",\"dir\":\"asc\"}", NOW);
  CHECK(q.find("Tulis laporan") < q.find("Rapikan arsip"));  // tanpa deadline di akhir
  q = tm.query("{\"sort\":\"deadline\",\"dir\":\"desc\"}", NOW);
  CHECK(q.find("Tulis laporan") < q.find("Rapikan arsip"));
  q = tm.query("{\"sort\":\"created\",\"dir\":\"asc\"}", NOW);
  CHECK(q.find("Tulis laporan") < q.find("Belanja bulanan") && q.find("Belanja bulanan") < q.find("Rapikan arsip"));
  q = tm.query("{\"sort\":\"priority\",\"dir\":\"desc\"}", NOW);
  CHECK(q.find("Tulis laporan") < q.find("Rapikan arsip"));  // skor auto 4.8 > skor 1.0 (keduanya High)
  q = tm.query("{\"deadline\":\"overdue\"}", NOW + 3 * D);
  CHECK(has(q, "\"count\":1") && has(q, "\"overdue\":true"));
  q = tm.query("{\"deadline\":\"week\",\"dayStart\":" + std::to_string(NOW) + "}", NOW);
  CHECK(has(q, "\"count\":1"));
  CHECK(has(tm.query("{rusak", NOW), "\"ok\":false"));

  // --- Statistik ---
  std::string st = tm.stats(NOW);
  CHECK(has(st, "\"total\":3") && has(st, "\"completed\":1") && has(st, "\"completionRate\":33") && has(st, "\"overdue\":0"));

  // --- Simpan, muat, ekspor/impor ---
  std::string saved = tm.serialize(NOW);
  TaskManager tm2;
  CHECK(tm2.load(saved, NOW).ok && tm2.size() == 3);
  CHECK(tm2.serialize(NOW) == saved);
  CHECK(tm2.add("{\"title\":\"baru\"}", NOW).id == 4);
  CHECK(tm2.load("", NOW).ok && tm2.size() == 0);
  tm2.load(saved, NOW);
  // Impor rusak tidak mengubah data
  std::string before = tm2.serialize(NOW);
  const char* bad[] = {
      "{\"tasks\":[{\"title\":\"ok\"},{\"title\":123}]}", "{\"tasks\":\"x\"}", "[1,2]", "{\"tasks\":[{\"title\":\"a\",\"id\":1},{\"title\":\"b\",\"id\":1}]}",
      "{\"tasks\":[{\"title\":\"a\",\"deadline\":\"besok\"}]}", "{\"tasks\":[{\"title\":\"a\"}]} sisa", "", "\"teks\"", "{\"tasks\":[{\"title\":\"a\",\"urgency\":2.5}]}"};
  for (const char* b : bad) {
    CHECK(!tm2.importData(b, 1, NOW).ok);
    CHECK(!tm2.importData(b, 2, NOW).ok);
    if (std::strlen(b) > 0) CHECK(!tm2.load(b, NOW).ok);  // load("") sengaja = daftar kosong
  }
  CHECK(tm2.serialize(NOW) == before);
  CHECK(tm2.importData(saved, 0, NOW).ok && tm2.serialize(NOW) == before);  // validasi saja
  CHECK(tm2.importData("[{\"title\":\"x\"},{\"title\":\"y\"}]", 2, NOW).ok && tm2.size() == 5);  // gabung
  CHECK(tm2.importData("[{\"title\":\"x\"}]", 1, NOW).ok && tm2.size() == 1);  // ganti
  CHECK(tm2.remove(1).ok && tm2.size() == 0);


  // --- Pengingat deadline & kalender ---
  {
    auto mk = [&](TaskManager& m, int64_t dl, const char* st) {
      std::string j = "{\"title\":\"t\",\"status\":\"" + std::string(st) + "\",\"deadline\":" + std::to_string(dl) + "}";
      return m.add(j, NOW).ok;
    };
    TaskManager c;
    CHECK(mk(c, NOW + 2 * H, "todo"));          // A: 15 Jan 10:00Z, dekat
    CHECK(mk(c, NOW + 30 * H, "todo"));         // B: 16 Jan 14:00Z, di luar 24 jam
    CHECK(mk(c, NOW - D, "todo"));              // C: 14 Jan, terlambat
    CHECK(mk(c, NOW + 3 * H, "completed"));     // D: selesai, tidak diingatkan
    CHECK(c.add("{\"title\":\"tanpa deadline\"}", NOW).ok);
    CHECK(mk(c, NOW + 3 * D, "inprogress"));    // F: 18 Jan
    CHECK(mk(c, NOW + 16 * H, "todo"));         // G: 16 Jan 00:00Z
    std::string u = c.upcoming(NOW, 24 * H);
    CHECK(has(u, "\"count\":2") && has(u, "\"overdue\":1"));  // A dan G
    CHECK(u.find("\"id\":1") < u.find("\"id\":7"));            // urut deadline
    CHECK(!has(u, "\"id\":4"));                                 // yang selesai tidak ikut
    CHECK(has(c.upcoming(NOW, 48 * H), "\"count\":3"));         // jendela diperlebar: A, B, G
    std::string cal = c.calendar(2027, 1, 0, NOW);
    CHECK(has(cal, "\"daysInMonth\":31") && has(cal, "\"firstWeekday\":5"));
    CHECK(has(cal, "{\"day\":14,\"count\":1,\"pending\":1,\"overdue\":1}"));
    CHECK(has(cal, "{\"day\":15,\"count\":2,\"pending\":1,\"overdue\":0}"));  // A + D(selesai)
    CHECK(has(cal, "{\"day\":16,\"count\":2,\"pending\":2,\"overdue\":0}"));  // B + G
    CHECK(has(cal, "{\"day\":18,\"count\":1"));
    CHECK(!has(cal, "\"day\":17"));
    // zona waktu: UTC-8 menggeser G (16 Jan 00:00Z) ke 15 Jan
    std::string calW = c.calendar(2027, 1, -480, NOW);
    CHECK(has(calW, "{\"day\":15,\"count\":3,\"pending\":2,\"overdue\":0}"));
    CHECK(has(c.calendar(2027, 2, 0, NOW), "\"daysInMonth\":28") && has(c.calendar(2028, 2, 0, NOW), "\"daysInMonth\":29"));
    CHECK(has(c.calendar(2027, 13, 0, NOW), "\"ok\":false") && has(c.calendar(1900, 1, 0, NOW), "\"ok\":false"));
    CHECK(has(c.calendar(2027, 12, 0, NOW), "\"ok\":true"));  // lintas tahun
  }

  // --- Lapisan C (API WASM) ---
  CHECK(has(ptm_load("", NOW), "\"ok\":true"));
  CHECK(has(ptm_add("{\"title\":\"via api\"}", NOW), "\"id\":1"));
  CHECK(has(ptm_query("", NOW), "via api"));
  CHECK(has(ptm_state(NOW), "\"nextId\":2"));
  CHECK(has(ptm_import("{", 0, NOW), "\"ok\":false"));
  CHECK(has(ptm_load(nullptr, NOW), "\"ok\":true"));

  std::printf("%d lulus, %d gagal\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
