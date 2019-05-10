#include "utils.h"

struct JoinOperation
{
  private:
    uint32_t chart1_id;
    uint32_t chart2_id;

  public:
    double cost;

    bool active = true;

    JoinOperation(uint32_t _c1, uint32_t _c2) : chart1_id(_c1), chart2_id(_c2)
    {
        order_chart_ids();
        cost = 0;
    }
    JoinOperation(uint32_t _c1, uint32_t _c2, double _cost) : chart1_id(_c1), chart2_id(_c2), cost(_cost)
    {
        if(_c1 == _c2)
        {
            std::cerr << "ERROR: Join added between the same chart! (chart " << _c1 << ")\n";
        }

        order_chart_ids();
    }

    // maintains ascending order of chart 1 to chart 2
    void order_chart_ids()
    {
        if(chart1_id > chart2_id)
        {
            uint32_t temp = chart1_id;
            chart1_id = chart2_id;
            chart2_id = temp;
        }
    }

    void set_chart1_id(uint32_t id)
    {
        chart1_id = id;
        order_chart_ids();
    }
    void set_chart2_id(uint32_t id)
    {
        chart2_id = id;
        order_chart_ids();
    }
    uint32_t get_chart1_id() { return chart1_id; }
    uint32_t get_chart2_id() { return chart2_id; }

    void replace_id_with(uint32_t to_replace, uint32_t replace_with)
    {
        if(get_chart1_id() == to_replace)
        {
            set_chart1_id(replace_with);
        }
        if(get_chart2_id() == to_replace)
        {
            set_chart2_id(replace_with);
        }
        order_chart_ids();
    }

    // calculates cost of joining these 2 charts
    static double cost_of_join(Chart& c1, Chart& c2, CLUSTER_SETTINGS& cluster_settings)
    {
        double error = 0;

        if(c1.area == 0 || c2.area == 0)
        {
            // std::cout << "0 area: " << c2.id << std::endl;
            return std::numeric_limits<float>::max();
        }

        Vector fit_plane_normal;

        double e_fit = cluster_settings.e_fit_cf * Chart::get_fit_error(c1, c2, fit_plane_normal);

        // std::cout << "e_fit " << e_fit;

        fit_plane_normal = utils::normalise(fit_plane_normal);

        double e_direction = cluster_settings.e_ori_cf * Chart::get_direction_error(c1, c2, fit_plane_normal);

        // std::cout << "e_direction " << e_direction;

        double e_shape = 0.f; //   cluster_settings.e_shape_cf * Chart::get_compactness_of_merged_charts(c1,c2);

        // std::cout << "e_shape " << e_shape << std::endl;

        error = std::abs(e_fit) + std::abs(e_direction) + std::abs(e_shape);

        if(std::isnan(error))
        {
            std::cout << "Nan error [" << c1.id << ", " << c2.id << "]: " << error << ", e_fit: " << e_fit << ", e_ori: " << e_direction << ", e_shape: " << e_shape << std::endl;
        }
        /*
            float area_coeff = 0;
            if (c1.area > c2.area) {
              area_coeff = c2.area / c1.area;
            }
            else {
              area_coeff = c1.area / c2.area;
            }

            if (area_coeff < 0.1) {
              error *= 0.5;
            }
        */
        return error;
    }

    static bool sort_joins(JoinOperation j1, JoinOperation j2) { return (j1.cost < j2.cost); }

    static bool sort_join_ptrs(std::shared_ptr<JoinOperation> j1, std::shared_ptr<JoinOperation> j2) { return (j1->cost < j2->cost); }

    static bool compare(std::shared_ptr<JoinOperation> j1, std::shared_ptr<JoinOperation> j2) { return (j1->get_chart1_id() == j2->get_chart1_id() && j1->get_chart2_id() == j2->get_chart2_id()); }

    int results_in_chart_with_neighbours(std::vector<Chart>& charts, std::map<uint32_t, uint32_t>& chart_id_map)
    {
        Chart& c1 = charts[this->chart1_id];
        Chart& c2 = charts[this->chart2_id];

        int combined_nbrs = c1.has_border_edge | c2.has_border_edge;

        // get number of neighbours per chart
        c1.create_neighbour_set(chart_id_map);
        c2.create_neighbour_set(chart_id_map);

        combined_nbrs += c1.neighbour_charts.size();
        combined_nbrs += c2.neighbour_charts.size();

        return combined_nbrs - 2;
    }
};
