#pragma once
#include <stddef.h>
#include <stdint.h>

// Partial, explicitly scoped OCT2011 data-layout checks. This is not a complete
// J1979-DA catalogue or a transport conformance claim. Unknown stays unknown.
namespace obd {
enum class DataValidation { LayoutChecked, Unverified, WrongLength, InvalidValue };
inline const char *validationName(DataValidation v) {
  switch(v) {
    case DataValidation::LayoutChecked:return "LAYOUT_CHECKED";
    case DataValidation::WrongLength:return "INVALID_LENGTH";
    case DataValidation::InvalidValue:return "INVALID_VALUE";
    default:return "UNVERIFIED_DEFINITION";
  }
}
inline bool invalidData(DataValidation v) {
  return v==DataValidation::WrongLength || v==DataValidation::InvalidValue;
}
inline bool supportIdentifier(uint8_t pid) {return pid<=0xE0 && !(pid%32);}
// Data bytes only, excluding service, PID and freeze-frame identifier.
// Reviewed: Appendix A; B2/B3/B5/B6/B12-B18/B23/B26-B29/B35/B39/B47/B54/B115.
// All other identifiers intentionally return 0 (not reviewed).
inline size_t reviewedPidLength(uint8_t pid) {
  if(supportIdentifier(pid))return 4;
  switch(pid) {
    case 0x01:return 4;
    case 0x02:case 0x0C:case 0x10:case 0x1F:case 0x21:case 0x23:case 0x42:return 2;
    case 0x04:case 0x05:case 0x0B:case 0x0D:case 0x0F:case 0x11:
    case 0x1C:case 0x2F:case 0x33:case 0x49:return 1;
    case 0x88:return 13;
    default:return 0;
  }
}
inline bool vinCharacter(uint8_t c) {
  return (c>='0'&&c<='9') || (c>='A'&&c<='Z'&&c!='I'&&c!='O'&&c!='Q');
}
// Only validate a fully reassembled, matched, single-identifier response.
// LayoutChecked means length/selected fields checked, NOT all semantics checked.
inline DataValidation validateData(const uint8_t *p,size_t n) {
  if(!p||!n)return DataValidation::WrongLength;
  if(p[0]==0x7F)return n==3?DataValidation::LayoutChecked:DataValidation::WrongLength;
  if(p[0]==0x44)return n==1?DataValidation::LayoutChecked:DataValidation::WrongLength;
  if(p[0]!=0x41&&p[0]!=0x42&&p[0]!=0x46&&p[0]!=0x49)return DataValidation::Unverified;
  const size_t offset=p[0]==0x42?3:2;
  if(n<offset)return DataValidation::WrongLength;
  if(supportIdentifier(p[1])) {
    if(n!=offset+4)return DataValidation::WrongLength;
    // E0's final bit is reserved, not a continuation to 100.
    return p[1]==0xE0&&(p[offset+3]&1)?DataValidation::InvalidValue:DataValidation::LayoutChecked;
  }
  if(p[0]==0x41||p[0]==0x42) {
    size_t count=reviewedPidLength(p[1]);
    if(!count)return DataValidation::Unverified;
    return n==offset+count?DataValidation::LayoutChecked:DataValidation::WrongLength;
  }
  // G2: CAN VIN. The single record is exactly 17 VIN characters.
  if(p[0]==0x49&&p[1]==0x02) {
    if(n!=20)return DataValidation::WrongLength;
    if(p[2]!=1)return DataValidation::InvalidValue;
    for(size_t i=3;i<20;++i)if(!vinCharacter(p[i]))return DataValidation::InvalidValue;
    return DataValidation::LayoutChecked;
  }
  return DataValidation::Unverified;
}
} // namespace obd
