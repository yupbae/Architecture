#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>

#include "architecture.h"

int addALU(int var_1, int var_2, struct flags_struct *flags){

    int carry=0;
    int sum=var_1;
    int temp_1=var_1;
    int temp_2=var_2;
        while (temp_2 != 0)
        {
            carry=sum & temp_2;
            sum=sum ^ temp_2;
            temp_2=carry<<1;
        }
        if (sum<0){
            flags->sign=true;
        }
        if (sum==0){
            flags->zero=true;
        }
        //overflow conditions
        if( (var_1<0 && var_2<0 && sum>=0) || (var_1>0 && var_2>0 && sum<=0) ){
            flags->overflow=true;
        }
        if (carry<=-1){
            flags->carry=true;
        }

        return sum;
    }

int subALU(int var_1, int var_2,struct flags_struct *flags)
    {
        int borrow=0;
        int sub=var_1;
        int temp_2=var_2;
        while (temp_2 != 0)
        {
            borrow=(~sub) & temp_2;
            sub=sub ^ temp_2;
            temp_2=borrow<<1;
        }
        if (borrow<=-1){
            flags->carry=true;
        }
        if (sub<0){
            flags->sign=true;
        }
        if (sub==0){
            flags->zero=true;
        }
        //overflow conditions
        if( (var_1< 0 && var_2<0 && sub>=0) || (var_1>0 && var_2>0 && sub<0) ){
            flags->overflow=true;
        }
        return sub;
    }


    int mulALU(int var_1, int var_2,struct flags_struct *flags)
        {
             int mult=0;
             int temp_1=var_1;
             int temp_2=var_2;

            while (temp_1!=0)
            {
                if (temp_2 & 1)
                {
                    mult=addALU(mult,temp_1,flags);
                }
                temp_1<<=1;
                temp_2>>=1;
            }
            if (mult<0){
                flags->sign=true;
            }
            if (mult==0){
                flags->zero=true;
            }
            //overflow conditions
            if( (var_1<0 && var_2<0 && mult<=0) || (var_1>0 && var_2>0 && mult<=0)||(var_1>0 && var_2<0 && mult>=0)||(var_1<0 && var_2>0 && mult>=0)){
                flags->overflow=true;
                flags->carry=true;
            }
            return mult;
        }

int divALU(int dividend, int divisor,struct flags_struct *flags){

    int quotient=0;
    int neg_result=0;
    //setting up a sign if either divienden or divisor is negative
    if ((dividend<0 && divisor>-1) || (dividend>-1 && divisor<0) ){
        neg_result=1;
    }
    // check if Dividend is negative
    if (dividend<0){
        dividend=addALU(~dividend, 1,flags);
    }
    // Check if Divisor is negative
    if (divisor<0){
        divisor=addALU(~divisor,1,flags);
    }

    while (dividend>=divisor) {
        dividend=subALU(dividend,divisor,flags);
        quotient=addALU(quotient,1,flags);
    }
    // negating quotient
    if(neg_result==1) {
        quotient=addALU(~quotient, 1,flags) ;
    }
    flags->sign=false;
    flags->zero=false;
    flags->overflow=false;
    flags->carry=false;

    if (quotient<0 || neg_result){
            flags->sign=true;
    }
    if (quotient==0){
        flags->zero=true;
    }
    return quotient;
}

int modALU(int var_1, int var_2,struct flags_struct *flags){

    // using formula : remender= a - quotient * b
       int quotient=divALU(var_1,var_2,flags);
       int remender=subALU(var_1, mulALU(quotient,var_2,flags),flags);

       flags->sign=false;
       flags->zero=false;
       flags->overflow=false;
       flags->carry=false;
       if (remender==0){
           flags->zero=true;
       }
       return remender;
}

int andALU(int var_1, int var_2,struct flags_struct *flags){

    int temp_1=var_1;
    int temp_2=var_2;
    int andp;
        {
            andp=temp_1 & temp_2;

        }
        if (andp<=0){
            flags->zero=true;
        }
        return andp;
    }

int orALU(int var_1, int var_2,struct flags_struct *flags)
    {
        int temp_1=var_1;
        int temp_2=var_2;
        int orp;
        {
        orp=temp_1 | temp_2;
        }

        if (orp==0){
            flags->zero=true;
        }
            return orp;
    }


int xorALU(int var_1, int var_2,struct flags_struct *flags)
{
     int temp_1=var_1;
     int temp_2=var_2;
     int xrop;
     {
         xrop=temp_1 ^ temp_2;
    }
    if (xrop==0){
        flags->zero=true;
    }
    return xrop;
}

int notALU(unsigned int var_1,struct flags_struct *flags)
{
     unsigned int temp_1=var_1;
         unsigned int nt;
     {
         nt= ~temp_1;
    }
    if (nt==0){
        flags->zero=true;
    }

    return nt;
}

int shiftrALU(int var_1,int var_2,struct flags_struct *flags)
{
     int temp_1=var_1;
     int temp_2=var_2;
     int srl;

    srl=temp_1 >> temp_2;
    if (srl==0){
        flags->zero=true;
    }
    return srl;
}
int shiftlALU(int var_1,int var_2,struct flags_struct *flags){
     int temp_1=var_1;
     int temp_2=var_2;
     int sll;
     {
         sll=temp_1 << temp_2;
    }
    if (sll==0){
        flags->zero=true;
    }
    return sll;
}
