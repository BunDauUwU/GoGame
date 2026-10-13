#include "ByoYomiClock.h"
#include <cstdio>
#include <cstdlib>
void check(bool ok,const char *text) { if (!ok) { std::fprintf(stderr,"%s\n",text); std::exit(1); } }
int main() {
 ByoYomiClock clock;
 check(clock.advance(599000) && clock.mainRemainingMs==1000 && clock.periodsRemaining==3,"Main time deduction failed");
 clock.completeMove(); check(clock.mainRemainingMs==1000,"Main time reset on move");
 check(clock.advance(1000) && clock.mainRemainingMs==0 && clock.periodRemainingMs==20000
       && clock.periodsRemaining==3,"Main time boundary must start full byo-yomi");
 check(clock.advance(19000) && clock.periodRemainingMs==1000,"Byo-yomi countdown failed");
 clock.completeMove(); check(clock.periodRemainingMs==20000&&clock.periodsRemaining==3,"Fast move consumed a period");
 check(clock.advance(20000)&&clock.periodsRemaining==2&&clock.periodRemainingMs==20000,"First expiry failed");
 check(clock.advance(5000)&&clock.periodRemainingMs==15000,"Second period countdown failed");
 clock.completeMove(); check(clock.periodRemainingMs==20000&&clock.periodsRemaining==2,"Lost period was restored");
 check(clock.advance(20000)&&clock.periodsRemaining==1,"Second expiry failed");
 check(!clock.advance(20000)&&clock.periodsRemaining==0&&clock.periodRemainingMs==0,"Final expiry did not forfeit");
 ByoYomiClock late;
 check(late.advance(645000)&&late.periodsRemaining==1&&late.periodRemainingMs==15000,"Delayed timer skipped elapsed time");
 check(!late.advance(15000),"Delayed final boundary failed");
 ByoYomiClock invalid; invalid.mainRemainingMs=0;
 check(invalid.advance(12000)&&invalid.advance(9000)&&invalid.periodsRemaining==2
       &&invalid.periodRemainingMs==19000,"Repeated invalid moves reset byo-yomi");
 std::puts("Byo-yomi clock checks passed");
}
