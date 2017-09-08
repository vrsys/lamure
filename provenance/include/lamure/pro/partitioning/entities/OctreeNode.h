#ifndef LAMURE_NODE_H
#define LAMURE_NODE_H

#include "lamure/pro/data/entities/DenseMetaData.h"
#include "lamure/pro/data/entities/DensePoint.h"
#include "lamure/pro/partitioning/entities/Partition.h"
#include "lamure/pro/partitioning/interfaces/Partitionable.h"

#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

namespace prov
{
class OctreeNode : public Partition<pair<DensePoint, DenseMetaData>, DenseMetaData>, public Partitionable<OctreeNode>
{
  public:
    static const bool CUBIC_NODES = false;
    static const uint8_t MAX_DEPTH = 10;
    static const uint8_t MIN_POINTS_PER_NODE = 1;

    OctreeNode() : Partition<pair<DensePoint, DenseMetaData>, DenseMetaData>(), Partitionable<OctreeNode>()
    {
        this->_min = vec3f(FLT_MAX, FLT_MAX, FLT_MAX);
        this->_max = vec3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        this->_depth = 0;
        //        printf("\nOctreeNode created at depth: 0\n");
    }
    OctreeNode(uint8_t depth) : Partition<pair<DensePoint, DenseMetaData>, DenseMetaData>(), Partitionable<OctreeNode>()
    {
        this->_min = vec3f(FLT_MAX, FLT_MAX, FLT_MAX);
        this->_max = vec3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        this->_depth = depth;
        //        printf("\nOctreeNode created at depth: %u\n", this->_depth);
    }

    void partition()
    {
        this->identify_boundaries();

        if(this->_depth < MAX_DEPTH && this->_pairs.size() / 8 > MIN_POINTS_PER_NODE)
        {
            pdqsort(this->_pairs.begin(), this->_pairs.end(),
                    [](const pair<DensePoint, DenseMetaData> &pair1, const pair<DensePoint, DenseMetaData> &pair2) { return pair1.first.get_position().x < pair2.first.get_position().x; });

            uint64_t mid_x_pos = this->_pairs.size() / 2;

            pdqsort(this->_pairs.begin(), this->_pairs.begin() + mid_x_pos,
                    [](const pair<DensePoint, DenseMetaData> &pair1, const pair<DensePoint, DenseMetaData> &pair2) { return pair1.first.get_position().y < pair2.first.get_position().y; });
            pdqsort(this->_pairs.begin() + mid_x_pos, this->_pairs.end(),
                    [](const pair<DensePoint, DenseMetaData> &pair1, const pair<DensePoint, DenseMetaData> &pair2) { return pair1.first.get_position().y < pair2.first.get_position().y; });

            uint64_t mid_y_pos_1 = this->_pairs.size() / 4;
            uint64_t mid_y_pos_2 = mid_x_pos + this->_pairs.size() / 4;

            pdqsort(this->_pairs.begin(), this->_pairs.begin() + mid_y_pos_1,
                    [](const pair<DensePoint, DenseMetaData> &pair1, const pair<DensePoint, DenseMetaData> &pair2) { return pair1.first.get_position().z < pair2.first.get_position().z; });
            pdqsort(this->_pairs.begin() + mid_y_pos_1, this->_pairs.begin() + mid_x_pos,
                    [](const pair<DensePoint, DenseMetaData> &pair1, const pair<DensePoint, DenseMetaData> &pair2) { return pair1.first.get_position().z < pair2.first.get_position().z; });
            pdqsort(this->_pairs.begin() + mid_x_pos, this->_pairs.begin() + mid_y_pos_2,
                    [](const pair<DensePoint, DenseMetaData> &pair1, const pair<DensePoint, DenseMetaData> &pair2) { return pair1.first.get_position().z < pair2.first.get_position().z; });
            pdqsort(this->_pairs.begin() + mid_y_pos_2, this->_pairs.end(),
                    [](const pair<DensePoint, DenseMetaData> &pair1, const pair<DensePoint, DenseMetaData> &pair2) { return pair1.first.get_position().z < pair2.first.get_position().z; });

            //            uint64_t mid_z_pos_1 = this->_pairs.size() / 8;
            //            uint64_t mid_z_pos_2 = mid_y_pos_1 + this->_pairs.size() / 8;
            //            uint64_t mid_z_pos_3 = mid_x_pos + this->_pairs.size() / 8;
            //            uint64_t mid_z_pos_4 = mid_y_pos_2 + this->_pairs.size() / 8;

            uint64_t offset = this->_pairs.size() / 8;
            for(uint8_t i = 0; i < 8; i++)
            {
                OctreeNode octree_node(this->_depth + 1);
                vec<pair<DensePoint, DenseMetaData>> pairs(&this->_pairs[i * offset], &this->_pairs[(i + 1) * offset]);
                octree_node.set_pairs(pairs);
                this->_partitions.push_back(octree_node);
            }

            for(uint8_t i = 0; i < 8; i++)
            {
                this->_partitions.at(i).partition();
            }
        }

        float photometric_consistency = 0;
        std::set<uint32_t> seen = std::set<uint32_t>();
        std::set<uint32_t> not_seen = std::set<uint32_t>();

        for(uint64_t i = 0; i < this->_pairs.size(); i++)
        {
            photometric_consistency = photometric_consistency + (this->_pairs.at(i).second.get_photometric_consistency() - photometric_consistency) / (i + 1);
            for(uint32_t k = 0; k < this->_pairs.at(i).second.get_images_seen().size(); k++)
            {
                seen.insert(this->_pairs.at(i).second.get_images_seen().at(k));
            }
            for(uint32_t k = 0; k < this->_pairs.at(i).second.get_images_not_seen().size(); k++)
            {
                not_seen.insert(this->_pairs.at(i).second.get_images_not_seen().at(k));
            }
        }

        //            printf("\nSeen set length: %u\n", seen.size());

        vec<uint32_t> images_seen = vec<uint32_t>(seen.begin(), seen.end());
        vec<uint32_t> images_not_seen = vec<uint32_t>(not_seen.begin(), not_seen.end());

        _aggregate_metadata.set_photometric_consistency(photometric_consistency);
        _aggregate_metadata.set_images_seen(images_seen);
        _aggregate_metadata.set_images_not_seen(images_not_seen);

        //            printf("\nPoints in leaf node: %u\n", this->_pairs.size());
        //            printf("\nNode NCC: %lf\n", _aggregate_metadata.get_photometric_consistency());
        //            printf("\nNode num seen: %u\n", _aggregate_metadata.get_images_seen().size());
        //            printf("\nNode num not seen: %u\n", _aggregate_metadata.get_images_not_seen().size());

        cleanup();
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

        uint8_t curr_best = 0;
        float curr_best_distance = 0;
        for(uint8_t i = 0; i < this->_partitions.size(); i++)
        {
            vec3d center = this->_partitions.at(i).get_center();
            if(curr_best_distance < (const float)scm::math::distance(center, position))
            {
                curr_best_distance = (const float)scm::math::distance(center, position);
                curr_best = i;
            }
        }

        OctreeNode &node = this->_partitions.at(curr_best);
        if(!node.fits_in_boundaries(position))
        {
            position = node.clamp(position, node._min, node._max);
        }

        return node.lookup_node_at_position(position);
    }

    uint8_t get_depth() { return this->_depth; }
    vec3d get_center() { return vec3d((_max.x + _min.x) / 2, (_max.y + _min.y) / 2, (_max.z + _min.z) / 2); }

  protected:
    vec3d clamp(vec3d position, vec3f min, vec3f max)
    {
        if(position.x < min.x)
        {
            position.x = min.x;
        }
        if(position.x > max.x)
        {
            position.x = max.x;
        }
        if(position.y < min.y)
        {
            position.y = min.y;
        }
        if(position.y > max.y)
        {
            position.y = max.y;
        }
        if(position.z < min.z)
        {
            position.z = min.z;
        }
        if(position.z > max.z)
        {
            position.z = max.z;
        }
        return position;
    }

    void identify_boundaries()
    {
        for(uint64_t i = 0; i < this->_pairs.size(); i++)
        {
            if(this->_min.x > this->_pairs.at(i).first.get_position().x)
            {
                this->_min.x = this->_pairs.at(i).first.get_position().x;
            }

            if(this->_min.y > this->_pairs.at(i).first.get_position().y)
            {
                this->_min.y = this->_pairs.at(i).first.get_position().y;
            }

            if(this->_min.z > this->_pairs.at(i).first.get_position().z)
            {
                this->_min.z = this->_pairs.at(i).first.get_position().z;
            }

            if(this->_max.x < this->_pairs.at(i).first.get_position().x)
            {
                this->_max.x = this->_pairs.at(i).first.get_position().x;
            }

            if(this->_max.y < this->_pairs.at(i).first.get_position().y)
            {
                this->_max.y = this->_pairs.at(i).first.get_position().y;
            }

            if(this->_max.z < this->_pairs.at(i).first.get_position().z)
            {
                this->_max.z = this->_pairs.at(i).first.get_position().z;
            }
        }

        // printf("\nMin: %f, %f, %f", this->_min.x, this->_min.y, this->_min.z);
        // printf("\nMax: %f, %f, %f\n", this->_max.x, this->_max.y, this->_max.z);
    }

    void cleanup() { this->_pairs.clear(); }

    vec3f _min;
    vec3f _max;
    uint8_t _depth;

    bool fits_in_boundaries(vec3d position)
    {
        // bool test = !(position.x > this->_max.x || position.x < this->_min.x || position.y > this->_max.y || position.y < this->_min.y || position.z > this->_max.z || position.z < this->_min.z);
        return !(position.x > this->_max.x || position.x < this->_min.x || position.y > this->_max.y || position.y < this->_min.y || position.z > this->_max.z || position.z < this->_min.z);
    }
};
};
#endif // LAMURE_NODE_H
