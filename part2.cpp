﻿#include <iostream>
#if defined WIN32 || defined _WIN32 || defined _WIN64
#include <windows.h>
#ifndef TIME_POINT_TYPE
#define TIME_POINT_TYPE chrono::steady_clock::time_point
#endif
#else
#ifndef TIME_POINT_TYPE
#define TIME_POINT_TYPE chrono::system_clock::time_point
#endif
#endif
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <chrono>
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif
#ifndef EOF
#define EOF (-1)
#endif
#ifndef NULL
#define NULL 0
#endif
#ifndef INITIAL_VALUE
#define INITIAL_VALUE 0
#endif
#ifndef WINDOWS_MAX_PATH
#define WINDOWS_MAX_PATH 32762
#endif
using namespace std;
char path[WINDOWS_MAX_PATH + 1] = { 0 };


struct Point
{
	const size_t index = INITIAL_VALUE;
	const long double x = INITIAL_VALUE, y = INITIAL_VALUE;
};


class LocationSearch
{
private:
	vector<string> lines{};
	vector<vector<vector<Point>>> cells{};
	long int gridSize = 0; // 0 for non-initialization
	long double xLB = INITIAL_VALUE, xUB = INITIAL_VALUE, yLB = INITIAL_VALUE, yUB = INITIAL_VALUE, xSize = INITIAL_VALUE, ySize = INITIAL_VALUE;
	
	bool getTxt(const string filePath)
	{
		try
		{
			ifstream fp = ifstream(filePath);
			if (fp.is_open())
			{
				string line{};
				while (!fp.eof())
				{
					getline(fp, line);
					this->lines.push_back(line);
				}
				if (!this->lines.empty() && this->lines[this->lines.size() - 1].empty()) // remove the empty line at the end of the file
					this->lines.pop_back();
				return true;
			}
			else
				return false;
		}
		catch (...)
		{
			return false;
		}
	}
	
public:
	static const size_t INVALID = (size_t)(-1);
	
	LocationSearch() {}
	bool build(const string filePath, long int gSize)
	{
		/* Initialization */
		if (gSize < 1 || !this->getTxt(filePath))
			return false;
		
		/* Cells */
		vector<Point> points{};
		const size_t lineCount = this->lines.size();
		for (size_t i = 0; i < lineCount; ++i)
		{
			const size_t locationPos = this->lines[i].find("\tlocation: "), tagsPos = this->lines[i].find("\ttags: ");
			if (string::npos != locationPos && string::npos != tagsPos)
			{
				const string subString = this->lines[i].substr(locationPos + 11, tagsPos - locationPos - 11);
				const size_t lIdx = subString.find(","), rIdx = subString.rfind(",");
				if (string::npos != lIdx && lIdx == rIdx) // to guarantee that there is only a ','
				{
					try
					{
						points.push_back(Point{ i, stold(subString.substr(0, lIdx)), stold(subString.substr(lIdx + 1)) });
					}
					catch (...) {}
				}
			}
		}
		if (points.empty())
		{
			this->gridSize = 0; // 0 for non-initialization
			return false;
		}
		
		/* Bounds */
		this->xLB = ((vector<Point>::iterator)min_element(points.begin(), points.end(), [](const Point& a, const Point& b) { return a.x < b.x; }))->x;
		this->xUB = ((vector<Point>::iterator)max_element(points.begin(), points.end(), [](const Point& a, const Point& b) { return a.x < b.x; }))->x;
		this->yLB = ((vector<Point>::iterator)min_element(points.begin(), points.end(), [](const Point& a, const Point& b) { return a.y < b.y; }))->y;
		this->yUB = ((vector<Point>::iterator)max_element(points.begin(), points.end(), [](const Point& a, const Point& b) { return a.y < b.y; }))->y;
		const long double xDelta = xUB - xLB, yDelta = yUB - yLB;
		cout << "bounds: " << fixed << xLB << " " << xUB << " " << yLB << " " << yUB << endl << "widths: " << xDelta << " " << yDelta << endl;
		
		/* Cells */
		this->cells.clear();
		for (long int i = 0; i < gSize; ++i)
		{
			this->cells.push_back(vector<vector<Point>>{});
			for (long int j = 0; j < gSize; ++j)
				this->cells[i].push_back(vector<Point>{});
		}
		this->xSize = xDelta / gSize;
		this->ySize = yDelta / gSize;
		for (const Point& point : points)
			this->cells[min(gSize - 1, (long int)((point.x - xLB) / xSize))][min(gSize - 1, (long int)((point.y - yLB) / ySize))].push_back(point);
		for (long int i = 0; i < gSize; ++i)
			for (long int j = 0; j < gSize; ++j)
				if (!this->cells[i][j].empty())
					cout << i << " " << j << " " << this->cells[i][j].size() << endl;
		cout << endl;
#ifdef _DEBUG
		cout << "grid[5][36] = [";
		if (!this->cells[5][36].empty())
		{
			cout << this->cells[5][36][0].index;
			const size_t tmpSize = this->cells[5][36].size();
			for (size_t k = 1; k < tmpSize; ++k)
				cout << ", " << this->cells[5][36][k].index;
		}
		cout << "]" << endl << endl;
#endif
		this->gridSize = gSize;
		return true;
	}
	bool build(const string filePath) { return this->build(filePath, 50); }
	size_t spaSearchRaw(long double xLow, long double xHigh, long double yLow, long double yHigh)
	{
		if (this->gridSize >= 1)
		{
			vector<size_t> results{};
			const size_t lineCount = this->lines.size();
			const TIME_POINT_TYPE startTime = chrono::high_resolution_clock::now();
			for (size_t idx = 0; idx < lineCount; ++idx)
			{
				const size_t locationPos = this->lines[idx].find("\tlocation: "), tagsPos = this->lines[idx].find("\ttags: ");
				if (string::npos != locationPos && string::npos != tagsPos)
				{
					const string subString = this->lines[idx].substr(locationPos + 11, tagsPos - locationPos - 11);
					const size_t lIdx = subString.find(","), rIdx = subString.rfind(",");
					if (string::npos != lIdx && lIdx == rIdx) // to guarantee that there is only a ','
					{
						long double x = INITIAL_VALUE, y = INITIAL_VALUE;
						try
						{
							x = stold(subString.substr(0, lIdx));
							y = stold(subString.substr(lIdx + 1));
						}
						catch (...)
						{
							continue;
						}
						if (xLow <= x && x <= xHigh && yLow <= y && y <= yHigh)
							results.push_back(idx);
					}
				}
			}
			const long double timeDelta = (long double)(chrono::high_resolution_clock::now() - startTime).count() / 1000000000;
			cout << "spaSearchRaw: " << results.size() << " result(s), cost = " << fixed << timeDelta << " second(s)" << endl;
			for (const size_t& idx : results)
				cout << this->lines[idx] << endl;
			cout << endl;
			return results.size();
		}
		else
			return INVALID;
	}
	size_t spaSearchGrid(long double xLow, long double xHigh, long double yLow, long double yHigh)
	{
		if (this->gridSize >= 1)
		{
			vector<size_t> results{};
			const TIME_POINT_TYPE startTime = chrono::high_resolution_clock::now();
			const long int xLowIdx = this->xLB < xLow && xLow < this->xUB ? (long int)((xLow - this->xLB) / this->xSize) : (xLow <= this->xLB ? -1 : this->gridSize);
			const long int xHighIdx = this->xLB < xHigh && xHigh < this->xUB ? (long int)((xHigh - this->xLB) / this->xSize) : (xHigh <= this->xLB ? -1 : this->gridSize);
			const long int yLowIdx = this->yLB < yLow && yLow < this->yUB ? (long int)((yLow - this->yLB) / this->ySize) : (yLow <= this->yLB ? -1 : this->gridSize);
			const long int yHighIdx = this->yLB < yHigh && yHigh < this->yUB ? (long int)((yHigh - this->yLB) / this->ySize) : (yHigh <= this->yLB ? -1 : this->gridSize);
#ifdef _DEBUG
			cout << "The four boundary values locate at (" << xLowIdx << ", " << xHighIdx << ", " << yLowIdx << ", " << yHighIdx << ") of the grid. " << endl;
#endif
			if (0 <= yHighIdx && yHighIdx < this->gridSize) // the upper side (from left to right and exclude the rightmost cell)
				for (long int i = max((long int)0, xLowIdx); i < min(xHighIdx, this->gridSize - 1); ++i)
				{
#ifdef _DEBUG
					cout << "The program is accessing grid[" << i << "][" << yHighIdx << "]. " << endl;
#endif
					for (const Point& point : this->cells[i][yHighIdx])
						if (xLow <= point.x && point.x <= xHigh && yLow <= point.y && point.y <= yHigh)
							results.push_back(point.index);
				}
			if (0 <= xHighIdx && xHighIdx < this->gridSize) // the right side (from up to bottom and exclude the bottom cell)
				for (long int j = min(yHighIdx, this->gridSize - 1); j > max((long int)0, yLowIdx); --j)
				{
#ifdef _DEBUG
					cout << "The program is accessing grid[" << xHighIdx << "][" << j << "]. " << endl;
#endif
					for (const Point& point : this->cells[xHighIdx][j])
						if (xLow <= point.x && point.x <= xHigh && yLow <= point.y && point.y <= yHigh)
							results.push_back(point.index);
				}
			if (0 <= yLowIdx && yLowIdx < this->gridSize) // the lower side (from right to left and exclude the leftmost cell)
				for (long int i = min(xHighIdx, this->gridSize - 1); i > max((long int)0, xLowIdx); --i)
				{
#ifdef _DEBUG
					cout << "The program is accessing grid[" << i << "][" << yLowIdx << "]. " << endl;
#endif
					for (const Point& point : this->cells[i][yLowIdx])
						if (xLow <= point.x && point.x <= xHigh && yLow <= point.y && point.y <= yHigh)
							results.push_back(point.index);
				}
			if (0 <= xLowIdx && xLowIdx < this->gridSize) // the left side (from bottom to up and exclude the top cell)
				for (long int j = max((long int)0, yLowIdx); j < min(yHighIdx, this->gridSize - 1); ++j)
				{
#ifdef _DEBUG
					cout << "The program is accessing grid[" << xLowIdx << "][" << j << "]. " << endl;
#endif
					for (const Point& point : this->cells[xLowIdx][j])
						if (xLow <= point.x && point.x <= xHigh && yLow <= point.y && point.y <= yHigh)
							results.push_back(point.index);
				}
			for (long int i = xLowIdx + 1; i < xHighIdx; ++i) // all the points in the cells within but not on the four sides must be within the given range
				for (long int j = yLowIdx + 1; j < yHighIdx; ++j)
				{
#ifdef _DEBUG
					cout << "The program is accessing grid[" << i << "][" << j << "]. " << endl;
#endif
					for (const Point& point : this->cells[i][j])
						results.push_back(point.index);
				}
			const long double timeDelta = (long double)(chrono::high_resolution_clock::now() - startTime).count() / 1000000000;
			sort(results.begin(), results.end());
			cout << "spaSearchGrid: " << results.size() << " result(s), cost = " << fixed << timeDelta << " second(s)" << endl;
			for (const size_t& idx : results)
				cout << this->lines[idx] << endl;
			cout << endl;
			return results.size();
		}
		else
			return INVALID;
	}
};


#if defined WIN32 || defined _WIN32 || defined _WIN64
static bool cdCurrentDirectory()
{
	GetModuleFileNameA(NULL, path, WINDOWS_MAX_PATH);
	const string::size_type pos = string(path).find_last_of("\\/");
	if (string::npos != pos)
	{
		const string dir = string(path).substr(0, pos + 1); // to remain the path separator to avoid paths like "C:\\"
		SetCurrentDirectoryA(dir.c_str());
		GetCurrentDirectoryA(MAX_PATH, path);
#if _DEBUG
		cout << "The working directory has been changed to \"" << path << "\". " << endl;
#endif
		return true;
	}
	else
		return false;
}
#endif

static int pressAnyKeyToContinue()
{
	rewind(stdin);
	fflush(stdin);
	cout << "The program is about to exit. Please press any key to continue. " << endl;
	return getchar();
}



int main(int argc, char* argv[])
{
	long double xLow = INITIAL_VALUE, xHigh = INITIAL_VALUE, yLow = INITIAL_VALUE, yHigh = INITIAL_VALUE;
	if (5 == argc) // 51 51.20 -0.5 0
	{
		try
		{
			xLow = stold(argv[1]);
			xHigh = stold(argv[2]);
			yLow = stold(argv[3]);
			yHigh = stold(argv[4]);
		}
		catch (...)
		{
			cout << "Please provide the four boundary values in the form of ``xLow xHigh yLow yHigh`` correctly. " << endl;
			pressAnyKeyToContinue();
			return EOF;
		}
	}
	else
	{
		cout << "Please provide the locations for searching via the command line. " << endl;
		pressAnyKeyToContinue();
		return EOF;
	}
#if defined WIN32 || defined _WIN32 || defined _WIN64
	cdCurrentDirectory();
	const UINT currentConsoleOutputCP = GetConsoleOutputCP(); // backup the active code page
	if (!SetConsoleOutputCP(65001))
		cout << "Warning: As the file is encoded by UTF-8 while the program fails to change the active code page of the current console to 65001, the display may be incorrect. " << endl;
#endif
	LocationSearch locationSearch{};
	if (!(locationSearch.build("Restaurants_London_England.tsv") || locationSearch.build("../../Restaurants_London_England.tsv") || locationSearch.build("../../../Restaurants_London_England.tsv")))
	{
		cout << "Cannot read the file \"Restaurants_London_England.tsv\". " << endl;
		cout << "Please also make sure that the grid size passed is a positive integer. " << endl;
		pressAnyKeyToContinue();
		return EOF;
	}
	const size_t flagRaw = locationSearch.spaSearchRaw(xLow, xHigh, yLow, yHigh), flagGrid = locationSearch.spaSearchGrid(xLow, xHigh, yLow, yHigh);
#if defined WIN32 || defined _WIN32 || defined _WIN64
	SetConsoleOutputCP(currentConsoleOutputCP); // restore the active code page
#endif
	pressAnyKeyToContinue();
	return LocationSearch::INVALID != flagRaw && LocationSearch::INVALID != flagGrid && flagRaw > 0 && flagGrid > 0 && flagRaw == flagGrid ? EXIT_SUCCESS : EXIT_FAILURE;
}