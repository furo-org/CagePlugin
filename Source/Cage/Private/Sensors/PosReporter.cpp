// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "PosReporter.h"
#include "Comm/Types.h"
#include "JsonUtilities/Public/JsonObjectConverter.h"
#include "GeoReference.h"
#include "EngineUtils.h"

void UPosReporter::BeginPlay()
{
    Super::BeginPlay();
    GeoReference=nullptr;
}

TSharedPtr<FJsonObject> UPosReporter::CommSend(const float DeltaTime, UActorCommMgr *CommMgr)
{
    FActorPositionReport rep;
    //const auto Trans=GetOwner()->GetActorTransform();
    const auto Trans=CommMgr->GetBaseTransform();

    rep.Position = Trans.GetLocation(); //GetOwner()->GetActorLocation();
    rep.Pose = Trans.GetRotation();
    rep.Yaw = rep.Pose.Euler().Z;

    auto jo=FJsonObjectConverter::UStructToJsonObject(rep);

    //GeoReferenceがあればその参照を取得
    if(SendGeoLocation)
    {
        if( !IsValid(GeoReference))
        {
            for(TActorIterator<AGeoReference> itr(GetWorld());itr;++itr)
            {
                UE_LOG(LogTemp, Verbose, TEXT("UPosReporter[%s]: AGeoReference found"),*GetOwner()->GetName());
                GeoReference=*itr;
            }
        
        }
        if(IsValid(GeoReference))
        {
            FActorGeolocation geo;
            double x=rep.Position.X;
            double y=rep.Position.Y;
            GeoReference->GetBL(x,y,geo.Lat,geo.Lon);
            auto gj=FJsonObjectConverter::UStructToJsonObject(geo);
            jo->Values.Append(gj->Values);
        }
    }
    return jo;
}
