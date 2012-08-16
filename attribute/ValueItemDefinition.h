/*=========================================================================

Copyright (c) 1998-2012 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO
PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
=========================================================================*/
// .NAME ValueItemDefinition.h -
// .SECTION Description
// .SECTION See Also

#ifndef __slctk_attribute_ValueItemDefinition_h
#define __slctk_attribute_ValueItemDefinition_h

#include <string>
#include <set>
#include <vector>

#include "AttributeExports.h"
#include "attribute/ItemDefinition.h"
#include "attribute/PublicPointerDefs.h"

namespace slctk
{
  namespace attribute
  {
    class Attribute;
    class AttributeRefItem;
    class AttributeRefItemDefinition;
    class Definition;
    class Cluster;
    class SLCTKATTRIBUTE_EXPORT ValueItemDefinition : 
      public slctk::attribute::ItemDefinition
    {
    public:
      ValueItemDefinition(const std::string &myname);
      virtual ~ValueItemDefinition();

      const std::string &units() const
      { return this->m_units;}
      void setUnits(const std::string &newUnits)
      { this->m_units = newUnits;}

      bool isDiscrete() const
      {return (this->m_discreteValueLabels.size() != 0);}
      
      int defaultDiscreteIndex() const
      {return this->m_defaultDiscreteIndex;}
      void setDefaultDiscreteIndex(int discreteIndex)
      {this->m_defaultDiscreteIndex = discreteIndex;}
      
      bool allowsExpressions() const;
      bool isValidExpression(slctk::AttributePtr exp) const;
      slctk::AttributeDefinitionPtr expressionDefinition() const;
      void setExpressionDefinition(slctk::AttributeDefinitionPtr exp);
      slctk::AttributeRefItemPtr buildExpressionItem() const;

      bool hasDefault() const
      {return this->m_hasDefault;}

      virtual bool hasRange() const = 0;

      int numberOfValues() const
      {return this->m_numberOfValues;}
      void setNumberOfValues(int esize);

      bool hasValueLabels() const
      {return this->m_valueLabels.size();}

      void setValueLabel(int element, const std::string &elabel);
      void setCommonValueLabel(const std::string &elabel);
      std::string valueLabel(int element) const;
      bool isDiscreteIndexValid(int index) const
      {return ((index > -1) && (index < this->m_discreteValueLabels.size()));}

    protected:

      bool m_hasDefault;
      bool m_useCommonLabel;
      std::vector<std::string> m_valueLabels;
      std::vector<std::string> m_discreteValueLabels;
      int m_defaultDiscreteIndex;
      int m_numberOfValues;
      std::string m_units;
      slctk::AttributeRefItemDefinitionPtr m_expressionDefinition;
    private:
      
    };
  };
};

#endif /* __slctk_attribute_ValueItemDefinition_h */