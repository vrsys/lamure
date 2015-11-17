// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_LOGGER_H_
#define PRE_LOGGER_H_

#include <lamure/pre/platform.h>

#include <sstream>
#include <mutex>
#include <iostream>

namespace lamure {
namespace pre {

/**
* Replacement class for log4cplus. Initial dummy version.
*/
class PREPROCESSING_DLL Logger
{
public:

                        Logger(const Logger&) = delete;
                        Logger& operator=(const Logger&) = delete;
    virtual             ~Logger();

    static Logger&      GetInstance();

    template <typename T>
    friend Logger& operator <<(Logger& log, const T& value) {
        std::cout << value;
        return log;
    }



protected:
                        Logger();
    static Logger       single_;

private:
    static std::mutex   mutex_;

};


#define LOGGER_TRACE(msg) Logger::GetInstance()<<msg<<"\n"
#define LOGGER_INFO(msg) Logger::GetInstance()<<msg<<"\n"
#define LOGGER_WARN(msg) Logger::GetInstance()<<msg<<"\n"
#define LOGGER_ERROR(msg) Logger::GetInstance()<<msg<<"\n"
#define LOGGER_DEBUG(msg) 0
#define LOGGER_TEXT(msg) Logger::GetInstance()<<msg<<"\n"



} } //namespace lamure

#endif // PRE_LOGGER_H_

