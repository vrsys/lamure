// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_surfel_disk_array_H_
#define PRE_surfel_disk_array_H_

#include <lamure/pre/array_abstract.h>
#include <lamure/pre/io/file.h>
#include <lamure/pre/surfel.h>
#include <lamure/pre/prov.h>

namespace lamure
{
namespace pre
{

class PREPROCESSING_DLL surfel_disk_array: public array_abstract<surfel>
{
public:

    explicit surfel_disk_array()
        : array_abstract<surfel>() { 
      reset(); 
    }

    explicit surfel_disk_array(const surfel_disk_array &other,
                               const size_t offset,
                               const size_t length)
        : array_abstract<surfel>(), has_provenance_(other.has_provenance_) {
      if (has_provenance_) {
        reset(other.surfel_file_, other.prov_file_, offset, length); 
      }
      else {
        reset(other.surfel_file_, offset, length); 
      }
    }

    //to be removed
    explicit surfel_disk_array(const std::shared_ptr<file<surfel>> &surfel_file,
                               const size_t offset,
                               const size_t length)
        : array_abstract<surfel>(), has_provenance_(false) { 
      reset(surfel_file, offset, length); 
    }

    explicit surfel_disk_array(const std::shared_ptr<file<surfel>> &surfel_file,
                               const std::shared_ptr<file<prov>> &prov_file,
                               const size_t offset,
                               const size_t length)
        : array_abstract<surfel>(), has_provenance_(true) { 
      reset(surfel_file, prov_file, offset, length);
    }

    surfel read_surfel(const size_t index) const override;
    void write_surfel(const surfel &surfel, const size_t index) const override;

    prov read_prov(const size_t index) const;
    void write_prov(const prov &prov, const size_t index) const;

    std::shared_ptr<file<surfel>> &get_file() { return surfel_file_; }
    const std::shared_ptr<file<surfel>> &get_file() const { return surfel_file_; }

    std::shared_ptr<file<prov>> &get_prov_file() { return prov_file_; }
    const std::shared_ptr<file<prov>> &get_prov_file() const { return prov_file_; }

    void reset() override;

    //to be removed
    void reset(const std::shared_ptr<file<surfel>> &surfel_file,
               const size_t offset,
               const size_t length);
    void reset(const std::shared_ptr<file<surfel>> &surfel_file,
               const std::shared_ptr<file<prov>> &prov_file,
               const size_t offset,
               const size_t length);

    //to be removed
    std::shared_ptr<std::vector<surfel>> read_all() const;

    std::shared_ptr<std::vector<prov>> read_all_prov() const;

    //to be removed
    void write_all(const std::shared_ptr<std::vector<surfel>> &surfel_data,
                   const size_t offset_in_vector);

    void write_all(const std::shared_ptr<std::vector<surfel>> &surfel_data,
                   const std::shared_ptr<std::vector<prov>> &prov_data,
                   const size_t offset_in_vector);


    bool has_provenance() const { return has_provenance_; }

protected:

    std::shared_ptr<file<surfel>> surfel_file_;
    std::shared_ptr<file<prov>> prov_file_;

    bool has_provenance_;

};

template<>
struct array_traits<surfel_disk_array>
{
    static const bool is_out_of_core = true;
    static const bool is_in_core = false;
};


} // namespace pre
} // namespace lamure


#endif // PRE_surfel_disk_array_H_
