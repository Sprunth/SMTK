#include "smtk/paraview/vtk/vtkModelView.h"

#include "vtkObjectFactory.h"

vtkInstantiatorNewMacro(vtkModelView);


vtkModelView* vtkModelView::New()
{
  VTK_STANDARD_NEW_BODY(vtkModelView);
}

vtkModelView::vtkModelView()
{
}

vtkModelView::~vtkModelView()
{
  // Even though the base class will call this again, we must
  // call RemoveAllRepresentations() now.
  // Otherwise, representations which downcast the view pointer
  // to a subclass of vtkView (in order to call subclass methods)
  // will fail.
  this->RemoveAllRepresentations();
}

void vtkModelView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}