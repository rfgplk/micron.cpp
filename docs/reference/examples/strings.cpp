#include <micron/string/strings.hpp>

int main()
{
  micron::string text("hello");
  text += ", micron";
  return text.size() == 13 && text.find(',') == 5 ? 0 : 1;
}
