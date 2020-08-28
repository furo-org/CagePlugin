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
#include "Comm/Comm.h"

#include "GeoReference.generated.h"

USTRUCT()
struct CAGE_API FGeoLocation
{
    GENERATED_USTRUCT_BODY()
    UPROPERTY()
    FVector Latitude;
    UPROPERTY()
    FVector Longitude;
};

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
    void GetXY(double B, double L, double& Xout, double& Yout);

    bool IsValid(){return Initialized;}
private:
    bool Initialized=false;
    std::array<double, 6> Alpha;  // Alpha[1] ... Alpha[5]
    std::array<double, 6> Beta;   // Beta[1] ... Beta[5]
    std::array<double, 7> Delta;  // Delta[1] ... Delta[6]
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
class CAGE_API AGeoReference : public AActor
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
    void GetBL(float x, float y, FVector &Bout, FVector &Lout)
    {
        double B, L;
        GetBL(x, y, B, L);
        Bout=Encode60(B);
        Lout=Encode60(L);
        float x2,y2;
        GetXY(Bout, Lout, x2,y2);
        //UE_LOG(LogTemp,Warning,TEXT("float: in=(%f,%f) BL=(%s, %s), retxy=(%f,%f)"),x,y,*Bout.ToString(), *Lout.ToString(),x2,y2);
        //UE_LOG(LogTemp,Warning,TEXT(" B:%f  Bout: %s  Bret:%f"),B, *Bout.ToString(), Decode60(Bout));
        //UE_LOG(LogTemp,Warning,TEXT(" L:%f  Lout: %s  Lret:%f"),L, *Lout.ToString(), Decode60(Lout));
    }
    void GetBL(double x, double y, double& Bout, double& Lout)
    {
        if(!Geo.IsValid()) InitializeGeoConv();
        auto l=GetActorLocation();
        auto r=GetActorRotation()*-1;
        auto rv=r.RotateVector(FVector(x-l.X,y-l.Y,0));
        Geo.GetBL(rv.X/100.,rv.Y/100.,Bout,Lout);
        //UE_LOG(LogTemp,Warning,TEXT("in=(%f,%f) rot=(%f, %f) BL=(%f, %f)"),x,y,rv.X,rv.Y,Bout, Lout);
        double x2,y2;
        GetXY(Bout, Lout, x2,y2);
        //UE_LOG(LogTemp,Warning,TEXT("in=(%f,%f) rot=(%f, %f) BL=(%f, %f), retxy=(%f,%f)"),x,y,rv.X,rv.Y,Bout, Lout,x2,y2);
    }

    UFUNCTION(BlueprintCallable)
    void GetXY(FVector B, FVector L, float& Xout, float& Yout)
    {
        double X, Y;
        //UE_LOG(LogTemp,Warning,TEXT("float  GetXY: B=%f L=%f "),Decode60(B), Decode60(L));
        GetXY(Decode60(B), Decode60(L), X, Y);
        Xout = X;
        Yout = Y;
        //UE_LOG(LogTemp,Warning,TEXT("float  GetXY: B=%f L=%f double=(%f,%f) float=(%f,%f)"),b,l, X,Y, Xout, Yout);
    }
    void GetXY(double B, double L, double& Xout, double& Yout)
    {
        //UE_LOG(LogTemp,Warning,TEXT("double GetXY: B=%lf L=%lf "),B,L);
        if(!Geo.IsValid()) InitializeGeoConv();
        double x,y;
        Geo.GetXY(B,L,x,y);
        x*=100;  // [m] ->[cm]
        y*=100;
        auto r=GetActorRotation();
        auto rv=r.RotateVector(FVector(x,y,0));
        auto l=GetActorLocation();
        Xout=rv.X+l.X;
        Yout=rv.Y+l.Y;
    }
    UFUNCTION(BlueprintCallable)
    void InitializeGeoConv();    // Rebuild internal tables

protected:
    void BeginPlay() override;
    GeoConv Geo;
    CommEndpointIO<FSimpleMessage> Comm;
    double Decode60(const FVector &v)
    {
        double d=v.X;
        double m=v.Y;
        double s=v.Z;
        return ((s/60.)+m)/60.+d;
    }
    FVector Encode60(const double &v)
    {
        double x=floor(v);
        double y=floor((v-x)*60.);
        double z=(v-x-y/60.)*3600.;
        return FVector(x,y,z);
    }
    private:
};
