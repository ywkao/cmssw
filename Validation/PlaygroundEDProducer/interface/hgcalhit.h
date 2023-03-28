#ifndef __hgcalhit__
#define __hgcalhit__

#include <iostream>

#include "DetectorId.h"

class RecHit {
public:
  RecHit(){;}
  RecHit(int event, DetectorId detid, float adc, float adcm, float adc_cm, int toa, int tot, int latency) :
         m_event(event), m_detid(detid), m_adc(adc), m_toa(toa), m_triglatency(latency) {}
  ~RecHit(){}

  inline int event() const {return m_event;}
  inline DetectorId detid() const {return m_detid;}
  inline float adc() const {return m_adc;}
  inline float adcm() const {return m_adcm;}
  inline float adc_cm() const {return m_adc_cm;}
  inline float toa() const {return m_toa;}
  inline float tot() const {return m_tot;}
  inline int triglatency() const {return m_triglatency;}

  inline void set_adc(float adc){m_adc=adc;}
  inline void set_adc_CM(float adc){m_adc_cm=adc;}

  friend std::ostream& operator<<(std::ostream& out,const RecHit& rs);  

private:
  int m_event;
  DetectorId m_detid;
  float m_adc;
  float m_adcm;
  float m_adc_cm;
  float m_toa;
  float m_tot;
  int m_triglatency;

  // variables expected to have
  float energy_fC;
  float energy_MIP;
  float energy_GeV;
  float time_ns;
  int32_t flags;
};

#endif
