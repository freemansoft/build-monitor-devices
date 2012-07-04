#ifndef DEMOS_H
#define DEMOS_H

#define checkReturn(v) {if(v){v = 0;return;}}

void flashMatrixDemo(void);
void lightLedStripNumberDemo(void);
void lightCubeDemo(void);
void lightLedStripNumber(int num,unsigned short color);

void runDemo();

#endif

