#include "bits.h"

int main()
{
  bitmap foo;
  foo.initialize();
  bitmap tester(7,4);
  do cout << tester << endl;
  while (tester.next_perm(7));
  return 0;
}
