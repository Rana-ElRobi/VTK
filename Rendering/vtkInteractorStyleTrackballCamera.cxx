/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackballCamera.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkCommand.h"

vtkCxxRevisionMacro(vtkInteractorStyleTrackballCamera, "1.23");
vtkStandardNewMacro(vtkInteractorStyleTrackballCamera);

//----------------------------------------------------------------------------
vtkInteractorStyleTrackballCamera::vtkInteractorStyleTrackballCamera() 
{
  this->MotionFactor   = 10.0;

  // This prevent vtkInteractorStyle::StartState to fire the timer
  // that is used to handle joystick mode

  this->UseTimers = 0;
}

//----------------------------------------------------------------------------
vtkInteractorStyleTrackballCamera::~vtkInteractorStyleTrackballCamera() 
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::OnMouseMove(int vtkNotUsed(ctrl), 
                                                    int vtkNotUsed(shift),
                                                    int x, 
                                                    int y) 
{ 
  switch (this->State) 
    {
    case VTKIS_ROTATE:
      this->FindPokedRenderer(x, y);
      this->Rotate();
      break;

    case VTKIS_PAN:
      this->FindPokedRenderer(x, y);
      this->Pan();
      break;

    case VTKIS_DOLLY:
      this->FindPokedRenderer(x, y);
      this->Dolly();
      break;

    case VTKIS_SPIN:
      this->FindPokedRenderer(x, y);
      this->Spin();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::OnLeftButtonDown(int ctrl, 
                                                         int shift, 
                                                         int x, 
                                                         int y) 
{ 
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  if (shift) 
    {
    if (ctrl) 
      {
      this->StartDolly();
      }
    else 
      {
      this->StartPan();
      }
    } 
  else 
    {
    if (ctrl) 
      {
      this->StartSpin();
      }
    else 
      {
      this->StartRotate();
      }
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::OnLeftButtonUp(int vtkNotUsed(ctrl),
                                                       int vtkNotUsed(shift), 
                                                       int vtkNotUsed(x),
                                                       int vtkNotUsed(y))
{
  switch (this->State) 
    {
    case VTKIS_DOLLY:
      this->EndDolly();
      break;

    case VTKIS_PAN:
      this->EndPan();
      break;

    case VTKIS_SPIN:
      this->EndSpin();
      break;

    case VTKIS_ROTATE:
      this->EndRotate();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::OnMiddleButtonDown(int vtkNotUsed(ctrl),
                                                           int vtkNotUsed(shift), 
                                                           int x, 
                                                           int y) 
{
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartPan();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::OnMiddleButtonUp(int vtkNotUsed(ctrl),
                                                         int vtkNotUsed(shift), 
                                                         int vtkNotUsed(x),
                                                         int vtkNotUsed(y))
{
  switch (this->State) 
    {
    case VTKIS_PAN:
      this->EndPan();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::OnRightButtonDown(int vtkNotUsed(ctrl),
                                                          int vtkNotUsed(shift), 
                                                          int x, 
                                                          int y) 
{
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartDolly();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::OnRightButtonUp(int vtkNotUsed(ctrl),
                                                        int vtkNotUsed(shift), 
                                                        int vtkNotUsed(x),
                                                        int vtkNotUsed(y))
{
  switch (this->State) 
    {
    case VTKIS_DOLLY:
      this->EndDolly();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::Rotate()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;

  int dx = rwi->GetEventPosition()[0] - rwi->GetLastEventPosition()[0];
  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
  
  int *size = this->CurrentRenderer->GetRenderWindow()->GetSize();

  float delta_elevation = -20.0 / size[1];
  float delta_azimuth = -20.0 / size[0];
  
  double rxf = (double)dx * delta_azimuth * this->MotionFactor;
  double ryf = (double)dy * delta_elevation * this->MotionFactor;
  
  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  camera->Azimuth(rxf);
  camera->Elevation(ryf);
  camera->OrthogonalizeViewUp();

  this->ResetCameraClippingRange();

  if (rwi->GetLightFollowCamera())
    {
    vtkLight* light = this->CurrentRenderer->GetFirstLight();
    if (light != NULL) 
      {
      light->SetPosition(camera->GetPosition());
      light->SetFocalPoint(camera->GetFocalPoint());
      }
    }   

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::Spin()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;

  float *center = this->CurrentRenderer->GetCenter();

  double newAngle = 
    atan2((double)rwi->GetEventPosition()[1] - (double)center[1],
          (double)rwi->GetEventPosition()[0] - (double)center[0]);

  double oldAngle = 
    atan2((double)rwi->GetLastEventPosition()[1] - (double)center[1],
          (double)rwi->GetLastEventPosition()[0] - (double)center[0]);
  
  newAngle *= vtkMath::RadiansToDegrees();
  oldAngle *= vtkMath::RadiansToDegrees();

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  camera->Roll(newAngle - oldAngle);
  camera->OrthogonalizeViewUp();
      
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::Pan()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;

  double viewFocus[4], focalDepth, viewPoint[3];
  float newPickPoint[4], oldPickPoint[4], motionVector[3];
  
  // Calculate the focal depth since we'll be using it a lot

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  camera->GetFocalPoint(viewFocus);
  this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1], viewFocus[2], 
                              viewFocus);
  focalDepth = viewFocus[2];

  this->ComputeDisplayToWorld((double)rwi->GetEventPosition()[0], 
                              (double)rwi->GetEventPosition()[1],
                              focalDepth, 
                              newPickPoint);
    
  // Has to recalc old mouse point since the viewport has moved,
  // so can't move it outside the loop

  this->ComputeDisplayToWorld((double)rwi->GetLastEventPosition()[0],
                              (double)rwi->GetLastEventPosition()[1],
                              focalDepth, 
                              oldPickPoint);
  
  // Camera motion is reversed

  motionVector[0] = oldPickPoint[0] - newPickPoint[0];
  motionVector[1] = oldPickPoint[1] - newPickPoint[1];
  motionVector[2] = oldPickPoint[2] - newPickPoint[2];
  
  camera->GetFocalPoint(viewFocus);
  camera->GetPosition(viewPoint);
  camera->SetFocalPoint(motionVector[0] + viewFocus[0],
                        motionVector[1] + viewFocus[1],
                        motionVector[2] + viewFocus[2]);

  camera->SetPosition(motionVector[0] + viewPoint[0],
                      motionVector[1] + viewPoint[1],
                      motionVector[2] + viewPoint[2]);
      
  if (rwi->GetLightFollowCamera())
    {
    vtkLight* light = this->CurrentRenderer->GetFirstLight();
    if (light != NULL) 
      {
      light->SetPosition(camera->GetPosition());
      light->SetFocalPoint(camera->GetFocalPoint());
      }
    }
    
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::Dolly()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;

  float *center = this->CurrentRenderer->GetCenter();

  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
  double dyf = this->MotionFactor * (double)(dy) / (double)(center[1]);
  double zoomFactor = pow((double)1.1, dyf);
  
  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  if (camera->GetParallelProjection())
    {
    camera->SetParallelScale(camera->GetParallelScale()/zoomFactor);
    }
  else
    {
    camera->Dolly(zoomFactor);
    this->ResetCameraClippingRange();
    }
  
  if (rwi->GetLightFollowCamera())
    {
    vtkLight* light = this->CurrentRenderer->GetFirstLight();
    if (light != NULL) 
      {
      light->SetPosition(camera->GetPosition());
      light->SetFocalPoint(camera->GetFocalPoint());
      }
    }
  
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}

