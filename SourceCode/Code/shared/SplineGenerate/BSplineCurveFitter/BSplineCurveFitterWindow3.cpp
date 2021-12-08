// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2020
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.08.13

#include "BSplineCurveFitterWindow3.h"
#include <Graphics/VertexColorEffect.h>
#include <random>
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
  
 
using namespace std;
#define mDimension 3

float *smd_ = nullptr;
int width_;

int totalSample = 0;
int TotalControlNum; 
int DeterminedNumControls, DeterminedDegree;
float minError; 
vector<Vector3<float>> controlData(10000);

ofstream OutFile, OutFile1;
int width, height;
float minErrorThreshold;
float storevector[mDimension]={0,0,0};
vector<Vector3<float>> controlDataVec(1000);
unique_ptr<BSplineCurveGenerate<float>> SplineGeneratePtr;
bool mergeOrNot = false;   
bool deleteshort = false;
unsigned int MinAllowableLength = 4;

BSplineCurveFitterWindow3::BSplineCurveFitterWindow3()
{
    //cout<<"good-----!!"<<endl;
}
int BSplineCurveFitterWindow3::SplineFit(vector<vector<Vector3<float>>> BranchSet, float hausdorff_,
float diagonal_, int layerNum, vector<int *> connection_,int width, float *smd)
{
    TotalControlNum = 0;
    sampleSet = BranchSet;
    minErrorThreshold = hausdorff_;
    diagonal = diagonal_;
    connection = connection_;
    width_ = width;
    smd_ = smd;
    
    if (mergeOrNot) {Merge();}

    for (unsigned int i = 0; i < sampleSet.size();i++)
    {
        if (sampleSet[i].size()>3) 
            CalculateNeededCP(sampleSet[i]);
    }
    smd_= nullptr; 
    return TotalControlNum; 
}

void BSplineCurveFitterWindow3::SplineFit2(vector<vector<Vector3<float>>> BranchSet, float hausdorff_,
float diagonal_, int layerNum, vector<int *> connection_, int sign,int width, float *smd)
{
    sampleSet = BranchSet;
    minErrorThreshold = hausdorff_;
    diagonal = diagonal_;
    connection = connection_; 
    width_ = width;
    smd_ = smd;
    
    if (mergeOrNot) { Merge();}

    OutFile1.open("controlPoint.txt",ios_base::app);
    if (layerNum == 255) OutFile1<<254<<endl; // avoid conflict to the final end tag.
    else OutFile1<<layerNum<<endl;

    if(sign==1) OutFile1<<1<<endl;
    else OutFile1<<0<<endl;
    //cout<<"sampleSet.size(): "<<sampleSet.size()<<endl;
    for (unsigned int i = 0; i < sampleSet.size();i++)
    {
        if (sampleSet[i].size()>3) 
            CreateBSplinePolyline(sampleSet[i]);
    }
    OutFile1<<0<<endl;  //end sign for one layer.  
    OutFile1.close(); 
    smd_= nullptr;
}


void BSplineCurveFitterWindow3::SplineGenerate(int SuperR)
{
    int layerNum;
    int CPnum,degree;
    unsigned int numSamples;
    string str;
    float controlData;
    OutFile.open("sample.txt");

    vector<float> mControlData;

    ifstream ifs1("ControlPoints.txt"); 
    ifs1 >> str;
    int width = (int)atof(str.c_str());
    OutFile << width <<" ";
    ifs1 >> str;
    int height = (int)atof(str.c_str());
    OutFile << height <<endl;

    diagonal = sqrt((float)(width * width + height * height));
    
    while(ifs1)
    { 
        ifs1 >> str;
        layerNum = (int)atof(str.c_str());
        if(ifs1.fail())  break;

        if(layerNum == 255)
        {
            OutFile << 65536 <<endl;//final end tag.
            ifs1 >> str;
            layerNum = (int)atof(str.c_str());
            if(ifs1.fail())  break;
            if(layerNum == 255) {OutFile << 65536 <<endl; break;}
        }

        OutFile << layerNum <<endl;
        
        while (true)
        {
            ifs1 >> str;
            CPnum = (int)atof(str.c_str());
            if(CPnum == 0) break;

            ifs1 >> str;
            degree = (int)atof(str.c_str());
            ifs1 >> str;
            numSamples = (unsigned int)atof(str.c_str());
        
            
            for (int i = 0; i< CPnum; ++i)
            {
                for (int j = 0; j < mDimension; ++j)
                {
                    ifs1 >> str;
                    controlData = atof(str.c_str())/diagonal;
                    controlDataVec[i][j] = controlData;
                    mControlData.push_back(controlData); 
                }
            }
                
            totalSample += numSamples;
            SplineGeneratePtr = std::make_unique<BSplineCurveGenerate<float>>(mDimension, degree, mControlData, CPnum);
            //cout<<"mControlData.size(): "<<mControlData.size()/3<<" count: "<<count1<<endl;
        
            controlDataVec.resize(CPnum);
            CreateGraphics(numSamples, SuperR);
            mControlData.clear(); 
        }
        OutFile << 65535 <<endl;//end tag for each layer.
    }
  
    OutFile.close();
    ifs1.close();

}


void BSplineCurveFitterWindow3::CreateGraphics(unsigned int numSamples, int superR)
{
    
    unsigned int numSplineSample = (unsigned int)(numSamples*superR*1.5);//sub-pixel.
   // unsigned int numSplineSample = numSamples; //uniform sampling
    float vector[mDimension]={0,0,0};
    float multiplier = 1.0f / (numSplineSample - 1.0f);

    for (unsigned int i = 0; i < numSplineSample; ++i)
    {
        float t = multiplier * i;
       
        SplineGeneratePtr->GetPosition(t, reinterpret_cast<float*>(vector));
        OutFile<<vector[0]*diagonal<<" "<<vector[1]*diagonal<<" "<<vector[2]*diagonal<<endl;      //save to the txt file.
    }
}

float BSplineCurveFitterWindow3::Judge(vector<Vector3<float>> Sample)
{
    unsigned int numSamples = (unsigned int)Sample.size();
    vector<Vector3<float>> SplineSamples(10000);
    
    float CPandError = 0;
     float factor = 1.0;
    
    unsigned int numSplineSamples = (unsigned int)(numSamples * 1.4);
    //unsigned int numSplineSamples = numSamples; // uniform sampling.
    float multiplier = 1.0f / (numSplineSamples - 1.0f);
    SplineSamples.resize(numSplineSamples);
    
    int minDegree;
    int numControls = 2;
   // int iter = 0;
    while (1)
    {
        //iter ++;
        minError = 100.0f; minDegree = 10;
        for (int degree = 1; degree < numControls; degree++)
        {
            if (degree > 10) break;
            mSpline = std::make_unique<BSplineCurveFit<float>>(mDimension, static_cast<int>(Sample.size()),
            reinterpret_cast<float const*>(&Sample[0]), degree, numControls);

            for (unsigned int i = 0; i < numSplineSamples; ++i)
            {
                float t = multiplier * i;
                mSpline->GetPosition(t, reinterpret_cast<float*>(storevector));
                 for(int y=0;y<mDimension;y++)
                    SplineSamples[i][y] = storevector[y];
            }
            
        // Compute error measurements.
            float maxLength = 0.0f;
            Vector3<float> diff;
            float sqrLength, minLength;
            float weight = 0.0;
            for (unsigned int i = 0; i < numSamples; ++i)
            {
                minLength = 100.0f;
                for (unsigned int j = 0; j < numSplineSamples; ++j)
                {
                    diff = Sample[i] - SplineSamples[j];
                    sqrLength = Dot(diff, diff);
                    if (sqrLength < minLength) {minLength = sqrLength; if (minLength == 0) break;}
                }
                if (minLength > maxLength) maxLength = minLength;
                if(smd_!= nullptr){
                    int index = (int)(Sample[i][1]*diagonal) * width_ + (int)(Sample[i][0]*diagonal);
                    //cout<<Sample[i][0]<<"/ "<<Sample[i][1]<<" ";
                    weight += pow(2.0, (smd_[index]/255.0 - 1.0));//from 1/2 to 1
                }
            }
            if(smd_!= nullptr) factor = weight/(float)numSamples; //saliency factor
            hausdorff = std::sqrt(maxLength);
            if (minError > hausdorff) { minError = hausdorff; minDegree = degree;}
            //cout<<numControls<<" hausdorff: "<<hausdorff<<" degree: "<<degree<<endl;
        }
        //cout<<numControls<<" minError: "<<minError<<" minDegree: "<<minDegree<<endl;
        if (numControls > 15) //If numControl > 13, it's easy to get crush.
        {
            CPandError = 100;//assign an big enough value
            return CPandError;
        }
        if (minError < (minErrorThreshold/factor))
        {
            DeterminedNumControls = numControls;
            DeterminedDegree = minDegree;
            //cout<<" minError: "<<minError<<endl;
            CPandError = numControls + minError;

            break;
        }
        else numControls ++;
    }
   
    return CPandError;
}

void BSplineCurveFitterWindow3::CalculateNeededCP(vector<Vector3<float>> Sample)
{
    float cpError = Judge(Sample);
    
    if (cpError == (float)100) //the branch may be too long to fit well.
    {
        vector<Vector3<float>> first = Sample;//init to VOID segmentation fault.
        vector<Vector3<float>> second = Sample;
        
        for (unsigned int i = Sample.size()/2; i < Sample.size(); ++i) //split in the half
            first[i] = 0;
        first.resize(Sample.size()/2);  
        for (unsigned int i = Sample.size()/2; i < Sample.size(); ++i)
            second[i - Sample.size()/2] = Sample[i];
        second.resize(Sample.size() - Sample.size()/2);  
   
        CalculateNeededCP(first);
        CalculateNeededCP(second);
    }
    else 
        TotalControlNum += DeterminedNumControls;
}

void BSplineCurveFitterWindow3::CreateBSplinePolyline(vector<Vector3<float>> Sample)
{
    float cpError = Judge(Sample);
    
    if (cpError == (float)100) //the branch may be too long to fit well.
    {
        cout<<"---------"<<endl;
        vector<Vector3<float>> first = Sample;//init to VOID segmentation fault.
        vector<Vector3<float>> second = Sample;
        
        for (unsigned int i = Sample.size()/2; i < Sample.size(); ++i) //split in the half
            first[i] = 0;
        first.resize(Sample.size()/2);  
        for (unsigned int i = Sample.size()/2; i < Sample.size(); ++i)
            second[i - Sample.size()/2] = Sample[i];
        second.resize(Sample.size() - Sample.size()/2);  
   
        CreateBSplinePolyline(first);
        CreateBSplinePolyline(second);
    }
    else
    {                      
        mSpline = std::make_unique<BSplineCurveFit<float>>(mDimension, static_cast<int>(Sample.size()),
            reinterpret_cast<float const*>(&Sample[0]), DeterminedDegree, DeterminedNumControls);
    
        OutFile1<< DeterminedNumControls <<" "<<DeterminedDegree<<" "; 
        OutFile1<< Sample.size() <<" ";  
        
        float const* controlDataPtr = mSpline->GetControlData();
        for (int i = 0; i< DeterminedNumControls; ++i)
        {
            for (int j = 0; j < mDimension; ++j)
            {
                controlData[i][j] = (*controlDataPtr);
                OutFile1<<(round)(*controlDataPtr*diagonal)<<" ";
                //mControlData.push_back(*controlDataPtr);
                controlDataPtr++;
            }
            
        }    
        OutFile1<<endl;
    }
}

void BSplineCurveFitterWindow3::Merge()
{
    vector<Vector3<float>> first, second;
    float minEandContlNum = 100.0;
    //float maxdiff = 0.0;
    int minIndex = 1000;
    float firstE,secondE,mergeE;
    
    //vector<int> GapFill;////
    //outMerge.open("outMerge.txt");
  //time:0.007
    //clock_t start = clock();
    for(auto it = connection.begin();it!=connection.end();it++)
    {
        int *sampleIndex = *it;
        //cout<<out[0]<<"--"<<out[1]<<"--"<<out[2]<<"--"<<out[3]<<endl;
        first = sampleSet[sampleIndex[0]];
        if (deleteshort) firstE = Judge(first);
        else 
        {
            if (first.size()<MinAllowableLength) firstE = 2.01;//
            else firstE = Judge(first);
        }

        for(int index = 1; index < 4; index++)//index for 'sampleIndex'.
        {
            if(sampleIndex[index]==0) continue;
            second = sampleSet[sampleIndex[index]];
                
            if (deleteshort) secondE = Judge(second);
            else 
            {
                if(second.size()<MinAllowableLength) secondE = 2.01;//assign a big num.
                else secondE = Judge(second);
            }

            merge.resize(10000);//This is very important!!
            for (unsigned int i = 0; i < first.size(); ++i)
                merge[i] = first[i];
            
            for (unsigned int i = first.size(); i < (first.size()+second.size()); ++i)
                merge[i] = second[i-first.size()];
                
            merge.resize(first.size()+second.size());
           

            if (deleteshort) mergeE = Judge(merge);
            else
            {
                if (merge.size()<MinAllowableLength) mergeE = 4; //about 3CP + 0.- error.
                else mergeE = Judge(merge);
            }
    
            //cout<<"mergeE: "<<mergeE<<" firstE: "<<firstE<<" secondE: "<<secondE<<endl;
            if (mergeE < (firstE + secondE)) 
                if (minEandContlNum > mergeE) { minEandContlNum = mergeE; minIndex = sampleIndex[index];}
            
        }
        if (minIndex!=1000) //meet the merge condition.
        {
            //cout<<"Merge！ first: "<<sampleIndex[0]<<" second: "<<minIndex<<endl;
            second = sampleSet[minIndex];
            merge.resize(10000);//This is very important!!
            for (unsigned int i = 0; i < first.size(); ++i)
                merge[i] = first[i];
            
            for (unsigned int i = first.size(); i < (first.size()+second.size()); ++i)
                merge[i] = second[i-first.size()];
            merge.resize(first.size()+second.size());

            //for (unsigned int i = 0; i<merge.size(); i++)
            //    outMerge<<iter<<" "<<merge[i][0]*diagonal<<" "<<merge[i][1]*diagonal<<" "<<merge[i][2]*diagonal<<endl;
        

            sampleSet.erase(sampleSet.begin()+sampleIndex[0]);
            sampleSet.insert(sampleSet.begin()+sampleIndex[0],merge);
            //sampleSet.erase(sampleSet.begin()+minIndex);
            sampleSet[minIndex].clear();//still occupy the position.

            minEandContlNum = 100.0f;
            minIndex = 1000;
        }
        
    }
    
    //cout<<"--"<<sampleSet.size()<<endl;
        for (unsigned int i = sampleSet.size()-1; i > 0;i--)
        {
            if (sampleSet[i].size()<1) 
                sampleSet.erase(sampleSet.begin()+i);//when merged two branches, we'll leave one branch empty, 
            //cout<<"--"<<i<<endl;                   //so we squeeze the vector, that' why we start from the end.

        }
        
    

    //outMerge.close();
    //cout<<"--"<<sampleSet.size()<<endl;
     
}
