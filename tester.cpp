#include <cmath>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <numeric>
#include <cassert>
#include <algorithm>
#include <mutex>
#include <thread>
#include <string.h>             // memset

using namespace std;

template<class T>
static ostream &operator<<(ostream &os, const vector<T> &v)
{
  for (auto &i : v) os << ((&i == &v[0])? "" : " ") << i;
  return os;
}

template<class T>
static ostream &operator<<(ostream &os, const set<T> &s)
{
  for (auto &i : s) os << ((&i == &*s.begin())? "" : " ") << i;
  return os;
}

void send_to_firefox(const string &str)
{
#ifdef __linux__
  system("xdotool search \"Mozilla Firefox\" > /tmp/windowid");
  ifstream inf("/tmp/windowid");
  size_t id;
  inf >> id;
  inf.close();
  remove("/tmp/windowid");
  ostringstream cmd;
  cmd << "xdotool key --delay 1 --window " << id;
  for (char c : str)
    if (c == '\t') cmd << " Tab";
    else cmd << ' ' << c;
  system(cmd.str().c_str());
#else
  ostringstream cmd;
  cmd << "osascript -e 'tell app \"Firefox\" to activate' "
      << "-e 'tell app \"System Events\" to keystroke \""
      << str << "\"'";
  system(cmd.str().c_str());
#endif
}

int main(int argc, const char **argv)
{
  send_to_firefox("abc\tdef");
  return 0;
}
