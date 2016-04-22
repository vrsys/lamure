
#ifndef LAMURE_LOGGER_H_
#define LAMURE_LOGGER_H_

#include <lamure/lamure_config.h>
#include <lamure/exception.h>
#include <lamure/util/to_string.h>

#include <fstream>
#include <sstream>
#include <iostream>

namespace lamure {

class logger_t {
public:
  enum level_t {
    LOGGER_LEVEL_INFO = 0,
    LOGGER_LEVEL_WARN = 1,
    LOGGER_LEVEL_ERROR = 2,
  };

  ~logger_t();

  static logger_t& get();

  template <typename T>
  friend logger_t& operator <<(logger_t& log, const T& value) {
#ifdef LAMURE_LOG_TO_FILE
    std::fstream file(LAMURE_LOG_FILE_NAME, std::ios::out | std::ios::app);
    if (!file.is_open()) {
      throw exception_t("unable to open logfile for writing");
      return log;
    }
    std::string str = lamure::util::to_string(value).c_str();
    str.append("\n");
    file.write(str.c_str(), str.size());
    file.close();
#else
    printf("%s", (lamure::util::to_string(value)).c_str());
#endif
    return log;
  }

  void set_level(const level_t level) { level_ = level; };
  const level_t get_level() const { return level_; };

protected:
  logger_t();

private:
  static logger_t single_;
  level_t level_;

};

#define LAMURE_LOG_INFO(msg) if (lamure::logger_t::get().get_level() <= lamure::logger_t::LOGGER_LEVEL_INFO) lamure::logger_t::get() << "LAMURE INFO: " << msg << "\n";
#define LAMURE_LOG_WARN(msg) if (lamure::logger_t::get().get_level() <= lamure::logger_t::LOGGER_LEVEL_WARN) lamure::logger_t::get() << "LAMURE WARNING: " << msg << "\n";
#define LAMURE_LOG_ERROR(msg) lamure::logger_t::get() << "LAMURE ERROR: " << msg << "\n";
#define LAMURE_LOG_EXIT(msg) lamure::logger_t::get() << "LAMURE ERROR: " << msg << "\n"; exit(-1);

}

#endif

