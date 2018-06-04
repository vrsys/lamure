//
// Created by moqe3167 on 30/05/18.
//

#ifndef LAMURE_GUI_H
#define LAMURE_GUI_H


#include <scm/core/math.h>
#include "settings.h"
#include "window.h"
#include "input.h"
#include "selection.h"

class gui {
public:
    gui();

    void
    status_screen(settings &setting, input &input, selection &selection, lamure::ren::Data_Provenance &data_provenance, int32_t num_models, double fps, uint64_t rendered_splats, uint64_t rendered_nodes);

private:
    bool view_settings_;
    bool lod_settings_;
    bool visual_settings_;
    bool provenance_settings_;

    void view_settings(settings &setting, input &input);
    void lod_settings(settings &setting, input &input);
    void visual_settings(settings &setting, input &input, lamure::ren::Data_Provenance &data_provenance);
    void provenance_settings(settings &setting, input &input);
};

#endif //LAMURE_GUI_H
