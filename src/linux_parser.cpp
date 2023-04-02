#include "linux_parser.h"

#include <dirent.h>
#include <unistd.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using std::ifstream;
using std::istringstream;
using std::map;
using std::stof;
using std::string;
using std::to_string;
using std::vector;
using std::filesystem::directory_iterator;
using std::filesystem::path;

// Generic function to match value by key in text file
template <typename T>
T findValByKey(const string &fname, const string &searchKey) {
  string key, line;
  T val;

  ifstream fs{fname};
  if (fs.is_open()) {
    while (getline(fs, line)) {
      istringstream lineStream{line};
      lineStream >> key >> val;
      if (key == searchKey) {
        fs.close();
        return val;
      }
    }
    fs.close();
  }
  return val;
}

// An example of how to read data from the filesystem
string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

// An example of how to read data from the filesystem
string LinuxParser::Kernel() {
  string os, version, kernel;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

vector<int> LinuxParser::Pids() {
  vector<int> pids;
  for (auto p : directory_iterator(kProcDirectory)) {
    std::string dir = p.path().filename();
    if (std::all_of(dir.begin(), dir.end(), isdigit)) {
      pids.push_back(stoi(dir));
    }
  }
  return pids;
}

// Read and return the system memory utilization
float LinuxParser::MemoryUtilization() {
  string str;
  int memTotal;
  int memFree;
  ifstream memFile;
  memFile.open(kProcDirectory + kMeminfoFilename);

  while (memFile >> str) {
    if (str == "MemTotal:") {
      memFile >> str;
      memTotal = stoi(str);
    } else if (str == "MemFree:") {
      memFile >> str;
      memFree = stoi(str);
    }
  }

  memFile.close();
  return 1.0 - memFree / (memTotal + 0.0);
}

// Read and return the system uptime
long LinuxParser::UpTime() {
  string str;
  long uptime = 0;
  ifstream uptimeFile;
  uptimeFile.open(kProcDirectory + kUptimeFilename);

  while (uptimeFile >> str) {
    uptime = stoi(str);
    uptimeFile.close();
    return uptime;
  }
  uptimeFile.close();
  return uptime;
}

long LinuxParser::CalculateJiffPartialSum(vector<int> &indices) {
  ifstream statFile;
  statFile.open(kProcDirectory + kStatFilename);
  string str;
  long partialSum = 0;
  vector<long> cpuUsage;

  int pos = 0;
  while ((pos >= 0) && (pos <= 10)) {
    statFile >> str;
    if (str != "cpu") {
      cpuUsage.push_back(stoi(str));
    }
    pos++;
  }

  for (int i : indices) {
    partialSum += cpuUsage[i];
  }

  statFile.close();
  return partialSum;
}

// Read and return the number of jiffies for the system
long LinuxParser::Jiffies() {
  vector<int> indices{kUser_,    kNice_,  kSystem_, kIRQ_,
                      kSoftIRQ_, kSteal_, kIdle_,   kIOwait_};
  return LinuxParser::CalculateJiffPartialSum(indices);
}

// Read and return the number of active jiffies for the system
long LinuxParser::ActiveJiffies() {
  vector<int> indices{kUser_, kNice_, kSystem_, kIRQ_, kSoftIRQ_, kSteal_};
  return LinuxParser::CalculateJiffPartialSum(indices);
}

// Read and return the number of idle jiffies for the system
long LinuxParser::IdleJiffies() {
  vector<int> indices{kIdle_, kIOwait_};
  return LinuxParser::CalculateJiffPartialSum(indices);
}

// Read and return CPU utilization
float LinuxParser::CpuUtilization(int pid) {
  ifstream statFile;

  string str;
  int utime, stime, starttime;
  long int uptime = LinuxParser::UpTime();

  statFile.open(kProcDirectory + '/' + to_string(pid) + '/' + kStatFilename);
  for (int i = 1; i <= 22; i++) {
    statFile >> str;
    if (i == 14) {
      utime = stoi(str);
    } else if (i == 15) {
      stime = stoi(str);
    } else if (i == 22) {
      starttime = stoi(str);
    }
  }
  int total_time = utime + stime;
  int seconds = uptime - (starttime / sysconf(_SC_CLK_TCK));
  return total_time / sysconf(_SC_CLK_TCK) / (0.0 + seconds);
}

// Read and return the total number of processes
int LinuxParser::TotalProcesses() {
  return findValByKey<int>(kProcDirectory + kStatFilename, "processes");
}

// Read and return the number of running processes
int LinuxParser::RunningProcesses() {
  return findValByKey<int>(kProcDirectory + kStatFilename, "procs_running");
}

string LinuxParser::Command(int pid) {
  ifstream cmdFile;
  cmdFile.open(kProcDirectory + '/' + to_string(pid) + '/' + kCmdlineFilename);
  string str;
  getline(cmdFile, str);
  cmdFile.close();
  return str;
}

string LinuxParser::Ram(int pid) {
  /*
  the key used in here to read memory usage is `VmRSS` 
  to measure real **physical** mem usage
  instead of `VmSize` (virtual mem size)
  */
  string statusFile =
      kProcDirectory + '/' + to_string(pid) + '/' + kstatus_filename;
  int VmSizeInKB = stoi(findValByKey<string>(statusFile, "VmRSS:"));
  int VmSizeInMB = VmSizeInKB / 1024;
  return std::to_string(VmSizeInMB);
}

string LinuxParser::User(int pid) {
  ifstream passwd(kPasswordPath);
  string line;
  string delim = ":";

  static map<string, string> idNameMap;
  while (getline(passwd, line)) {
    int first = line.find(delim, 0);
    int second = line.find(delim, first + 1);
    int third = line.find(delim, second + 1);

    string uname = line.substr(0, first);
    string uid = line.substr(second + 1, third - second - 1);
    idNameMap[uid] = uname;
  }
  passwd.close();
  return idNameMap[LinuxParser::Uid(pid)];
}

string LinuxParser::Uid(int pid) {
  string statusFile =
      kProcDirectory + '/' + to_string(pid) + '/' + kstatus_filename;
  return findValByKey<string>(statusFile, "Uid:");
}

long LinuxParser::UpTime(int pid) {
  ifstream statFile;
  statFile.open(kProcDirectory + '/' + to_string(pid) + '/' + kStatFilename);
  string str;
  int START_TIME_POSITION = 22;
  for (int i = 1; i <= START_TIME_POSITION; i++) {
    statFile >> str;
  }
  return stol(str) / sysconf(_SC_CLK_TCK);
}
