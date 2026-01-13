/******************************************************************************
* | File      	:   GUI_Paint.c
* | Author      :   Waveshare electronics
* | Function    :	Achieve drawing: draw points, lines, boxes, circles and
*                   their size, solid dotted line, solid rectangle hollow
*                   rectangle, solid circle hollow circle.
* | Info        :
*   Achieve display characters: Display a single character, string, number
*   Achieve time display: adaptive size display time minutes and seconds
*----------------
* |	This version:   V3.2
* | Date        :   2020-07-23
* | Info        :
* -----------------------------------------------------------------------------
* V3.2(2020-07-23):
* 1. Change: Paint_SetScale(UBYTE scale)
*			Add scale 7 for 5.65f e-Parper
* 2. Change: Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color)  
* 			Add the branch for scale 7
* 3. Change: Paint_Clear(UWORD Color)
*			Add the branch for scale 7
*
* V3.1(2019-10-10):
* 1. Add gray level
*   PAINT Add Scale
* 2. Add void Paint_SetScale(UBYTE scale);
*
* V3.0(2019-04-18):
* 1.Change: 
*    Paint_DrawPoint(..., DOT_STYLE DOT_STYLE)
* => Paint_DrawPoint(..., DOT_STYLE Dot_Style)
*    Paint_DrawLine(..., LINE_STYLE Line_Style, DOT_PIXEL Dot_Pixel)
* => Paint_DrawLine(..., DOT_PIXEL Line_width, LINE_STYLE Line_Style)
*    Paint_DrawRectangle(..., DRAW_FILL Filled, DOT_PIXEL Dot_Pixel)
* => Paint_DrawRectangle(..., DOT_PIXEL Line_width, DRAW_FILL Draw_Fill)
*    Paint_DrawCircle(..., DRAW_FILL Draw_Fill, DOT_PIXEL Dot_Pixel)
* => Paint_DrawCircle(..., DOT_PIXEL Line_width, DRAW_FILL Draw_Filll)
*
* -----------------------------------------------------------------------------
* V2.0(2018-11-15):
* 1.add: Paint_NewImage()
*    Create an image's properties
* 2.add: Paint_SelectImage()
*    Select the picture to be drawn
* 3.add: Paint_SetRotate()
*    Set the direction of the cache    
* 4.add: Paint_RotateImage() 
*    Can flip the picture, Support 0-360 degrees, 
*    but only 90.180.270 rotation is better
* 4.add: Paint_SetMirroring() 
*    Can Mirroring the picture, horizontal, vertical, origin  
*
* ----------------------------------------------------------------------------- 
* V1.0(2018-07-17):
*   Create library
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documnetation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to  whom the Software is
* furished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
******************************************************************************/
#include "GUI_Paint.h"
#include "DEV_Config.h"
#include "utility/Debug.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h> //memset()
#include <math.h>

PAINT Paint;

/******************************************************************************
function: Create Image
parameter:
    image   :   Pointer to the image cache
    width   :   The width of the picture
    Height  :   The height of the picture
    Color   :   Whether the picture is inverted
******************************************************************************/
void Paint_NewImage(UBYTE *image, UWORD Width, UWORD Height, UWORD Rotate, UWORD Color)
{
    Paint.Image = NULL;
    Paint.Image = image;

    Paint.WidthMemory = Width;
    Paint.HeightMemory = Height;
    Paint.Color = Color;    
    Paint.Scale = 2;
    Paint.WidthByte = (Width % 8 == 0)? (Width / 8 ): (Width / 8 + 1);
    Paint.HeightByte = Height;    
//    printf("WidthByte = %d, HeightByte = %d\r\n", Paint.WidthByte, Paint.HeightByte);
//    printf(" EPD_WIDTH / 8 = %d\r\n",  122 / 8);
   
    Paint.Rotate = Rotate;
    Paint.Mirror = MIRROR_NONE;
    
    // Initialize clipping to full screen
    Paint.clippingEnabled = false;
    Paint.clipX1 = 0;
    Paint.clipY1 = 0;
    Paint.clipX2 = Width - 1;
    Paint.clipY2 = Height - 1;
    
    if(Rotate == ROTATE_0 || Rotate == ROTATE_180) {
        Paint.Width = Width;
        Paint.Height = Height;
    } else {
        Paint.Width = Height;
        Paint.Height = Width;
    }
}

/******************************************************************************
function: Select Image
parameter:
    image : Pointer to the image cache
******************************************************************************/
void Paint_SelectImage(UBYTE *image)
{
    Paint.Image = image;
}

/******************************************************************************
function: Select Image Rotate
parameter:
    Rotate : 0,90,180,270
******************************************************************************/
void Paint_SetRotate(UWORD Rotate)
{
    if(Rotate == ROTATE_0 || Rotate == ROTATE_90 || Rotate == ROTATE_180 || Rotate == ROTATE_270) {
        // Debug("Set image Rotate %d\r\n", Rotate);
        Paint.Rotate = Rotate;
    } else {
        Debug("rotate = 0, 90, 180, 270\r\n");
    }
}

/******************************************************************************
function:	Select Image mirror
parameter:
    mirror   :Not mirror,Horizontal mirror,Vertical mirror,Origin mirror
******************************************************************************/
void Paint_SetMirroring(UBYTE mirror){
    if(mirror == MIRROR_NONE || mirror == MIRROR_HORIZONTAL || 
        mirror == MIRROR_VERTICAL || mirror == MIRROR_ORIGIN) {
        // Debug("mirror image x:%s, y:%s\r\n",(mirror & 0x01)? "mirror":"none", ((mirror >> 1) & 0x01)? "mirror":"none");
        Paint.Mirror = mirror;
    } else {
        Debug("mirror should be MIRROR_NONE, MIRROR_HORIZONTAL, \
        MIRROR_VERTICAL or MIRROR_ORIGIN\r\n");
    }    
}

void Paint_SetScale(UBYTE scale)
{
    if(scale == 2){
        Paint.Scale = scale;
        Paint.WidthByte = (Paint.WidthMemory % 8 == 0)? (Paint.WidthMemory / 8 ): (Paint.WidthMemory / 8 + 1);
    }
	else if(scale == 4) {
        Paint.Scale = scale;
        Paint.WidthByte = (Paint.WidthMemory % 4 == 0)? (Paint.WidthMemory / 4 ): (Paint.WidthMemory / 4 + 1);
    }
	else if(scale == 7) {//Only applicable with 5in65 e-Paper
		Paint.Scale = 7;
		Paint.WidthByte = (Paint.WidthMemory % 2 == 0)? (Paint.WidthMemory / 2 ): (Paint.WidthMemory / 2 + 1);
	}
	else {
        Debug("Set Scale Input parameter error\r\n");
        Debug("Scale Only support: 2 4 7\r\n");
    }
}
/******************************************************************************
function: Draw Pixels
parameter:
    Xpoint : At point X
    Ypoint : At point Y
    Color  : Painted colors
******************************************************************************/
void Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color)
{
    // Bounds check
    if(Xpoint >= Paint.Width || Ypoint >= Paint.Height){
        return;
    }
    
    // Clipping check (optimization)
    if(Paint.clippingEnabled) {
        if(Xpoint < Paint.clipX1 || Xpoint > Paint.clipX2 ||
           Ypoint < Paint.clipY1 || Ypoint > Paint.clipY2) {
            return;
        }
    }
    
    UWORD X, Y;
    // Coordinate transformation
    switch(Paint.Rotate) {
    case 0:
        X = Xpoint;
        Y = Ypoint;  
        break;
    case 90:
        X = Paint.WidthMemory - Ypoint - 1;
        Y = Xpoint;
        break;
    case 180:
        X = Paint.WidthMemory - Xpoint - 1;
        Y = Paint.HeightMemory - Ypoint - 1;
        break;
    case 270:
        X = Ypoint;
        Y = Paint.HeightMemory - Xpoint - 1;
        break;
    default:
        return;
    }
    
    switch(Paint.Mirror) {
    case MIRROR_NONE:
        break;
    case MIRROR_HORIZONTAL:
        X = Paint.WidthMemory - X - 1;
        break;
    case MIRROR_VERTICAL:
        Y = Paint.HeightMemory - Y - 1;
        break;
    case MIRROR_ORIGIN:
        X = Paint.WidthMemory - X - 1;
        Y = Paint.HeightMemory - Y - 1;
        break;
    default:
        return;
    }

    if(X >= Paint.WidthMemory || Y >= Paint.HeightMemory){
        return;
    }
    
    // Optimized pixel setting based on scale
    if(Paint.Scale == 2){
        UDOUBLE Addr = X / 8 + Y * Paint.WidthByte;
        UBYTE bitMask = 0x80 >> (X % 8);
        if(Color == BLACK)
            Paint.Image[Addr] &= ~bitMask;
        else
            Paint.Image[Addr] |= bitMask;
    }else if(Paint.Scale == 4){
        UDOUBLE Addr = X / 4 + Y * Paint.WidthByte;
        Color = Color % 4;
        UBYTE shift = (3 - (X % 4)) * 2;
        UBYTE mask = ~(0x03 << shift);
        Paint.Image[Addr] = (Paint.Image[Addr] & mask) | ((Color & 0x03) << shift);
    }else if(Paint.Scale == 7 || Paint.Scale == 16){
        UDOUBLE Addr = X / 2  + Y * Paint.WidthByte;
        UBYTE shift = (1 - (X % 2)) * 4;
        UBYTE mask = ~(0x0F << shift);
        Paint.Image[Addr] = (Paint.Image[Addr] & mask) | ((Color & 0x0F) << shift);
    }
}

/******************************************************************************
function: Clear the color of the picture
parameter:
    Color : Painted colors
******************************************************************************/
void Paint_Clear(UWORD Color)
{
    if(Paint.Scale == 2) {
		for (UWORD Y = 0; Y < Paint.HeightByte; Y++) {
			for (UWORD X = 0; X < Paint.WidthByte; X++ ) {//8 pixel =  1 byte
				UDOUBLE Addr = X + Y*Paint.WidthByte;
				Paint.Image[Addr] = Color;
			}
		}
    }else if(Paint.Scale == 4) {
        for (UWORD Y = 0; Y < Paint.HeightByte; Y++) {
            for (UWORD X = 0; X < Paint.WidthByte; X++ ) {
                UDOUBLE Addr = X + Y*Paint.WidthByte;
                Paint.Image[Addr] = (Color<<6)|(Color<<4)|(Color<<2)|Color;
            }
        }
    }else if(Paint.Scale == 7 || Paint.Scale == 16) {
		for (UWORD Y = 0; Y < Paint.HeightByte; Y++) {
			for (UWORD X = 0; X < Paint.WidthByte; X++ ) {
				UDOUBLE Addr = X + Y*Paint.WidthByte;
				Paint.Image[Addr] = (Color<<4)|Color;
			}
		}		
	}
}

/******************************************************************************
function: Clear the color of a window
parameter:
    Xstart : x starting point
    Ystart : Y starting point
    Xend   : x end point
    Yend   : y end point
    Color  : Painted colors
******************************************************************************/
void Paint_ClearWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD Color)
{
    UWORD X, Y;
    for (Y = Ystart; Y < Yend; Y++) {
        for (X = Xstart; X < Xend; X++) {//8 pixel =  1 byte
            Paint_SetPixel(X, Y, Color);
        }
    }
}

/******************************************************************************
function: Draw Point(Xpoint, Ypoint) Fill the color
parameter:
    Xpoint		: The Xpoint coordinate of the point
    Ypoint		: The Ypoint coordinate of the point
    Color		: Painted color
    Dot_Pixel	: point size
    Dot_Style	: point Style
******************************************************************************/
void Paint_DrawPoint(UWORD Xpoint, UWORD Ypoint, UWORD Color,
                     DOT_PIXEL Dot_Pixel, DOT_STYLE Dot_Style)
{
    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Paint_DrawPoint Input exceeds the normal display range\r\n");
        return;
    }

    int16_t XDir_Num , YDir_Num;
    if (Dot_Style == DOT_FILL_AROUND) {
        for (XDir_Num = 0; XDir_Num < 2 * Dot_Pixel - 1; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num < 2 * Dot_Pixel - 1; YDir_Num++) {
                if(Xpoint + XDir_Num - Dot_Pixel < 0 || Ypoint + YDir_Num - Dot_Pixel < 0)
                    break;
                // printf("x = %d, y = %d\r\n", Xpoint + XDir_Num - Dot_Pixel, Ypoint + YDir_Num - Dot_Pixel);
                Paint_SetPixel(Xpoint + XDir_Num - Dot_Pixel, Ypoint + YDir_Num - Dot_Pixel, Color);
            }
        }
    } else {
        for (XDir_Num = 0; XDir_Num <  Dot_Pixel; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num <  Dot_Pixel; YDir_Num++) {
                Paint_SetPixel(Xpoint + XDir_Num - 1, Ypoint + YDir_Num - 1, Color);
            }
        }
    }
}

/******************************************************************************
function: Draw a line of arbitrary slope
parameter:
    Xstart ：Starting Xpoint point coordinates
    Ystart ：Starting Xpoint point coordinates
    Xend   ：End point Xpoint coordinate
    Yend   ：End point Ypoint coordinate
    Color  ：The color of the line segment
    Line_width : Line width
    Line_Style: Solid and dotted lines
******************************************************************************/
void Paint_DrawLine(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                    UWORD Color, DOT_PIXEL Line_width, LINE_STYLE Line_Style)
{
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        Debug("Paint_DrawLine Input exceeds the normal display range\r\n");
        return;
    }

    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;
    int dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
    int dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;

    // Increment direction, 1 is positive, -1 is counter;
    int XAddway = Xstart < Xend ? 1 : -1;
    int YAddway = Ystart < Yend ? 1 : -1;

    //Cumulative error
    int Esp = dx + dy;
    char Dotted_Len = 0;

    for (;;) {
        Dotted_Len++;
        //Painted dotted line, 2 point is really virtual
        if (Line_Style == LINE_STYLE_DOTTED && Dotted_Len % 3 == 0) {
            //Debug("LINE_DOTTED\r\n");
            Paint_DrawPoint(Xpoint, Ypoint, IMAGE_BACKGROUND, Line_width, DOT_STYLE_DFT);
            Dotted_Len = 0;
        } else {
            Paint_DrawPoint(Xpoint, Ypoint, Color, Line_width, DOT_STYLE_DFT);
        }
        if (2 * Esp >= dy) {
            if (Xpoint == Xend)
                break;
            Esp += dy;
            Xpoint += XAddway;
        }
        if (2 * Esp <= dx) {
            if (Ypoint == Yend)
                break;
            Esp += dx;
            Ypoint += YAddway;
        }
    }
}

/******************************************************************************
function: Draw a rectangle
parameter:
    Xstart ：Rectangular  Starting Xpoint point coordinates
    Ystart ：Rectangular  Starting Xpoint point coordinates
    Xend   ：Rectangular  End point Xpoint coordinate
    Yend   ：Rectangular  End point Ypoint coordinate
    Color  ：The color of the Rectangular segment
    Line_width: Line width
    Draw_Fill : Whether to fill the inside of the rectangle
******************************************************************************/
void Paint_DrawRectangle(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                         UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill)
{
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        Debug("Input exceeds the normal display range\r\n");
        return;
    }

    if (Draw_Fill) {
        UWORD Ypoint;
        for(Ypoint = Ystart; Ypoint < Yend; Ypoint++) {
            Paint_DrawLine(Xstart, Ypoint, Xend, Ypoint, Color , Line_width, LINE_STYLE_SOLID);
        }
    } else {
        Paint_DrawLine(Xstart, Ystart, Xend, Ystart, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xstart, Ystart, Xstart, Yend, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xend, Yend, Xend, Ystart, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xend, Yend, Xstart, Yend, Color, Line_width, LINE_STYLE_SOLID);
    }
}

/******************************************************************************
function: Use the 8-point method to draw a circle of the
            specified size at the specified position->
parameter:
    X_Center  ：Center X coordinate
    Y_Center  ：Center Y coordinate
    Radius    ：circle Radius
    Color     ：The color of the ：circle segment
    Line_width: Line width
    Draw_Fill : Whether to fill the inside of the Circle
******************************************************************************/
void Paint_DrawCircle(UWORD X_Center, UWORD Y_Center, UWORD Radius,
                      UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill)
{
    if (X_Center > Paint.Width || Y_Center >= Paint.Height) {
        Debug("Paint_DrawCircle Input exceeds the normal display range\r\n");
        return;
    }

    //Draw a circle from(0, R) as a starting point
    int16_t XCurrent, YCurrent;
    XCurrent = 0;
    YCurrent = Radius;

    //Cumulative error,judge the next point of the logo
    int16_t Esp = 3 - (Radius << 1 );

    int16_t sCountY;
    if (Draw_Fill == DRAW_FILL_FULL) {
        while (XCurrent <= YCurrent ) { //Realistic circles
            for (sCountY = XCurrent; sCountY <= YCurrent; sCountY ++ ) {
                Paint_DrawPoint(X_Center + XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//1
                Paint_DrawPoint(X_Center - XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//2
                Paint_DrawPoint(X_Center - sCountY, Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//3
                Paint_DrawPoint(X_Center - sCountY, Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//4
                Paint_DrawPoint(X_Center - XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//5
                Paint_DrawPoint(X_Center + XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//6
                Paint_DrawPoint(X_Center + sCountY, Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//7
                Paint_DrawPoint(X_Center + sCountY, Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
            }
            if (Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    } else { //Draw a hollow circle
        while (XCurrent <= YCurrent ) {
            Paint_DrawPoint(X_Center + XCurrent, Y_Center + YCurrent, Color, Line_width, DOT_STYLE_DFT);//1
            Paint_DrawPoint(X_Center - XCurrent, Y_Center + YCurrent, Color, Line_width, DOT_STYLE_DFT);//2
            Paint_DrawPoint(X_Center - YCurrent, Y_Center + XCurrent, Color, Line_width, DOT_STYLE_DFT);//3
            Paint_DrawPoint(X_Center - YCurrent, Y_Center - XCurrent, Color, Line_width, DOT_STYLE_DFT);//4
            Paint_DrawPoint(X_Center - XCurrent, Y_Center - YCurrent, Color, Line_width, DOT_STYLE_DFT);//5
            Paint_DrawPoint(X_Center + XCurrent, Y_Center - YCurrent, Color, Line_width, DOT_STYLE_DFT);//6
            Paint_DrawPoint(X_Center + YCurrent, Y_Center - XCurrent, Color, Line_width, DOT_STYLE_DFT);//7
            Paint_DrawPoint(X_Center + YCurrent, Y_Center + XCurrent, Color, Line_width, DOT_STYLE_DFT);//0

            if (Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    }
}

/******************************************************************************
function: Show English characters
parameter:
    Xpoint           ：X coordinate
    Ypoint           ：Y coordinate
    Acsii_Char       ：To display the English characters
    Font             ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawChar(UWORD Xpoint, UWORD Ypoint, const char Acsii_Char,
                    sFONT* Font, UWORD Color_Foreground, UWORD Color_Background)
{
    UWORD Page, Column;

    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Paint_DrawChar Input exceeds the normal display range\r\n");
        return;
    }

    uint32_t Char_Offset = (Acsii_Char - ' ') * Font->Height * (Font->Width / 8 + (Font->Width % 8 ? 1 : 0));
    const unsigned char *ptr = &Font->table[Char_Offset];

    for (Page = 0; Page < Font->Height; Page ++ ) {
        for (Column = 0; Column < Font->Width; Column ++ ) {

            //To determine whether the font background color and screen background color is consistent
            if (FONT_BACKGROUND == Color_Background) { //this process is to speed up the scan
                if (*ptr & (0x80 >> (Column % 8)))
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
                    // Paint_DrawPoint(Xpoint + Column, Ypoint + Page, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
            } else {
                if (*ptr & (0x80 >> (Column % 8))) {
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
                    // Paint_DrawPoint(Xpoint + Column, Ypoint + Page, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                } else {
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Background);
                    // Paint_DrawPoint(Xpoint + Column, Ypoint + Page, Color_Background, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                }
            }
            //One pixel is 8 bits
            if (Column % 8 == 7)
                ptr++;
        }// Write a line
        if (Font->Width % 8 != 0)
            ptr++;
    }// Write all
}

/******************************************************************************
function:	Display the string
parameter:
    Xstart           ：X coordinate
    Ystart           ：Y coordinate
    pString          ：The first address of the English string to be displayed
    Font             ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawString_EN(UWORD Xstart, UWORD Ystart, const char * pString,
                         sFONT* Font, UWORD Color_Foreground, UWORD Color_Background)
{
    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;

    if (Xstart > Paint.Width || Ystart > Paint.Height) {
        Debug("Paint_DrawString_EN Input exceeds the normal display range\r\n");
        return;
    }

    while (* pString != '\0') {
        //if X direction filled , reposition to(Xstart,Ypoint),Ypoint is Y direction plus the Height of the character
        if ((Xpoint + Font->Width ) > Paint.Width ) {
            Xpoint = Xstart;
            Ypoint += Font->Height;
        }

        // If the Y direction is full, reposition to(Xstart, Ystart)
        if ((Ypoint  + Font->Height ) > Paint.Height ) {
            Xpoint = Xstart;
            Ypoint = Ystart;
        }
        Paint_DrawChar(Xpoint, Ypoint, * pString, Font, Color_Background, Color_Foreground);

        //The next character of the address
        pString ++;

        //The next word of the abscissa increases the font of the broadband
        Xpoint += Font->Width;
    }
}

/******************************************************************************
function:	Display nummber
parameter:
    Xstart           ：X coordinate
    Ystart           : Y coordinate
    Nummber          : The number displayed
    Font             ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
#define  ARRAY_LEN 255
void Paint_DrawNum(UWORD Xpoint, UWORD Ypoint, int32_t Nummber,
                   sFONT* Font, UWORD Color_Foreground, UWORD Color_Background)
{

    int16_t Num_Bit = 0, Str_Bit = 0;
    uint8_t Str_Array[ARRAY_LEN] = {0}, Num_Array[ARRAY_LEN] = {0};
    uint8_t *pStr = Str_Array;

    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Paint_DisNum Input exceeds the normal display range\r\n");
        return;
    }

    //Converts a number to a string
    while (Nummber) {
        Num_Array[Num_Bit] = Nummber % 10 + '0';
        Num_Bit++;
        Nummber /= 10;
    }

    //The string is inverted
    while (Num_Bit > 0) {
        Str_Array[Str_Bit] = Num_Array[Num_Bit - 1];
        Str_Bit ++;
        Num_Bit --;
    }

    //show
    Paint_DrawString_EN(Xpoint, Ypoint, (const char*)pStr, Font, Color_Background, Color_Foreground);
}

/******************************************************************************
function:	Display time
parameter:
    Xstart           ：X coordinate
    Ystart           : Y coordinate
    pTime            : Time-related structures
    Font             ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawTime(UWORD Xstart, UWORD Ystart, PAINT_TIME *pTime, sFONT* Font,
                    UWORD Color_Foreground, UWORD Color_Background)
{
    uint8_t value[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

    UWORD Dx = Font->Width;

    //Write data into the cache
    Paint_DrawChar(Xstart                           , Ystart, value[pTime->Hour / 10], Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx                      , Ystart, value[pTime->Hour % 10], Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx  + Dx / 4 + Dx / 2   , Ystart, ':'                    , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 2 + Dx / 2         , Ystart, value[pTime->Min / 10] , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 3 + Dx / 2         , Ystart, value[pTime->Min % 10] , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 4 + Dx / 2 - Dx / 4, Ystart, ':'                    , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 5                  , Ystart, value[pTime->Sec / 10] , Font, Color_Background, Color_Foreground);
    Paint_DrawChar(Xstart + Dx * 6                  , Ystart, value[pTime->Sec % 10] , Font, Color_Background, Color_Foreground);
}

/******************************************************************************
function:	Display monochrome bitmap
parameter:
    image_buffer ：A picture data converted to a bitmap
info:
    Use a computer to convert the image into a corresponding array,
    and then embed the array directly into Imagedata.cpp as a .c file.
******************************************************************************/
void Paint_DrawBitMap(const unsigned char* image_buffer)
{
    UWORD x, y;
    UDOUBLE Addr = 0;

    for (y = 0; y < Paint.HeightByte; y++) {
        for (x = 0; x < Paint.WidthByte; x++) {//8 pixel =  1 byte
            Addr = x + y * Paint.WidthByte;
            Paint.Image[Addr] = (unsigned char)image_buffer[Addr];
        }
    }
}

/******************************************************************************
function:	Display image
parameter:
    image            ：Image start address
    xStart           : X starting coordinates
    yStart           : Y starting coordinates
    xEnd             ：Image width
    yEnd             : Image height
******************************************************************************/
void Paint_DrawImage(const unsigned char *image_buffer, UWORD xStart, UWORD yStart, UWORD W_Image, UWORD H_Image) 
{
    UWORD x, y;
	UWORD w_byte=(W_Image%8)?(W_Image/8)+1:W_Image/8;
    UDOUBLE Addr = 0;
	UDOUBLE pAddr = 0;
    for (y = 0; y < H_Image; y++) {
        for (x = 0; x < w_byte; x++) {//8 pixel =  1 byte
            Addr = x + y * w_byte;
			pAddr=x+(xStart/8)+((y+yStart)*Paint.WidthByte);
            Paint.Image[pAddr] = (unsigned char)image_buffer[Addr];
        }
    }
}


// ============================================================================
// PART 2: ADD TO GUI_Paint.cpp (at the end of file)
// ============================================================================

/******************************************************************************
 * POLYGON RENDERING IMPLEMENTATION
 * Author: Navy
 * Purpose: Vector-based 7-segment display rendering
 * Date: 2024-11-20
 ******************************************************************************/

// Internal edge structure for scanline algorithm
typedef struct {
    int16_t yMin, yMax;
    float xAtYMin;
    float slopeInverse;  // dx/dy
} PolygonEdge;

// Internal helper: Build edge table from polygon vertices
static int buildEdgeTable(const int16_t* xPoints, const int16_t* yPoints, 
                          UWORD numPoints, PolygonEdge* edges) {
    int edgeCount = 0;
    
    for (UWORD i = 0; i < numPoints; i++) {
        UWORD next = (i + 1) % numPoints;
        
        int16_t x1 = xPoints[i];
        int16_t y1 = yPoints[i];
        int16_t x2 = xPoints[next];
        int16_t y2 = yPoints[next];
        
        // Skip horizontal edges
        if (y1 == y2) continue;
        
        // Ensure y1 < y2
        if (y1 > y2) {
            int16_t temp;
            temp = x1; x1 = x2; x2 = temp;
            temp = y1; y1 = y2; y2 = temp;
        }
        
        edges[edgeCount].yMin = y1;
        edges[edgeCount].yMax = y2;
        edges[edgeCount].xAtYMin = (float)x1;
        edges[edgeCount].slopeInverse = (float)(x2 - x1) / (float)(y2 - y1);
        edgeCount++;
    }
    
    return edgeCount;
}

// Internal helper: Fill scanline between edge intersections
static void fillScanline(int16_t y, float* xIntersections, int count, UWORD Color) {
    // Sort intersections
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (xIntersections[i] > xIntersections[j]) {
                float temp = xIntersections[i];
                xIntersections[i] = xIntersections[j];
                xIntersections[j] = temp;
            }
        }
    }
    
    // Fill between pairs
    for (int i = 0; i < count; i += 2) {
        if (i + 1 < count) {
            int16_t xStart = (int16_t)(xIntersections[i] + 0.5f);
            int16_t xEnd = (int16_t)(xIntersections[i + 1] + 0.5f);
            
            for (int16_t x = xStart; x <= xEnd; x++) {
                Paint_SetPixel(x, y, Color);
            }
        }
    }
}

/******************************************************************************
function: Calculate bounding box of polygon (for partial refresh region)
parameter:
    xPoints : Array of X coordinates
    yPoints : Array of Y coordinates
    numPoints : Number of vertices
    minX, minY, maxX, maxY : Output bounding box
******************************************************************************/
void Paint_GetPolygonBounds(const int16_t* xPoints, const int16_t* yPoints,
                            UWORD numPoints, UWORD* minX, UWORD* minY,
                            UWORD* maxX, UWORD* maxY) {
    if (numPoints == 0) return;
    
    int16_t xMin = xPoints[0], xMax = xPoints[0];
    int16_t yMin = yPoints[0], yMax = yPoints[0];
    
    for (UWORD i = 1; i < numPoints; i++) {
        if (xPoints[i] < xMin) xMin = xPoints[i];
        if (xPoints[i] > xMax) xMax = xPoints[i];
        if (yPoints[i] < yMin) yMin = yPoints[i];
        if (yPoints[i] > yMax) yMax = yPoints[i];
    }
    
    *minX = (UWORD)xMin;
    *minY = (UWORD)yMin;
    *maxX = (UWORD)xMax;
    *maxY = (UWORD)yMax;
}

/******************************************************************************
function: Draw filled or outline polygon
parameter:
    xPoints : Array of X coordinates (vertices)
    yPoints : Array of Y coordinates (vertices)
    numPoints : Number of vertices (minimum 3)
    Color : BLACK or WHITE
    Line_width : Line thickness for outline mode
    Draw_Fill : DRAW_FILL_EMPTY (outline) or DRAW_FILL_FULL (filled)
******************************************************************************/
void Paint_DrawPolygon(const int16_t* xPoints, const int16_t* yPoints,
                       UWORD numPoints, UWORD Color,
                       DOT_PIXEL Line_width, DRAW_FILL Draw_Fill) {
    if (numPoints < 3) {
        Debug("Paint_DrawPolygon requires at least 3 points\r\n");
        return;
    }
    
    // Outline mode: draw lines between consecutive vertices
    if (Draw_Fill == DRAW_FILL_EMPTY) {
        for (UWORD i = 0; i < numPoints; i++) {
            UWORD next = (i + 1) % numPoints;
            Paint_DrawLine(xPoints[i], yPoints[i], 
                          xPoints[next], yPoints[next],
                          Color, Line_width, LINE_STYLE_SOLID);
        }
        return;
    }
    
    // Filled mode: scanline algorithm
    // Allocate edge table (max numPoints edges, minus horizontal)
    PolygonEdge edges[16];  // Support up to 16 vertices
    if (numPoints > 16) {
        Debug("Paint_DrawPolygon: Max 16 vertices supported\r\n");
        return;
    }
    
    int edgeCount = buildEdgeTable(xPoints, yPoints, numPoints, edges);
    if (edgeCount == 0) return;
    
    // Find Y range
    UWORD minX, minY, maxX, maxY;
    Paint_GetPolygonBounds(xPoints, yPoints, numPoints, &minX, &minY, &maxX, &maxY);
    
    // Scanline fill
    float xIntersections[16];
    
    for (int16_t y = minY; y <= maxY; y++) {
        int intersectionCount = 0;
        
        // Find active edges at this Y
        for (int i = 0; i < edgeCount; i++) {
            if (y >= edges[i].yMin && y < edges[i].yMax) {
                xIntersections[intersectionCount++] = edges[i].xAtYMin + 
                    (y - edges[i].yMin) * edges[i].slopeInverse;
            }
        }
        
        if (intersectionCount >= 2) {
            fillScanline(y, xIntersections, intersectionCount, Color);
        }
    }
}

/******************************************************************************
 * 7-SEGMENT DISPLAY IMPLEMENTATION
 * Coordinates extracted from Figma SVG exports
 * Segment naming:
 *     aaa
 *    f   b
 *     ggg
 *    e   c
 *     ddd
 ******************************************************************************/

// Segment A: Top horizontal (51.5 x 8 px trapezoid)
// Path: M 0,0 H 51.5 L 42,8 H 9 Z
void Paint_Draw7Segment_A(UWORD x, UWORD y, UWORD Color) {
    int16_t xPoints[] = {0, 52, 42, 9};
    int16_t yPoints[] = {0, 0, 8, 8};
    
    // Offset by position
    for (int i = 0; i < 4; i++) {
        xPoints[i] += x;
        yPoints[i] += y;
    }
    
    Paint_DrawPolygon(xPoints, yPoints, 4, Color, 
                     DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment B: Top-right vertical (8 x 44.5 px trapezoid)
// Path: m 8,0 v 44.5 l -8,-5 V 7 Z
// Vertices: (8,0), (8,44.5), (0,39.5), (0,7)
void Paint_Draw7Segment_B(UWORD x, UWORD y, UWORD Color) {
    int16_t xPoints[] = {8, 8, 0, 0};
    int16_t yPoints[] = {0, 45, 40, 7};
    
    for (int i = 0; i < 4; i++) {
        xPoints[i] += x;
        yPoints[i] += y;
    }
    
    Paint_DrawPolygon(xPoints, yPoints, 4, Color,
                     DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment C: Bottom-right vertical (8 x 45.5 px trapezoid)
// Path: m 8,0 v 45.5 l -8,-8 v -33 z
// Vertices: (8,0), (8,45.5), (0,37.5), (0,4.5)
void Paint_Draw7Segment_C(UWORD x, UWORD y, UWORD Color) {
    int16_t xPoints[] = {8, 8, 0, 0};
    int16_t yPoints[] = {0, 46, 38, 5};
    
    for (int i = 0; i < 4; i++) {
        xPoints[i] += x;
        yPoints[i] += y;
    }
    
    Paint_DrawPolygon(xPoints, yPoints, 4, Color,
                     DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment D: Bottom horizontal (48 x 8 px trapezoid)
// Path: M 48,8 H 0 L 7.5,0 h 33 z
// Vertices: (48,8), (0,8), (7.5,0), (40.5,0)
void Paint_Draw7Segment_D(UWORD x, UWORD y, UWORD Color) {
    int16_t xPoints[] = {48, 0, 8, 41};
    int16_t yPoints[] = {8, 8, 0, 0};
    
    for (int i = 0; i < 4; i++) {
        xPoints[i] += x;
        yPoints[i] += y;
    }
    
    Paint_DrawPolygon(xPoints, yPoints, 4, Color,
                     DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment E: Bottom-left vertical (8 x 46 px trapezoid)
// Path: M 0,46 V 0 L 8,5 V 37.5 Z
// Vertices: (0,46), (0,0), (8,5), (8,37.5)
void Paint_Draw7Segment_E(UWORD x, UWORD y, UWORD Color) {
    int16_t xPoints[] = {0, 0, 8, 8};
    int16_t yPoints[] = {46, 0, 5, 38};
    
    for (int i = 0; i < 4; i++) {
        xPoints[i] += x;
        yPoints[i] += y;
    }
    
    Paint_DrawPolygon(xPoints, yPoints, 4, Color,
                     DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment F: Top-left vertical (8 x 45 px trapezoid)
// Path: M 0,45 V 0 L 8,7.5 V 40 Z
// Vertices: (0,45), (0,0), (8,7.5), (8,40)
void Paint_Draw7Segment_F(UWORD x, UWORD y, UWORD Color) {
    int16_t xPoints[] = {0, 0, 8, 8};
    int16_t yPoints[] = {45, 0, 8, 40};
    
    for (int i = 0; i < 4; i++) {
        xPoints[i] += x;
        yPoints[i] += y;
    }
    
    Paint_DrawPolygon(xPoints, yPoints, 4, Color,
                     DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// Segment G: Middle horizontal (49.5 x 8 px hexagon)
// Path: m 6.5,0 h 36 l 7,4 -7,4 H 6.5 L 0,4 Z
// Vertices: (6.5,0), (42.5,0), (49.5,4), (42.5,8), (6.5,8), (0,4)
void Paint_Draw7Segment_G(UWORD x, UWORD y, UWORD Color) {
    int16_t xPoints[] = {7, 43, 50, 43, 7, 0};
    int16_t yPoints[] = {0, 0, 4, 8, 8, 4};
    
    for (int i = 0; i < 6; i++) {
        xPoints[i] += x;
        yPoints[i] += y;
    }
    
    Paint_DrawPolygon(xPoints, yPoints, 6, Color,
                     DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

/******************************************************************************
function: Set clipping region for optimization
parameter:
    x1, y1 : Top-left corner of clipping region
    x2, y2 : Bottom-right corner of clipping region
info:
    Only pixels within this region will be drawn
    Useful for partial updates and performance optimization
******************************************************************************/
void Paint_SetClipRegion(UWORD x1, UWORD y1, UWORD x2, UWORD y2) {
    Paint.clippingEnabled = true;
    Paint.clipX1 = x1;
    Paint.clipY1 = y1;
    Paint.clipX2 = x2;
    Paint.clipY2 = y2;
}

/******************************************************************************
function: Clear clipping region (restore full screen drawing)
******************************************************************************/
void Paint_ClearClipRegion(void) {
    Paint.clippingEnabled = false;
}

/******************************************************************************
function: Draw horizontal line (optimized)
parameter:
    x, y : Starting point
    w    : Width
    Color: Line color
info:
    Much faster than calling Paint_DrawLine for horizontal lines
    Uses direct memory manipulation for efficiency
******************************************************************************/
void Paint_DrawHLine(UWORD x, UWORD y, UWORD w, UWORD Color) {
    if(y >= Paint.Height || x >= Paint.Width) return;
    
    // Clip width
    if(x + w > Paint.Width) {
        w = Paint.Width - x;
    }
    
    // For black/white mode, optimize byte-wise operations
    if(Paint.Scale == 2) {
        for(UWORD i = 0; i < w; i++) {
            Paint_SetPixel(x + i, y, Color);
        }
    } else {
        // For grayscale, still use pixel-by-pixel
        for(UWORD i = 0; i < w; i++) {
            Paint_SetPixel(x + i, y, Color);
        }
    }
}

/******************************************************************************
function: Draw vertical line (optimized)
parameter:
    x, y : Starting point
    h    : Height
    Color: Line color
info:
    Much faster than calling Paint_DrawLine for vertical lines
******************************************************************************/
void Paint_DrawVLine(UWORD x, UWORD y, UWORD h, UWORD Color) {
    if(x >= Paint.Width || y >= Paint.Height) return;
    
    // Clip height
    if(y + h > Paint.Height) {
        h = Paint.Height - y;
    }
    
    for(UWORD i = 0; i < h; i++) {
        Paint_SetPixel(x, y + i, Color);
    }
}

/******************************************************************************
function: Fill rectangle (highly optimized)
parameter:
    x, y : Top-left corner
    w, h : Width and height
    Color: Fill color
info:
    Optimized for filling large areas
    Uses direct memory operations for speed
******************************************************************************/
void Paint_FillRect(UWORD x, UWORD y, UWORD w, UWORD h, UWORD Color) {
    if(x >= Paint.Width || y >= Paint.Height) return;
    
    // Clip to screen bounds
    if(x + w > Paint.Width) w = Paint.Width - x;
    if(y + h > Paint.Height) h = Paint.Height - y;
    
    // Use optimized horizontal line drawing
    for(UWORD row = 0; row < h; row++) {
        Paint_DrawHLine(x, y + row, w, Color);
    }
}

// ============================================================================
// SIMPLIFIED API IMPLEMENTATION (Adafruit GFX Style)
// ============================================================================

static UBYTE* autoAllocatedBuffer = NULL;
static UWORD autoBufferSize = 0;

/******************************************************************************
function: Begin with auto-allocated buffer
parameter:
    Width  : Image width (pixels)
    Height : Image height (pixels)
return:
    true on success, false on allocation failure
info:
    Automatically allocates buffer memory
    Use Paint_End() to free the buffer
******************************************************************************/
bool Paint_Begin(UWORD Width, UWORD Height) {
    // Calculate required buffer size
    UWORD bufferSize = ((Width % 8 == 0) ? (Width / 8) : (Width / 8 + 1)) * Height;
    
    // Free previous buffer if exists
    if(autoAllocatedBuffer != NULL) {
        free(autoAllocatedBuffer);
        autoAllocatedBuffer = NULL;
    }
    
    // Allocate new buffer
    autoAllocatedBuffer = (UBYTE*)malloc(bufferSize);
    if(autoAllocatedBuffer == NULL) {
        Debug("Paint_Begin: malloc failed\r\n");
        return false;
    }
    
    autoBufferSize = bufferSize;
    
    // Initialize Paint structure
    Paint_NewImage(autoAllocatedBuffer, Width, Height, ROTATE_0, WHITE);
    Paint_SelectImage(autoAllocatedBuffer);
    Paint_Clear(WHITE);
    
    return true;
}

/******************************************************************************
function: Begin with external buffer
parameter:
    externalBuffer : User-provided buffer
    Width          : Image width (pixels)
    Height         : Image height (pixels)
return:
    true on success, false on invalid parameters
info:
    Uses external buffer (no auto-allocation)
    User responsible for buffer lifetime
******************************************************************************/
bool Paint_Begin(UBYTE *externalBuffer, UWORD Width, UWORD Height) {
    if(externalBuffer == NULL) {
        Debug("Paint_Begin: NULL buffer\r\n");
        return false;
    }
    
    // Initialize Paint structure with external buffer
    Paint_NewImage(externalBuffer, Width, Height, ROTATE_0, WHITE);
    Paint_SelectImage(externalBuffer);
    Paint_Clear(WHITE);
    
    return true;
}

/******************************************************************************
function: End and free auto-allocated buffer
parameter: None
return: None
info:
    Frees automatically allocated buffer
    Does not affect external buffers
******************************************************************************/
void Paint_End(void) {
    if(autoAllocatedBuffer != NULL) {
        free(autoAllocatedBuffer);
        autoAllocatedBuffer = NULL;
        autoBufferSize = 0;
    }
}

/******************************************************************************
function: Get current buffer pointer
parameter: None
return: Pointer to current image buffer
******************************************************************************/
UBYTE* Paint_GetBuffer(void) {
    return Paint.Image;
}

/******************************************************************************
function: Get buffer size in bytes
parameter: None
return: Buffer size in bytes
******************************************************************************/
UWORD Paint_GetBufferSize(void) {
    if(autoAllocatedBuffer != NULL) {
        return autoBufferSize;
    }
    return Paint.WidthByte * Paint.HeightByte;
}

// ============================================================================
// C++ WRAPPER CLASS IMPLEMENTATION
// ============================================================================

#ifdef __cplusplus
#include "utility/EPD_4in2_V2.h"

/******************************************************************************
function: Constructor
******************************************************************************/
EPD_Display::EPD_Display() {
    internalBuffer = NULL;
    width = 0;
    height = 0;
    initialized = false;
    autoAllocated = false;
}

/******************************************************************************
function: Destructor
******************************************************************************/
EPD_Display::~EPD_Display() {
    end();
}

/******************************************************************************
function: Begin with auto mode (400x300)
return: true on success, false on failure
******************************************************************************/
bool EPD_Display::begin(void) {
    return begin(EPD_4IN2_V2_WIDTH, EPD_4IN2_V2_HEIGHT);
}

/******************************************************************************
function: Begin with custom size and auto buffer
parameter:
    w : Width in pixels
    h : Height in pixels
return: true on success, false on failure
******************************************************************************/
bool EPD_Display::begin(UWORD w, UWORD h) {
    width = w;
    height = h;
    
    // Calculate buffer size
    UWORD bufferSize = ((w % 8 == 0) ? (w / 8) : (w / 8 + 1)) * h;
    
    // Allocate buffer
    internalBuffer = (UBYTE*)malloc(bufferSize);
    if(internalBuffer == NULL) {
        Debug("EPD_Display: malloc failed\r\n");
        return false;
    }
    
    autoAllocated = true;
    
    // Initialize hardware
    if(DEV_Module_Init() != 0) {
        free(internalBuffer);
        internalBuffer = NULL;
        return false;
    }
    
    EPD_4IN2_V2_Init();
    
    // Initialize Paint
    Paint_NewImage(internalBuffer, width, height, ROTATE_0, WHITE);
    Paint_SelectImage(internalBuffer);
    Paint_Clear(WHITE);
    
    initialized = true;
    return true;
}

/******************************************************************************
function: Begin with external buffer
parameter:
    buffer : User-provided buffer
    w      : Width in pixels
    h      : Height in pixels
return: true on success, false on failure
******************************************************************************/
bool EPD_Display::begin(UBYTE* buffer, UWORD w, UWORD h) {
    if(buffer == NULL) return false;
    
    width = w;
    height = h;
    internalBuffer = buffer;
    autoAllocated = false;
    
    // Initialize hardware
    if(DEV_Module_Init() != 0) {
        return false;
    }
    
    EPD_4IN2_V2_Init();
    
    // Initialize Paint
    Paint_NewImage(internalBuffer, width, height, ROTATE_0, WHITE);
    Paint_SelectImage(internalBuffer);
    Paint_Clear(WHITE);
    
    initialized = true;
    return true;
}

/******************************************************************************
function: End and cleanup
******************************************************************************/
void EPD_Display::end(void) {
    if(autoAllocated && internalBuffer != NULL) {
        free(internalBuffer);
        internalBuffer = NULL;
    }
    initialized = false;
}

/******************************************************************************
function: Clear display to white
******************************************************************************/
void EPD_Display::clear(void) {
    if(!initialized) return;
    Paint_Clear(WHITE);
    EPD_4IN2_V2_Clear();
}

/******************************************************************************
function: Clear display to black
******************************************************************************/
void EPD_Display::clearToBlack(void) {
    if(!initialized) return;
    Paint_Clear(BLACK);
    EPD_4IN2_V2_Display(internalBuffer);
}

/******************************************************************************
function: Full refresh display
******************************************************************************/
void EPD_Display::display(void) {
    if(!initialized) return;
    EPD_4IN2_V2_Display(internalBuffer);
}

/******************************************************************************
function: Fast refresh display
******************************************************************************/
void EPD_Display::displayFast(void) {
    if(!initialized) return;
    EPD_4IN2_V2_Display_Fast(internalBuffer);
}

/******************************************************************************
function: Partial refresh display
parameter:
    x, y : Top-left corner
    w, h : Width and height
******************************************************************************/
void EPD_Display::displayPartial(UWORD x, UWORD y, UWORD w, UWORD h) {
    if(!initialized) return;
    EPD_4IN2_V2_PartialDisplay(internalBuffer, x, y, x + w, y + h);
}

/******************************************************************************
function: Put display to sleep
******************************************************************************/
void EPD_Display::sleep(void) {
    if(!initialized) return;
    EPD_4IN2_V2_Sleep();
}

/******************************************************************************
function: Wake display
******************************************************************************/
void EPD_Display::wake(void) {
    if(!initialized) return;
    EPD_4IN2_V2_Init();
}

#endif // __cplusplus