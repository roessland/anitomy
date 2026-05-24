#include <cstdlib>
#include <cstring>
#include <string>

#include <anitomy.hpp>
#include <anitomy/detail/cli/util.hpp>
#include <anitomy/detail/json.hpp>
#include <anitomy/version.hpp>

// C ABI over the Anitomy parsing pipeline, mirroring the CLI's behaviour
// minus presentation. Callers pass a UTF-8 filename, receive a UTF-8 JSON
// string allocated with malloc, and must call anitomy_free to release it.

using anitomy::Options;
using anitomy::detail::Parser;
using anitomy::detail::to_json;
using anitomy::detail::Tokenizer;
using anitomy::detail::json::serialize;

namespace {

char* duplicate(const std::string& source) noexcept {
  char* out = static_cast<char*>(std::malloc(source.size() + 1));
  if (!out) return nullptr;
  std::memcpy(out, source.data(), source.size() + 1);
  return out;
}

}  // namespace

extern "C" {

const char* anitomy_version(void) noexcept {
  static const std::string v = std::string{anitomy::version()};
  return v.c_str();
}

char* anitomy_parse(const char* input, int pretty) noexcept {
  if (!input) return nullptr;
  Tokenizer tokenizer{input};
  const Options options;
  tokenizer.tokenize(options);
  Parser parser{tokenizer.tokens()};
  parser.parse(options);
  return duplicate(serialize(to_json(parser.elements()), pretty != 0));
}

char* anitomy_parse_tokens(const char* input, int pretty, int verbose) noexcept {
  if (!input) return nullptr;
  Tokenizer tokenizer{input};
  const Options options;
  tokenizer.tokenize(options);
  Parser parser{tokenizer.tokens()};
  parser.parse(options);
  return duplicate(serialize(to_json(parser.tokens(), verbose != 0), pretty != 0));
}

void anitomy_free(char* ptr) noexcept {
  std::free(ptr);
}

}  // extern "C"
