//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// chrono
//
// units.hpp      duration_t/fduration_t, the unit enum, duration<Rep,Period>, timespec arithmetic
// calendar.hpp   civil_from_days/days_from_civil, weekday, iso week, time_of_day, year_month_day
// civil.hpp      the integer broken-down time, tz-offset conversion, the mktime inverse
// clock.hpp      system_clock<C>, time_point, the never-throwing readers, deadlines, scoped timers
// format.hpp     iso8601/rfc3339/rfc2822/strftime/duration writers + formatter<> specialisations
// parse.hpp      the strict parsers and the duration grammar
// epochs.hpp     unix <-> ntp / filetime / gps / julian / dos
// posix.hpp      tm_t, gmtime_r, timegm, mktime, strftime, difftime, usleep, sysconf

#include "chrono/calendar.hpp"
#include "chrono/civil.hpp"
#include "chrono/clock.hpp"
#include "chrono/epochs.hpp"
#include "chrono/format.hpp"
#include "chrono/parse.hpp"
#include "chrono/posix.hpp"
#include "chrono/units.hpp"
