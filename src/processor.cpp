#include "processor.h"

#include <string>

#include "linux_parser.h"

using LinuxParser::ActiveJiffies;
using LinuxParser::IdleJiffies;
using LinuxParser::Jiffies;
using std::string;

// Return the aggregate CPU utilization
float Processor::Utilization() {
  // int idle_HZ = IdleJiffies();
  int nonidle_HZ = ActiveJiffies();
  int total_HZ = Jiffies();
  return nonidle_HZ / (total_HZ + 0.0);
}