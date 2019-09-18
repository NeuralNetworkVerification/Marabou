#if defined(__linux__)
#include <sched.h>
#define getCPUId( cpu ) { cpu = sched_getcpu(); }
#elif __APPLE__
#include <cpuid.h>
//
#define CPUId( INFO, LEAF, SUBLEAF ) __cpuid_count( LEAF, SUBLEAF, INFO[0], INFO[1], INFO[2], INFO[3] )
#define getCPUId( cpu ) {                           \
    uint32_t CPUInfo[4];                            \
    CPUId( CPUInfo, 1, 0 );                         \
    if ( ( CPUInfo[3] & ( 1 << 9 ) ) == 0) {        \
      cpu = -1;                                     \
    }                                               \
    else {                                          \
      cpu = ( unsigned )CPUInfo[1] >> 24;           \
    }                                               \
    if ( cpu < 0 ) cpu = 0;                         \
  }
#else
#define getCPUId( cpu ) "#"
#endif
