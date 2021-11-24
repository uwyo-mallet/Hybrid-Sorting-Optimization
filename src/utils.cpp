#include "utils.hpp"

#include <sstream>
#include <string>

/** Remove leading and trailing whitespace. */
std::string &trim(std::string &s)
{
  const char *t = " \t\n\r\f\v";
  s = s.erase(s.find_last_not_of(t) + 1);
  s = s.erase(0, s.find_first_not_of(t));
  return s;
}

std::vector<std::string> parse_comma_sep_args(const std::string &args)
{
  std::vector<std::string> tokens;
  std::stringstream stream(args);
  std::string element;

  while (std::getline(stream, element, ','))
  {
    tokens.push_back(trim(element));
  }

  return tokens;
}
