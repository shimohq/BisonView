#ifndef BISON_COMMON_BV_PATHS_H_
#define BISON_COMMON_BV_PATHS_H_

namespace bison {

enum {
  PATH_START = 11000,

  DIR_CRASH_DUMPS = PATH_START,  // Directory where crash dumps are written.

  PATH_END
};

// Call once to register the provider for the path keys defined above.
void RegisterPathProvider();

} // namespace bison

#endif  // BISON_COMMON_BV_PATHS_H_
