// Imagine++ project
// Project:  Fundamental
// Author:   Pascal Monasse

#include "./Imagine/Features.h"
#include <Imagine/Graphics.h>
#include <Imagine/LinAlg.h>
#include <vector>
#include <cstdlib>
#include <ctime>
using namespace Imagine;
using namespace std;

static const float BETA = 0.01f; // Probability of failure

struct Match {
    float x1, y1, x2, y2;
};

// Display SIFT points and fill vector of point correspondences
void algoSIFT(Image<Color,2> I1, Image<Color,2> I2,
              vector<Match>& matches) {
    // Find interest points
    SIFTDetector D;
    D.setFirstOctave(-1);
    Array<SIFTDetector::Feature> feats1 = D.run(I1);
    drawFeatures(feats1, Coords<2>(0,0));
    cout << "Im1: " << feats1.size() << flush;
    Array<SIFTDetector::Feature> feats2 = D.run(I2);
    drawFeatures(feats2, Coords<2>(I1.width(),0));
    cout << " Im2: " << feats2.size() << flush;

    const double MAX_DISTANCE = 100.0*100.0;
    for(size_t i=0; i < feats1.size(); i++) {
        SIFTDetector::Feature f1=feats1[i];
        for(size_t j=0; j < feats2.size(); j++) {
            double d = squaredDist(f1.desc, feats2[j].desc);
            if(d < MAX_DISTANCE) {
                Match m;
                m.x1 = f1.pos.x();
                m.y1 = f1.pos.y();
                m.x2 = feats2[j].pos.x();
                m.y2 = feats2[j].pos.y();
                matches.push_back(m);
            }
        }
    }
}

vector<Match> randseed(vector<Match>& matches, int num=8){
    // the number of selection is entre 0 and 8
    if(num<=0||num>matches.size()){
        cerr << "Invalid number of points to select."<<endl;
        return 0;
    }
    
    vector<Match> eight_point;
    eight_point.reserve(num);
    
    //创建了一个名为 eight_point 的整数向量，用于存储随机选择的点对。
    //使用 reserve 函数预分配了 num 个元素的空间，以避免不必要的重新分配。
    
    random_device rd;
    mt19937 gen(rd());//生成随机数的设定和引擎初始化
    //创建一个伪随机数生成器 gen
    
    vector<Match> match_copy = matches;
    
    for(int i=0;i<num,i++){
        uniform_int_distribution<int>distribution(0, match_copy.size()-1);
        // 该分布将生成介于 0 和 match_copy.size() - 1 之间的随机整数
        //这个范围确保了随机选择的索引在 match_copy 中是有效的。
        int index = distribution(gen);//使用均匀分布对象生成一个随机整数索引，并将其存储在 index 变量中。
        eight_point.push_back(match_copy[index]);
        match_copy.erase(match_copy.begin()+index);
        return eight_point;
    }
}
FMatrix<float,3,3>computeF(vector<Match>& eight_point){
    
    //create normalization matrix
    FMatrix<float,3,3> Matrix_norma(0.f);
    Matrix_norma(0,0)=0.001;
    Matrix_norma(1,1)=0.001;
    Matrix_norma(2,2)=1;
    
    // calculate F
    FMatrix<float,9,9> A;
    
    for (int i=0;i<8；i++){
        vector<float> point1(3,0.0f);
        vector<float> point2(3,0.0f);
        
        point1[0] = eight_point[i].x1;
        point1[1] = eight_point[i].y1;
        point1[2] = 1;
        point2[0] = eight_point[i].x2;
        point2[1] = eight_point[i].y2;
        point2[2] = 1;
        
        //normalisation
        point1 = Matrix_norma*point1;
        point2 = Matrix_norma*point2;
        
        float x1 = point1[0];
        float y1 = point1[1];
        float x2 = point2[0];
        float y2 = point2[1];
        
        A(i,0) = x1 * x2;
        A(i,1) = x1 * y2;
        A(i,2) = x1;
        A(i,3) = y1 * x2;
        A(i,4) = y1 * y2;
        A(i,5) = y1;
        A(i,6) = x2;
        A(i,7) = y2;
        A(i,8) = 1;
    }
    // rank(A) = 2
    for (int j=0;j<9,j++){
        A(8,j) = 0;
    }
    //SVD and obtain U,S,V
    FVector<float,9>S;
    FMatrix<float,9,9>U,V;
    svd(A,U,S,V);
    
    //将SVD结果中对应于最小特征值的特征向量的元素复制到估计的基础矩阵F中
    //以构建最终的F矩阵。
    FMatrix<float,3,3>result_F;
    for (int i=0;i<3;i++){
        for (int j=0;j<3;j++){
            result_F(i,j) = V.getRow(8)[3*i+j]
        }
    }
    
    // Add constraint rank(F)=2
    FVector<float,3> S2;
    FMatrix<float,3,3> U2, V2;
    svd(result_F,U2,S2,V2); // obtain U2,S2,V2
    //S2种的奇异值按升序排序
    S2[2] = 0;
    //这一行将 S2 向量中的最小奇异值（索引2的位置）设置为0。
    //rank(F)=2
    result_F = U2 * Diagonal(S2) * V2;
    //

    // Normalization of computedF
    result_F = Matrix_norma * result_F * Matrix_norma;
    return result_F;
}

vector<int> computeInliers(vector<Match& matches,FMatrix<float,3,3>& result_F, float dist)){
    vector<int> inliers;
    for(int i=0;i<matches.size();i++){
        
        float x1 = matches[i].x1;
        float y1 = matches[i].y1;
        float x2 = matches[i].x2;
        float y2 = matches[i].y2;
        
        //创建齐次坐标
        FVector<float, 3> point1(x1, y1, 1);
        FVector<float, 3> point2(x2, y2, 1);
        
        //计算极线
        FVector<float,3> line = result_F*point2;
        //计算极线的归一化
        float norme = sqrt(line[0]*line[0] + line[1]*line[1]);
        line /= norme;
        //计算点 point1 到极线的距离
        float distance = abs(point1*line)
        if(distance < dist){
            inliers.push_back(i);
        }
    }
    return inliers;
}

// 动态调整RANSAC算法的迭代次数
int nbIter(int nb_currentIter, int nb_inlier, int nb_sample, int nb_match){
    int nb_nextIter;
    nb_nextIter = log(BETA)/log(1- pow( (float)nb_inlier/(float)nb_match, nb_sample));
    if(nb_nextIter > nb_currentIter){
        nb_nextIter = nb_currentIter;
    }
    return nb_nextIter;
}
// RANSAC algorithm to compute F from point matches (8-point algorithm)
// Parameter matches is filtered to keep only inliers as output.
FMatrix<float,3,3> computeF(vector<Match>& matches) {
    // 点对中的点距离小于distMax的为内点
    const float distMax = 1.5f; // Pixel error for inlier/outlier discrimination
    // RANSAC算法会随机选择一组点对，估计基础矩阵
    //然后统计内点的数量。这个过程会重复进行Niter次，以找到具有最多内点的最佳估计。
    int Niter=100000; // Adjusted dynamically 
    FMatrix<float,3,3> bestF;
    vector<int> bestInliers;
    // --------------- TODO ------------
    // DO NOT FORGET NORMALIZATION OF POINTS
    while (n<Niter){
        eight_point = randseed(matches,num=8);
        result_F = computeF(eight_point);
        inliers = computeInliers(matches,result_F,distMax);
        if(inliers.size()>bestInliers.size()){
            bestInliers = inliers;
            bestF = result_F;
            if (bestInliers.size()>50){
                Niter = nbIter(Niter,bestInliers.size(),num=8,matches.size());
            }
            cout <<Niter<<" iterations needed." << endl;
        }
    }
    vector<Match> all = matches;
    matches.clear();
    for (size_t i=0; i<bestInliers.size();i++){
        matches.push_back(all[bestInliers[i]]);
    }
    return bestF;
}

// Expects clicks in one image and show corresponding line in other image.
// Stop at right-click.
void displayEpipolar(Image<Color> I1, Image<Color> I2,
                     const FMatrix<float,3,3>& F) {
    while(true) {
        int x,y;
        if(getMouse(x,y) == 3)
            break;
        // --------------- TODO ------------

        cout << "Point clicked: " << x << ' ' << y << endl << flush;
        int view = static_cast<int>(x<w);
        cout << "The epipolar view is in Image : " << view + 1 << endl << flush;
        v[1] = static_cast<double>(y);
        v[0] = static_cast<double>(x - (1 - view) * w) ;
        v = ( static_cast<double>(view) * transpose(F) + static_cast<double>(1 - view) * F) * v;
        IntPoint2 left,right;
        int h = (view * I2.height() + (1-view) * I1.height());
        line( v, left, right, w, h, view);
        cout << "The line goes from " << left << " to  " << right << endl << flush;
        drawLine( left, right, RED, 2);
    }
}

int main(int argc, char* argv[])
{
    srand((unsigned int)time(0));

    const char* s1 = argc>1? argv[1]: srcPath("im1.jpg");
    const char* s2 = argc>2? argv[2]: srcPath("im2.jpg");

    // Load and display images
    Image<Color,2> I1, I2;
    if( ! load(I1, s1) ||
        ! load(I2, s2) ) {
        cerr<< "Unable to load images" << endl;
        return 1;
    }
    int w = I1.width();
    openWindow(2*w, I1.height());
    display(I1,0,0);
    display(I2,w,0);

    vector<Match> matches;
    algoSIFT(I1, I2, matches);
    const int n = (int)matches.size();
    cout << " matches: " << n << endl;
    drawString(100,20,std::to_string(n)+ " matches",RED);
    click();
    
    FMatrix<float,3,3> F = computeF(matches);
    cout << "F="<< endl << F;

    // Redisplay with matches
    display(I1,0,0);
    display(I2,w,0);
    for(size_t i=0; i<matches.size(); i++) {
        Color c(rand()%256,rand()%256,rand()%256);
        fillCircle(matches[i].x1+0, matches[i].y1, 2, c);
        fillCircle(matches[i].x2+w, matches[i].y2, 2, c);        
    }
    drawString(100, 20, to_string(matches.size())+"/"+to_string(n)+" inliers", RED);
    click();

    // Redisplay without SIFT points
    display(I1,0,0);
    display(I2,w,0);
    displayEpipolar(I1, I2, F);

    endGraphics();
    return 0;
}
