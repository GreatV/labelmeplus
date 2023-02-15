#ifndef INIT_H
#define INIT_H

#include <QString>

inline QString __appname__{"labelmeplus"};

/* Semantic Versioning 2.0.0: https://semver.org/
 * 1. MAJOR version when you make incompatible API changes;
 * 2. MINOR version when you add functionality in a backwards-compatible manner;
 * 3. PATCH version when you make backwards-compatible bug fixes.
 */
inline std::string __version__{"5.1.1"};

#endif  // INIT_H
