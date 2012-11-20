/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME qtSection - a class that encapsulates the UI of an Attribute 
// .SECTION Description

#ifndef __slctk_attribute_qtSection_h
#define __slctk_attribute_qtSection_h

#include <QObject>
#include "AttributeExports.h"
#include "attribute/PublicPointerDefs.h"
class qtSectionInternals;

namespace slctk
{
  namespace attribute
  {
    class SLCTKATTRIBUTE_EXPORT qtSection : public QObject
    {
      Q_OBJECT

    public:         
      qtSection(slctk::SectionPtr, QWidget* parent);
      virtual ~qtSection();

      slctk::SectionPtr getObject();
      QWidget* widget()
      {return this->Widget;}
      QWidget* parentWidget();
     
    public slots:
      virtual void showAdvanced(int show){;}

    protected slots:
      virtual void updateAttributeData() {;}
      
    protected:
      virtual void createWidget(){;}
      QWidget* Widget;
    private:

      qtSectionInternals *Internals;
      
    }; // class
  }; // namespace attribute
}; // namespace slctk

#endif