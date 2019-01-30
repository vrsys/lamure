    //write output lod file
    std::string out_lod_filename = bvh_filename.substr(0, bvh_filename.size()-4) + "_uv.lod";
    std::shared_ptr<lamure::ren::lod_stream> lod_out = std::make_shared<lamure::ren::lod_stream>();
    lod_out->open_for_writing(out_lod_filename);
   
    for (uint32_t node_id = 0; node_id < first_leaf; ++node_id) { //loops only inner nodes
     
      //load the node
      lod->read((char*)&vertices[0], (vertices_per_node*node_id*size_of_vertex),
        (vertices_per_node * size_of_vertex));

      //TODO: at this point we will need to project all triangles of inner nodes to their respective charts using the corresponding chart plane
     
      //afterwards, write the node to new file
      lod_out->write((char*)&vertices[0], (vertices_per_node*node_id*size_of_vertex),
        (vertices_per_node * size_of_vertex));

    }

    //for all leaf nodes, we will just write the entire triangles array to disk (which stores all leaf triangles)

    lod_out->write((char*)&triangles[0], first_leaf*vertices_per_node*size_of_vertex,
      vertices_per_node * size_of_vertex * triangles.size());

    lod_out->close();
    lod_out.reset();
    lod->close();
    lod.reset();

    std::cout << "OUTPUT: " << out_lod_filename << std::endl;

    //write output bvh
    std::string out_bvh_filename = bvh_filename.substr(0, bvh_filename.size()-4) + "_uv.bvh";
    bvh->write_bvh_file(out_bvh_filename);
    bvh.reset();

    std::cout << "OUTPUT: " << out_bvh_filename << std::endl;