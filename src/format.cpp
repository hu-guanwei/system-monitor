#include "format.h"

#include <iomanip>
#include <sstream>
#include <string>

using std::ostringstream;
using std::setfill;
using std::setw;
using std::string;
using std::to_string;

string padLeadingZero(int num, int n = 2) {
  ostringstream ss;
  ss << setw(n) << setfill('0') << num;
  return ss.str();
}

// Complete this helper function
// INPUT: Long int measuring seconds
// OUTPUT: HH:MM:SS
string Format::ElapsedTime(long seconds) {
  int hours = seconds / 3600;
  int minutes = (seconds - hours * 3600) / 60;
  int seconds_left = (seconds - hours * 3600) % 60;
  return padLeadingZero(hours) + ':' + padLeadingZero(minutes) + ':' +
         padLeadingZero(seconds_left);
}
