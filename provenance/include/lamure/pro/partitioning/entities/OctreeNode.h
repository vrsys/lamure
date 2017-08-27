#ifndef LAMURE_NODE_H
#define LAMURE_NODE_H

#include "lamure/pro/data/entities/DenseMetaData.h"
#include "lamure/pro/data/entities/DensePoint.h"
#include "lamure/pro/partitioning/entities/Partition.h"
#include "lamure/pro/partitioning/entities/PointMetaDataPair.h"
#include "lamure/pro/partitioning/interfaces/Partitionable.h"

namespace prov
{
class OctreeNode : public Partition<PointMetaDataPair<DensePoint, DenseMetaData>, DenseMetaData>, public Partitionable<OctreeNode>
{
  public:
    static const bool CUBIC_NODES = true;
    static const uint8_t MAX_DEPTH = 10;
    static const uint8_t MIN_POINTS_PER_NODE = 1;

    OctreeNode() : Partition<PointMetaDataPair<DensePoint, DenseMetaData>, DenseMetaData>(), Partitionable<OctreeNode>()
    {
        this->_min = vec3d();
        this->_max = vec3d();
        this->_depth = 0;
//        printf("\nOctreeNode created at depth: 0\n");
    }
    OctreeNode(uint8_t depth) : Partition<PointMetaDataPair<DensePoint, DenseMetaData>, DenseMetaData>(), Partitionable<OctreeNode>()
    {
        this->_min = vec3d();
        this->_max = vec3d();
        this->_depth = depth;
//        printf("\nOctreeNode created at depth: %u\n", this->_depth);
    }

    void partition()
    {
        identify_boundaries();

        if(this->_depth < MAX_DEPTH && this->_pairs.size() / 8 > MIN_POINTS_PER_NODE)
        {
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

            std::sort(this->_pairs.begin(), this->_pairs.begin() + mid_y_pos_1,
                      [](const PointMetaDataPair<DensePoint, DenseMetaData> &pair1, const PointMetaDataPair<DensePoint, DenseMetaData> &pair2) {
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

            //            uint64_t mid_z_pos_1 = this->_pairs.size() / 8;
            //            uint64_t mid_z_pos_2 = mid_y_pos_1 + this->_pairs.size() / 8;
            //            uint64_t mid_z_pos_3 = mid_x_pos + this->_pairs.size() / 8;
            //            uint64_t mid_z_pos_4 = mid_y_pos_2 + this->_pairs.size() / 8;

            uint64_t offset = this->_pairs.size() / 8;
            for(uint8_t i = 0; i < 8; i++)
            {
                OctreeNode octree_node(this->_depth + 1);
                vec<PointMetaDataPair<DensePoint, DenseMetaData>> pairs(&this->_pairs[i * offset], &this->_pairs[(i + 1) * offset]);
                octree_node.set_pairs(pairs);
                this->_partitions.push_back(octree_node);
            }

            this->_pairs.clear();

            for(uint8_t i = 0; i < 8; i++)
            {
                this->_partitions.at(i).partition();
            }
        }
        else
        {
            DenseMetaData meta_data = DenseMetaData();

            float photometric_consistency = 1;
            std::set<uint32_t> seen = std::set<uint32_t>();
            std::set<uint32_t> not_seen = std::set<uint32_t>();

            for(uint64_t i = 0; i < this->_pairs.size(); i++)
            {
                // Keep minimum NCC value
                // printf("\nNCC value: %lf\n", this->_pairs.at(i).get_metadata().get_photometric_consistency());
                photometric_consistency = std::min(photometric_consistency, this->_pairs.at(i).get_metadata().get_photometric_consistency());
                for(uint32_t k = 0; k < this->_pairs.at(i).get_metadata().get_images_seen().size(); k++)
                {
                    seen.insert(this->_pairs.at(i).get_metadata().get_images_seen().at(k));
                }
                for(uint32_t k = 0; k < this->_pairs.at(i).get_metadata().get_images_not_seen().size(); k++)
                {
                    not_seen.insert(this->_pairs.at(i).get_metadata().get_images_not_seen().at(k));
                }
            }

//            printf("\nSeen set length: %u\n", seen.size());

            vec<uint32_t> images_seen = vec<uint32_t>(seen.begin(), seen.end());
            vec<uint32_t> images_not_seen = vec<uint32_t>(not_seen.begin(), not_seen.end());

            meta_data.set_photometric_consistency(photometric_consistency);
            meta_data.set_images_seen(images_seen);
            meta_data.set_images_not_seen(images_not_seen);

//            printf("\nPoints in leaf node: %u\n", this->_pairs.size());
//            printf("\nNode NCC: %lf\n", meta_data.get_photometric_consistency());
//            printf("\nNode num seen: %u\n", meta_data.get_images_seen().size());
//            printf("\nNode num not seen: %u\n", meta_data.get_images_not_seen().size());
            this->_aggregate_metadata = meta_data;
        }
    }

    OctreeNode *lookup_node_at_position(vec3d position)
    {
        //        printf("\nEnter lookup node at position\n");

        if(!fits_in_boundaries(position))
        {
            //            printf("\nPosition does not fit into boundaries\n");
            return nullptr;
        }

        if(this->_partitions.empty())
        {
            //            printf("\nMaximum depth reached at this position\n");
            return this;
        }

        for(uint8_t i = 0; i < this->_partitions.size(); i++)
        {
            //            printf("\nEnter partition lookup\n");
            OctreeNode *node = this->_partitions.at(i).lookup_node_at_position(position);
            if(node != nullptr)
            {
                return node;
            }
        }
       
        return nullptr;
    }

  protected:
    void identify_boundaries()
    {
        for(uint64_t i = 0; i < this->_pairs.size(); i++)
        {
            if(this->_min.x > this->_pairs.at(i).get_point().get_position().x)
            {
                this->_min.x = this->_pairs.at(i).get_point().get_position().x;
            }

            if(this->_min.y > this->_pairs.at(i).get_point().get_position().y)
            {
                this->_min.y = this->_pairs.at(i).get_point().get_position().y;
            }

            if(this->_min.z > this->_pairs.at(i).get_point().get_position().z)
            {
                this->_min.z = this->_pairs.at(i).get_point().get_position().z;
            }

            if(this->_max.x < this->_pairs.at(i).get_point().get_position().x)
            {
                this->_max.x = this->_pairs.at(i).get_point().get_position().x;
            }

            if(this->_max.y < this->_pairs.at(i).get_point().get_position().y)
            {
                this->_max.y = this->_pairs.at(i).get_point().get_position().y;
            }

            if(this->_max.z < this->_pairs.at(i).get_point().get_position().z)
            {
                this->_max.z = this->_pairs.at(i).get_point().get_position().z;
            }
        }

        // printf("\nMin: %f, %f, %f", this->_min.x, this->_min.y, this->_min.z);
        // printf("\nMax: %f, %f, %f\n", this->_max.x, this->_max.y, this->_max.z);
    }

    vec3d _min;
    vec3d _max;
    uint8_t _depth;

    bool fits_in_boundaries(vec3d position)
    {
        return !(position.x > this->_max.x || position.x < this->_min.x || position.y > this->_max.y || position.y < this->_min.y || position.z > this->_max.z || position.z < this->_min.z);
    }
};
};
#endif // LAMURE_NODE_H
