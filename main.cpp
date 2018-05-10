#include <iostream>
#include <ctime>
#include <string>
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/core/mat.hpp"
#include <sstream>

template <class T>
inline std::string ToString (const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

using namespace cv;
using namespace std;

int DEBUG = 0; //tryb debugu czyli z okienkami do podglądu
int timeToMakePhoto = 50; //w milisekundach * 10 czas pomiędzy poszczególnymi zdjęciami
int SENSITIVITY_VALUE = 60; //jak duże różnice ma chwytać
int BLUR_SIZE = 10;
int WindowW = 480;
int WindowH = 360;

//okno z parametrami
void ChangeParam(int, void*) {}
void ProgramParameters()
{
    namedWindow("Industrial Camera", 0);

    createTrackbar( "Framerate", "Industrial Camera", &(timeToMakePhoto), 100, ChangeParam);
    createTrackbar( "Sensitivity", "Industrial Camera", &(SENSITIVITY_VALUE), 100, ChangeParam);
    createTrackbar( "DEBUG", "Industrial Camera", &(DEBUG), 1, ChangeParam);
}

unsigned long long imageIndex = 0;

//spisywanie obecnej daty by dorzucoć ją jako nazwe pliku
time_t     now;
struct tm  tstruct;
char       buf[80];

string CurrentDateTime()
{
    now = time(0);
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d+%X", &tstruct);
    buf[13] = '_';
    buf[16] = '_';

    return buf;
}

Rect objectRectangle = Rect(0,0,0,0);

double CalculateDiffrencePower(Mat &tresh)
{
    Mat temp;
	cvtColor(tresh, temp, COLOR_BGR2GRAY);

	//szukanie konturów na obrazku
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;

	findContours(temp, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	//wyliczanie zajmowanego obszaru
    double power = 0;
    for(unsigned int i = 0; i < contours.size(); ++i)
    {
        objectRectangle = boundingRect(contours.at(i));
        power += objectRectangle.width * objectRectangle.height;
    }

    return power;
}

int main()
{
    //materiały grafik
    Mat RealView, NextRealView;
    Mat GrayView, NextGrayView;
    Mat DiffView;
    Mat TresholdView;

    //ustawienie rozmiaru grafik
    Size imageSize(WindowW, WindowH);

    //uchwyty do okien
    namedWindow("GrayView");
    namedWindow("TresholdView");
    resizeWindow("GrayView", WindowW, WindowH);
    resizeWindow("TresholdView", WindowW, WindowH);
    if(!DEBUG)
    {
            destroyWindow("GrayView");
            destroyWindow("TresholdView");
    }

    //typ kompresji dla plików
    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(80);

    string fileName = ""; //nazwa pliku

    //otwarcie kamery do nagrywania
    VideoCapture capture(0);
	capture.open(0);

	ProgramParameters();

	for(;;)
    {
        //wczytanie grafiki z kamerki
        capture.read(RealView);
        resize(RealView, RealView, imageSize);

        cvtColor(RealView, GrayView, COLOR_BGR2GRAY);

        //czas pomiędzy klatakami
        switch(waitKey((timeToMakePhoto + 1) * 10))
        {
			case 27: //ESCAPE wyłącza program
				return 0;
        }

        //wczytanie kolejnej grafiki po określonym czasie żeby móc ją porównać
        capture.read(NextRealView);
        resize(NextRealView, NextRealView, imageSize);

        cvtColor(NextRealView, NextGrayView, COLOR_BGR2GRAY);

        //obliczanie graiki z różnicą
        absdiff(RealView, NextRealView, DiffView);

        //przygotowanie grafiki z różnicą
        threshold(DiffView, TresholdView, SENSITIVITY_VALUE, 255, THRESH_BINARY);

        if(DEBUG) //wyświetlanie okien dla debugu
        {
            imshow("GrayView", GrayView);
            imshow("TresholdView", TresholdView);
        }
        else
        {
            destroyWindow("GrayView");
            destroyWindow("TresholdView");
        }

        if(CalculateDiffrencePower(TresholdView) > 500) //sprawdzanie czy był jakiś ruch i zapisanie do pliku
        {
            fileName = "./" + CurrentDateTime() + "+" + ToString(imageIndex) + ".jpg";
            ++imageIndex;

            imwrite(fileName, NextGrayView, compression_params);
        }
    }

    return 0;
}
