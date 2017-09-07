#ifndef LAMURE_SPARSEOCTREE_H
#define LAMURE_SPARSEOCTREE_H

#include <lamure/pro/data/DenseCache.h>
#include <lamure/pro/partitioning/entities/OctreeNode.h>
#include <lamure/pro/partitioning/interfaces/Partitionable.h>

namespace prov
{
class SparseOctree : public OctreeNode
{
  public:
    SparseOctree() : OctreeNode() {}
    SparseOctree(DenseCache &dense_cache) : OctreeNode() { glue_pairs(dense_cache); }
    void partition()
    {
        printf("\nStart partitioning\n");

        identify_boundaries();

        std::sort(this->_pairs.begin(), this->_pairs.end(), [](const PointMetaDataPair<DensePoint, DenseMetaData> &pair1, const PointMetaDataPair<DensePoint, DenseMetaData> &pair2) {
            return pair1.get_point().get_position().x < pair2.get_point().get_position().x;
        });

        uint64_t mid_x_pos = this->_pairs.size() / 2;

        std::sort(this->_pairs.begin(), this->_pairs.begin() + mid_x_pos, [](const PointMetaDataPair<DensePoint, DenseMetaData> &pair1, const PointMetaDataPair<DensePoint, DenseMetaData> &pair2) {
            return pair1.get_point().get_position().y < pair2.get_point().get_position().y;
        });
        std::sort(this->_pairs.begin() + mid_x_pos, this->_pairs.end(), [](const PointMetaDataPair<DensePoint, DenseMetaData> &pair1, const PointMetaDataPair<DensePoint, DenseMetaData> &pair2) {
            return pair1.get_point().get_position().y < pair2.get_point().get_position().y;
        });

        uint64_t mid_y_pos_1 = this->_pairs.size() / 4;
        uint64_t mid_y_pos_2 = mid_x_pos + this->_pairs.size() / 4;

        std::sort(this->_pairs.begin(), this->_pairs.begin() + mid_y_pos_1, [](const PointMetaDataPair<DensePoint, DenseMetaData> &pair1, const PointMetaDataPair<DensePoint, DenseMetaData> &pair2) {
            return pair1.get_point().get_position().z < pair2.get_point().get_position().z;
        });
        std::sort(this->_pairs.begin() + mid_y_pos_1, this->_pairs.begin() + mid_x_pos,
                  [](const PointMetaDataPair<DensePoint, DenseMetaData> &pair1, const PointMetaDataPair<DensePoint, DenseMetaData> &pair2) {
                      return pair1.get_point().get_position().z < pair2.get_point().get_position().z;
                  });
        std::sort(this->_pairs.begin() + mid_x_pos, this->_pairs.begin() + mid_y_pos_2,
                  [](const PointMetaDataPair<DensePoint, DenseMetaData> &pair1, const PointMetaDataPair<DensePoint, DenseMetaData> &pair2) {
                      return pair1.get_point().get_position().z < pair2.get_point().get_position().z;
                  });
        std::sort(this->_pairs.begin() + mid_y_pos_2, this->_pairs.end(), [](const PointMetaDataPair<DensePoint, DenseMetaData> &pair1, const PointMetaDataPair<DensePoint, DenseMetaData> &pair2) {
            return pair1.get_point().get_position().z < pair2.get_point().get_position().z;
        });

        uint64_t offset = this->_pairs.size() / 8;
        for(uint8_t i = 0; i < 8; i++)
        {
            OctreeNode octree_node(this->_depth + uint8_t(1));
            vec<PointMetaDataPair<DensePoint, DenseMetaData>> pairs(&this->_pairs[i * offset], &this->_pairs[(i + 1) * offset]);
            octree_node.set_pairs(pairs);
            this->_partitions.push_back(octree_node);
        }

        for(uint8_t i = 0; i < 8; i++)
        {
            this->_partitions.at(i).partition();
        }

        float photometric_consistency = 0;
        std::set<uint32_t> seen = std::set<uint32_t>();
        std::set<uint32_t> not_seen = std::set<uint32_t>();

        for(uint64_t i = 0; i < this->_pairs.size(); i++)
        {
            // Keep minimum NCC value
            // printf("\nNCC value: %lf\n", this->_pairs.at(i).get_metadata().get_photometric_consistency());
            //            photometric_consistency = std::min(photometric_consistency, this->_pairs.at(i).get_metadata().get_photometric_consistency());
            photometric_consistency = photometric_consistency + (this->_pairs.at(i).get_metadata().get_photometric_consistency() - photometric_consistency) / (i + 1);
            for(uint32_t k = 0; k < this->_pairs.at(i).get_metadata().get_images_seen().size(); k++)
            {
                seen.insert((unsigned int)this->_pairs.at(i).get_metadata().get_images_seen().at(k));
            }
            for(uint32_t k = 0; k < this->_pairs.at(i).get_metadata().get_images_not_seen().size(); k++)
            {
                not_seen.insert((unsigned int)this->_pairs.at(i).get_metadata().get_images_not_seen().at(k));
            }
        }

        //            printf("\nSeen set length: %u\n", seen.size());

        vec<uint32_t> images_seen = vec<uint32_t>(seen.begin(), seen.end());
        vec<uint32_t> images_not_seen = vec<uint32_t>(not_seen.begin(), not_seen.end());

        _aggregate_metadata.set_photometric_consistency(photometric_consistency);
        _aggregate_metadata.set_images_seen(images_seen);
        _aggregate_metadata.set_images_not_seen(images_not_seen);

        cleanup();

        // TODO: figure out how to avoid parasite copying of nodes; maybe try promises & futures
        //        boost::asio::io_service ioService;
        //        boost::thread_group threadpool;
        //
        //        boost::asio::io_service::work work(ioService);
        //
        //        for(uint8_t i = 0; i < 8; i++)
        //        {
        //            threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));
        //        }
        //
        //        for(uint8_t i = 0; i < 8; i++)
        //        {
        //            OctreeNode &node = this->_partitions.at(i);
        //            ioService.post(boost::bind(&OctreeNode::partition, node));
        //        }
        //
        //        ioService.stop();
        //        threadpool.join_all();

        cleanup();

        printf("\nEnd partitioning\n");
    }

    //    void output_tree(string output_path)
    //    {
    //        // TODO
    //    }

    //    static SparseOctree load_tree()
    //    {
    //        // TODO
    //        SparseOctree sparse_octree = SparseOctree();
    //        return sparse_octree;
    //    }

    void debug_information_loss(DenseCache &dense_cache, uint64_t num_probes)
    {
        printf("\nEnter debug information loss\n");
        double information_loss = 0;
        num_probes = std::min(num_probes, dense_cache.get_points().size());
        for(uint64_t i = 0; i < num_probes; i++)
        {
            size_t ind = (size_t)(((float)rand() / RAND_MAX) * dense_cache.get_points().size());
            OctreeNode *node_ptr = lookup_node_at_position(dense_cache.get_points().at(ind).get_position());
            if(node_ptr == nullptr)
            {
                vec3d pos = dense_cache.get_points().at(ind).get_position();
                printf("\nnullptr hit during lookup: %f, %f, %f\n", pos.x, pos.y, pos.z);
                throw new std::runtime_error("\nnullptr hit during lookup\n");
            }
            //            else
            //            {
            //                printf("\ndepth returned: %u\n", (*node_ptr).get_depth());
            //            }
            information_loss += compare_metadata((*node_ptr).get_aggregate_metadata(), dense_cache.get_points_metadata().at(ind)) / (double)num_probes;
            //            printf("\nIntermediate information loss: %lf\n", information_loss);
        }
        printf("\nFinal information loss: %lf%%\n", information_loss * 100);
    }

    void debug_randomized_lookup(uint64_t num_probes)
    {
        for(uint64_t i = 0; i < num_probes; i++)
        {
            float rand_x = rand() / (float)RAND_MAX * (this->_max.x - this->_min.x) + this->_min.x;
            float rand_y = rand() / (float)RAND_MAX * (this->_max.y - this->_min.y) + this->_min.y;
            float rand_z = rand() / (float)RAND_MAX * (this->_max.z - this->_min.z) + this->_min.z;
            vec3d rand_pos(rand_x, rand_y, rand_z);
            OctreeNode *node_ptr = lookup_node_at_position(rand_pos);
            if(node_ptr == nullptr)
            {
                printf("\nnullptr hit during lookup: %f, %f, %f\n", rand_pos.x, rand_pos.y, rand_pos.z);
                throw new std::runtime_error("\nnullptr hit during lookup\n");
            }
        }
    }

    OctreeNode *lookup_node_at_position(vec3d position)
    {
        if(!fits_in_boundaries(position))
        {
            return this;
        }

        if(this->_partitions.empty())
        {
            return this;
        }

        for(uint8_t i = 0; i < this->_partitions.size(); i++)
        {
            OctreeNode *node = this->_partitions.at(i).lookup_node_at_position(position);
            if(node != nullptr)
            {
                return node;
            }
        }

        uint8_t curr_best = 0;
        float curr_best_distance = 0;
        for(uint8_t i = 0; i < this->_partitions.size(); i++)
        {
            vec3d center = this->_partitions.at(i).get_center();
            if(curr_best_distance < (const float) scm::math::distance(center, position)){
                curr_best_distance = (const float) scm::math::distance(center, position);
                curr_best = i;
            }
        }

        return &(this->_partitions.at(curr_best));
    }

  private:
    double compare_metadata(const DenseMetaData &data, const DenseMetaData &ref_data)
    {
        double information_loss = 0;
        information_loss += std::abs(data.get_photometric_consistency() - ref_data.get_photometric_consistency());
        //        information_loss += std::abs(int(data.get_images_seen().size()) - int(ref_data.get_images_seen().size())) / (double)(1 + int(ref_data.get_images_seen().size()));
        //        information_loss += std::abs(int(data.get_images_not_seen().size()) - int(ref_data.get_images_not_seen().size())) / (double)(1 + int(ref_data.get_images_not_seen().size()));
        return information_loss; // / 3;
    }
    void glue_pairs(DenseCache &dense_cache)
    {
        printf("\nStart gluing pairs\n");
        for(uint64_t i = 0; i < dense_cache.get_points().size(); i++)
        {
            PointMetaDataPair<DensePoint, DenseMetaData> pair(dense_cache.get_points().at(i), dense_cache.get_points_metadata().at(i));
            this->_pairs.push_back(pair);
        }
        printf("\nEnd gluing pairs\n");
    }
};
};
#endif // LAMURE_SPARSEOCTREE_H
