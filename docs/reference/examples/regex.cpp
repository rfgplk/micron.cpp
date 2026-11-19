#include <micron/regex.hpp>

int main()
{
  micron::regex expression("b+");
  micron::rmatch result = expression.search("aabbbcc");
  return expression.valid() && result.has_match() && result.group_start(0) == 2
         && result.group_end(0) == 5 ? 0 : 1;
}
