// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr


#ifndef LAMURE_PREPROCESSOBJTRANSFORMER_H
#define LAMURE_PREPROCESSOBJTRANSFORMER_H


#include <string>

#include <lamure/vt/pre/AtlasFile.h>

using namespace vt::pre;

class preprocess_obj_transformer {
public:
    preprocess_obj_transformer(const std::string &obj_file, const std::string &atlas_file);

    virtual ~preprocess_obj_transformer();

    void set_obj_path(const std::string &obj_path);

    void set_atlas(const std::string &atlas_file);

    std::string scale() const;

private:
    std::string obj_in_path;
    std::string obj_out_path;

    AtlasFile *atlas;

    std::vector<std::string> split(const std::string &phrase, const std::string &delimiter) const;
};


#endif //LAMURE_PREPROCESSOBJTRANSFORMER_H
