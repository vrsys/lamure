#ifndef STATISTIC_H
#define STATISTIC_H


#include <vector>

class Statistic{

 public:
  Statistic();
  ~Statistic();

  void add(const float& v);
  void calc();
  float getMean() const;
  float getSD() const;

 private:
  std::vector<float> m_values;
  float m_mean;
  float m_sd;

};




#endif // #ifndef STATISTIC_H
