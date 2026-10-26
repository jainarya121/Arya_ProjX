#include <cstdio>

float lastPosition = 0;

float linePosition(const int s[5], bool &lost) {
    const int weight[5] = {-2, -1, 0, 1, 2};
    long sum = 0, weightedSum = 0;
    for (int i = 0; i < 5; i++) {
        sum += s[i];
        weightedSum += weight[i] * s[i];
    }

    if (sum == 0) {              
        lost = true;
        if (lastPosition > 0) return 2.0f;
        if (lastPosition < 0) return -2.0f;
        return 0.0f;             
    }

    lost = false;
    lastPosition = (float)weightedSum / sum;
    return lastPosition;
}

int limit(int command, int maxValue) {
    if (command > maxValue) return maxValue;
    if (command < -maxValue) return -maxValue;
    return command;
}

int main() {
    bool lost;
    const int centred[5]    = {0, 0, 1000, 0, 0};
    const int slightLeft[5] = {0, 600, 1000, 0, 0};
    const int farRight[5]   = {0, 0, 0, 500, 1000};
    const int nothing[5]    = {0, 0, 0, 0, 0};

    float p = linePosition(centred, lost);
    printf("centred       -> %+.3f  lost=%d   (expect  0.000)\n", p, lost);
    p = linePosition(slightLeft, lost);
    printf("slightly left -> %+.3f  lost=%d   (expect -0.375 = -600/1600)\n", p, lost);
    p = linePosition(farRight, lost);
    printf("far right     -> %+.3f  lost=%d   (expect +1.667 = 2500/1500)\n", p, lost);
    p = linePosition(nothing, lost);
    printf("no line       -> %+.3f  lost=%d   (expect +2, last seen on the right)\n", p, lost);

    printf("limit(300, 255)  = %d   (expect 255)\n", limit(300, 255));
    printf("limit(-400, 255) = %d  (expect -255, still reverse)\n", limit(-400, 255));
    printf("limit(120, 255)  = %d   (expect 120, unchanged)\n", limit(120, 255));
    return 0;
}
