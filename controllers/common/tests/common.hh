
#ifndef AEGIR_FERMD_TESTS_COMMON
#define AEGIR_FERMD_TESTS_COMMON

#include <string>

class FileGuard {
public:
  FileGuard();
  virtual ~FileGuard();
  inline std::string getFilename() const {return c_filename;};
private:
  std::string c_filename;
};

#endif
