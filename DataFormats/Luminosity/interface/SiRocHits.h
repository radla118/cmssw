#ifndef SIROCHITS_h
#define SIROCHITS_h

#include <ctime>
#include <map>
#include <string>

namespace reco {
  class SiRocHits {

  public:

    SiRocHits() : m_eventID() {};
/*
    void fillRocs(int detid, int rocid) {
      m_rocID.push_back(rocid);
      m_detID.push_back(detid);
    }
*/
/*
    void fillRocs(int detid, int rocid, int adc){
      //m_ID.push_back(std::make_pair(detid,rocid));
      m_rocID.push_back(rocid);
      m_detID.push_back(detid);
      m_adc.push_back(adc);
    }
*/
    void fillCounts(int detid, int rocid, int counts){
      m_rocID.push_back((long int)detid*100+rocid); //unique ID for each ROC
      //m_rocID.push_back(rocid);
      //m_detID.push_back(detid);
      m_counts.push_back(counts);
    }
   
/*
    void updateRocs(int key, int adc){
        m_adc[key] = adc;
    }
*/
    void seteventID(unsigned int inputeventID) { m_eventID = inputeventID ;} 

    std::vector<long int> const& rocID() const { return (m_rocID); }
    //std::vector<int> const& detID() const { return (m_detID); }
    //std::pair<int,int> const& ID() const { return (m_ID); }
    unsigned int const& eventID() const { return (m_eventID); }

  private:
    std::vector<long int> m_rocID;
    //std::vector<int> m_detID;
    //std::vector<std::pair<int,int>> m_ID;
    //std::vector<int> m_adc;
    std::vector<int> m_counts; 
    unsigned int m_eventID;
    
  };
}
#endif 
