// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pvs/visibility_test.h>
#include <lamure/pvs/visibility_test_id_histogram_renderer.h>

int main(int argc, char** argv)
{
    lamure::pvs::visibility_test* vt = new lamure::pvs::visibility_test_id_histogram_renderer();

    vt->initialize(argc, argv);
    vt->test_visibility();
    vt->shutdown();

    delete vt;
    return 0;
}
