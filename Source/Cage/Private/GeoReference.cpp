// Copyright 2020 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "GeoReference.h"
#include <array>


#include "ActorCommMgr.h"
#include "JsonObjectConverter.h"
#include "MathUtil.h"

void AGeoReference::InitializeGeoConv()
{
    double phi0=Decode60(FVector(Lat_Deg, Lat_Min, Lat_Sec))*Pi_d/180;
    double lambda0=Decode60(FVector(Lon_Deg, Lon_Min, Lon_Sec))*Pi_d/180;
    //UE_LOG(LogTemp,Warning, TEXT("GeoReference::InitializeGeoConv phi0=%f  lambda0=%f"), phi0, lambda0);
    Geo.BuildTable(phi0, lambda0, A,F,M0);
}

void AGeoReference::BeginPlay()
{
    InitializeGeoConv();
    TSharedRef<FJsonObject> meta=MakeShared<FJsonObject>();

    FTransformReport rep;
    rep.Translation=GetTransform().GetTranslation();
    rep.Rotation = GetTransform().GetRotation();
    meta->SetObjectField("Transform",FJsonObjectConverter::UStructToJsonObject(rep));

    FGeoLocation loc;
    loc.Latitude=FVector(Lat_Deg, Lat_Min, Lat_Sec);
    loc.Longitude=FVector(Lon_Deg, Lon_Min, Lon_Sec);
    meta->SetObjectField("GeoLocation",FJsonObjectConverter::UStructToJsonObject(loc));

    FString Metadata;
    JsonObjectToFString(meta,Metadata);
    Comm.setup(GetFName(), "GeoReference", Metadata);
}

FString dtofs(const char *fmt, const double v)
{
    char buff[256];
    snprintf(buff,sizeof(buff),fmt, v);
    return FString(ANSI_TO_TCHAR(buff));
}

// http://www.gsi.go.jp/common/000061216.pdf

template<int lmax>
constexpr double GeoConv::S(const double phi)
{
    return A / (1 + N) * Sigma(0, lmax, [&](int j)
    {
        return Sq(Product(1, j, [&](int k) { return 3. * N / (2. * k) - N; }))
            * (phi + Sigma(1,2*j,[&](int l){return (1./l - 4.*l)*sin(2.*l*phi)*Product(1,l,
                [&](int m){return InvN(m, 3*N/(2*(j+FlipN(m)*floor(m/2.)))-N);}); }));
    }) ;
}
template<int lmax>
double GeoConv::Sx(const double phi)
{
    const int lmax2=lmax*2;
    const double n15 = N * 1.5;
    const double phi2=phi*2.;
    std::vector<double> e;
    std::vector<double> t;
    std::vector<double> s;
    e.resize(lmax2+1);
    t.resize(lmax2+1);
    s.resize(lmax2+2);
    s[0] = 0;

    double ep = 1.0;
    for (int k = 1; k <= lmax; k++)
    {
        e[k] = n15 / k - N;
        e[k + lmax] = n15 / (k + lmax) - N;
        ep *= e[k];
    //UE_LOG(LogTemp, Warning, TEXT("e[%d] = %s"),k, *dtofs("%.16g",e[k]));
    }

    double dc = 2.0 * cos(phi2);
    s[1] = sin(phi2);
    for (int i = 1; i <= lmax2; i++)
    {
        // sin(2 l phi) table
        // sin(2 l phi) = 2 cos(2phi) sin(2 (l-1) phi) - sin(2 (l-2) phi)
        s[i + 1] = dc * s[i] - s[i - 1];

        // (1/l - 4l)sin(2 l phi)  table
        t[i] = (1.0 / i - 4.0 * i) * s[i];
    }
    double sum = 0.0;
    double c1 = ep;
    for(int j=lmax; j>0;--j)
    {
        double c2 = phi2;
        double c3 = 2.0;
        for(int m=1, l=j;l>0;--l,m+=2)
        {
            c3 /= e[l];
            c2 += c3 * t[m];
            c3 *= e[2 * j - (l-1)];
            c2 += c3 * t[m+1];
        }
        sum += c1 * c1 * c2;
        c1 /= e[j];
    }
    return 0.5*A/(1.+N) * (sum + phi2);
}

void GeoConv::BuildTable(double phi0, double lambda0)
{
    Phi0=phi0; Lambda0=lambda0;
    // Build Bata and Delta
    std::array<double, 7> n;
    n[1] = 1. / (2 * F - 1.);
    N=n[1];
    // fill n^1 ... n^6 in n[0] ... n[5]
    for (int i = 2; i < n.size(); ++i)  // 1 base
    {
        n[i] = n[1] * n[i - 1];
    }

    constexpr std::array<double,5> ATable[]  // 0 base
    {
        {1./2, -2./3, 5./16, 41./180, -127./288 },
        {0, 13./48, -3./5, 557./1440, 281./630 },
        { 0, 0, 61./240, -103./140, 15061./26880,},
        { 0, 0, 0, 49561./161280, -179./168},
        { 0, 0, 0, 0, 34729./80640}
    };
    
    constexpr std::array<double, 5> BTable[] // 0 base
    {
        {1. / 2, -2. / 3., 37. / 96., -1. / 360., -81. / 512.},
        {0, 1. / 48., 1. / 15., -437. / 1440., 46. / 105.},
        {0, 0, 17. / 480., -37. / 840., -209. / 4480.},
        {0, 0, 0, 4397. / 161280., -11. / 504.},
        {0, 0, 0, 0, 4583. / 161280.}
    };

    constexpr std::array<double, 6> DTable[] // 0 base
    {
        {2., -2. / 3., -2., 116. / 45., 26. / 45., -2854. / 675.},
        {0, 7. / 3., -8. / 5., -227. / 45., 2704. / 315., 2323. / 945.},
        {0, 0, 56. / 15., -136. / 35., -1262. / 105., 73814. / 2835.},
        {0, 0, 0, 4279. / 630., -332. / 35., -399572. / 14175.},
        {0, 0, 0, 0, 4174. / 315., -144838. / 6237.},
        {0, 0, 0, 0, 0, 601676. / 22275.}
    };


    auto Lookup = [&](const auto& table)
    {
        double ret = 0;
        for (int j = 1; j <= table.size(); ++j)
        {
            ret += n[j] * table[j-1];
        }
        return ret;
    };

    for (int i = 1; i < Alpha.size(); ++i) // beta[1]...beta[5]
        Alpha[i] = Lookup(ATable[i-1]);
    for (int i = 1; i < Beta.size(); ++i) // beta[1]...beta[5]
        Beta[i] = Lookup(BTable[i-1]);
    for (int i = 1; i < Delta.size(); ++i) // delta[1]...delta[6]
        Delta[i] = Lookup(DTable[i-1]);

    Sp=Sx<5>(Pi_d/2.);
    Sphi0=Sx<5>(Phi0);;
    //Sp=S(Pi_d/2.);
    //Sphi0=S(Phi0);;
    Initialized=true;
#if 0
    UE_LOG(LogTemp, Warning, TEXT("A=%s, F=%s, N=%s n^1=%s"),*dtofs("%10.9lf",A), *dtofs("%10.9lf",F), *dtofs("%10.9lg",N), *dtofs("%10.9lg",n[1]));
    UE_LOG(LogTemp, Warning, TEXT("Sp=%s, Sphi0=%s, Phi0=%s Lambda0=%f"),*dtofs("%10.16lf",Sp), *dtofs("%10.16lf",Sphi0), *dtofs("%10.16lf",Phi0), Lambda0);

    for(int i=1;i<Beta.size();++i)
        UE_LOG(LogTemp, Warning, TEXT("Beta[%d]=%s"),i,*dtofs("%.16lg",Beta[i]));
    for(int i=1;i<Delta.size();++i)
        UE_LOG(LogTemp, Warning, TEXT("Delta[%d]=%s"),i,*dtofs("%.16lg",Delta[i]));
    for(int i=1;i<n.size();++i)
        UE_LOG(LogTemp, Warning, TEXT("n^%d = %s"),i,*dtofs("%10.16lg",n[i]));
    double B,L;
    UE_LOG(LogTemp, Warning, TEXT("Phi0 = %s  Lambda0=%s"),*dtofs("%10.8lg",Phi0*180/Pi_d),*dtofs("%10.8lg",Lambda0*180/Pi_d));
    GetBL(0,0,B,L);
    UE_LOG(LogTemp, Warning, TEXT("0,0 -> B = %s  L=%s"),*dtofs("%10.8lg",B),*dtofs("%10.8lg",L));
    GetBL(1000,0,B,L);
    UE_LOG(LogTemp, Warning, TEXT("1000,0 -> B = %s  L=%s"),*dtofs("%10.8lg",B),*dtofs("%10.8lg",L));
    GetBL(0,1000,B,L);
    UE_LOG(LogTemp, Warning, TEXT("0,1000 -> B = %s  L=%s"),*dtofs("%10.8lg",B),*dtofs("%10.8lg",L));
    GetBL(1000,1000,B,L);
    UE_LOG(LogTemp, Warning, TEXT("1000,1000 -> B = %s  L=%s"),*dtofs("%10.8lg",B),*dtofs("%10.8lg",L));
#endif
}

void GeoConv::GetBL(double x, double y, double& Bout, double& Lout)
{
    double xi = Pi_d/(2*m0*Sp)*(x+m0*Sphi0);
    double eta = Pi_d*y/(2*m0*Sp);
    double xip = xi - Sigma(1, 5, [&](int j) { return Beta[j] * sin(2 * j * xi) * cosh(2 * j * eta); });
    double etap = eta - Sigma(1, 5, [&](int j) { return Beta[j] * cos(2 * j * xi) * sinh(2 * j * eta); });
    double chi = asin(sin(xip) / cosh(etap));
    double kai=asin(sin(xip)/cosh(etap));
    double phi=kai+Sigma(1,6,[&](int j){return Delta[j]*sin(2*j*kai); });
    double dLambda=atan2(sinh(etap),cos(xip));
    double lambda=Lambda0 +dLambda;
    Bout=phi*180/Pi_d;
    Lout=lambda*180/Pi_d;
    //UE_LOG(LogTemp,Warning,TEXT("xi=%s  xip=%s  eta=%s etap=%s"),*dtofs("%.16g",xi),*dtofs("%.16g",xip),*dtofs("%.16g",eta),*dtofs("%.16g",etap));
}
void GeoConv::GetXY(double B, double L, double& Xout, double& Yout)
{
    double dLambda=L*Pi_d/180.-Lambda0;
    double phi=B*Pi_d/180.;
    double e2n=2*sqrt(N)/(1+N);
    double tanChi=sinh(atanh(sin(phi))-e2n*atanh(e2n*sin(phi)));
    double xip=atan2(tanChi,cos(dLambda));
    double etap=atanh(sin(dLambda)/sqrt(1+Sq(tanChi)));
    Xout=(2*m0*Sp)/Pi_d*(xip  + Sigma(1,5,[&](int j){return Alpha[j]*sin(2*j*xip)*cosh(2*j*etap);}))-m0*Sphi0;
    Yout=(2*m0*Sp)/Pi_d*(etap + Sigma(1,5,[&](int j){return Alpha[j]*cos(2*j*xip)*sinh(2*j*etap);}));
}
