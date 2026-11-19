// Lapisan ekspor C untuk WebAssembly (Emscripten). Semua logika ada di TaskManager.
#include <string>

#include "task_manager.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define PTM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define PTM_EXPORT
#endif

namespace {
ptm::TaskManager g_manager;
std::string g_out;
const char* ret(std::string s) { g_out = std::move(s); return g_out.c_str(); }
int64_t T(double now) { return (int64_t)now; }
const char* S(const char* p) { return p ? p : ""; }
}  // namespace

extern "C" {
PTM_EXPORT const char* ptm_load(const char* json, double now) { return ret(ptm::resultToJson(g_manager.load(S(json), T(now)))); }
PTM_EXPORT const char* ptm_add(const char* json, double now) { return ret(ptm::resultToJson(g_manager.add(S(json), T(now)))); }
PTM_EXPORT const char* ptm_update(const char* json, double now) { return ret(ptm::resultToJson(g_manager.update(S(json), T(now)))); }
PTM_EXPORT const char* ptm_delete(int id) { return ret(ptm::resultToJson(g_manager.remove(id))); }
PTM_EXPORT const char* ptm_get(int id, double now) { return ret(g_manager.get(id, T(now))); }
PTM_EXPORT const char* ptm_query(const char* filter, double now) { return ret(g_manager.query(S(filter), T(now))); }
PTM_EXPORT const char* ptm_stats(double now) { return ret(g_manager.stats(T(now))); }
PTM_EXPORT const char* ptm_state(double now) { return ret(g_manager.serialize(T(now))); }
PTM_EXPORT const char* ptm_export(double now) { return ret(g_manager.serialize(T(now))); }
PTM_EXPORT const char* ptm_import(const char* json, int mode, double now) { return ret(ptm::resultToJson(g_manager.importData(S(json), mode, T(now)))); }
}
