#include "vns_loop_functions.h"

namespace argos {

   /****************************************/
   /****************************************/

   CVNSLoopFunctions::CVNSLoopFunctions() :
      m_cSpace(CSimulator::GetInstance().GetSpace()) {}

   /****************************************/
   /****************************************/

   void CVNSLoopFunctions::Init(TConfigurationNode& t_tree) {}

   /****************************************/
   /****************************************/

   CColor CVNSLoopFunctions::GetFloorColor(const CVector2& c_position) {
      return CColor::GRAY90;
   }

   /****************************************/
   /****************************************/

   REGISTER_LOOP_FUNCTIONS(CVNSLoopFunctions, "vns_loop_functions");

}
