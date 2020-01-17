#include <argos3/core/simulator/simulator.h>
#include <argos3/core/simulator/space/space.h>
#include <argos3/core/simulator/entity/composable_entity.h>

#include <argos3/plugins/simulator/visualizations/qt-opengl/qtopengl_render.h>
#include <argos3/plugins/simulator/visualizations/qt-opengl/qtopengl_main_window.h>
#include <argos3/plugins/simulator/entities/debug_entity.h>

#include "vns_qtopengl_user_functions.h"

#define GL_NUMBER_VERTICES 36u

namespace argos {

   /********************************************************************************/
   /********************************************************************************/

   CVNSQTOpenGLUserFunctions::CVNSQTOpenGLUserFunctions() :
      m_cSpace(CSimulator::GetInstance().GetSpace()),
      m_unCameraIndex(0),
      m_unLastSimulationClock(-1) {
      RegisterUserFunction<CVNSQTOpenGLUserFunctions, CPiPuckEntity>(&CVNSQTOpenGLUserFunctions::Annotate);
      RegisterUserFunction<CVNSQTOpenGLUserFunctions, CDroneEntity>(&CVNSQTOpenGLUserFunctions::Annotate);
   }

   /********************************************************************************/
   /********************************************************************************/

   void CVNSQTOpenGLUserFunctions::Init(TConfigurationNode& t_tree) {
      if(NodeExists(t_tree, "camera_paths")) {
         TConfigurationNode& tCameraPaths = GetNode(t_tree, "camera_paths");
         GetNodeAttribute(tCameraPaths, "use_camera", m_unCameraIndex);
         if(m_unCameraIndex > 11) {
            THROW_ARGOSEXCEPTION("use_camera must be in the range [0:11].");
         }
         TConfigurationNodeIterator itPath("path");
         UInt32 unDuration, unStartFocalLength, unEndFocalLength;
         CVector3 cStartPosition, cEndPosition, cStartLookAt, cEndLookAt, cStartUp, cEndUp;
         for(itPath = itPath.begin(&tCameraPaths); itPath != itPath.end(); ++itPath) {
            GetNodeAttribute(*itPath, "duration", unDuration);
            TConfigurationNode tStartNode = GetNode(*itPath, "start");
            GetNodeAttribute(tStartNode, "position", cStartPosition);
            GetNodeAttribute(tStartNode, "look_at", cStartLookAt);
            GetNodeAttributeOrDefault(tStartNode, "up", cStartUp, cStartUp);
            GetNodeAttribute(tStartNode, "lens_focal_length", unStartFocalLength);
            TConfigurationNode tEndNode = GetNode(*itPath, "end");
            GetNodeAttribute(tEndNode, "position", cEndPosition);
            GetNodeAttribute(tEndNode, "look_at", cEndLookAt);
            GetNodeAttributeOrDefault(tEndNode, "up", cEndUp, cEndUp);
            GetNodeAttribute(tEndNode, "lens_focal_length", unEndFocalLength);
            m_vecCameraPaths.emplace_back(unDuration,
                                          unStartFocalLength,
                                          unEndFocalLength,
                                          cStartPosition,
                                          cEndPosition,
                                          cStartLookAt,
                                          cEndLookAt,
                                          cStartUp,
                                          cEndUp);
         }
      }
   }

   /********************************************************************************/
   /********************************************************************************/

   void CVNSQTOpenGLUserFunctions::DrawInWorld() {
      UInt32 unSimulationClock = m_cSpace.GetSimulationClock();
      if(m_unLastSimulationClock != unSimulationClock) {
         m_unLastSimulationClock = unSimulationClock;
         /* get camera settings */
         CQTOpenGLCamera::SSettings& sSettings = 
            GetQTOpenGLWidget().GetCamera().GetSetting(m_unCameraIndex);
         /* find current camera path */
         UInt32 unPreviousPathDurations = 0;
         for(const SCameraPath& s_camera_path : m_vecCameraPaths) {
            if(unPreviousPathDurations + s_camera_path.Duration < unSimulationClock) {
               unPreviousPathDurations += s_camera_path.Duration;
            }
            else {
               Real fPathTimeFraction =
                  static_cast<Real>(unSimulationClock - unPreviousPathDurations) /
                  static_cast<Real>(s_camera_path.Duration);
               CVector3 cPosition(s_camera_path.EndPosition - s_camera_path.StartPosition);
               cPosition *= fPathTimeFraction;
               cPosition += s_camera_path.StartPosition;
               CVector3 cLookAt(s_camera_path.EndLookAt - s_camera_path.StartLookAt);
               cLookAt *= fPathTimeFraction;
               cLookAt += s_camera_path.StartLookAt;
               CVector3 cUp(s_camera_path.EndUp - s_camera_path.StartUp);
               cUp *= fPathTimeFraction;
               cUp += s_camera_path.StartUp;
               sSettings.Position = cPosition;
               sSettings.Target = cLookAt;
               sSettings.Forward = 
                  (sSettings.Target - sSettings.Position).Normalize();
               if(cUp == CVector3::ZERO) {
                  if(sSettings.Forward.GetX() != 0 || sSettings.Forward.GetY() != 0) {
                     CVector2 cLeftXY(sSettings.Forward.GetX(), sSettings.Forward.GetY());
                     cLeftXY.Perpendicularize();
                     sSettings.Left.Set(cLeftXY.GetX(), cLeftXY.GetY(), 0.0f);
                     sSettings.Left.Normalize();
                  }
                  else {
                     sSettings.Left.Set(0.0f, 1.0f, 0.0f);
                  }
                  /* Calculate the Up vector with a cross product */
                  sSettings.Up = sSettings.Forward;
                  sSettings.Up.CrossProduct(sSettings.Left).Normalize();
               }
               else {
                  sSettings.Up = cUp;
                  sSettings.Up.Normalize();
                  /* Calculate the Left vector with a cross product */
                  sSettings.Left = sSettings.Up;
                  sSettings.Left.CrossProduct(sSettings.Forward).Normalize();
               }
               break;
            }
         }
      }
   }

   /********************************************************************************/
   /********************************************************************************/

   CVNSQTOpenGLUserFunctions::~CVNSQTOpenGLUserFunctions() {}

   /********************************************************************************/
   /********************************************************************************/

   void CVNSQTOpenGLUserFunctions::Annotate(CDebugEntity& c_debug_entity,
                                            const SAnchor& s_anchor) {
      glDisable(GL_LIGHTING);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      const CVector3& cPosition = s_anchor.Position;
      const CQuaternion& cOrientation = s_anchor.Orientation;
      CRadians cZAngle, cYAngle, cXAngle;
      cOrientation.ToEulerAngles(cZAngle, cYAngle, cXAngle);
      glPushMatrix();
      glTranslatef(cPosition.GetX(), cPosition.GetY(), cPosition.GetZ());
      glRotatef(ToDegrees(cXAngle).GetValue(), 1.0f, 0.0f, 0.0f);
      glRotatef(ToDegrees(cYAngle).GetValue(), 0.0f, 1.0f, 0.0f);
      glRotatef(ToDegrees(cZAngle).GetValue(), 0.0f, 0.0f, 1.0f);

      std::istringstream issInstructions, issArgument;
      issInstructions.str(c_debug_entity.GetBuffer("draw"));
      for(std::string strInstruction; std::getline(issInstructions, strInstruction); ) {
         std::vector<std::string> vecArguments;
         Tokenize(strInstruction, vecArguments, "()");
         if(vecArguments.size() == 4 && vecArguments[0] == "arrow") {
            CColor cColor;
            CVector3 cTo, cFrom;
            std::istringstream(vecArguments[1]) >> cColor;
            std::istringstream(vecArguments[2]) >> cFrom;
            std::istringstream(vecArguments[3]) >> cTo;
            glColor4ub(cColor.GetRed(), cColor.GetGreen(), cColor.GetBlue(), 128u);
            DrawArrow3(cFrom, cTo);
         }
         else if(vecArguments.size() == 4 && vecArguments[0] == "ring") {
            CColor cColor;
            CVector3 cCenter;
            Real fRadius;
            std::istringstream(vecArguments[1]) >> cColor;
            std::istringstream(vecArguments[2]) >> cCenter;
            std::istringstream(vecArguments[3]) >> fRadius;
            glColor4ub(cColor.GetRed(), cColor.GetGreen(), cColor.GetBlue(), 128u);
            DrawRing3(cCenter, fRadius);
         }
      }
      glPopMatrix();
      glDisable(GL_BLEND);
      glEnable(GL_LIGHTING);
   }

   /********************************************************************************/
   /********************************************************************************/

   void CVNSQTOpenGLUserFunctions::DrawRing3(const CVector3& c_center, Real f_radius) {
      const CCachedShapes& cCachedShapes = CCachedShapes::GetCachedShapes();
      const Real fRingHeight = 0.015625;
      const Real fRingThickness = 0.015625;
      const Real fHalfRingThickness = fRingThickness * 0.5;
      const Real fDiameter = 2.0 * f_radius;
      /* draw inner ring surface */
      glPushMatrix();
      glTranslatef(c_center.GetX(), c_center.GetY(), c_center.GetZ());
      glScalef(fDiameter, fDiameter, fRingHeight);
      glCallList(cCachedShapes.GetRing());
      glPopMatrix();
      /* draw outer ring surface */
      glPushMatrix();
      glTranslatef(c_center.GetX(), c_center.GetY(), c_center.GetZ());
      glScalef(fDiameter + fRingThickness, fDiameter + fRingThickness, fRingHeight);
      glCallList(cCachedShapes.GetRing());
      glPopMatrix();
      /* draw top */
      glPushMatrix();
      glTranslatef(c_center.GetX(), c_center.GetY(), c_center.GetZ());
      CVector2 cInnerVertex(f_radius, 0.0f);
      CVector2 cOuterVertex(f_radius + fHalfRingThickness, 0.0f);
      const CRadians cAngle(CRadians::TWO_PI / GL_NUMBER_VERTICES);
      glBegin(GL_QUAD_STRIP);
      glNormal3f(0.0f, 0.0f, 1.0f);
      for(GLuint i = 0; i <= GL_NUMBER_VERTICES; i++) {
         glVertex3f(cInnerVertex.GetX(), cInnerVertex.GetY(), fRingHeight);
         glVertex3f(cOuterVertex.GetX(), cOuterVertex.GetY(), fRingHeight);
         cInnerVertex.Rotate(cAngle);
         cOuterVertex.Rotate(cAngle);
      }
      glEnd();
      glPopMatrix();
   }

   /********************************************************************************/
   /********************************************************************************/

   void CVNSQTOpenGLUserFunctions::DrawArrow3(const CVector3& c_from, const CVector3& c_to) {
      const CCachedShapes& cCachedShapes = CCachedShapes::GetCachedShapes();
      const Real fArrowThickness = 0.015625f;
      const Real fArrowHead =      0.031250f;
      CVector3 cArrow(c_to - c_from);
      CQuaternion cRotation(CVector3::Z, cArrow / cArrow.Length());
      CRadians cZAngle, cYAngle, cXAngle;
      cRotation.ToEulerAngles(cZAngle, cYAngle, cXAngle);
      /* draw arrow body */
      glPushMatrix();
      glTranslatef(c_to.GetX(), c_to.GetY(), c_to.GetZ());
      glRotatef(ToDegrees(cXAngle).GetValue(), 1.0f, 0.0f, 0.0f);
      glRotatef(ToDegrees(cYAngle).GetValue(), 0.0f, 1.0f, 0.0f);
      glRotatef(ToDegrees(cZAngle).GetValue(), 0.0f, 0.0f, 1.0f);
      glScalef(fArrowHead, fArrowHead, fArrowHead);
      glCallList(cCachedShapes.GetCone());
      glPopMatrix();
      /* draw arrow head */
      glPushMatrix();
      glTranslatef(c_from.GetX(), c_from.GetY(), c_from.GetZ());
      glRotatef(ToDegrees(cXAngle).GetValue(), 1.0f, 0.0f, 0.0f);
      glRotatef(ToDegrees(cYAngle).GetValue(), 0.0f, 1.0f, 0.0f);
      glRotatef(ToDegrees(cZAngle).GetValue(), 0.0f, 0.0f, 1.0f);
      glScalef(fArrowThickness, fArrowThickness, cArrow.Length() - fArrowHead);
      glCallList(cCachedShapes.GetCylinder());
      glPopMatrix();
   }

   /********************************************************************************/
   /********************************************************************************/

   void CVNSQTOpenGLUserFunctions::CCachedShapes::MakeCylinder() {
      /* Side surface */
      CVector2 cVertex(0.5f, 0.0f);
      CRadians cAngle(CRadians::TWO_PI / GL_NUMBER_VERTICES);
      glBegin(GL_QUAD_STRIP);
      for(GLuint i = 0; i <= GL_NUMBER_VERTICES; i++) {
         glNormal3f(cVertex.GetX(), cVertex.GetY(), 0.0f);
         glVertex3f(cVertex.GetX(), cVertex.GetY(), 1.0f);
         glVertex3f(cVertex.GetX(), cVertex.GetY(), 0.0f);
         cVertex.Rotate(cAngle);
      }
      glEnd();
      /* Top disk */
      cVertex.Set(0.5f, 0.0f);
      glBegin(GL_POLYGON);
      glNormal3f(0.0f, 0.0f, 1.0f);
      for(GLuint i = 0; i <= GL_NUMBER_VERTICES; i++) {
         glVertex3f(cVertex.GetX(), cVertex.GetY(), 1.0f);
         cVertex.Rotate(cAngle);
      }
      glEnd();
      /* Bottom disk */
      cVertex.Set(0.5f, 0.0f);
      cAngle = -cAngle;
      glBegin(GL_POLYGON);
      glNormal3f(0.0f, 0.0f, -1.0f);
      for(GLuint i = 0; i <= GL_NUMBER_VERTICES; i++) {
         glVertex3f(cVertex.GetX(), cVertex.GetY(), 0.0f);
         cVertex.Rotate(cAngle);
      }
      glEnd();
   }

   /********************************************************************************/
   /********************************************************************************/

   void CVNSQTOpenGLUserFunctions::CCachedShapes::MakeCone() {
      /* Cone surface */
      CVector2 cVertex(0.5f, 0.0f);
      CRadians cAngle(CRadians::TWO_PI / GL_NUMBER_VERTICES);
      glBegin(GL_QUAD_STRIP);
      for(GLuint i = 0; i <= GL_NUMBER_VERTICES; i++) {
         glNormal3f(cVertex.GetX(), cVertex.GetY(), 0.0f);
         glVertex3f(0.0f, 0.0f, 0.0f);
         glVertex3f(cVertex.GetX(), cVertex.GetY(), -1.0f);
         cVertex.Rotate(cAngle);
      }
      glEnd();
      /* Bottom disk */
      cVertex.Set(0.5f, 0.0f);
      cAngle = -cAngle;
      glBegin(GL_POLYGON);
      glNormal3f(0.0f, 0.0f, -1.0f);
      for(GLuint i = 0; i <= GL_NUMBER_VERTICES; i++) {
         glVertex3f(cVertex.GetX(), cVertex.GetY(), -1.0f);
         cVertex.Rotate(cAngle);
      }
      glEnd();
   }

   /********************************************************************************/
   /********************************************************************************/

   void CVNSQTOpenGLUserFunctions::CCachedShapes::MakeRing() {
      CVector2 cVertex;
      const CRadians cAngle(CRadians::TWO_PI / GL_NUMBER_VERTICES);
      /* draw front surface */
      cVertex.Set(0.5f, 0.0f);     
      glBegin(GL_QUAD_STRIP);
      for(GLuint i = 0; i <= GL_NUMBER_VERTICES; i++) {
         glNormal3f(cVertex.GetX(), cVertex.GetY(), 0.0f);
         glVertex3f(cVertex.GetX(), cVertex.GetY(), 1.0f);
         glVertex3f(cVertex.GetX(), cVertex.GetY(), 0.0f);
         cVertex.Rotate(cAngle);
      }
      glEnd();
      /* draw back surface */
      cVertex.Set(0.5f, 0.0f);     
      glBegin(GL_QUAD_STRIP);
      for(GLuint i = 0; i <= GL_NUMBER_VERTICES; i++) {
         glNormal3f(cVertex.GetX(), cVertex.GetY(), 0.0f);
         glVertex3f(cVertex.GetX(), cVertex.GetY(), 0.0f);
         glVertex3f(cVertex.GetX(), cVertex.GetY(), 1.0f);
         cVertex.Rotate(cAngle);
      }
      glEnd();
   }

   /********************************************************************************/
   /********************************************************************************/

   REGISTER_QTOPENGL_USER_FUNCTIONS(CVNSQTOpenGLUserFunctions, "vns_qtopengl_user_functions");

   /********************************************************************************/
   /********************************************************************************/

}
