#ifndef KIRA_UTIL_H_
#define KIRA_UTIL_H_

#define STATIC_ASSERT(x) do { enum { assertion = 1/!!(x) }; } while (0)

void report_and_abort(const char *file, int line,
		      const char *msg1, const char *msg2);

#define CHECK(x) do { \
    if (!(x)) { \
      report_and_abort(__FILE__, __LINE__, "CHECK failed: ", #x); \
    } \
  } while (0)

#define CRASH(msg) do { \
    report_and_abort(__FILE__, __LINE__, "CRASH: ", #msg); \
  } while (0)

#define UNREACHABLE() CRASH("Unreachable.")

#endif /* KIRA_UTIL_H_ */
