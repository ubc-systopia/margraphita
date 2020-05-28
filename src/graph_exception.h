#include <iostream>
#include <exception>


class GraphException: public std::exception
{
  std::string _msg;
  public:
  GraphException(const std::string& msg){
    _msg = msg;
  } 
  virtual const char* what() const noexcept
  {
    return _msg.c_str();
  }

};
