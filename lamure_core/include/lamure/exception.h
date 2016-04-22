
#ifndef LAMURE_EXCEPTION_H_
#define LAMURE_EXCEPTION_H_

#include <stdexcept>
#include <string>

namespace lamure {

class exception_t : public std::exception {
public:
  exception_t(const std::string& _error);
  ~exception_t() throw();

  const std::string& get_error() const;

protected:
  std::string error_;
  
};

} // namespace lamure

#endif
