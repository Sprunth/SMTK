#include "smtk/model/Storage.h"

#include "smtk/model/AttributeAssignments.h"

#include "smtk/model/Chain.h"
#include "smtk/model/Edge.h"
#include "smtk/model/EdgeUse.h"
#include "smtk/model/Face.h"
#include "smtk/model/FaceUse.h"
#include "smtk/model/GroupEntity.h"
#include "smtk/model/Loop.h"
#include "smtk/model/ModelEntity.h"
#include "smtk/model/Shell.h"
#include "smtk/model/Vertex.h"
#include "smtk/model/VertexUse.h"
#include "smtk/model/Volume.h"
#include "smtk/model/VolumeUse.h"

#include <algorithm>
#include <set>
#include <map>
#include <vector>

#include <sstream>

//#include <boost/variant.hpp>

using namespace std;
using namespace smtk::util;

namespace smtk {
  namespace model {

Storage::Storage() :
  BRepModel(shared_ptr<UUIDsToEntities>(new UUIDsToEntities)),
  m_arrangements(new UUIDsToArrangements),
  m_tessellations(new UUIDsToTessellations),
  m_attributeAssignments(new UUIDsToAttributeAssignments)
{
}

Storage::Storage(
  shared_ptr<UUIDsToEntities> inTopology,
  shared_ptr<UUIDsToArrangements> inArrangements,
  shared_ptr<UUIDsToTessellations> tess,
  shared_ptr<UUIDsToAttributeAssignments> attribs)
  :
    BRepModel(inTopology), m_arrangements(inArrangements),
    m_tessellations(tess), m_attributeAssignments(attribs)
{
}

Storage::~Storage()
{
}

UUIDsToArrangements& Storage::arrangements()
{
  return *this->m_arrangements.get();
}

const UUIDsToArrangements& Storage::arrangements() const
{
  return *this->m_arrangements.get();
}

UUIDsToTessellations& Storage::tessellations()
{
  return *this->m_tessellations.get();
}

const UUIDsToTessellations& Storage::tessellations() const
{
  return *this->m_tessellations.get();
}

UUIDsToAttributeAssignments& Storage::attributeAssignments()
{
  return *this->m_attributeAssignments;
}

const UUIDsToAttributeAssignments& Storage::attributeAssignments() const
{
  return *this->m_attributeAssignments;
}

Storage::tess_iter_type Storage::setTessellation(const UUID& cellId, const Tessellation& geom)
{
  if (cellId.isNull())
    {
    throw std::string("Nil cell ID");
    }
  tess_iter_type result = this->m_tessellations->find(cellId);
  if (result == this->m_tessellations->end())
    {
    std::pair<UUID,Tessellation> blank;
    blank.first = cellId;
    result = this->m_tessellations->insert(blank).first;
    }
  result->second = geom;
  return result;
}

/**\brief Add or replace information about the arrangement of a cell.
  *
  * When \a index is -1, the arrangement is considered new and added to the end of
  * the vector of arrangements of the given \a kind.
  * Otherwise, it should be positive and refer to a pre-existing arrangement to be replaced.
  * The actual \a index location used is returned.
  */
int Storage::arrangeEntity(const UUID& cellId, ArrangementKind kind, const Arrangement& arr, int index)
{
  UUIDsToArrangements::iterator cit = this->m_arrangements->find(cellId);
  if (cit == this->m_arrangements->end())
    {
    if (index >= 0)
      { // failure: can't replace information that doesn't exist.
      return -1;
      }
    KindsToArrangements blank;
    cit = this->m_arrangements->insert(std::pair<UUID,KindsToArrangements>(cellId, blank)).first;
    }
  KindsToArrangements::iterator kit = cit->second.find(kind);
  if (kit == cit->second.end())
    {
    if (index >= 0)
      { // failure: can't replace information that doesn't exist.
      return -1;
      }
    Arrangements blank;
    kit = cit->second.insert(std::pair<ArrangementKind,Arrangements>(kind, blank)).first;
    }

  if (index >= 0)
    {
    if (index >= static_cast<int>(kit->second.size()))
      { // failure: can't replace information that doesn't exist.
      return -1;
      }
    kit->second[index] = arr;
    }
  else
    {
    index = static_cast<int>(kit->second.size());
    kit->second.push_back(arr);
    }
  return index;
}

/**\brief Returns true when the given \a entity has any arrangements of the given \a kind (otherwise false).
  *
  * Use this to avoid accidentally inserting a new array of arrangements with arrangementsOfKindForEntity().
  * Since this actually requires a lookup, you may pass in a pointer \a arr to an array of arrangements;
  * if true is returned, the pointer will be aimed at the existing array. Otherwise, \a arr will be unchanged.
  */
Arrangements* Storage::hasArrangementsOfKindForEntity(
  const smtk::util::UUID& entity, ArrangementKind kind)
{
  UUIDWithArrangementDictionary cellEntry = this->m_arrangements->find(entity);
  if (cellEntry != this->m_arrangements->end())
    {
    ArrangementKindWithArrangements useIt = cellEntry->second.find(kind);
    if (useIt != cellEntry->second.end())
      {
      return &useIt->second;
      }
    }
  return NULL;
}

/**\brief This is a const version of hasArrangementsOfKindForEntity().
  */
const Arrangements* Storage::hasArrangementsOfKindForEntity(
  const smtk::util::UUID& entity, ArrangementKind kind) const
{
  UUIDWithArrangementDictionary cellEntry = this->m_arrangements->find(entity);
  if (cellEntry != this->m_arrangements->end())
    {
    ArrangementKindWithArrangements useIt = cellEntry->second.find(kind);
    if (useIt != cellEntry->second.end())
      {
      return &useIt->second;
      }
    }
  return NULL;
}

/**\brief Return an array of arrangements of the given \a kind for the given \a entity.
  *
  * NOTE: This method will create an empty array and attach it to the entity
  * if none exists, thus increasing storage costs. Unless you intend to append
  * new relationships, you should not use this method without first calling
  * hasArrangementsOfKindForEntity() to determine whether the array already exists.
  */
Arrangements& Storage::arrangementsOfKindForEntity(
  const smtk::util::UUID& entity,
  ArrangementKind kind)
{
  return (*this->m_arrangements)[entity][kind];
}

/**\brief Retrieve arrangement information for a cell.
  *
  * This version does not allow the arrangement to be altered.
  */
const Arrangement* Storage::findArrangement(const UUID& cellId, ArrangementKind kind, int index) const
{
  if (cellId.isNull() || index < 0)
    {
    return NULL;
    }

  UUIDsToArrangements::iterator cit = this->m_arrangements->find(cellId);
  if (cit == this->m_arrangements->end())
    {
    return NULL;
    }

  KindsToArrangements::iterator kit = cit->second.find(kind);
  if (kit == cit->second.end())
    {
    return NULL;
    }

  if (index >= static_cast<int>(kit->second.size()))
    { // failure: can't replace information that doesn't exist.
    return NULL;
    }
  return &kit->second[index];
}

/**\brief Retrieve arrangement information for a cell.
  *
  * This version allows the arrangement to be altered.
  */
Arrangement* Storage::findArrangement(const UUID& cellId, ArrangementKind kind, int index)
{
  if (cellId.isNull() || index < 0)
    {
    return NULL;
    }

  UUIDsToArrangements::iterator cit = this->m_arrangements->find(cellId);
  if (cit == this->m_arrangements->end())
    {
    return NULL;
    }

  KindsToArrangements::iterator kit = cit->second.find(kind);
  if (kit == cit->second.end())
    {
    return NULL;
    }

  if (index >= static_cast<int>(kit->second.size()))
    { // failure: can't replace information that doesn't exist.
    return NULL;
    }
  return &kit->second[index];
}

/**\brief Find a particular arrangement: a cell's HAS_USE with a given sense.
  *
  * The index of the matching arrangement is returned (or -1 if no such sense
  * exists).
  *
  * The sense is a non-negative integer corresponding to a particular
  * use of a cell. However, the model may be altered in a way that some
  * senses are no longer used.
  * Rather than rewrite the cell and cell-use records to keep senses
  * as a continuous integer sequence, we allow "holes" to exist in the
  * list of senses. Just because a cell has a use with sense 5 does not
  * imply that there is also a use with sense 4.
  *
  * You may find all the HAS_USE arrangements of the cell and iterator over
  * them to discover all the sense numbers.
  * There should be no duplicate senses for any given cell.
  */
int Storage::findCellHasUseWithSense(
  const smtk::util::UUID& cellId, int sense) const
{
  const Arrangements* arrs = this->hasArrangementsOfKindForEntity(cellId, HAS_USE);
  if (arrs)
    {
    int i = 0;
    for (Arrangements::const_iterator it = arrs->begin(); it != arrs->end(); ++it, ++i)
      {
      int itIdx, itSense;
      Orientation itOrient;
      if (
        it->IndexSenseAndOrientationFromCellHasUse(itIdx, itSense, itOrient) &&
        itSense == sense)
        {
        return i;
        }
      }
    }
  return -1;
}

/**\brief Find HAS_USE arrangements of a cell with a given orientation.
  *
  * The indices of the matching arrangements are returned.
  */
std::set<int> Storage::findCellHasUsesWithOrientation(
  const smtk::util::UUID& cellId, Orientation orient) const
{
  std::set<int> result;
  const Arrangements* arrs = this->hasArrangementsOfKindForEntity(cellId, HAS_USE);
  if (arrs)
    {
    int i = 0;
    for (Arrangements::const_iterator it = arrs->begin(); it != arrs->end(); ++it, ++i)
      {
      int itIdx, itSense;
      Orientation itOrient;
      if (
        it->IndexSenseAndOrientationFromCellHasUse(itIdx, itSense, itOrient) &&
        itOrient == orient)
        {
        result.insert(i);
        }
      }
    }
  return result;
}

/**\brief Return the UUID of a use record for the
  * given \a cell and \a sense, or NULL if it does not exist.
  */
smtk::util::UUID Storage::cellHasUseOfSenseAndOrientation(
  const smtk::util::UUID& cell, int sense, Orientation orient) const
{
  const smtk::model::Arrangements* arr;
  if ((arr = this->hasArrangementsOfKindForEntity(cell, HAS_USE)) && !arr->empty())
    { // See if any of this cell's uses match our sense.
    for (smtk::model::Arrangements::const_iterator ait = arr->begin(); ait != arr->end(); ++ait)
      {
      int itIdx;
      int itSense;
      Orientation itOrient;
      ait->IndexSenseAndOrientationFromCellHasUse(itIdx, itSense, itOrient);
      if (itSense == sense && itOrient == orient)
        {
        return this->findEntity(cell)->relations()[itIdx];
        }
      }
    }
  return smtk::util::UUID::null();
}

/**\brief Find a use record for the given \a cell and \a sense,
  * creating one if it does not exist.
  */
smtk::util::UUID Storage::findOrCreateCellUseOfSenseAndOrientation(
  const smtk::util::UUID& cell, int sense, Orientation orient)
{
  Entity* entity = this->findEntity(cell);
  if (!entity)
    {
    return smtk::util::UUID::null();
    }
  smtk::model::Arrangements& arr(
    this->arrangementsOfKindForEntity(cell, HAS_USE));

  // See if any of this cell's uses match our sense...
  int arrIdx = -1;
  smtk::model::Arrangements::const_iterator ait;
  int relIdx = 0;
  for (ait = arr.begin(); ait != arr.end(); ++ait, ++relIdx)
    {
    int itIdx;
    int itSense;
    Orientation itOrient;
    ait->IndexSenseAndOrientationFromCellHasUse(itIdx, itSense, itOrient);
    if (itSense == sense && itOrient == orient)
      {
      if (itIdx >= 0)
        { // Found a valid use.
        return entity->relations()[itIdx];
        }
      else
        { // We found an existing but invalid use... replace it below.
        arrIdx = relIdx;
        break;
        }
      }
    }

  // ...nope, we need to create a new use with
  // the specified sense relative to the cell.
  // Note that there may still be an entry in arr
  // which we should overwrite (with itIdx == -1).
  UUIDWithEntity use = this->insertEntityOfTypeAndDimension(
    USE_ENTITY | entity->dimensionBits(), entity->dimension());
  // We must re-fetch entity since inserting the use
  // may have invalidated our reference to it.
  entity = this->findEntity(cell);

  // Now add the use to the cell and the cell to the use:
  smtk::util::UUIDArray::size_type useIdx = entity->relations().size();
  entity->appendRelation(use->first);
  smtk::util::UUIDArray::size_type cellIdx = use->second.relations().size();
  use->second.appendRelation(cell);

  this->arrangeEntity(
    cell, HAS_USE,
    Arrangement::CellHasUseWithIndexSenseAndOrientation(
      static_cast<int>(useIdx), sense, orient),
    arrIdx);
  this->arrangeEntity(
    use->first, HAS_CELL,
    Arrangement::UseHasCellWithIndexAndSense(
      static_cast<int>(cellIdx), sense));

  return use->first;
}

/**\brief Return the UUIDs of all shells included by the given cell-use or shell.
  *
  * Cell-uses of dimension d may include shells that span dimensions d and (d-1).
  * Shells may include other shells of the same dimensionality.
  * These relationships define a hierarchy that enumerate the oriented boundary of
  * the top-level cell-use.
  */
smtk::util::UUIDs Storage::useOrShellIncludesShells(
  const smtk::util::UUID& cellUseOrShell) const
{
  smtk::util::UUIDs shells;
  const Entity* ent = this->findEntity(cellUseOrShell);
  if (ent && (ent->entityFlags() & (USE_ENTITY | SHELL_ENTITY)))
    {
    const smtk::util::UUIDArray& rels(ent->relations());
    const smtk::model::Arrangements* arr;
    if ((arr = this->hasArrangementsOfKindForEntity(cellUseOrShell, INCLUDES)) && !arr->empty())
      {
      for (smtk::model::Arrangements::const_iterator ait = arr->begin(); ait != arr->end(); ++ait)
        {
        int itIdx;
        ait->IndexFromUseOrShellIncludesShell(itIdx);
        // Only insert if the inclusion is a shell
        if ((ent = this->findEntity(rels[itIdx])) && (ent->entityFlags() & SHELL_ENTITY))
          {
          shells.insert(rels[itIdx]);
          }
        }
      }
    }
  return shells;
}

/**\brief Add a new shell to the specified \a useOrShell entity as an inclusion.
  *
  * Cell-uses *include* shells relating the use to a grouping of lower-dimensional uses.
  * Each shell included directly by a cell-use represents a disconnected component
  * of the cell.
  * Shells may also include other shells of the same dimensionality representing voids
  * or material within voids for odd or even depths from the parent cell-use, respectively.
  */
smtk::util::UUID Storage::createIncludedShell(const smtk::util::UUID& useOrShell)
{
  Entity* entity = this->findEntity(useOrShell);
  if (!entity)
    {
    return smtk::util::UUID::null();
    }
  int shellDim =
    (entity->entityFlags() & USE_ENTITY || entity->entityFlags() & CELL_ENTITY) ?
    // k-shells span dimensions (d, d-1), where d = dimension of the cell/cell-use:
    entity->dimensionBits() | (entity->dimensionBits() >> 1) :
    // included k-shell must have same dimension as parent:
    entity->dimensionBits();
  int indexOfNewShell = static_cast<int>(entity->relations().size());
  UUIDWithEntity shell =
    this->insertEntityOfTypeAndDimension(SHELL_ENTITY | shellDim, -1);
  this->arrangeEntity(useOrShell, INCLUDES,
    Arrangement::UseOrShellIncludesShellWithIndex(indexOfNewShell));
  // We must re-find the entity record since insertEntityOfTypeAndDimension
  // invalidates entity when SMTK_HASH_STORAGE is true:
  this->findEntity(useOrShell)->appendRelation(shell->first);
  this->arrangeEntity(shell->first, EMBEDDED_IN,
    Arrangement::ShellEmbeddedInUseOrShellWithIndex(
      static_cast<int>(shell->second.relations().size())));
  shell->second.appendRelation(useOrShell);

  return shell->first;
}

/**\brief Add a cell-use to a shell if it is not already contained in the shell.
  *
  * Note that cell-uses may have relations to shells of 2 different dimensions.
  * This method should only be called when adding d-dimensional
  * use-records to a shell bridging dimensions (d, d+1).
  * For example, when adding an edge-use to a loop and not
  * when setting the edge-use associated with a chain.
  * Use the setUseForShell() method to do the latter.
  *
  * The reason for this is that d-shells must have exactly one parent
  * cell-use (or cell, for d=3), but may have many child cell-uses
  * that are one dimension lower.
  * A different type of relationship (INCLUDES/EMBEDDED_IN vs HAS_USE/HAS_SHELL)
  * is employed depending on the dimension so that the distinction can be made
  * easily.
  */
bool Storage::findOrAddUseToShell(
  const smtk::util::UUID& shell, const smtk::util::UUID& use)
{
  Entity* shellEnt;
  Entity* useEnt;
  // Check that the shell and use are valid and that the use has the proper dimension for the shell.
  if (
    (shellEnt = this->findEntity(shell)) &&
    (useEnt = this->findEntity(use)))
    {
    // Verify that the cell-use uses the lower dimension-bit of the shell.
    // E.g., if the shell spans dimensions 1 and 2, the use must be of dimension 1.
    if ((shellEnt->dimensionBits() & useEnt->dimensionBits()) < shellEnt->dimensionBits())
      {
      // Now, is the use already listed in a HAS_USE relationship?
      int anum = -1;
      int srsize = static_cast<int>(shellEnt->relations().size());
      Arrangements* arr = this->hasArrangementsOfKindForEntity(shell, HAS_USE);
      if (srsize && arr)
        {
        int a = 0;
        for (Arrangements::const_iterator ait = arr->begin(); ait != arr->end(); ++ait, ++a)
          {
          int i0, i1;
          ait->IndexRangeFromShellHasUse(i0, i1);
          for (int ii = i0; ii < i1; ++ii)
            {
            if (shellEnt->relations()[ii] == use)
              return false; // The shell already has the use-record. Do nothing.
            }
          // OK, does this HAS_USE arrangement go to the end of shellEnt->relations()?
          if (i1 == srsize)
            { // Yes, so we can extend it below instead of adding a new HAS_USE.
            anum = a;
            }
          }
        }
      // So far, no existing HAS_USE arrangement contained it;
      // add the use to the shell.
      shellEnt->appendRelation(use);
      if (anum >= 0)
        { // We can "extend" an existing arrangement to include the use.
        int i0, i1;
        (*arr)[anum].IndexRangeFromShellHasUse(i0, i1);
        (*arr)[anum] = Arrangement::ShellHasUseWithIndexRange(i0, i1 + 1);
        }
      else
        { // Create a new arrangement
        this->arrangeEntity(shell, HAS_USE, Arrangement::ShellHasUseWithIndexRange(srsize, srsize + 1));
        }
      // Now set the inverse relationship (the cell-use mapping to its parent shell).
      // Every cell-use must have a HAS_SHELL relation which we overwrite --
      // except in the case of vertex-use where we add a new relation since
      // they may have multiple HAS_SHELL relationships.
      int shellIdx = -1;
      bool previousShell = true; // Was there a previous, valid HAS_SHELL arrangement? Assume so.
      Arrangement* a;
      if ((a = this->findArrangement(use, HAS_SHELL, 0)))
        {
        a->IndexFromUseHasShell(shellIdx);
        if (shellIdx < 0)
          {
          shellIdx = useEnt->findOrAppendRelation(shell);
          *this->findArrangement(use, HAS_SHELL, 0) =
            Arrangement::UseHasShellWithIndex(shellIdx);
          previousShell = false;
          }
        }
      else
        {
        shellIdx = this->arrangeEntity(use, HAS_SHELL, Arrangement::UseHasShellWithIndex(-1));
        previousShell = false;
        }
      // Here is where vertex-uses are handled differently:
      // If we aren't replacing an invalid (-1) index in the arrangement,
      // then we should create a new arrangement instead of overwrite
      // what's at shellIdx in useEnt->relations().
      if (isVertexUse(useEnt->entityFlags()) && previousShell)
        {
        shellIdx = this->arrangeEntity(
          use, HAS_SHELL,
          Arrangement::UseHasShellWithIndex(
            useEnt->findOrAppendRelation(shell)));
        }
      else
        {
        useEnt->relations()[shellIdx] = shell;
        }
      return true;
      }
    // FIXME: Should we throw() when dimension is wrong?
    }
  // garbage-in => garbage-out
  // FIXME: Should we throw() here?
  return false;
}

/**\brief Add an entity to a cell as a geometric inclusion.
  *
  * This attempts to add relationships and arrangements to
  * both the \a cell and \a inclusion that indicate the
  * latter is geometrically interior to the \a cell.
  *
  * Thus, the \a inclusion must have a dimension less-than
  * or equal to the \a cell.
  */
bool Storage::findOrAddInclusionToCell(
  const smtk::util::UUID& cell, const smtk::util::UUID& inclusion)
{
  Entity* cellEnt;
  Entity* incEnt;
  // Check that the cell and inclusion are valid and that the inclusion has the proper dimension for the cell.
  if (
    (cellEnt = this->findEntity(cell)) &&
    (incEnt = this->findEntity(inclusion)))
    {
    // Verify that the inclusion has a lower-or-equal than the cell.
    // E.g., if the cell has dimension 2, the inclusion must be of dimension 2 or less.
    if (cellEnt->dimensionBits() >= incEnt->dimensionBits())
      {
      // Now, is the inclusion already listed in a INCLUDES relationship?
      int srsize = static_cast<int>(cellEnt->relations().size());
      Arrangements* arr = this->hasArrangementsOfKindForEntity(cell, INCLUDES);
      if (srsize && arr)
        {
        int a = 0;
        for (Arrangements::const_iterator ait = arr->begin(); ait != arr->end(); ++ait, ++a)
          {
          int i;
          ait->IndexFromCellIncludesEntity(i);
          if (i >= 0 && cellEnt->relations()[i] == inclusion)
            {
            return false; // The cell already has the inclusion-record. Do nothing.
            }
          }
        }
      // No existing INCLUDES arrangement contained it;
      // add the inclusion to the cell.
      cellEnt->appendRelation(inclusion);
      this->arrangeEntity(cell, INCLUDES, Arrangement::CellIncludesEntityWithIndex(srsize));
      // Now set the inverse relationship (the cell-inclusion mapping to its parent cell).
      // Every cell-inclusion must have a matching EMBEDDED_IN relation.
      int cellIdx = incEnt->findOrAppendRelation(cell);
      this->arrangeEntity(inclusion, EMBEDDED_IN, Arrangement::CellEmbeddedInEntityWithIndex(cellIdx));
      return true;
      }
    // FIXME: Should we throw() when dimension is wrong?
    }
  // garbage-in => garbage-out
  // FIXME: Should we throw() here?
  return false;
}

/**\brief Report whether an entity has been assigned an attribute.
  *
  */
bool Storage::hasAttribute(int attribId, const smtk::util::UUID& toEntity)
{
  UUIDWithAttributeAssignments it = this->m_attributeAssignments->find(toEntity);
  if (it == this->m_attributeAssignments->end())
    {
    return false;
    }
  return it->second.isAssociated(attribId);
}

/**\brief Assign an attribute to an entity.
  *
  */
bool Storage::attachAttribute(int attribId, const smtk::util::UUID& toEntity)
{
  return this->attributeAssignments()[toEntity].attachAttribute(attribId);
}

/**\brief Unassign an attribute from an entity.
  *
  */
bool Storage::detachAttribute(int attribId, const smtk::util::UUID& fromEntity, bool reverse)
{
  bool didRemove = false;
  UUIDWithAttributeAssignments ref = this->m_attributeAssignments->find(fromEntity);
  if (ref == this->m_attributeAssignments->end())
    {
    return didRemove;
    }
  if ((didRemove = ref->second.detachAttribute(attribId)) && reverse)
    {
    /* FIXME: Let manager know.
    smtk::attribute::Manager* mgr = smtk::attribute::Manager::getGlobalManager();
    if (mgr)
      {
      smtk::attribute::AttributePtr attrib = mgr->findAttribute(attribId);
      if (attrib)
        {
        // We don't need a shared pointer to ourselves since we are
        // passing reverse=false here. Thus we can get by without
        // inheriting shared_from_this(), which is a rat's nest
        // should anyone ever derive a class from Storage.
        smtk::model::StoragePtr model; // a NULL shared pointer.
        attrib->disassociateEntity(model, fromEntity, false);
        }
      }
      */
    }
  return didRemove;
}

/// Add an edge to storage (without any relationships)
Vertex Storage::addVertex()
{
  return Vertex(
    shared_from_this(),
    this->addEntityOfTypeAndDimension(CELL_ENTITY, 0));
}

/// Add an edge to storage (without any relationships)
Edge Storage::addEdge()
{
  return Edge(
    shared_from_this(),
    this->addEntityOfTypeAndDimension(CELL_ENTITY, 1));
}

/**\brief Add a face to storage (without any relationships)
  *
  * While this method does not add any relations, it
  * does create two HAS_USE arrangements to hold
  * FaceUse instances (assuming the BRepModel may be
  * downcast to a Storage instance).
  */
Face Storage::addFace()
{
  return Face(
    shared_from_this(),
    this->addEntityOfTypeAndDimension(CELL_ENTITY, 2));
}

/// Add a volume to storage (without any relationships)
Volume Storage::addVolume()
{
  return Volume(
    shared_from_this(),
    this->addEntityOfTypeAndDimension(CELL_ENTITY, 3));
}

/// Add a vertex-use to storage (without any relationships)
VertexUse Storage::addVertexUse()
{
  return VertexUse(
    shared_from_this(),
    this->addEntityOfTypeAndDimension(USE_ENTITY, 0));
}

/// Find or add a vertex-use to storage with a relationship back to a vertex.
VertexUse Storage::addVertexUse(const Vertex& src, int sense)
{
  if (src.isValid() && src.storage().get() == this)
    {
    return VertexUse(
      src.storage(),
      this->findOrCreateCellUseOfSenseAndOrientation(src.entity(), sense, POSITIVE));
    }
  return VertexUse(); // invalid vertex use if source vertex was invalid or from different storage.
}

/// Add an edge-use to storage (without any relationships)
EdgeUse Storage::addEdgeUse()
{
  return EdgeUse(
    shared_from_this(),
    this->addEntityOfTypeAndDimension(USE_ENTITY, 1));
}

/// Find or add a edge-use to storage with a relationship back to a edge.
EdgeUse Storage::addEdgeUse(const Edge& src, int sense, Orientation orient)
{
  if (src.isValid() && src.storage().get() == this)
    {
    return EdgeUse(
      src.storage(),
      this->findOrCreateCellUseOfSenseAndOrientation(src.entity(), sense, orient));
    }
  return EdgeUse(); // invalid edge use if source edge was invalid or from different storage.
}

/// Add a face-use to storage (without any relationships)
FaceUse Storage::addFaceUse()
{
  return FaceUse(
    shared_from_this(),
    this->addEntityOfTypeAndDimension(USE_ENTITY, 2));
}

/// Find or add a face-use to storage with a relationship back to a face.
FaceUse Storage::addFaceUse(const Face& src, int sense, Orientation orient)
{
  if (src.isValid() && src.storage().get() == this)
    {
    return FaceUse(
      src.storage(),
      src.storage()->findOrCreateCellUseOfSenseAndOrientation(src.entity(), sense, orient));
    }
  return FaceUse(); // invalid face use if source face was invalid or from different storage.
}

/// Add a volume-use to storage (without any relationships)
VolumeUse Storage::addVolumeUse()
{
  return VolumeUse(
    shared_from_this(),
    this->addEntityOfTypeAndDimension(USE_ENTITY, 3));
}

/// Find or add a volume-use to storage with a relationship back to a volume.
VolumeUse Storage::addVolumeUse(const Volume& src)
{
  if (src.isValid() && src.storage().get() == this)
    {
    return VolumeUse(
      src.storage(),
      src.storage()->findOrCreateCellUseOfSenseAndOrientation(src.entity(), 0, POSITIVE));
    }
  return VolumeUse(); // invalid volume use if source volume was invalid or from different storage.
}

/// Add a 0/1-d shell (a vertex chain) to storage (without any relationships)
Chain Storage::addChain()
{
  return Chain(
    shared_from_this(),
    this->addEntityOfTypeAndDimension(SHELL_ENTITY | DIMENSION_0 | DIMENSION_1, -1));
}

/// Add a 0/1-d shell (a vertex chain) to storage with a relation to its edge use
Chain Storage::addChain(const EdgeUse& eu)
{
  if (eu.isValid() && eu.storage().get() == this)
    {
    return Chain(
      eu.storage(),
      eu.storage()->createIncludedShell(eu.entity()));
    }
  return Chain();
}

/// Add a 0/1-d shell (a vertex chain) to storage with a relation to its edge use
Chain Storage::addChain(const Chain& c)
{
  if (c.isValid() && c.storage().get() == this)
    {
    return Chain(
      c.storage(),
      c.storage()->createIncludedShell(c.entity()));
    }
  return Chain();
}

/// Add a 1/2-d shell (an edge loop) to storage (without any relationships)
Loop Storage::addLoop()
{
  return Loop(
    shared_from_this(),
    this->addEntityOfTypeAndDimension(SHELL_ENTITY | DIMENSION_1 | DIMENSION_2, -1));
}

/// Add a 1/2-d shell (an edge loop) to storage with a relation to its parent face use
Loop Storage::addLoop(const FaceUse& fu)
{
  if (fu.isValid() && fu.storage().get() == this)
    {
    return Loop(
      fu.storage(),
      fu.storage()->createIncludedShell(fu.entity()));
    }
  return Loop();
}

/// Add a 1/2-d shell (an edge loop) to storage with a relation to its parent loop
Loop Storage::addLoop(const Loop& lp)
{
  if (lp.isValid() && lp.storage().get() == this)
    {
    return Loop(
      lp.storage(),
      lp.storage()->createIncludedShell(lp.entity()));
    }
  return Loop();
}

/// Add a 2/3-d shell (a face-shell) to storage (without any relationships)
Shell Storage::addShell()
{
  return Shell(
    shared_from_this(),
    this->addEntityOfTypeAndDimension(SHELL_ENTITY | DIMENSION_2 | DIMENSION_3, -1));
}

/// A convenience method to find or create a volume use for the volume plus a shell.
Shell Storage::addShell(const Volume& v)
{
  VolumeUse vu;
  if (!v.uses<VolumeUses>().empty())
    {
    vu = v.uses<VolumeUses>()[0];
    }
  if (!vu.isValid())
    {
    vu = this->addVolumeUse(v);
    }
  return this->addShell(vu);
}

/// Add a 2/3-d shell (an face shell) to storage with a relation to its volume
Shell Storage::addShell(const VolumeUse& v)
{
  if (v.isValid() && v.storage().get() == this)
    {
    return Shell(
      v.storage(),
      v.storage()->createIncludedShell(v.entity()));
    }
  return Shell();
}

/**\brief Add an entity group to storage (without any relationships).
  *
  * Any non-zero bits set in \a extraFlags are OR'd with entityFlags() of the group.
  * This is an easy way to constrain the dimension of entities allowed to be members
  * of the group.
  *
  * You may also specify a \a name for the group. If \a name is empty, then no
  * name is assigned.
  */
GroupEntity Storage::addGroup(int extraFlags, const std::string& groupName)
{
  smtk::util::UUID uid =
    this->addEntityOfTypeAndDimension(GROUP_ENTITY | extraFlags, -1);
  if (!groupName.empty())
    this->setStringProperty(uid, "name", groupName);
  return GroupEntity(shared_from_this(), uid);
}

/**\brief Add a model to storage.
  *
  * The model will have the specified \a embeddingDim set as an integer property
  * named "embedding dimension." This is the dimension of the space in which
  * vertex coordinates live.
  *
  * A model may also be given a parametric dimension
  * which is the maximum parametric dimension of any cell inserted into the model.
  * The parametric dimension is the rank of the space spanned by the shape functions
  * (for "parametric" meshes) or (for "discrete" meshes) barycentric coordinates of cells.
  *
  * You may also specify a \a name for the model. If \a name is empty, then no
  * name is assigned.
  *
  * A model maintains counters used to number model entities by type (uniquely within the
  * model). Any entities related to the model (directly or indirectly via topological
  * relationships) may have these numbers assigned as names by calling assignDefaultNames().
  */
ModelEntity Storage::addModel(
  int parametricDim, int embeddingDim, const std::string& modelName)
{
  smtk::util::UUID uid = this->addEntityOfTypeAndDimension(MODEL_ENTITY, parametricDim);
  if (embeddingDim > 0)
    {
    this->setIntegerProperty(uid, "embedding dimension", embeddingDim);
    }
  if (!modelName.empty())
    this->setStringProperty(uid, "name", modelName);
  else
    this->assignDefaultName(uid, this->type(uid));
  return ModelEntity(shared_from_this(), uid);
}

  } // namespace model
} //namespace smtk
