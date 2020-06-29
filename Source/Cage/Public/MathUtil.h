#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "MathUtil.generated.h"

UCLASS()
class CAGE_API UCageMathFunctionLibrary:public UBlueprintFunctionLibrary
{
GENERATED_BODY()
    public:
    UFUNCTION(BlueprintPure, BlueprintCallable, Category="CageMath")
    static float sinh(float x){return(exp(x)-exp(-x))/2.;}
};

constexpr double Pi_d=3.1415926535897932;
constexpr long double Pi_ld=3.1415926535897932384626433832795028841971L;


// returns F(s)+F(s+1)+...+F(e)
template <typename T>
constexpr auto Sigma(const int& s, const int& e, const T F)
{
    if (s>e) return decltype(F(0))(0);
    if (s == e) return F(s);
    return F(s) + Sigma(s + 1, e, F);
}

// returns F(s)*F(s+1)*...*F(e)
template <typename T>
constexpr auto Product(const int& s, const int& e, const T F)
{
    if (s>e) return decltype(F(0))(1);
    if (s == e) return F(s);
    return F(s) * Sigma(s + 1, e, F);
}

// returns v^2
template <typename N>
constexpr auto Sq(const N &v)
{
    return v*v;
}

// returns v^(-1)^m
template <typename N>
constexpr auto InvN(const int m, const N &v)
{
    if(m%2==1) return 1./v;
    return v;
}
// return (-1)^m
constexpr auto FlipN(const int n)
{
    if(n%2==1) return -1;
    return 1;
}

