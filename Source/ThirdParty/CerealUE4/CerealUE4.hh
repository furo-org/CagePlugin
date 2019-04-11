#pragma once
#include<string>

#include "cereal-detail-traits.hpp"
#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"
#include <cereal-UE4.hxx>

template<typename AT>
void CerealFromNVPImpl(AT &ar) {
}

template<typename AT, typename T, typename... Ts>
void CerealFromNVPImpl(AT &ar, const char* name, const T &data, const Ts&... args)
{
  ar(cereal::make_nvp(name, data));
  CerealFromNVPImpl(ar, args...);
}

template<typename AT = cereal::JSONOutputArchive, typename... Ts>
FString CerealFromNVP(Ts&&... args) {
  std::stringstream buffer;
  {
    AT a(buffer);
    CerealFromNVPImpl(a, std::forward<Ts>(args)...);
  }
  return UTF8_TO_TCHAR(buffer.str().data());
}


template<typename AT>
void CerealToNVPImpl(AT &&ar) {
}

template<typename AT, typename T, typename... Ts>
void CerealToNVPImpl(AT &&ar, const char* name, T &data, Ts&&... args)
{
  ar(cereal::make_nvp(name, data));
  CerealToNVPImpl(std::forward<AT>(ar), std::forward<Ts>(args)...);
}

template<typename AT = cereal::JSONInputArchive, typename... Ts>
void CerealToNVP(const TCHAR* data, Ts&... args) {
  //std::string str(TCHAR_TO_UTF8(data));
  std::istringstream buffer(TCHAR_TO_UTF8(data));
  CerealToNVPImpl(AT(buffer), args...);
}

template<typename AT, typename... Ts>
void CerealArchiveToNVP(AT &Ar, const TCHAR* data, Ts&... args) {
  CerealToNVPImpl(Ar, args...);
}
