#include <cstdlib>
#include <cstring>
#include <string>

#include <anitomy.hpp>
#include <anitomy/detail/cli/util.hpp>
#include <anitomy/detail/format.hpp>
#include <anitomy/detail/json.hpp>

// C ABI over the Anitomy parsing pipeline. Callers pass a UTF-8 filename,
// receive a UTF-8 JSON string allocated with malloc, and must call
// anitomy_free to release it.
//
// Options are a bitmask matching anitomy::Options field order:
//   bit 0 parse_episode           bit 5 parse_release_group
//   bit 1 parse_episode_title     bit 6 parse_season
//   bit 2 parse_file_checksum     bit 7 parse_title
//   bit 3 parse_file_extension    bit 8 parse_video_resolution
//   bit 4 parse_part              bit 9 parse_year
// Pass 0x3FF to enable everything (the default).

using anitomy::Options;
using anitomy::detail::Parser;
using anitomy::detail::to_json;
using anitomy::detail::Tokenizer;
using anitomy::detail::json::serialize;
using anitomy::detail::json::Value;

namespace {

char* duplicate(const std::string& source) noexcept {
  char* out = static_cast<char*>(std::malloc(source.size() + 1));
  if (!out) return nullptr;
  std::memcpy(out, source.data(), source.size() + 1);
  return out;
}

Options optionsFromMask(int mask) noexcept {
  Options o;
  o.parse_episode          = (mask & (1 << 0)) != 0;
  o.parse_episode_title    = (mask & (1 << 1)) != 0;
  o.parse_file_checksum    = (mask & (1 << 2)) != 0;
  o.parse_file_extension   = (mask & (1 << 3)) != 0;
  o.parse_part             = (mask & (1 << 4)) != 0;
  o.parse_release_group    = (mask & (1 << 5)) != 0;
  o.parse_season           = (mask & (1 << 6)) != 0;
  o.parse_title            = (mask & (1 << 7)) != 0;
  o.parse_video_resolution = (mask & (1 << 8)) != 0;
  o.parse_year             = (mask & (1 << 9)) != 0;
  return o;
}

std::vector<anitomy::Element> runParser(const char* input, int mask) {
  const Options options = optionsFromMask(mask);
  Tokenizer tokenizer{input};
  tokenizer.tokenize(options);
  Parser parser{tokenizer.tokens()};
  parser.parse(options);
  return std::vector<anitomy::Element>{parser.elements().begin(), parser.elements().end()};
}

}  // namespace

extern "C" {

// Returns map-shaped JSON: {kind: value | [values...]}.
// Key order follows insertion order of the first occurrence of each kind.
char* anitomy_parse(const char* input, int options_mask, int pretty) noexcept {
  if (!input) return nullptr;
  auto elements = runParser(input, options_mask);
  return duplicate(serialize(to_json(elements), pretty != 0));
}

// Returns array-shaped JSON: [{"kind": "...", "value": "...", "position": N}, ...].
// Preserves element order (sorted by position, as the parser emits them).
char* anitomy_parse_elements(const char* input, int options_mask) noexcept {
  if (!input) return nullptr;
  const auto elements = runParser(input, options_mask);

  Value arr{Value::array_t{}};
  for (const auto& e : elements) {
    Value item{Value::object_t{}};
    item.as_object().emplace("kind", Value{std::string{anitomy::detail::to_string(e.kind)}});
    item.as_object().emplace("value", Value{std::string{e.value}});
    item.as_object().emplace("position", Value{static_cast<int>(e.position)});
    arr.as_array().push_back(std::move(item));
  }
  return duplicate(serialize(arr, false));
}

void anitomy_free(char* ptr) noexcept {
  std::free(ptr);
}

}  // extern "C"
