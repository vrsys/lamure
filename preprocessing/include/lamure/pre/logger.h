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
class PREPROCESSING_DLL logger
{
public:

                        logger(const logger&) = delete;
                        logger& operator=(const logger&) = delete;
    virtual             ~logger();

    static logger&      get_instance();

    template <typename T>
    friend logger& operator <<(logger& log, const T& value) {
        std::cout << value;
        return log;
    }



protected:
                        logger();
    static logger       single_;

private:
    static std::mutex   mutex_;

};


#define LOGGER_TRACE(msg) logger::get_instance()<<msg<<"\n"
#define LOGGER_INFO(msg) logger::get_instance()<<msg<<"\n"
#define LOGGER_WARN(msg) logger::get_instance()<<msg<<"\n"
#define LOGGER_ERROR(msg) logger::get_instance()<<msg<<"\n"
#ifdef NDEBUG
    #define LOGGER_DEBUG(msg)
#else
    #define LOGGER_DEBUG(msg) logger::get_instance()<<msg<<"\n"
#endif
#define LOGGER_TEXT(msg) logger::get_instance()<<msg<<"\n"



} } //namespace lamure

#endif // PRE_LOGGER_H_

