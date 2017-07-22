// SplitMovie.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
//#include <stdlib.h>
#include <regex>
#include <ctime>
#include <iomanip>
#include <fstream>

#include <opencv2\opencv.hpp>
#include <opencv2\gpu\gpu.hpp>

#ifndef UNICODE  
typedef std::string String;
#else
typedef std::wstring String;
#endif

struct MovieStat{
	double width;
	double height;
	double totalFrames;
	double frameRate;
};
struct Period{
	std::string start;
	std::string end;
};
std::vector<int> timeList; // stored blocks location
std::vector<int> timeListAll; // stored frame time along the whole movie
std::vector<double> meanList; // stored mean power per frame along the whole movie
cv::VideoCapture cap;
MovieStat movieStat;
std::string fileName;
std::string choice;
double threshold;
int lastFrameTime;
std::vector<Period> periodList;
std::vector<double> minPowers;
std::vector<double> maxPowers;

std::string double2string(const double& in){
	std::stringstream ss;
	ss << in;
	return ss.str();
}
std::string sfGetSubStr(const std::string& target, const std::string& pattern){
	std::smatch match;

	if (std::regex_search(target.begin(), target.end(), match, std::regex(pattern)))
		return match[1].str();

	return "";
}
std::string int2string(const int& in){
	std::stringstream ss;
	ss << in;
	return ss.str();
}
double MatMean(const cv::Mat& frame){
	return cv::mean(frame).val[0];
}
std::string Argv2String(const _TCHAR* temp){
	String Arg = temp;
	std::string out(Arg.begin(), Arg.end());
	return out;
}
double Argv2Double(const _TCHAR* temp){
	String Arg = temp;
	std::string argString(Arg.begin(), Arg.end());
	return std::stod(argString.c_str());
}
void GetMovieStat(){
	movieStat.width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	movieStat.height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	movieStat.totalFrames = cap.get(CV_CAP_PROP_FRAME_COUNT);
	movieStat.frameRate = cap.get(CV_CAP_PROP_FPS);

	std::cout << "Movie width: " << movieStat.width << "\n";
	std::cout << "Movie height: " << movieStat.height << "\n";
	std::cout << "Movie totalFrames: " << movieStat.totalFrames << "\n";
	std::cout << "Movie frameRate: " << movieStat.frameRate << "\n";
}
void FileChecking(){
	if (!cap.isOpened()){
		std::cerr << "Error: Cannot open file " << fileName << "\n";
		exit(0);
	}
	std::cout << "Open file " << fileName << " success.\n";
}
bool isBlock(const double& value){
	if(choice == "s"){
		return (value < threshold);
	}else{
		return (value > threshold);
	}
}
std::vector<double> MakeSteps(const double& value, const double& step, const int& iter){
	std::vector<double> vec;
	for(int i = 0; i <= iter; i++){
		vec.push_back(value + (step * i));
	}
	return vec;
}
void OutPutGNUCmd(const std::string& fileName, const std::string& dataFile){
	std::ofstream FH(fileName);
	FH << "set title \"Movie Power\"\n";
	FH << "set xlabel \"Frames\"\n";
	FH << "set ylabel \"Power\"\n";
	FH << "set ytics \"5\"\n";
	FH << "plot \"" + dataFile + "\" title \"\" with lines\n";
	FH << "set term png medium size 1680,1024\n";
	FH << "set output \"" + fileName + ".png\"\n";
	FH << "replot\n";
	FH.close();

	std::string cmd = "gnuplot " + fileName;
	std::system(cmd.c_str());
}
void OutPutPowerStat(const std::string& fileName){
	std::ofstream FH(fileName);
	for(size_t i = 1; i < meanList.size(); i++){
		FH << meanList[i] << "\n";
	}
	FH.close();
}
void GetBlockTime(){
	timeList.clear();
	for(size_t i = 0; i < timeListAll.size(); i++){
		if(isBlock(meanList[i])){
			timeList.push_back(timeListAll[i]);
		}
	}
}
void FillUpTimeList(){
	std::vector<int>::iterator it = timeList.begin();
	timeList.insert(it, 0);
	timeList.push_back(lastFrameTime);
}
void UniqTimeList(){
	std::vector<int>::iterator it = timeList.begin();
	it = std::unique(timeList.begin(), timeList.end());
	timeList.resize(std::distance(timeList.begin(), it));
}
std::vector<int> CleanList(const std::vector<int>& list){
	std::vector<int> outList;

	if (list.size() < 1)
		return outList;

	int lastVal = list[0];
	for (size_t i = 1; i < list.size(); i++){
		if (std::abs(lastVal - list[i]) <= 5){ // if difference smaller than 5 sec
			lastVal = list[i];
		}
		else{
			outList.push_back(lastVal);
			lastVal = list[i];
		}
	}
	outList.push_back(lastVal);
	return outList;
}
bool ascTrue(int i, int j) { return (i < j); }
bool decTrue(int i, int j) { return (i > j); }
std::vector<int> MakeEndList(const std::vector<int>& list){
	return CleanList(list);
}
std::vector<int> MakeStartList(std::vector<int> list){
	std::sort(list.begin(), list.end(), decTrue);
	std::vector<int> outList = CleanList(list);
	std::sort(outList.begin(), outList.end());
	return outList;
}
int Sec2Hour(const int& sec){
	return sec / 3600;
}
int Sec2Min(const int& sec){
	return (sec % 3600) / 60;
}
int Sec2Sec(const int& sec){
	return (sec % 3600) % 60;
}
std::string Sec2TimeString(const int& sec){
	std::stringstream buffer;
	buffer << std::setw(2) << std::setfill('0') << Sec2Hour(sec) << ":";
	buffer << std::setw(2) << std::setfill('0') << Sec2Min(sec) << ":";
	buffer << std::setw(2) << std::setfill('0') << Sec2Sec(sec);
	return buffer.str();
}
std::vector<Period> MakePeriodList(const std::vector<int>& startList, const std::vector<int>& endList){
	std::vector<Period> outList;

	if (startList.size() < 1 || endList.size() < 1 || startList.size() != endList.size())
		return outList;

	for (size_t i = 0; i < startList.size()-1; i++){
		Period temp;
		temp.start = Sec2TimeString(startList[i]);
		temp.end = Sec2TimeString(endList[i+1]);
		outList.push_back(temp);
	}
	return outList;
}
void MakePeriodList(){
	periodList.clear();
	periodList = MakePeriodList(MakeStartList(timeList), MakeEndList(timeList));
}
void CallFunction(){
	std::string basename = sfGetSubStr(fileName, "(.*)\\..*");
	std::string extension = sfGetSubStr(fileName, ".*(\\..*)");

	for (size_t i = 0; i < periodList.size(); i++){
		std::string cmd = "ffmpeg.exe -i " + fileName + " -vcodec copy -acodec copy -ss "
			+ periodList[i].start + " -to " + periodList[i].end + " "
			+ basename + "_" + int2string(static_cast<int>(i)) + extension;
		std::system(cmd.c_str());
	}

}
void PrintProgressBar(float fStepTime, float fAccuTime, float progress)
{
	//while (progress < 1.0)
	//{
		int barWidth = 70;

		std::cout << "One minute movie used:" << fStepTime;
		std::cout << " Accumulated used:" << fAccuTime;
		std::cout << " [";
		int pos = barWidth * progress;
		for (int i = 0; i < barWidth; ++i) {
			if (i < pos) std::cout << "=";
			else if (i == pos) std::cout << ">";
			else std::cout << " ";
		}
		std::cout << "] " << int(progress * 100.0) << " %\r";
		std::cout.flush();

		//progress += 0.16; // for demonstration only
	//}
	//std::cout << std::endl;
}
void ProcessWholeMovie(){
	cv::Mat frame, grayFrame;
	timeListAll.clear();
	meanList.clear();
	int curFrame = 0;
	std::clock_t c_start = std::clock();
	std::clock_t first_start = std::clock();

	while(cap.read(frame)){
		cv::cvtColor(frame, grayFrame, CV_BGR2GRAY);

		timeListAll.push_back(static_cast<int>(cap.get(CV_CAP_PROP_POS_MSEC) / 1000));
		meanList.push_back(MatMean(grayFrame));

		if(++curFrame % (static_cast<int>(movieStat.frameRate) * 60) == 0)
		{
			//std::cout << "Process one Minute of Movie used "
			//	<< float(clock() - c_start) / CLOCKS_PER_SEC << " sec.\n"
			//	<< "Total sec spent " << float(clock() - first_start) / CLOCKS_PER_SEC << "\n"
			//	<< "Processed " << curFrame / movieStat.totalFrames << "\n";
			PrintProgressBar(float(clock() - c_start) / CLOCKS_PER_SEC,
				float(clock() - first_start) / CLOCKS_PER_SEC,
				curFrame / movieStat.totalFrames);
			c_start = std::clock();
		}
	}
	lastFrameTime = static_cast<int>(cap.get(CV_CAP_PROP_POS_MSEC) / 1000);
	PrintProgressBar(float(clock() - c_start) / CLOCKS_PER_SEC,
		float(clock() - first_start) / CLOCKS_PER_SEC,
		1.0);
	std::cout << std::endl;
}
void OutPutText(){
	std::string basename = sfGetSubStr(fileName, "(.*)\\..*");
	std::string extension = sfGetSubStr(fileName, ".*(\\..*)");

	std::ofstream FH(basename + "_" + double2string(threshold) + ".bat");

	for (size_t i = 0; i < periodList.size(); i++){
		FH << "ffmpeg.exe -i " + fileName + " -vcodec copy -acodec copy -ss "
			+ periodList[i].start + " -to " + periodList[i].end + " "
			+ basename + "_" + int2string(static_cast<int>(i)) + extension << "\n";
	}
	
	FH.close();
}
void ProcessStack(){
	GetBlockTime();

	FillUpTimeList();
	UniqTimeList();

	MakePeriodList();
	OutPutText();
}
void ProcessMinCases(){
	choice = "s";

	for(size_t i = 0; i < minPowers.size(); i++){
		threshold = minPowers[i];
		ProcessStack();		
	}
}
void ProcessMaxCases(){
	choice = "b";

	for(size_t i = 0; i < maxPowers.size(); i++){
		threshold = maxPowers[i];
		ProcessStack();
	}
}
int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2){
		std::cerr << "Usage: splitMovie.exe fileName\n";
		return 1;
	}

	fileName = Argv2String(argv[1]);
	//choice = Argv2String(argv[2]);
	//threshold = Argv2Double(argv[3]);

	cap.open(fileName);
	FileChecking();
	GetMovieStat();	

	ProcessWholeMovie();

	minPowers = MakeSteps(*std::min_element(meanList.begin(), meanList.end()), 5, 4);
	maxPowers = MakeSteps(*std::max_element(meanList.begin(), meanList.end()), -5, 4);
	OutPutPowerStat(fileName+".power");
	OutPutGNUCmd(fileName+".plt", fileName+".power");

	ProcessMinCases();
	ProcessMaxCases();
	
	return 0;
}

