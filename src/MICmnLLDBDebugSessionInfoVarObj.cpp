//===-- MICmnLLDBDebugSessionInfoVarObj.cpp ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// In-house headers:
#include "MICmnLLDBDebugSessionInfoVarObj.h"
#include "MICmnLLDBProxySBValue.h"
#include "MICmnLLDBUtilSBValue.h"
#include "lldb/API/SBError.h"
#include "lldb/API/SBExpressionOptions.h"
#include "lldb/API/SBFrame.h"

// Instantiations:
const char *CMICmnLLDBDebugSessionInfoVarObj::ms_aVarFormatStrings[] = {
    // CODETAG_SESSIONINFO_VARFORMAT_ENUM
    // *** Order is import here.
    "<Invalid var format>", "binary", "octal", "decimal",
    "hexadecimal",          "natural"};
const char *CMICmnLLDBDebugSessionInfoVarObj::ms_aVarFormatChars[] = {
    // CODETAG_SESSIONINFO_VARFORMAT_ENUM
    // *** Order is import here.
    "<Invalid var format>", "t", "o", "d", "x", "N"};
CMICmnLLDBDebugSessionInfoVarObj::MapKeyToVarObj_t
    CMICmnLLDBDebugSessionInfoVarObj::ms_mapVarIdToVarObj;
std::map<CMIUtilString, std::set<CMIUtilString>>
    CMICmnLLDBDebugSessionInfoVarObj::ms_mapChildren;
MIuint CMICmnLLDBDebugSessionInfoVarObj::ms_nVarUniqueId = 0; // Index from 0
CMICmnLLDBDebugSessionInfoVarObj::varFormat_e
    CMICmnLLDBDebugSessionInfoVarObj::ms_eDefaultFormat = eVarFormat_Natural;

//++
// Details: CMICmnLLDBDebugSessionInfoVarObj constructor.
// Type:    Method.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmnLLDBDebugSessionInfoVarObj::CMICmnLLDBDebugSessionInfoVarObj()
    : m_eVarFormat(eVarFormat_Natural), m_eVarType(eVarType_Internal),
      m_eValObjKind(valObjKind_ec::eValObjKind_Other), m_bChanged(false) {
  // Do not call UpdateValue() in here as not necessary
}

//++
// Details: CMICmnLLDBDebugSessionInfoVarObj constructor.
// Type:    Method.
// Args:    vrStrNameReal   - (R) The actual name of the variable, the
// expression.
//          vrStrName       - (R) The name given for *this var object.
//          vrValue         - (R) The LLDB SBValue object represented by *this
//          object.
// Return:  None.
// Throws:  None.
//--
CMICmnLLDBDebugSessionInfoVarObj::CMICmnLLDBDebugSessionInfoVarObj(
    const CMIUtilString &vrStrNameReal, const CMIUtilString &vrStrName,
    const lldb::SBValue &vrValue, const valObjKind_ec eValObjKind)
    : m_eVarFormat(eVarFormat_Natural), m_eVarType(eVarType_Internal),
      m_eValObjKind(eValObjKind), m_strName(vrStrName), m_SBValue(vrValue),
      m_strNameReal(vrStrNameReal), m_bChanged(false) {
  m_expressionFrame = m_SBValue.GetFrame();
  m_lastUpdate = ReadValueState();
  UpdateValue();
}

//++
// Details: CMICmnLLDBDebugSessionInfoVarObj constructor.
// Type:    Method.
// Args:    vrStrNameReal           - (R) The actual name of the variable, the
// expression.
//          vrStrName               - (R) The name given for *this var object.
//          vrValue                 - (R) The LLDB SBValue object represented by
//          *this object.
//          vrStrVarObjParentName   - (R) The var object parent to *this var
//          object (LLDB SBValue equivalent).
// Return:  None.
// Throws:  None.
//--
CMICmnLLDBDebugSessionInfoVarObj::CMICmnLLDBDebugSessionInfoVarObj(
    const CMIUtilString &vrStrNameReal, const CMIUtilString &vrStrName,
    const lldb::SBValue &vrValue, const CMIUtilString &vrStrVarObjParentName,
    const valObjKind_ec eValObjKind, MIuint vnChildIndex)
    : m_eVarFormat(eVarFormat_Natural), m_eVarType(eVarType_Internal),
      m_eValObjKind(eValObjKind), m_strName(vrStrName), m_SBValue(vrValue),
      m_strNameReal(vrStrNameReal),
      m_strVarObjParentName(vrStrVarObjParentName), m_nChildIndex(vnChildIndex),
      m_bChanged(false) {
  m_lastUpdate = ReadValueState();
  UpdateValue();
}

//++
// Details: CMICmnLLDBDebugSessionInfoVarObj copy constructor.
// Type:    Method.
// Args:    vrOther - (R) The object to copy from.
// Return:  None.
// Throws:  None.
//--
CMICmnLLDBDebugSessionInfoVarObj::CMICmnLLDBDebugSessionInfoVarObj(
    const CMICmnLLDBDebugSessionInfoVarObj &vrOther) {
  CopyOther(vrOther);
}

//++
// Details: CMICmnLLDBDebugSessionInfoVarObj move constructor.
// Type:    Method.
// Args:    vrwOther    - (R) The object to copy from.
// Return:  None.
// Throws:  None.
//--
CMICmnLLDBDebugSessionInfoVarObj::CMICmnLLDBDebugSessionInfoVarObj(
    CMICmnLLDBDebugSessionInfoVarObj &&vrwOther) {
  MoveOther(std::move(vrwOther));
}

//++
// Details: CMICmnLLDBDebugSessionInfoVarObj assignment operator.
// Type:    Method.
// Args:    vrOther - (R) The object to copy from.
// Return:  CMICmnLLDBDebugSessionInfoVarObj & - Updated *this object.
// Throws:  None.
//--
CMICmnLLDBDebugSessionInfoVarObj &CMICmnLLDBDebugSessionInfoVarObj::operator=(
    const CMICmnLLDBDebugSessionInfoVarObj &vrOther) {
  // Check for self-assignment
  if (this == &vrOther)
    return *this;

  CopyOther(vrOther);
  return *this;
}

//++
// Details: CMICmnLLDBDebugSessionInfoVarObj assignment operator.
// Type:    Method.
// Args:    vrwOther    - (R) The object to copy from.
// Return:  CMICmnLLDBDebugSessionInfoVarObj & - Updated *this object.
// Throws:  None.
//--
CMICmnLLDBDebugSessionInfoVarObj &CMICmnLLDBDebugSessionInfoVarObj::operator=(
    CMICmnLLDBDebugSessionInfoVarObj &&vrwOther) {
  // Check for self-assignment
  if (this == &vrwOther)
    return *this;

  MoveOther(std::move(vrwOther));
  return *this;
}

//++
// Details: Copy the other instance of that object to *this object.
// Type:    Method.
// Args:    vrOther - (R) The object to copy from.
// Return:  None.
// Throws:  None.
//--
void CMICmnLLDBDebugSessionInfoVarObj::CopyOther(
    const CMICmnLLDBDebugSessionInfoVarObj &vrOther) {
  m_eVarFormat = vrOther.m_eVarFormat;
  m_eVarType = vrOther.m_eVarType;
  m_eValObjKind = vrOther.m_eValObjKind;
  m_strName = vrOther.m_strName;
  m_SBValue = vrOther.m_SBValue;
  m_expressionFrame = vrOther.m_expressionFrame;
  m_strNameReal = vrOther.m_strNameReal;
  m_strFormattedValue = vrOther.m_strFormattedValue;
  m_strVarObjParentName = vrOther.m_strVarObjParentName;
  m_nChildIndex = vrOther.m_nChildIndex;
  m_lastUpdate = vrOther.m_lastUpdate;
  m_bChanged = vrOther.m_bChanged;
}

//++
// Details: Move that object to *this object.
// Type:    Method.
// Args:    vrwOther    - (RW) The object to move from.
// Return:  None.
// Throws:  None.
//--
void CMICmnLLDBDebugSessionInfoVarObj::MoveOther(
    CMICmnLLDBDebugSessionInfoVarObj &&vrwOther) {
  m_eVarFormat = vrwOther.m_eVarFormat;
  m_eVarType = vrwOther.m_eVarType;
  m_eValObjKind = vrwOther.m_eValObjKind;
  m_strName = std::move(vrwOther.m_strName);
  m_SBValue = std::move(vrwOther.m_SBValue);
  m_expressionFrame = std::move(vrwOther.m_expressionFrame);
  m_strNameReal = std::move(vrwOther.m_strNameReal);
  m_strFormattedValue = std::move(vrwOther.m_strFormattedValue);
  m_strVarObjParentName = std::move(vrwOther.m_strVarObjParentName);
  m_nChildIndex = vrwOther.m_nChildIndex;
  m_lastUpdate = std::move(vrwOther.m_lastUpdate);
  m_bChanged = vrwOther.m_bChanged;

  vrwOther.m_eVarFormat = eVarFormat_Natural;
  vrwOther.m_eVarType = eVarType_Internal;
  vrwOther.m_eValObjKind = valObjKind_ec::eValObjKind_Other;
  vrwOther.m_strName.clear();
  vrwOther.m_SBValue.Clear();
  vrwOther.m_expressionFrame = lldb::SBFrame();
  vrwOther.m_strNameReal.clear();
  vrwOther.m_strFormattedValue.clear();
  vrwOther.m_strVarObjParentName.clear();
  vrwOther.m_nChildIndex = UINT32_MAX;
  vrwOther.m_lastUpdate = ValueState();
  vrwOther.m_bChanged = false;
}

//++
// Details: CMICmnLLDBDebugSessionInfoVarObj destructor.
// Type:    Overridden.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmnLLDBDebugSessionInfoVarObj::~CMICmnLLDBDebugSessionInfoVarObj() {}

//++
// Details: Retrieve the var format enumeration for the specified string.
// Type:    Static method.
// Args:    vrStrFormat - (R) Text description of the var format.
// Return:  varFormat_e - Var format enumeration.
//                      - No match found return eVarFormat_Invalid.
// Throws:  None.
//--
CMICmnLLDBDebugSessionInfoVarObj::varFormat_e
CMICmnLLDBDebugSessionInfoVarObj::GetVarFormatForString(
    const CMIUtilString &vrStrFormat) {
  // CODETAG_SESSIONINFO_VARFORMAT_ENUM
  for (MIuint i = 0; i < eVarFormat_count; i++) {
    const char *pVarFormatString = ms_aVarFormatStrings[i];
    if (vrStrFormat == pVarFormatString)
      return static_cast<varFormat_e>(i);
  }

  return eVarFormat_Invalid;
}

//++
// Details: Retrieve the var format enumeration for the specified character.
// Type:    Static method.
// Args:    vcFormat    - Character representing the var format.
// Return:  varFormat_e - Var format enumeration.
//                      - No match found return eVarFormat_Invalid.
// Throws:  None.
//--
CMICmnLLDBDebugSessionInfoVarObj::varFormat_e
CMICmnLLDBDebugSessionInfoVarObj::GetVarFormatForChar(char vcFormat) {
  if ('r' == vcFormat)
    return eVarFormat_Hex;

  // CODETAG_SESSIONINFO_VARFORMAT_ENUM
  for (MIuint i = 0; i < eVarFormat_count; i++) {
    const char *pVarFormatChar = ms_aVarFormatChars[i];
    if (*pVarFormatChar == vcFormat)
      return static_cast<varFormat_e>(i);
  }

  return eVarFormat_Invalid;
}

//++
// Details: Return the equivalent var value formatted string for the given value
// type,
//          which was prepared for printing (i.e. value was escaped and now it's
//          ready
//          for wrapping into quotes).
//          The SBValue vrValue parameter is checked by LLDB private code for
//          valid
//          scalar type via MI Driver proxy function as the valued returned can
//          also be
//          an error condition. The proxy function determines if the check was
//          valid
//          otherwise return an error condition state by other means saying so.
// Type:    Static method.
// Args:    vrValue     - (R) The var value object.
//          veVarFormat - (R) Var format enumeration.
// Returns: CMIUtilString   - Value formatted string.
// Throws:  None.
//--
CMIUtilString CMICmnLLDBDebugSessionInfoVarObj::GetValueStringFormatted(
    const lldb::SBValue &vrValue,
    const CMICmnLLDBDebugSessionInfoVarObj::varFormat_e veVarFormat) {
  const CMICmnLLDBUtilSBValue utilValue(vrValue, true);
  const CMIUtilString defaultValue = utilValue.GetValue().AddSlashes();

  if (utilValue.IsIntegerType() || utilValue.IsPointerType()) {
    MIuint64 nValue = 0;
    if (CMICmnLLDBProxySBValue::GetValueAsUnsigned(vrValue, nValue)) {
      return GetStringFormatted(nValue, defaultValue, veVarFormat);
    }
  }

  return defaultValue;
}

//++
// Details: Return number formatted string according to the given value type.
// Type:    Static method.
// Args:    vnValue             - (R) The number value to get formatted.
//          vrStrValueNatural   - (R) The natural representation of the number
//          value.
//          veVarFormat         - (R) Var format enumeration.
// Returns: CMIUtilString       - Numerical formatted string.
// Throws:  None.
//--
CMIUtilString CMICmnLLDBDebugSessionInfoVarObj::GetStringFormatted(
    const MIuint64 vnValue, const CMIUtilString &vrStrValueNatural,
    const CMICmnLLDBDebugSessionInfoVarObj::varFormat_e veVarFormat) {
  CMIUtilString strFormattedValue;
  CMICmnLLDBDebugSessionInfoVarObj::varFormat_e veFormat = veVarFormat;
  if (ms_eDefaultFormat != eVarFormat_Invalid &&
      veVarFormat == eVarFormat_Natural) {
    veFormat = ms_eDefaultFormat;
  }

  switch (veFormat) {
  case eVarFormat_Binary:
    strFormattedValue = CMIUtilString::FormatBinary(vnValue);
    break;
  case eVarFormat_Octal:
    strFormattedValue = CMIUtilString::Format("0%llo", vnValue);
    break;
  case eVarFormat_Decimal:
    strFormattedValue = CMIUtilString::Format("%lld", vnValue);
    break;
  case eVarFormat_Hex:
    strFormattedValue = CMIUtilString::Format("0x%llx", vnValue);
    break;
  case eVarFormat_Natural:
  default: {
    strFormattedValue = vrStrValueNatural;
  }
  }

  return strFormattedValue;
}

//++
// Details: Delete internal container contents.
// Type:    Static method.
// Args:    None.
// Returns: None.
// Throws:  None.
//--
void CMICmnLLDBDebugSessionInfoVarObj::VarObjClear() {
  ms_mapVarIdToVarObj.clear();
  ms_mapChildren.clear();
}

//++
// Details: Add a var object to the internal container.
// Type:    Static method.
// Args:    vrVarObj    - (R) The var value object.
// Returns: None.
// Throws:  None.
//--
void CMICmnLLDBDebugSessionInfoVarObj::VarObjAdd(
    const CMICmnLLDBDebugSessionInfoVarObj &vrVarObj) {
  const auto &name = vrVarObj.GetName();
  const auto &parent = vrVarObj.GetVarParentName();
  const auto old = ms_mapVarIdToVarObj.find(name);
  if (old != ms_mapVarIdToVarObj.end() &&
      old->second.GetVarParentName() != parent) {
    auto siblings = ms_mapChildren.find(old->second.GetVarParentName());
    if (siblings != ms_mapChildren.end()) {
      siblings->second.erase(name);
      if (siblings->second.empty())
        ms_mapChildren.erase(siblings);
    }
  }
  // Replacing a value must not delete its registered descendants.
  ms_mapVarIdToVarObj.insert_or_assign(name, vrVarObj);
  if (!parent.empty())
    ms_mapChildren[parent].insert(name);
}

//++
// Details: Delete the var object from the internal container matching the
// specified name.
// Type:    Static method.
// Args:    vrVarName   - (R) The var value name.
// Returns: Number of registered objects removed.
// Throws:  None.
//--
MIuint
CMICmnLLDBDebugSessionInfoVarObj::VarObjDelete(const CMIUtilString &vrVarName,
                                               bool vbChildrenOnly) {
  std::vector<CMIUtilString> pending =
      vbChildrenOnly ? VarObjGetChildren(vrVarName)
                     : std::vector<CMIUtilString>{vrVarName};
  MIuint deleted = 0;
  while (!pending.empty()) {
    const CMIUtilString name = std::move(pending.back());
    pending.pop_back();
    const auto children = ms_mapChildren.find(name);
    if (children != ms_mapChildren.end()) {
      pending.insert(pending.end(), children->second.begin(),
                     children->second.end());
      ms_mapChildren.erase(children);
    }
    const auto node = ms_mapVarIdToVarObj.find(name);
    if (node == ms_mapVarIdToVarObj.end())
      continue;
    const auto siblings = ms_mapChildren.find(node->second.GetVarParentName());
    if (siblings != ms_mapChildren.end()) {
      siblings->second.erase(name);
      if (siblings->second.empty())
        ms_mapChildren.erase(siblings);
    }
    ms_mapVarIdToVarObj.erase(node);
    ++deleted;
  }
  return deleted;
}

std::vector<CMIUtilString> CMICmnLLDBDebugSessionInfoVarObj::VarObjGetChildren(
    const CMIUtilString &vrVarName) {
  const auto children = ms_mapChildren.find(vrVarName);
  if (children == ms_mapChildren.end())
    return {};
  return {children->second.begin(), children->second.end()};
}

//++
// Details: Update an existing var object in the internal container.
// Type:    Static method.
// Args:    vrVarObj    - (R) The var value object.
// Returns: None.
// Throws:  None.
//--
void CMICmnLLDBDebugSessionInfoVarObj::VarObjUpdate(
    const CMICmnLLDBDebugSessionInfoVarObj &vrVarObj) {
  VarObjAdd(vrVarObj);
}

//++
// Details: Retrieve the var object matching the specified name.
// Type:    Static method.
// Args:    vrVarName   - (R) The var value name.
//          vrwVarObj   - (W) A var object.
// Returns: bool    - True = object found, false = object not found.
// Throws:  None.
//--
bool CMICmnLLDBDebugSessionInfoVarObj::VarObjGet(
    const CMIUtilString &vrVarName,
    CMICmnLLDBDebugSessionInfoVarObj &vrwVarObj) {
  const MapKeyToVarObj_t::const_iterator it =
      ms_mapVarIdToVarObj.find(vrVarName);
  if (it != ms_mapVarIdToVarObj.end()) {
    const CMICmnLLDBDebugSessionInfoVarObj &rVarObj = (*it).second;
    vrwVarObj = rVarObj;
    return true;
  }

  return false;
}

//++
// Details: A count is kept of the number of var value objects created. This is
// count is
//          used to ID the var value object. Reset the count to 0.
// Type:    Static method.
// Args:    None.
// Returns: None.
// Throws:  None.
//--
void CMICmnLLDBDebugSessionInfoVarObj::VarObjIdResetToZero() {
  ms_nVarUniqueId = 0;
}

//++
// Details: Default format is globally used as the data format when "natural" is
// in effect, that is, this overrides the default
// Type:    Static method.
// Args:    None.
// Returns: None.
// Throws:  None.
//--
void CMICmnLLDBDebugSessionInfoVarObj::VarObjSetFormat(
    varFormat_e eDefaultFormat) {
  ms_eDefaultFormat = eDefaultFormat;
}

//++
// Details: A count is kept of the number of var value objects created. This is
// count is
//          used to ID the var value object. Increment the count by 1.
// Type:    Static method.
// Args:    None.
// Returns: None.
// Throws:  None.
//--
void CMICmnLLDBDebugSessionInfoVarObj::VarObjIdInc() { ms_nVarUniqueId++; }

//++
// Details: A count is kept of the number of var value objects created. This is
// count is
//          used to ID the var value object. Retrieve ID.
// Type:    Static method.
// Args:    None.
// Returns: None.
// Throws:  None.
//--
MIuint CMICmnLLDBDebugSessionInfoVarObj::VarObjIdGet() {
  return ms_nVarUniqueId;
}

//++
// Details: Retrieve the value formatted object's name.
// Type:    Method.
// Args:    None.
// Returns: CMIUtilString & - Value's var%u name text.
// Throws:  None.
//--
const CMIUtilString &CMICmnLLDBDebugSessionInfoVarObj::GetName() const {
  return m_strName;
}

//++
// Details: Retrieve the value formatted object's variable name as given in the
// MI command
//          to create the var object.
// Type:    Method.
// Args:    None.
// Returns: CMIUtilString & - Value's real name text.
// Throws:  None.
//--
const CMIUtilString &CMICmnLLDBDebugSessionInfoVarObj::GetNameReal() const {
  return m_strNameReal;
}

//++
// Details: Retrieve the value formatted string.
// Type:    Method.
// Args:    None.
// Returns: CMIUtilString & - Value formatted string.
// Throws:  None.
//--
const CMIUtilString &
CMICmnLLDBDebugSessionInfoVarObj::GetValueFormatted() const {
  return m_strFormattedValue;
}

//++
// Details: Retrieve the LLDB Value object.
// Type:    Method.
// Args:    None.
// Returns: lldb::SBValue & - LLDB Value object.
// Throws:  None.
//--
lldb::SBValue &CMICmnLLDBDebugSessionInfoVarObj::GetValue() {
  return m_SBValue;
}

//++
// Details: Retrieve the LLDB Value object.
// Type:    Method.
// Args:    None.
// Returns: lldb::SBValue & - Constant LLDB Value object.
// Throws:  None.
//--
const lldb::SBValue &CMICmnLLDBDebugSessionInfoVarObj::GetValue() const {
  return m_SBValue;
}

//++
// Details: Set the var format type for *this object and update the formatting.
// Type:    Method.
// Args:    None.
// Return:  MIstatus::success - Functional succeeded.
//          MIstatus::failure - Functional failed.
// Throws:  None.
//--
bool CMICmnLLDBDebugSessionInfoVarObj::SetVarFormat(
    const varFormat_e veVarFormat) {
  if (veVarFormat >= eVarFormat_count)
    return MIstatus::failure;

  m_eVarFormat = veVarFormat;
  UpdateValue();
  return MIstatus::success;
}

//++
// Details: Update *this var obj. Update it's value and type.
// Type:    Method.
// Args:    None.
// Returns: None.
// Throws:  None.
//--
void CMICmnLLDBDebugSessionInfoVarObj::UpdateValue() {
  m_strFormattedValue = GetValueStringFormatted(m_SBValue, m_eVarFormat);

  MIuint64 nValue = 0;
  m_eVarType = CMICmnLLDBProxySBValue::GetValueAsUnsigned(m_SBValue, nValue)
                   ? eVarType_Internal
                   : eVarType_Composite;

  CMICmnLLDBDebugSessionInfoVarObj::VarObjUpdate(*this);
}

void CMICmnLLDBDebugSessionInfoVarObj::RefreshValue() {
  if (!m_strVarObjParentName.empty()) {
    CMICmnLLDBDebugSessionInfoVarObj parent;
    lldb::SBValue value;
    if (VarObjGet(m_strVarObjParentName, parent) &&
        parent.GetValue().IsInScope() &&
        parent.GetValue().GetNumChildren(m_nChildIndex + 1) > m_nChildIndex) {
      value = parent.GetValue().GetChildAtIndex(m_nChildIndex);
      // An index can acquire a different member when a dynamic type changes.
      if (CMICmnLLDBUtilSBValue(value).GetName() != m_strNameReal)
        value.Clear();
    }
    m_SBValue = std::move(value);
  } else if (m_eValObjKind == valObjKind_ec::eValObjKind_ConstResult) {
    // Child expressions are parent-relative names, not standalone expressions.
    lldb::SBExpressionOptions options;
    options.SetSuppressPersistentResult(true);
    m_SBValue =
        m_expressionFrame.EvaluateExpression(m_strNameReal.c_str(), options);
  }
  VarObjUpdate(*this);
}

CMICmnLLDBDebugSessionInfoVarObj::ValueState
CMICmnLLDBDebugSessionInfoVarObj::ReadValueState() {
  ValueState state;
  // Update only this SBValue. LLDB's change flag describes an execution stop,
  // whereas MI needs changes since the previous -var-update. Keep our own
  // snapshot so evaluating a value cannot consume a notification and polling
  // twice at the same stop cannot repeat it.
  m_SBValue.GetValueDidChange();
  state.inScope = m_SBValue.IsValid() && m_SBValue.IsInScope() &&
                  m_SBValue.GetError().Success();
  if (state.inScope) {
    state.value = GetValueStringFormatted(m_SBValue, eVarFormat_Natural);
    state.type = CMICmnLLDBUtilSBValue(m_SBValue).GetTypeNameDisplay();
  } else {
    state.type = m_lastUpdate.type;
  }
  return state;
}

bool CMICmnLLDBDebugSessionInfoVarObj::CheckForChanges(bool &vrwbTypeChanged) {
  const auto state = ReadValueState();
  vrwbTypeChanged = state.inScope && state.type != m_lastUpdate.type;
  const bool changed = ExchangeChanged(false) ||
                       state.inScope != m_lastUpdate.inScope ||
                       state.value != m_lastUpdate.value || vrwbTypeChanged;
  m_lastUpdate = state;
  UpdateValue();
  return changed;
}

//++
// Details: Set the changed state of the var object.
// Type:    Method.
// Args:    bChanged - New changed state.
// Returns: m_bChanged - Previous changed state.
// Throws:  None.
//--
bool CMICmnLLDBDebugSessionInfoVarObj::ExchangeChanged(const bool bChanged) {
  const bool bPreviousChanged = m_bChanged;
  m_bChanged = bChanged;
  return bPreviousChanged;
}

//++
// Details: Retrieve the enumeration type of the var object.
// Type:    Method.
// Args:    None.
// Returns: varType_e   - Enumeration value.
// Throws:  None.
//--
CMICmnLLDBDebugSessionInfoVarObj::varType_e
CMICmnLLDBDebugSessionInfoVarObj::GetType() const {
  return m_eVarType;
}

//++
// Details: Retrieve the parent var object's name, the parent var object  to
// *this var
//          object (if assigned). The parent is equivalent to LLDB SBValue
//          variable's
//          parent.
// Type:    Method.
// Args:    None.
// Returns: CMIUtilString & - Pointer to var object, NULL = no parent.
// Throws:  None.
//--
const CMIUtilString &
CMICmnLLDBDebugSessionInfoVarObj::GetVarParentName() const {
  return m_strVarObjParentName;
}

//++
// Details: Retrieve the kind of the value object.
// Type:    Method.
// Args:    None.
// Returns: ValObjKind_ec - Enumeration value.
// Throws:  None.
//--
ValObjKind_ec CMICmnLLDBDebugSessionInfoVarObj::GetValObjKind() const {
  return m_eValObjKind;
}
