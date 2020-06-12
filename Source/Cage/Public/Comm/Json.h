// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once
#include "JsonUtilities/Public/JsonObjectConverter.h"

class CAGE_API FJsonSerializer : public FJsonObjectConverter{
public:
template<typename InStructType>
static TSharedPtr<FJsonObjejct> UStructToJsonObject(InStructType ustruct);
};
