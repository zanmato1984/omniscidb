/*
 * Copyright 2017 MapD Technologies, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ExtractFromTime.h"
#include "../Shared/funcannotations.h"

#ifndef __CUDACC__
#include <cstdlib>  // abort()
#endif

extern "C" NEVER_INLINE DEVICE int64_t extract_hour(const int64_t lcltime) {
  int64_t days = lcltime / kSecsPerDay - kEpochAdjustedDays;
  int64_t rem = lcltime % kSecsPerDay;
  if (rem < 0) {
    rem += kSecsPerDay;
    --days;
  }
  return static_cast<int32_t>(rem / kSecPerHour);
}

extern "C" NEVER_INLINE DEVICE int64_t extract_minute(const int64_t lcltime) {
  int64_t days = lcltime / kSecsPerDay - kEpochAdjustedDays;
  int64_t rem = lcltime % kSecsPerDay;
  if (rem < 0) {
    rem += kSecsPerDay;
    --days;
  }
  rem %= kSecPerHour;
  return rem / kSecsPerMin;
}

extern "C" NEVER_INLINE DEVICE int64_t extract_second(const int64_t lcltime) {
  return lcltime % kSecsPerMin;
}

extern "C" NEVER_INLINE DEVICE int64_t extract_millisecond(const int64_t lcltime) {
  return lcltime % (kSecsPerMin * kMilliSecsPerSec);
}

extern "C" NEVER_INLINE DEVICE int64_t extract_microsecond(const int64_t lcltime) {
  return lcltime % (kSecsPerMin * kMicroSecsPerSec);
}

extern "C" NEVER_INLINE DEVICE int64_t extract_nanosecond(const int64_t lcltime) {
  return lcltime % (kSecsPerMin * kNanoSecsPerSec);
}

extern "C" NEVER_INLINE DEVICE int64_t extract_dow(const int64_t lcltime) {
  int64_t days = lcltime / kSecsPerDay - kEpochAdjustedDays;
  int64_t rem = lcltime % kSecsPerDay;
  if (rem < 0) {
    rem += kSecsPerDay;
    --days;
  }

  int64_t weekday = (kEpochAdjustedWDay + days) % kDaysPerWeek;
  if (weekday < 0) {
    weekday += kDaysPerWeek;
  }
  return weekday;
}

extern "C" NEVER_INLINE DEVICE int64_t extract_quarterday(const int64_t lcltime) {
  const int64_t quarterdays = lcltime / kSecsPerQuarterDay;
  return (quarterdays % 4) + 1;
}

DEVICE int32_t extract_month_fast(const int64_t lcltime) {
  STATIC_QUAL const uint32_t cumulative_month_epoch_starts[kMonsPerYear] = {0,
                                                                            2678400,
                                                                            5270400,
                                                                            7948800,
                                                                            10540800,
                                                                            13219200,
                                                                            15897600,
                                                                            18489600,
                                                                            21168000,
                                                                            23760000,
                                                                            26438400,
                                                                            29116800};
  uint32_t seconds_march_1900 = lcltime + kEpochOffsetYear1900 - kSecsJanToMar1900;
  uint32_t seconds_past_4year_period = seconds_march_1900 % kSecondsPer4YearCycle;
  uint32_t year_seconds_past_4year_period =
      (seconds_past_4year_period / kSecondsPerNonLeapYear) * kSecondsPerNonLeapYear;
  if (seconds_past_4year_period >=
      kSecondsPer4YearCycle - kUSecsPerDay) {  // if we are in Feb 29th
    year_seconds_past_4year_period -= kSecondsPerNonLeapYear;
  }
  uint32_t seconds_past_march =
      seconds_past_4year_period - year_seconds_past_4year_period;
  uint32_t month =
      seconds_past_march / (30 * kUSecsPerDay);  // Will make the correct month either be
                                                 // the guessed month or month before
  month = month <= 11 ? month : 11;
  if (cumulative_month_epoch_starts[month] > seconds_past_march) {
    month--;
  }
  return (month + 2) % 12 + 1;
}

DEVICE int32_t extract_quarter_fast(const int64_t lcltime) {
  STATIC_QUAL const uint32_t cumulative_quarter_epoch_starts[4] = {
      0, 7776000, 15638400, 23587200};
  STATIC_QUAL const uint32_t cumulative_quarter_epoch_starts_leap_year[4] = {
      0, 7862400, 15724800, 23673600};
  uint32_t seconds_1900 = lcltime + kEpochOffsetYear1900;
  uint32_t leap_years = (seconds_1900 - kSecsJanToMar1900) / kSecondsPer4YearCycle;
  uint32_t year = (seconds_1900 - leap_years * kSecsPerDay) / kSecondsPerNonLeapYear;
  uint32_t base_year_leap_years = (year - 1) / 4;
  uint32_t base_year_seconds =
      year * kSecondsPerNonLeapYear + base_year_leap_years * kUSecsPerDay;
  bool is_leap_year = year % 4 == 0 && year != 0;
  const uint32_t* quarter_offsets = is_leap_year
                                        ? cumulative_quarter_epoch_starts_leap_year
                                        : cumulative_quarter_epoch_starts;
  uint32_t partial_year_seconds = seconds_1900 % base_year_seconds;
  uint32_t quarter = partial_year_seconds / (90 * kUSecsPerDay);
  quarter = quarter <= 3 ? quarter : 3;
  if (quarter_offsets[quarter] > partial_year_seconds) {
    quarter--;
  }
  return quarter + 1;
}

DEVICE int32_t extract_year_fast(const int64_t lcltime) {
  const uint32_t seconds_1900 = lcltime + kEpochOffsetYear1900;
  const uint32_t leap_years = (seconds_1900 - kSecsJanToMar1900) / kSecondsPer4YearCycle;
  const uint32_t year =
      (seconds_1900 - leap_years * kUSecsPerDay) / kSecondsPerNonLeapYear + 1900;
  return year;
}

DEVICE tm gmtime_r_newlib(const int64_t lcltime, tm& res) {
  const int32_t month_lengths[2][kMonsPerYear] = {
      {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
      {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};
  int64_t days, rem;
  int32_t year, month, yearday, weekday;
  int32_t years400, years100, years4, remainingyears;
  int32_t yearleap;
  const int32_t* ip;

  days = lcltime / kSecsPerDay - kEpochAdjustedDays;
  rem = lcltime % kSecsPerDay;
  if (rem < 0) {
    rem += kSecsPerDay;
    --days;
  }

  /* compute hour, min, and sec */
  res.tm_hour = static_cast<int32_t>(rem / kSecPerHour);
  rem %= kSecPerHour;
  res.tm_min = static_cast<int32_t>(rem / kSecsPerMin);
  res.tm_sec = static_cast<int32_t>(rem % kSecsPerMin);

  /* compute day of week */
  if ((weekday = ((kEpochAdjustedWDay + days) % kDaysPerWeek)) < 0) {
    weekday += kDaysPerWeek;
  }
  res.tm_wday = weekday;

  /* compute year & day of year */
  years400 = static_cast<int32_t>(days / kDaysPer400Years);
  days -= years400 * kDaysPer400Years;
  /* simplify by making the values positive */
  if (days < 0) {
    days += kDaysPer400Years;
    --years400;
  }

  years100 = static_cast<int32_t>(days / kDaysPer100Years);
  if (years100 == 4) { /* required for proper day of year calculation */
    --years100;
  }
  days -= years100 * kDaysPer100Years;
  years4 = days / kDaysPer4Years;
  days -= years4 * kDaysPer4Years;
  remainingyears = days / kDaysPerYear;
  if (remainingyears == 4) { /* required for proper day of year calculation */
    --remainingyears;
  }
  days -= remainingyears * kDaysPerYear;

  year =
      kEpochAdjustedYears + years400 * 400 + years100 * 100 + years4 * 4 + remainingyears;

  /* If remainingyears is zero, it means that the years were completely
   * "consumed" by modulo calculations by 400, 100 and 4, so the year is:
   * 1. a multiple of 4, but not a multiple of 100 or 400 - it's a leap year,
   * 2. a multiple of 4 and 100, but not a multiple of 400 - it's not a leap
   * year,
   * 3. a multiple of 4, 100 and 400 - it's a leap year.
   * If years4 is non-zero, it means that the year is not a multiple of 100 or
   * 400 (case 1), so it's a leap year. If years100 is zero (and years4 is zero
   * - due to short-circuiting), it means that the year is a multiple of 400
   * (case 3), so it's also a leap year. */
  yearleap = remainingyears == 0 && (years4 != 0 || years100 == 0);

  /* adjust back to 1st January */
  yearday = days + kDaysInJanuary + kDaysInFebruary + yearleap;
  if (yearday >= kDaysPerYear + yearleap) {
    yearday -= kDaysPerYear + yearleap;
    ++year;
  }
  res.tm_yday = yearday;
  res.tm_year = year - kYearBase;

  /* Because "days" is the number of days since 1st March, the additional leap
   * day (29th of February) is the last possible day, so it doesn't matter much
   * whether the year is actually leap or not. */
  ip = month_lengths[1];
  month = 2;
  while (days >= ip[month]) {
    days -= ip[month];
    if (++month >= kMonsPerYear) {
      month = 0;
    }
  }
  res.tm_mon = month;
  res.tm_mday = days + 1;

  res.tm_isdst = 0;

  return res;
}

extern "C" NEVER_INLINE DEVICE int64_t extract_epoch(const int64_t timeval) {
  return timeval;
}

extern "C" NEVER_INLINE DEVICE int64_t extract_dateepoch(const int64_t timeval) {
  return (timeval < 0 && kSecsPerDay != 0) ? ((timeval / kSecsPerDay) - 1) * kSecsPerDay
                                           : (timeval / kSecsPerDay) * kSecsPerDay;
}

extern "C" NEVER_INLINE DEVICE int64_t extract_isodow(const int64_t timeval) {
  int64_t dow = extract_dow(timeval);
  return (dow == 0 ? 7 : dow);
}

extern "C" NEVER_INLINE DEVICE int64_t extract_day(const int64_t timeval) {
  tm tm_struct;
  gmtime_r_newlib(timeval, tm_struct);
  return tm_struct.tm_mday;
}

extern "C" NEVER_INLINE DEVICE int64_t extract_day_of_week(const int64_t timeval) {
  tm tm_struct;
  gmtime_r_newlib(timeval, tm_struct);
  return tm_struct.tm_yday + 1;
}

extern "C" NEVER_INLINE DEVICE int64_t extract_week(const int64_t timeval) {
  tm tm_struct;
  gmtime_r_newlib(timeval, tm_struct);
  int32_t doy = tm_struct.tm_yday;         // numbered from 0
  int32_t dow = extract_dow(timeval) + 1;  // use Sunday 1 - Saturday 7
  int32_t week = (doy / 7) + 1;
  // now adjust for offset at start of year
  //      S M T W T F S
  // doy      0 1 2 3 4
  // doy  5 6
  // mod  5 6 0 1 2 3 4
  // dow  1 2 3 4 5 6 7
  // week 2 2 1 1 1 1 1
  if (dow > (doy % 7)) {
    return week;
  }
  return week + 1;
}

extern "C" NEVER_INLINE DEVICE int64_t extract_month(const int64_t timeval) {
  if (timeval >= 0L && timeval <= UINT32_MAX - kEpochOffsetYear1900) {
    return extract_month_fast(timeval);
  }

  tm tm_struct;
  gmtime_r_newlib(timeval, tm_struct);
  return tm_struct.tm_mon + 1;
}

extern "C" NEVER_INLINE DEVICE int64_t extract_quarter(const int64_t timeval) {
  if (timeval >= 0L && timeval <= UINT32_MAX - kEpochOffsetYear1900) {
    return extract_quarter_fast(timeval);
  }

  tm tm_struct;
  gmtime_r_newlib(timeval, tm_struct);
  return (tm_struct.tm_mon) / 3 + 1;
}

extern "C" NEVER_INLINE DEVICE int64_t extract_year(const int64_t timeval) {
  if (timeval >= 0L && timeval <= UINT32_MAX - kEpochOffsetYear1900) {
    return extract_year_fast(timeval);
  }

  tm tm_struct;
  gmtime_r_newlib(timeval, tm_struct);
  return 1900 + tm_struct.tm_year;
}

/*
 * @brief support the SQL EXTRACT function
 */
DEVICE int64_t ExtractFromTime(ExtractField field, const int64_t timeval) {
  switch (field) {
    case kEPOCH:
      return extract_epoch(timeval);
    case kDATEEPOCH:
      return extract_dateepoch(timeval);
    case kQUARTERDAY:
      return extract_quarterday(timeval);
    case kHOUR:
      return extract_hour(timeval);
    case kMINUTE:
      return extract_minute(timeval);
    case kSECOND:
      return extract_second(timeval);
    case kMILLISECOND:
      return extract_millisecond(timeval);
    case kMICROSECOND:
      return extract_microsecond(timeval);
    case kNANOSECOND:
      return extract_nanosecond(timeval);
    case kDOW:
      return extract_dow(timeval);
    case kISODOW:
      return extract_isodow(timeval);
    case kDAY:
      return extract_day(timeval);
    case kWEEK:
      return extract_week(timeval);
    case kDOY:
      return extract_day_of_week(timeval);
    case kMONTH:
      return extract_month(timeval);
    case kQUARTER:
      return extract_quarter(timeval);
    case kYEAR:
      return extract_year(timeval);
  }

#ifdef __CUDACC__
  return -1;
#else
  abort();
#endif
}
