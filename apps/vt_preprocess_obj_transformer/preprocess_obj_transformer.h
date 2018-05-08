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

    /**
     * Starts the transformation of .obj uv-coordinates into the virtual texuring uv-space
     * @return path of the transformed .obj file
     */
    std::string start() const;

private:
    std::vector<std::string> split(const std::string &phrase, const std::string &delimiter) const;

    std::string obj_in_path;
    std::string obj_out_path;
    AtlasFile *atlas;
};


#endif //LAMURE_PREPROCESSOBJTRANSFORMER_H
