#include "json.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ptm {
namespace json {
namespace {

struct Parser {
  const std::string& t;
  size_t i = 0;
  int depth = 0;
  std::string err;
  explicit Parser(const std::string& s) : t(s) {}

  void ws() { while (i < t.size() && (t[i] == ' ' || t[i] == '\t' || t[i] == '\n' || t[i] == '\r')) i++; }
  bool fail(const char* m) {
    if (err.empty()) err = std::string(m) + " (posisi " + std::to_string(i) + ")";
    return false;
  }
  bool lit(const char* w) {
    size_t n = std::strlen(w);
    if (t.compare(i, n, w) == 0) { i += n; return true; }
    return false;
  }
  bool hex4(unsigned& cp) {
    if (i + 4 > t.size()) return false;
    cp = 0;
    for (int k = 0; k < 4; k++) {
      char c = t[i++];
      cp <<= 4;
      if (c >= '0' && c <= '9') cp |= c - '0';
      else if (c >= 'a' && c <= 'f') cp |= c - 'a' + 10;
      else if (c >= 'A' && c <= 'F') cp |= c - 'A' + 10;
      else return false;
    }
    return true;
  }
  static void utf8(std::string& o, unsigned cp) {
    if (cp < 0x80) o += (char)cp;
    else if (cp < 0x800) { o += (char)(0xC0 | (cp >> 6)); o += (char)(0x80 | (cp & 0x3F)); }
    else if (cp < 0x10000) { o += (char)(0xE0 | (cp >> 12)); o += (char)(0x80 | ((cp >> 6) & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); }
    else { o += (char)(0xF0 | (cp >> 18)); o += (char)(0x80 | ((cp >> 12) & 0x3F)); o += (char)(0x80 | ((cp >> 6) & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); }
  }
  bool str(std::string& o) {
    i++;  // lewati tanda kutip pembuka
    while (i < t.size()) {
      unsigned char c = (unsigned char)t[i++];
      if (c == '"') return true;
      if (c < 0x20) return fail("karakter kontrol di dalam teks");
      if (c != '\\') { o += (char)c; continue; }
      if (i >= t.size()) break;
      char e = t[i++];
      switch (e) {
        case '"': o += '"'; break;
        case '\\': o += '\\'; break;
        case '/': o += '/'; break;
        case 'b': o += '\b'; break;
        case 'f': o += '\f'; break;
        case 'n': o += '\n'; break;
        case 'r': o += '\r'; break;
        case 't': o += '\t'; break;
        case 'u': {
          unsigned cp;
          if (!hex4(cp)) return fail("escape \\u tidak valid");
          if (cp >= 0xD800 && cp < 0xDC00) {
            unsigned lo;
            if (i + 1 < t.size() && t[i] == '\\' && t[i + 1] == 'u') {
              i += 2;
              if (!hex4(lo) || lo < 0xDC00 || lo > 0xDFFF) return fail("surrogate tidak valid");
              cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
            } else return fail("surrogate tidak valid");
          } else if (cp >= 0xDC00 && cp <= 0xDFFF) return fail("surrogate tidak valid");
          utf8(o, cp);
          break;
        }
        default: return fail("escape tidak dikenal");
      }
    }
    return fail("teks tidak ditutup");
  }
  bool num(Value& v) {
    size_t s = i;
    while (i < t.size() && (std::isdigit((unsigned char)t[i]) || t[i] == '-' || t[i] == '+' || t[i] == '.' || t[i] == 'e' || t[i] == 'E')) i++;
    std::string x = t.substr(s, i - s);
    if (x.empty()) return fail("token tidak dikenal");
    char* end = nullptr;
    v.n = std::strtod(x.c_str(), &end);
    if (*end != '\0' || !std::isfinite(v.n)) return fail("angka tidak valid");
    v.type = Value::Number;
    return true;
  }
  bool arr(Value& v) {
    if (++depth > 64) return fail("struktur terlalu dalam");
    v.type = Value::Array;
    i++;
    ws();
    if (i < t.size() && t[i] == ']') { i++; depth--; return true; }
    while (true) {
      Value e;
      if (!val(e)) return false;
      v.a.push_back(std::move(e));
      ws();
      if (i < t.size() && t[i] == ',') { i++; continue; }
      if (i < t.size() && t[i] == ']') { i++; depth--; return true; }
      return fail("tanda ',' atau ']' diharapkan");
    }
  }
  bool obj(Value& v) {
    if (++depth > 64) return fail("struktur terlalu dalam");
    v.type = Value::Object;
    i++;
    ws();
    if (i < t.size() && t[i] == '}') { i++; depth--; return true; }
    while (true) {
      ws();
      if (i >= t.size() || t[i] != '"') return fail("nama properti diharapkan");
      std::string k;
      if (!str(k)) return false;
      ws();
      if (i >= t.size() || t[i] != ':') return fail("tanda ':' diharapkan");
      i++;
      Value e;
      if (!val(e)) return false;
      v.o[k] = std::move(e);
      ws();
      if (i < t.size() && t[i] == ',') { i++; continue; }
      if (i < t.size() && t[i] == '}') { i++; depth--; return true; }
      return fail("tanda ',' atau '}' diharapkan");
    }
  }
  bool val(Value& v) {
    ws();
    if (i >= t.size()) return fail("data berakhir tiba-tiba");
    char c = t[i];
    if (c == '{') return obj(v);
    if (c == '[') return arr(v);
    if (c == '"') { v.type = Value::String; return str(v.s); }
    if (c == 't') { if (lit("true")) { v.type = Value::Bool; v.b = true; return true; } return fail("token tidak dikenal"); }
    if (c == 'f') { if (lit("false")) { v.type = Value::Bool; v.b = false; return true; } return fail("token tidak dikenal"); }
    if (c == 'n') { if (lit("null")) { v.type = Value::Null; return true; } return fail("token tidak dikenal"); }
    return num(v);
  }
};

}  // namespace

bool parse(const std::string& text, Value& out, std::string& err) {
  Parser p(text);
  Value v;
  if (!p.val(v)) { err = p.err; return false; }
  p.ws();
  if (p.i != text.size()) { err = "data tambahan setelah JSON (posisi " + std::to_string(p.i) + ")"; return false; }
  out = std::move(v);
  return true;
}

std::string escape(const std::string& s) {
  std::string o;
  o.reserve(s.size() + 8);
  for (unsigned char c : s) {
    switch (c) {
      case '"': o += "\\\""; break;
      case '\\': o += "\\\\"; break;
      case '\n': o += "\\n"; break;
      case '\r': o += "\\r"; break;
      case '\t': o += "\\t"; break;
      default:
        if (c < 0x20) { char b[8]; std::snprintf(b, sizeof b, "\\u%04x", c); o += b; }
        else o += (char)c;
    }
  }
  return o;
}

}  // namespace json
}  // namespace ptm
