// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_SURFEL_MEM_ARRAY_H_
#define PRE_SURFEL_MEM_ARRAY_H_

#include <lamure/pre/array_abstract.h>
#include <lamure/pre/surfel.h>
#include <lamure/pre/prov.h>

namespace lamure
{
namespace pre
{

class surfel_ext {

public:

  surfel surfel_;
  prov prov_;


  static bool compare_x(const surfel_ext &left_surfel, const surfel_ext &right_surfel) {
    return left_surfel.surfel_.pos().x < right_surfel.surfel_.pos().x;
  }

  static bool compare_y(const surfel_ext &left_surfel, const surfel_ext &right_surfel) {
    return left_surfel.surfel_.pos().y < right_surfel.surfel_.pos().y;
  }

  static bool compare_z(const surfel_ext &left_surfel, const surfel_ext &right_surfel) {
    return left_surfel.surfel_.pos().z < right_surfel.surfel_.pos().z;
  }

  static std::function<bool(const surfel_ext &left, const surfel_ext &right)> compare(const uint8_t axis) {
    assert(axis <= 2);
    switch (axis) {
        case 0: return &surfel_ext::compare_x;
            break;
        case 1: return &surfel_ext::compare_y;
            break;
        case 2: return &surfel_ext::compare_z;
            break;
    }
  }
};

class PREPROCESSING_DLL surfel_mem_array: public array_abstract<surfel>
{
public:

    explicit surfel_mem_array()
        : array_abstract<surfel>()
    { reset(); }

    explicit surfel_mem_array(const surfel_mem_array &other,
                              const size_t offset,
                              const size_t length)
        : array_abstract<surfel>(), has_provenance_(other.has_provenance_)
    {
      if (has_provenance_) { 
        reset(other.surfel_mem_data_, other.prov_mem_data_, offset, length);
      }
      else {
        reset(other.surfel_mem_data_, offset, length);
      }
    }

    //to be removed
    explicit surfel_mem_array(const std::shared_ptr<std::vector<surfel>> &surfel_mem_data,
                              const size_t offset,
                              const size_t length)
        : array_abstract<surfel>(), has_provenance_(false)
    { reset(surfel_mem_data, offset, length); }

    explicit surfel_mem_array(const std::shared_ptr<std::vector<surfel>> &surfel_mem_data,
                              const std::shared_ptr<std::vector<prov>> &prov_mem_data,
                              const size_t offset,
                              const size_t length)
        : array_abstract<surfel>(), has_provenance_(true)
    { reset(surfel_mem_data, prov_mem_data, offset, length); }


    surfel read_surfel(const size_t index) const override;
    surfel const &read_surfel_ref(const size_t index) const;
    void write_surfel(const surfel &surfel, const size_t index) const override;

    prov read_prov(const size_t index) const;
    prov const &read_prov_ref(const size_t index) const;
    void write_prov(const prov &surfel, const size_t index) const;

    std::shared_ptr<std::vector<surfel>> & surfel_mem_data() { return surfel_mem_data_; }
    const std::shared_ptr<std::vector<surfel>> & surfel_mem_data() const { return surfel_mem_data_; }

    std::shared_ptr<std::vector<prov>> & prov_mem_data() { return prov_mem_data_; }
    const std::shared_ptr<std::vector<prov>> & prov_mem_data() const { return prov_mem_data_; }

    void get(std::vector<surfel_ext>& data);
    void set(std::vector<surfel_ext>& data);

    void reset() override;

    //to be removed
    void reset(const std::shared_ptr<std::vector<surfel>> &mem_data,
               const size_t offset,
               const size_t length);

    void reset(const std::shared_ptr<std::vector<surfel>> &surfel_mem_data,
               const std::shared_ptr<std::vector<prov>> &prov_mem_data,
               const size_t offset,
               const size_t length);

    bool has_provenance() const { return has_provenance_; }

protected:

    std::shared_ptr<std::vector<surfel>> surfel_mem_data_;
    std::shared_ptr<std::vector<prov>> prov_mem_data_;

    bool has_provenance_;

};

template<>
struct array_traits<surfel_mem_array>
{
    static const bool is_out_of_core = false;
    static const bool is_in_core = true;
};

} // namespace pre
} // namespace lamure

#endif // PRE_SURFEL_MEM_ARRAY_H_
