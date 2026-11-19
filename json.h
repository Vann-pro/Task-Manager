#pragma once
// Parser dan helper JSON minimal (tanpa dependensi eksternal).
#include <map>
#include <string>
#include <vector>

namespace ptm {
namespace json {

struct Value {
  enum Type { Null, Bool, Number, String, Array, Object };
  Type type = Null;
  bool b = false;
  double n = 0;
  std::string s;
  std::vector<Value> a;
  std::map<std::string, Value> o;

  bool isObj() const { return type == Object; }
  const Value* get(const std::string& key) const {
    auto it = o.find(key);
    return it == o.end() ? nullptr : &it->second;
  }
};

// Mengembalikan false dan mengisi err jika JSON tidak valid.
bool parse(const std::string& text, Value& out, std::string& err);
std::string escape(const std::string& s);

}  // namespace json
}  // namespace ptm
