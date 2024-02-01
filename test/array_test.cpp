// g++ vector_vs_stack_array.cpp -O3 -o vector

#include <numeric>
#include <vector>
#include <chrono>
#include <iostream>
#include <cmath>
#include <array>
using namespace std::chrono;

constexpr int REP = 1000;

double sumNewArray( int N ) {
    double * smallarray = new double[N];
    for(unsigned int k = 0; k<N; k++){
        smallarray[k] = 1.0/pow(k+1,2);
    }
    
    double sum = 0.0;
    for(unsigned int k = 0; k<N; k++){
        sum += smallarray[k];
    }
    delete [] smallarray;
    return sum;
}

double sumStackArray( int N ) {

    double smallarray[N];
    for(unsigned int k = 0; k<N; k++){
        smallarray[k] = 1.0/pow(k+1,2);
    }

    double sum = 0.0;
    for(unsigned int k = 0; k<N; k++){
        sum += smallarray[k];
    }
    
    return sum;

}


double sumStdVector( int N ) {

    std::vector<double> smallarray;
    smallarray.reserve(N);

    for(unsigned int k = 0; k<N; ++k){
        smallarray[k] = 1.0/pow(k+1,2);
    }

    double sum = 0.0;
    for(unsigned int k = 0; k<N; k++){
        sum += smallarray[k];
    }
    return sum;
}


double sumVector_PushBack( int N ) {

    std::vector<double> smallarray;

    for(unsigned int k = 0; k<N; ++k){
        smallarray.push_back(1.0/pow(k+1,2));
    }
    double sum = 0.0;
    for(unsigned int k = 0; k<N; k++){
        sum += smallarray[k];
    }
    return sum;

}

// 
// With pre-allocation
// 


double sumArray_prealloc( int N, double smallarray[]  ) {

    for(unsigned int k = 0; k<N; ++k){
        smallarray[k] = 1.0/pow(k+1,2);
    }

    double sum = 0.0;
    for(unsigned int k = 0; k<N; k++){
        sum += smallarray[k];
    }
    return sum;
}


double sumStdVector_prealloc( std::vector<double>& smallarray  ) {

    int N = smallarray.size();

    for(unsigned int k = 0; k<N; ++k){
        smallarray[k] = 1.0/pow(k+1,2);
    }

    double sum = 0.0;
    for(unsigned int k = 0; k<N; k++){
        sum += smallarray[k];
    }
    return sum;
}

// 
// Timing functions
// 

int time_sumNewArray( int N, int REP ) {

    double x = 0.0;
    auto start = high_resolution_clock::now();
    for (int i=0; i<REP; i++){
        x = sumNewArray(N);
    }
    auto stop = high_resolution_clock::now();
    int t = duration_cast<microseconds>(stop - start).count();
    
    return t;
}


int time_sumStackArray( int N, int REP ) {

    double x = 0.0;
    auto start = high_resolution_clock::now();
    for (int i=0; i<REP; i++){
        x = sumStackArray(N);
    }
    auto stop = high_resolution_clock::now();
    int t = duration_cast<microseconds>(stop - start).count();
    
    return t;
}

int time_sumStdVector( int N, int REP ) {

    double x = 0.0;
    auto start = high_resolution_clock::now();
    for (int i=0; i<REP; i++){
        x = sumStdVector(N);
    }
    auto stop = high_resolution_clock::now();
    int t = duration_cast<microseconds>(stop - start).count();
    return t;
}


int time_sumStdVector_prealloc( int N, int REP ) {

    double x = 0.0;
    auto start = high_resolution_clock::now();
    std::vector<double> smallarray(N);
    // smallarray.reserve(N);
    for (int i=0; i<REP; i++){
        x = sumStdVector_prealloc(smallarray);
    }
    auto stop = high_resolution_clock::now();
    int t = duration_cast<microseconds>(stop - start).count();
    return t;
}


int time_sumNewArray_prealloc( int N, int REP ) {

    double x = 0.0;
    auto start = high_resolution_clock::now();
    double * smallarray = new double[N];
    // smallarray.reserve(N);
    for (int i=0; i<REP; i++){
        x = sumArray_prealloc( N, smallarray );
    }
    auto stop = high_resolution_clock::now();
    int t = duration_cast<microseconds>(stop - start).count();
    return t;
}


int time_sumStackArray_prealloc( int N, int REP ) {

    double x = 0.0;
    auto start = high_resolution_clock::now();
    double smallarray[N];
    // smallarray.reserve(N);
    for (int i=0; i<REP; i++){
        x = sumArray_prealloc( N, smallarray );
    }
    auto stop = high_resolution_clock::now();
    int t = duration_cast<microseconds>(stop - start).count();
    return t;
}


int time_sumVector_pushback( int N, int REP ) {

    double x = 0.0;
    auto start = high_resolution_clock::now();
    for (int i=0; i<REP; i++){
        x = sumVector_PushBack(N);
    }
    auto stop = high_resolution_clock::now();
    int t = duration_cast<microseconds>(stop - start).count();
    return t;
}

int printVec( std::string varname, std::vector<int> values )
{
    std::cout << varname;
    for (int i=0; i< values.size(); i++) {
        std::cout << " | " << values[i];
    }
    std::cout << std::endl;
    return 0;
}

void __attribute__((noinline)) sumStdArray ()
{
    std::cout << "StdArray";
    long times;
    
    //N = 10
    constexpr int N = 10;
    auto start = high_resolution_clock::now();
    for ( int i=0; i<REP; i++){
        std::array<double, N> smallarray;
    
        for(unsigned int k = 0; k<N; k++){
            smallarray[k] = 1.0/pow(k+1,2);
        }

        volatile double sum = 0.0;
        for(unsigned int k = 0; k<N; k++){
            sum += smallarray[k];
        }
        
    }
    auto stop = high_resolution_clock::now();
    times= duration_cast<microseconds>(stop - start).count();
    std::cout << " | " << times ;

    //N = 100
    constexpr int N1 = 100;
    start = high_resolution_clock::now();
    for ( int i=0; i<REP; i++){
        std::array<double, N1> smallarray;
    
        for(unsigned int k = 0; k<N1; k++){
            smallarray[k] = 1.0/pow(k+1,2);
        }

        volatile double sum = 0.0;
        for(unsigned int k = 0; k<N1; k++){
            sum += smallarray[k];
        }
        
    }
    stop = high_resolution_clock::now();
    times = duration_cast<microseconds>(stop - start).count();
    std::cout << " | " << times ;

    //N = 1000
    constexpr int N2 = 1000;
    start = high_resolution_clock::now();
    for ( int i=0; i<REP; i++){
        std::array<double, N2> smallarray;
    
        for(unsigned int k = 0; k<N2; k++){
            smallarray[k] = 1.0/pow(k+1,2);
        }

        volatile double sum = 0.0;
        for(unsigned int k = 0; k<N2; k++){
            sum += smallarray[k];
        }
        
    }
    stop = high_resolution_clock::now();
    times = duration_cast<microseconds>(stop - start).count();
    std::cout << " | " << times ;

    //N = 10000
    constexpr int  N3 = 10000;
    start = high_resolution_clock::now();
    for ( int i=0; i<REP; i++){
        std::array<double, N3> smallarray;
    
        for(unsigned int k = 0; k<N3; k++){
            smallarray[k] = 1.0/pow(k+1,2);
        }

        volatile double sum = 0.0;
        for(unsigned int k = 0; k<N3; k++){
            sum += smallarray[k];
        }
        
    }
    stop = high_resolution_clock::now();
    times = duration_cast<microseconds>(stop - start).count();
    std::cout << " | " << times ;

    //N = 100000
    constexpr int N4 = 100000;
    start = high_resolution_clock::now();
    for ( int i=0; i<REP; i++){
        std::array<double, N4> smallarray;
    
        for(unsigned int k = 0; k<N4; k++){
            smallarray[k] = 1.0/pow(k+1,2);
        }

        volatile double sum = 0.0;
        for(unsigned int k = 0; k<N4; k++){
            sum += smallarray[k];
        }
        
    }
    stop = high_resolution_clock::now();
    times = duration_cast<microseconds>(stop - start).count();
    std::cout << " | " << times ;

    //N = 1000000
    constexpr int N5 = 1000000;
    start = high_resolution_clock::now();
    for ( int i=0; i<REP; i++){
        std::array<double, N5> smallarray;
    
        for(unsigned int k = 0; k<N5; k++){
            smallarray[k] = 1.0/pow(k+1,2);
        }

        volatile double sum = 0.0;
        for(unsigned int k = 0; k<N5; k++){
            sum += smallarray[k];
        }
        
    }
    stop = high_resolution_clock::now();
    times = duration_cast<microseconds>(stop - start).count();
    std::cout << " | " << times << std::endl;
}


int main(){

    std::vector<int> Ns = {10, 100, 1000, 10000, 100000, 1000000};
    std::vector<int> t(Ns.size());

    freopen("output.txt","a",stdout);
    std::cout << "---------------------" << std::endl;
    std::cout << "Function | Miliseconds" << std::endl;

    // Warm up memory?
    t[0] = time_sumNewArray(20000, REP);

    for (int i=0; i<Ns.size(); i++) {
        t[i] = time_sumStackArray(Ns[i], REP);
    }
    printVec("StackArray", t);

    for (int i=0; i<Ns.size(); i++) {
        t[i] = time_sumNewArray(Ns[i], REP);
    }
    printVec("NewArray", t);

    for (int i=0; i<Ns.size(); i++) {
        t[i] = time_sumStdVector(Ns[i], REP);
    }
    printVec("StdVector", t);

    for (int i=0; i<Ns.size(); i++) {
        t[i] = time_sumVector_pushback(Ns[i], REP);
    }
    printVec("StdVector_pushback", t);
    
    for (int i=0; i<Ns.size(); i++) {
        t[i] = time_sumStackArray_prealloc(Ns[i], REP);
    }
    printVec("StackArray_prealloc", t);

    for (int i=0; i<Ns.size(); i++) {
        t[i] = time_sumNewArray_prealloc(Ns[i], REP);
    }
    printVec("NewArray_prealloc", t);  

    for (int i=0; i<Ns.size(); i++) {
        t[i] = time_sumStdVector_prealloc(Ns[i], REP);
    }
    printVec("StdVector_prealloc", t);

    //---------------------------------------
    sumStdArray();
  
    return 0;   
}