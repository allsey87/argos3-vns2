#ifndef VNS_LOOP_FUNCTIONS_H
#define VNS_LOOP_FUNCTIONS_H

#include <argos3/core/simulator/space/space.h>
#include <argos3/core/simulator/loop_functions.h>

namespace argos {

   class CVNSLoopFunctions : public CLoopFunctions {

   public:

      CVNSLoopFunctions();

      virtual ~CVNSLoopFunctions() {}

      virtual void Init(TConfigurationNode& t_tree);

      //virtual void Reset();

      //virtual void Destroy();

      //virtual void PreStep();

      //virtual void PostStep();

      virtual CColor GetFloorColor(const CVector2& c_position);

   private:
      CSpace& m_cSpace;

   };


}

#endif

