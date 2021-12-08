// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2020
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 4.0.2019.08.13

#pragma once

#include <Applications/Window3.h>
#include <Mathematics/BSplineCurveFit.h>
#include <Mathematics/BSplineCurveGenerate.h>
using namespace gte;
using namespace std; 

//class BSplineCurveFitterWindow3 : public Window3
class BSplineCurveFitterWindow3
{ 
public:
    //BSplineCurveFitterWindow3(Parameters& parameters);
    ///BSplineCurveFitterWindow3(vector<vector<Vector3<float>>> BranchSet, float hausdorff_, float diagonal);
    BSplineCurveFitterWindow3();
    //virtual void OnIdle() override;
    //virtual bool OnCharPress(unsigned char key, int x, int y) override;
    int SplineFit(vector<vector<Vector3<float>>> BranchSet, float hausdorff_,float diagonal_, int layerNum, vector<int *> connection_,int width, float *smd);
    void SplineFit2(vector<vector<Vector3<float>>> BranchSet, float hausdorff_, float diagonal_, int layerNum, vector<int *> connection_, int sign, int width, float *smd);
    void SplineGenerate(int SuperR);
private:
    
    void CreateBSplinePolyline(vector<Vector3<float>> Sample);
    void CalculateNeededCP(vector<Vector3<float>> Sample);
    void CreateGraphics(unsigned int numSamples, int superR);
    float Judge(vector<Vector3<float>> Sample);
	void Merge();
/*
    //void CreateScene();
    void CreateGraphics(unsigned int numSamples);
    float Judge(vector<Vector3<float>> Sample);
    void Merge();
    void CreateBSplinePolyline();
    //void drawSpline();
    void drawControlPoints(Vector3<float> controlPoint);
    void drawControlPointsLine();

    enum { NUM_SAMPLES = 10000};*/
    vector<Vector3<float>> merge;
    vector<vector<Vector3<float>>> sampleSet;
    unique_ptr<BSplineCurveFit<float>> mSpline = nullptr;
    float hausdorff = 0.0f;
    float minErrorThreshold = 0.0f;
    float diagonal= 0.0f;
    vector<int *> connection;
};
