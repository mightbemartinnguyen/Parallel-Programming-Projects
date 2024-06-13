#include <iostream>
#include <cmath>
#include <omp.h>

// Global variables
unsigned int seed = 0; // Global seed for random number generation
int NowYear = 2024;    // 2024 - 2029
int NowMonth = 0;      // 0 - 11

float NowPrecip;       // inches of rain per month
float NowTemp;         // temperature this month
float NowHeight = 25.; // grain height in inches
int NowNumDeer = 3;    // number of deer in the current population
int NowNumWolfs = 0;   // number of wolves in the current population

// Random number generation function
float Ranf(float low, float high) {
    float r = (float) rand();         // 0 - RAND_MAX
    float t = r / (float) RAND_MAX;   // 0. - 1.
    return low + t * (high - low);
}

float SQR(float x) {
    return x*x;
}

// Constants
const float GRAIN_GROWS_PER_MONTH = 12.0;
const float ONE_DEER_EATS_PER_MONTH = 2;

const float AVG_PRECIP_PER_MONTH = 12.0;
const float AMP_PRECIP_PER_MONTH = 6.0;
const float RANDOM_PRECIP = 2.0;

const float AVG_TEMP = 60.0;
const float AMP_TEMP = 20.0;
const float RANDOM_TEMP = 10.0;

const float MIDTEMP = 50.0;
const float MIDPRECIP = 10.0;

const float DEER_TO_WOLF_RATIO = 3.7;  // Number of deer needed to support one wolf
const float WOLF_IMPACT = 0.6;         // How many deer one wolf can consume per month

// Barrier setup
omp_lock_t Lock;
volatile int NumInThreadTeam;
volatile int NumAtBarrier;
volatile int NumGone;

// Function prototypes
void InitBarrier(int);
void WaitBarrier();
void Deer();
void Grain();
void Wolf();
void Watcher();

// Barrier initialization function
void InitBarrier(int n) {
    NumInThreadTeam = n;
    NumAtBarrier = 0;
    omp_init_lock(&Lock);
}

// Barrier waiting function
void WaitBarrier() {
    omp_set_lock(&Lock);
    NumAtBarrier++;
    if (NumAtBarrier == NumInThreadTeam) {
        NumGone = 0;
        NumAtBarrier = 0;
        while (NumGone != NumInThreadTeam - 1);
        omp_unset_lock(&Lock);
        return;
    }
    omp_unset_lock(&Lock);
    while (NumAtBarrier != 0);
    #pragma omp atomic
    NumGone++;
}

void Deer() {
    while (NowYear < 2030) {
        int nextNumDeer = NowNumDeer;
        int carryingCapacity = (int)(NowHeight);
        
        if (nextNumDeer < carryingCapacity)
            nextNumDeer++;
        else if (nextNumDeer > carryingCapacity)
            nextNumDeer--;

        nextNumDeer -= NowNumWolfs * WOLF_IMPACT;

        if (nextNumDeer < 0)
            nextNumDeer = 0;

        WaitBarrier();
        NowNumDeer = nextNumDeer;
        WaitBarrier();
        WaitBarrier();
    }
}

void Grain() {
    while (NowYear < 2030) {
        float tempFactor = exp(-SQR((NowTemp - MIDTEMP) / 10.));
        float precipFactor = exp(-SQR((NowPrecip - MIDPRECIP) / 10.));
        float nextHeight = NowHeight;
        
        nextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
        nextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;

        if (nextHeight < 0.0)
            nextHeight = 0.0;

        WaitBarrier();
        NowHeight = nextHeight;
        WaitBarrier();
        WaitBarrier();
    }
}

void Wolf() {
    while (NowYear < 2030) {
        int nextNumWolfs = NowNumWolfs;
        
        int idealWolfs = NowNumDeer / DEER_TO_WOLF_RATIO;
        
        if (nextNumWolfs < idealWolfs) {
            nextNumWolfs++;
        } else if (nextNumWolfs > idealWolfs) {
            nextNumWolfs--;
        }

        if (nextNumWolfs < 0) {
            nextNumWolfs = 0;
        }

        WaitBarrier();
        NowNumWolfs = nextNumWolfs;
        WaitBarrier();
        WaitBarrier();
    }
}

void Watcher() {
    while (NowYear < 2030) {
        WaitBarrier();

        float ang = (30.*(float)NowMonth + 15.) * (M_PI / 180.);
        float temp = AVG_TEMP - AMP_TEMP * cos(ang);
        NowTemp = temp + Ranf(-RANDOM_TEMP, RANDOM_TEMP);

        float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin(ang);
        NowPrecip = precip + Ranf(-RANDOM_PRECIP, RANDOM_PRECIP);
        if (NowPrecip < 0.)
            NowPrecip = 0.;

        NowMonth++;
        if (NowMonth > 11) {
            NowMonth = 0;
            NowYear++;
        }
        
        float NowTempCelsius = (NowTemp - 32) * (5.0f / 9.0f);
        float NowHeightCentimeters = NowHeight * 2.54f;
        float NowPrecipCentimeters = NowPrecip * 2.54f;

        std::cout << NowMonth << ", " << NowYear << ", " << NowTempCelsius << ", " << NowPrecipCentimeters << ", " << 
        NowHeightCentimeters << ", " << NowNumDeer  << ", " << NowNumWolfs << std::endl;

        WaitBarrier();
        WaitBarrier();
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    omp_set_num_threads(4); // Set the number of threads
    InitBarrier(4);

    #pragma omp parallel sections
    {
        #pragma omp section
        {
            Deer();
        }
        #pragma omp section
        {
            Grain();
        }
        #pragma omp section
        {
            Watcher();
        }
        #pragma omp section
        {
            Wolf();  
        }
    } // All threads must return here to proceed

    return 0;
}