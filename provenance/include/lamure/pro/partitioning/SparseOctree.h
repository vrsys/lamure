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
        //        printf("\nStart octree partitioning\n");
        OctreeNode::partition();
        //        printf("\nEnd octree partitioning\n");
    }

    void output_tree(string output_path)
    {
        // TODO
    }

    static SparseOctree load_tree()
    {
        // TODO
        SparseOctree sparse_octree = SparseOctree();
        return sparse_octree;
    }

    void debug_information_loss(DenseCache &dense_cache, uint64_t num_probes)
    {
        printf("\nEnter debug information loss\n");
        double information_loss = 0;
        num_probes = std::min(num_probes, dense_cache.get_points().size());
        for(uint64_t i = 0; i < num_probes; i++)
        {
            size_t ind = (size_t)(((float)rand() / RAND_MAX) * dense_cache.get_points().size());
            OctreeNode node = (*lookup_node_at_position(dense_cache.get_points().at(ind).get_position()));
            information_loss += compare_metadata(node.get_aggregate_metadata(), dense_cache.get_points_metadata().at(ind)) / (double)num_probes;
            //            printf("\nIntermediate information loss: %lf\n", information_loss);
        }
        printf("\nFinal information loss: %lf%%\n", information_loss * 100);
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
        //        printf("\nEnter glue pairs\n");
        for(uint64_t i = 0; i < dense_cache.get_points().size(); i++)
        {
            PointMetaDataPair<DensePoint, DenseMetaData> pair(dense_cache.get_points().at(i), dense_cache.get_points_metadata().at(i));
            this->_pairs.push_back(pair);
        }
    }
};
};
#endif // LAMURE_SPARSEOCTREE_H
