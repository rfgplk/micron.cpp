#include <micron/chrono/calendar.hpp>

int main()
{
  return micron::chrono::is_leap(2024) && micron::chrono::days_in_month(2024, 2) == 29 ? 0 : 1;
}
