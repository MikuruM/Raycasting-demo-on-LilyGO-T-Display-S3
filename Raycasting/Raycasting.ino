#include <TFT_eSPI.h>
#include <SPI.h>
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);

#define screenX 320
#define screenY 170
#define button_0 0
#define button_1 14 //此为两个按键的pin编号

//*******FOV视野角相关定义*******
float FOV = 60;

//*******弧度相关定义*******
const float pi = 3.14159;

//*******地图相关定义*******
float squareLength = 25; //正方形边长，自己定义并填入
int mapipuRow = int(screenY / squareLength); //这里是地图的行数
int mapipuCol = int(screenX / squareLength); //这里市地图的列数

//*******初始点相关定义*******
float dotX = squareLength + 3;
float dotY = squareLength + 3; //点的位置
float alpha = 0.2 * pi; //旋转角度（角度非弧度）
float walk = 3; //往前走的步长

//*******指向向量相关定义*******
float rayLength = 10;
float finX = 0;
float finY = 0;

//*******横向检测相关定义*******
float detectRowLength = 0;
float detectRowFinX = 0;
float detectRowFinY = 0; //检测线尾坐标
float nearestRow = 0;

//*******纵向检测相关定义*******
float detectColLength = 0;
float detectColFinX = 0;
float detectColFinY = 0; //检测线尾坐标
float nearestCol = 0;

//*******push3D相关定义*******
float Length = 0; //最终射线长度
bool rowOrCol = 0; //0为row，1为col
float angleNormalized = 0;
float lengthNormalized = 0;

int mapipu[] = //定义地图，这里是6行12列，行列元素个数分别是mapipuRow和mapipuCol，注意c语言中浮点转整型为直接去尾
{
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1,
  1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

void drawMapipu()
{
  int sum = 0;
  for(int i = 0; i < mapipuRow; i++)
  {
    for(int j = 0; j < mapipuCol; j++)
    {
      if(mapipu[sum] == 1)
      {
        img.drawRect(j * squareLength, i * squareLength, squareLength, squareLength, TFT_WHITE);
      }
      sum += 1;
    }
  }
}

bool judgeMapipu(float x, float y) //遍历所有的方块，判断射线是否在方块边上，是则返回1，不是则返回0
{
  int sum = 0;
  for(int i = 0; i < mapipuRow; i++)
  {
    for(int j = 0; j < mapipuCol; j++)
    {
      if(mapipu[sum] == 1)
      {
        //img.drawRect(j * squareLength, i * squareLength, squareLength, squareLength, TFT_WHITE);
        if(
            (((j * squareLength) <= x) && (x <= (((j + 1) * squareLength))) && ((((i + 1) * squareLength)) <= y) && (y <= (((i + 1) * squareLength)))) || //用于判断下边
            (((j * squareLength) <= x) && (x <= (((j + 1) * squareLength))) && (((i * squareLength)) <= y) && (y <= ((i * squareLength)))) || //用于判断上边
            (((i * squareLength) <= y) && (y <= (((i + 1) * squareLength))) && (((j * squareLength)) <= x) && (x <= ((j * squareLength)))) || //用于判断左边
            (((i * squareLength) <= y) && (y <= (((i + 1) * squareLength))) && ((((j + 1) * squareLength)) <= x) && (x <= (((j + 1) * squareLength)))) //用于判断右边
           )
        {
          return 1;
        }  
      }
      sum += 1;
    }
  }
  return 0;
}

float normalizeAngle(float dotAngle)
{
  if(dotAngle < 0)
  {
    dotAngle += 2 * pi;
  }

  if(dotAngle > 2 * pi)
  {
    dotAngle -= 2 * pi;
  }
  return dotAngle;
}

void dotPosition(float &dotAngle)
{
  if(digitalRead(button_0) == 0 && digitalRead(button_1) == 0)
  {
    dotX += walk * cos(dotAngle); 
    dotY += walk * sin(dotAngle);    
  }

  if(digitalRead(button_0) == 0 && digitalRead(button_1) == 1)
  {
    dotAngle -= 0.05;
  }

  else if(digitalRead(button_0) == 1 && digitalRead(button_1) == 0)
  {
    dotAngle += 0.05;
  }
  img.drawPixel(dotX, dotY, TFT_WHITE);
}

void ray(float dotAngle) //用来测试的指向向量
{
  finY = rayLength * sin(dotAngle);
  finX = rayLength * cos(dotAngle);
  img.drawLine(dotX, dotY, dotX + finX, dotY + finY, TFT_WHITE);
}

void upgradeRowLookUp(float dotAngle)
{
  if(dotAngle < 1.5 * pi) //如果向左上方看
  {
    detectRowLength = (dotY - detectRowFinY) / sin(dotAngle - pi);
    detectRowFinX = dotX - (detectRowLength * cos(dotAngle - pi));
  }
  else if(dotAngle > 1.5 * pi) //如果向右上方看
  {
    detectRowLength = (dotY - detectRowFinY) / sin(2 * pi - dotAngle);
    detectRowFinX = dotX + (detectRowLength * cos(2 * pi - dotAngle));
  }
  else //如果向正上方看
  {
    detectRowLength = dotY - detectRowFinY;
    detectRowFinX = dotX;
  }
}

void upgradeRowLookDown(float dotAngle)
{
  if(dotAngle < 0.5 * pi) //如果向右下方看
  {
    detectRowLength = (detectRowFinY - dotY) / sin(dotAngle);
    detectRowFinX = dotX + (detectRowLength * cos(dotAngle));
  }
  else if(dotAngle > 0.5 * pi) //如果向左下方看
  {
    detectRowLength = (detectRowFinY - dotY) / sin(pi - dotAngle);
    detectRowFinX = dotX - (detectRowLength * cos(pi - dotAngle));
  }
  else //如果向正下方看
  {
    detectRowLength = detectRowFinY - dotY;
    detectRowFinX = dotX;
  }
}


float detectRow(float dotAngle)
{  
  if(dotAngle > pi) //向上检测
  {
    nearestRow = squareLength * int(dotY / squareLength);
    detectRowFinY = nearestRow;
    int yRound = 0;
    
    //更新射线长度，之后计算detectRowFinX位置
    upgradeRowLookUp(dotAngle);
    
    while(judgeMapipu(detectRowFinX, detectRowFinY) != 1 && yRound < mapipuRow + 2)
    {
      yRound += 1;
      detectRowFinY = nearestRow - yRound * squareLength;

      //更新射线长度，之后计算detectRowFinX位置
      upgradeRowLookUp(dotAngle);
    }

    //img.drawLine(dotX, dotY, detectRowFinX, detectRowFinY, TFT_WHITE);
    return detectRowLength; 
  }

  else if(dotAngle < pi) //向下检测
  {
    nearestRow = squareLength * (int(dotY / squareLength) + 1);
    detectRowFinY = nearestRow;
    int yRound = 0;
    
    //更新射线长度，之后计算detectRowFinX位置
    upgradeRowLookDown(dotAngle);

    //img.drawRect(0, 0, 25, 25, TFT_WHITE); //宕机的话取消这个注释

    while(judgeMapipu(detectRowFinX, detectRowFinY) != 1 && yRound < mapipuRow + 2)
    {
      yRound += 1;
      detectRowFinY = nearestRow + yRound * squareLength;

      //更新射线长度，之后计算detectRowFinX位置
      upgradeRowLookDown(dotAngle);
    }
    //img.drawRect(0, 0, 25, 25, TFT_WHITE); //宕机的话取消这个注释

    //img.drawLine(dotX, dotY, detectRowFinX, detectRowFinY, TFT_WHITE);
    return detectRowLength; 
  }

  else
  {
    return 0;
  }
}

void upgradeColLookLeft(float dotAngle)
{
  if(dotAngle < pi) //如果向左下方看
  {
    detectColLength = (dotX - detectColFinX) / cos(pi - dotAngle);
    detectColFinY = dotY + (detectColLength * sin(pi - dotAngle));
  }
  else if(dotAngle > pi) //如果向左上方看
  {
    detectColLength = (dotX - detectColFinX) / cos(dotAngle - pi);
    detectColFinY = dotY - (detectColLength * sin(dotAngle - pi));
  }
  else //如果向正左方看
  {
    detectColLength = dotX - detectColFinX;
    detectColFinY = dotY;
  }
}

void upgradeColLookRight(float dotAngle)
{
  if(1.5 * pi < dotAngle) //如果向右上方看
  {
    detectColLength = (detectColFinX - dotX) / cos(2 * pi - dotAngle);
    detectColFinY = dotY - (detectColLength * sin(2 * pi - dotAngle));
  }
  else if(dotAngle < 0.5 * pi) //如果向右下方看
  {
    detectColLength = (detectColFinX - dotX) / cos(dotAngle);
    detectColFinY = dotY + (detectColLength * sin(dotAngle));
  }
  else //如果向正右方看
  {
    detectColLength = detectColFinX - dotX;
    detectColFinY = dotY;
  }
}


float detectCol(float dotAngle)
{  
  if(0.5 * pi < dotAngle && dotAngle < 1.5 * pi) //向左检测
  {
    nearestCol = squareLength * int(dotX / squareLength);
    detectColFinX = nearestCol;
    int xRound = 0;
    
    //更新射线长度，之后计算detectRowFinX位置
    upgradeColLookLeft(dotAngle);
    
    while(judgeMapipu(detectColFinX, detectColFinY) != 1 && xRound < mapipuCol + 2)
    {
      xRound += 1;
      detectColFinX = nearestCol - xRound * squareLength;

      //更新射线长度，之后计算detectColFinY位置
      upgradeColLookLeft(dotAngle);
    }

    //img.drawLine(dotX, dotY, detectColFinX, detectColFinY, TFT_BLUE);
    return detectColLength; 
  }
 
  else if( (0 <= dotAngle && dotAngle < 0.5 * pi) || (1.5001 * pi < dotAngle && dotAngle <= 1.9999 * pi) ) //向右检测
  {
    nearestCol = squareLength * (int(dotX / squareLength) + 1);
    detectColFinX = nearestCol;
    int xRound = 0;
    
    //更新射线长度，之后计算detectColFinY位置
    upgradeColLookRight(dotAngle);

    //img.drawRect(0, 0, 25, 25, TFT_WHITE); //宕机的话取消这个注释

    while(judgeMapipu(detectColFinX, detectColFinY) != 1 && xRound < mapipuCol + 2)
    {
      xRound += 1;
      detectColFinX = nearestCol + xRound * squareLength;

      //更新射线长度，之后计算detectRowFinX位置
      upgradeColLookRight(dotAngle);
    }
    //img.drawRect(0, 0, 25, 25, TFT_WHITE); //宕机的话取消这个注释

    //img.drawLine(dotX, dotY, detectColFinX, detectColFinY, TFT_BLUE);
    return detectColLength; 
  }
  
  else
  {
    return 0;
  }
}

float drawCompareResult(float RowLength, float ColLength) 
{
  if(RowLength < ColLength)
  {
    //img.drawLine(dotX, dotY, detectRowFinX, detectRowFinY, TFT_GREEN);
    Length = RowLength;
    rowOrCol = 0; 
  }
  else
  {
    //img.drawLine(dotX, dotY, detectColFinX, detectColFinY, TFT_GREEN); //这里是射线，要看的话就取消注释
    Length = ColLength;
    rowOrCol = 1; 
  }
  return Length;
}



void push3D()
{
  //img.fillRect(0, 0, screenX, screenY, TFT_BLACK);
  img.fillRect(0, 0, screenX, screenY / 2, TFT_LIGHTGREY);
  img.fillRect(0, screenY / 2, screenX, screenY / 2, TFT_DARKGREY);

  for(float i = 0; i < FOV + 4; i++) //如果要射线从左向右顺序射出，起始角度应为alpha - FOV * 0.017 / 2
  {
    angleNormalized = normalizeAngle(alpha - FOV * 0.017 / 2 + i * 0.017);
    drawCompareResult(detectRow(angleNormalized), detectCol(angleNormalized));
    lengthNormalized = (0 + cos(angleNormalized - alpha)) * Length; //鱼眼矫正，常数0可以改变以调整矫正幅度，值越大矫正越小
    if(rowOrCol == 0)
    {
      //img.fillRect(screenX / FOV * i, (screenY - squareLength * screenX / Length) / 2, screenX / FOV, squareLength * screenX / Length, TFT_WHITE);
      img.fillRect((squareLength * mapipuCol) / FOV * i, ((squareLength * (mapipuRow + 1)) - squareLength * (squareLength * mapipuCol) / lengthNormalized) / 2, (squareLength * mapipuCol) / FOV, squareLength * (squareLength * mapipuCol) / lengthNormalized, TFT_NAVY);
    }
    else
    {
      img.fillRect((squareLength * mapipuCol) / FOV * i, ((squareLength * (mapipuRow + 1)) - squareLength * (squareLength * mapipuCol) / lengthNormalized) / 2, (squareLength * mapipuCol) / FOV, squareLength * (squareLength * mapipuCol) / lengthNormalized, TFT_BLUE);
    }  
  }
}

void setup() {
  pinMode(button_0, INPUT_PULLUP);
  pinMode(button_1, INPUT_PULLUP);
  tft.init();  
  tft.setRotation(1);
  img.createSprite(screenX, screenY);
  img.fillRect(0, 0, screenX, screenY, TFT_BLACK);
  img.drawString("This is the demo of Ray Casting", 65, 62, 2);
  img.pushSprite(0, 0);
  delay(1000);
}

void loop() {
  img.fillRect(0, 0, screenX, screenY, TFT_BLACK);

  alpha = normalizeAngle(alpha);

  img.drawFloat(alpha * 100, 0, 0, 2);

  drawMapipu();

  //ray(alpha);

  dotPosition(alpha);

  push3D();
   
  img.pushSprite(0, 0);

  //delay(50);

}
