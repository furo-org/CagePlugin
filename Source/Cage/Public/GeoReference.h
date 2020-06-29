// Copyright 2020 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include <array>
#include <stdbool.h>

#include "GeometryCollectionSimulationCoreTypes.h"


#include "GeoReference.generated.h"

class GeoConv
{
public:
    void BuildTable(double phi0, double lamda0, double a, double f, double scale)
    {
        A=a;F=f;m0=scale;
        BuildTable(phi0,lamda0);
    }
    void BuildTable(double phi0, double lamda0);
    void GetBL(double x, double y, double& Bout, double& Lout);

    bool IsValid(){return Initialized;}
private:
    bool Initialized=false;
    std::array<double, 6> Beta;
    std::array<double, 7> Delta;
    double A=6378137.;
    double F=298.257222101;  // GRS80
    double m0 = 1; // 平面直角座標系では 通常 0.9999。で適用範囲が160km。ここではそんなに遠くは見ないことにして1とする。
    double Phi0, Lambda0; // 平面の中心座標

    double N;
    double Sp;
    double Sphi0;
    template<int lmax=5>
    constexpr double S(const double phi);
    template<int lmax=5>
    double Sx(const double phi);
};


/**
 * 
 */
UCLASS()
class CAGE_API AGeoReference : public ATargetPoint
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, meta = (ClampMin = -90, ClampMax = 90), Category="Location")
    double Lat_Deg=36.;
    UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 60), Category="Location")
    double Lat_Min=0;
    UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 60), Category="Location")
    double Lat_Sec=0;
    UPROPERTY(EditAnywhere, meta = (ClampMin = -180, ClampMax = 180), Category="Location")
    double Lon_Deg=139.;
    UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 60), Category="Location")
    double Lon_Min=50.;
    UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 60), Category="Location")
    double Lon_Sec=0.;
    UPROPERTY(EditAnywhere, Category="Elipsoid")
    double A=6378137.;       // GRS80
    UPROPERTY(EditAnywhere, Category="Elipsoid")
    double F=298.257222101;  // GRS80
    UPROPERTY(EditAnywhere, Category="Elipsoid")
    double M0=1.;             // 平面直角座標系では 通常 0.9999。で適用範囲が160km。ここではそんなに遠くは見ないことにして1とする。

    UFUNCTION(BlueprintCallable)
    void GetBL(float x, float y, float& Bout, float& Lout)
    {
        double B, L;
        GetBL(x, y, B, L);
        Bout = B;
        Lout = L;
    }
    void GetBL(double x, double y, double& Bout, double& Lout)
    {
        if(!Geo.IsValid()) InitializeGeoConv();
        auto l=GetActorLocation();
        auto r=GetActorRotation()*-1;
        auto rv=r.RotateVector(FVector(x-l.X,y-l.Y,0));
        // flip Y axis
        Geo.GetBL(rv.X/100.,rv.Y/100.,Bout,Lout);
        //UE_LOG(LogTemp,Warning,TEXT("in=(%f,%f) rot=(%f, %f) BL=(%f, %f)"),x,y,rv.X,rv.Y,Bout, Lout);
    }

    UFUNCTION(BlueprintCallable)
    void InitializeGeoConv();    // Rebuild internal tables

protected:
    void BeginPlay() override;
    GeoConv Geo;
private:
};
