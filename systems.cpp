#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <iostream>
#include <vector>
#include <getopt.h>
#include <set>
#include <chrono>
#include <algorithm>

void printHelp()
{
    std::cout << "--profile <number> or -p <number>:       Make request <number> times" << std::endl
              << "--url <link> or -u <link>:               Make request to <link>. <link> should be without http:// and without www. (Ex: my-worker.daniel-evans.workers.dev/links)" << std::endl
              << "--help or -h:                            Show help" << std::endl;
}

std::string getHostName(std::string url)
{
    return url.substr(0, url.find('/'));
}

std::string getPath(std::string url)
{
    if (url.find('/') != std::string::npos)
        return url.substr(url.find('/'), url.length());
    else
    {
        return "/"; // if no path is given, return the home path by default
    }
}

std::string charArrToStr(char *a, int len)
{
    std::string str = "";
    for (int index = 0; index < len; index++)
    {
        str += a[index];
    }
    return str;
}

struct responseInfo
{
    double time;
    int htmlCode;
    int bytes;
};

bool compareByTime(responseInfo &a, responseInfo &b)
{
    return a.time < b.time;
}

class responseList
{
private:
    int indexOfMinTime;
    int indexOfMaxTime;
    int indexOfMinBytes;
    int indexOfMaxBytes;
    int totalReqs = 0;
    double totalTime = 0.0;
    double meanTime;
    double medianTime;
    int numSuccesses = 0;
    std::vector<responseInfo> responses;
    std::set<int> otherCodes;
    void incrementTotalReqs();
    void incrementSuccesses();
    void updateTotalTime(double time);

public:
    void setIndexOfMinTime(int index);
    int getIndexOfMinTime();
    void setIndexOfMaxTime(int index);
    int getIndexOfMaxTime();
    void setIndexOfMinBytes(int index);
    int getIndexOfMinBytes();
    void setIndexOfMaxBytes(int index);
    int getIndexOfMaxBytes();
    int getTotalReqs();
    double getTotalTime();
    void calcMeanAndMedian();
    void addResponse(responseInfo temp);
    responseInfo getElementAt(int index);
    double getMeanTime();
    double getMedianTime();
    int getNumSuccesses();
    void printOtherCodes();
};

int main(int argc, char **argv)
{
    // code for arg parcing from https://codeyarns.com/tech/2015-01-30-how-to-parse-program-options-in-c-using-getopt_long.html
    // code for timing from https://www.pluralsight.com/blog/software-development/how-to-measure-execution-time-intervals-in-c--
    if (argc < 2)
    {
        printHelp();
        return 0;
    }
    int c;
    int profileVal = 0;
    std::string urlVal = "";
    static struct option long_options[] = {
        {"profile", required_argument, NULL, 'p'},
        {"url", required_argument, NULL, 'u'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}};
    int option_index = 0;
    while ((c = getopt_long(argc, argv, "p:u:h", long_options, &option_index)) != -1)
    {
        switch (c)
        {
        case 'p':
            profileVal = std::stoi(optarg);
            break;
        case 'u':
            urlVal = optarg;
            break;
        case 'h':
        case '?':
        default:
            printHelp();
            return -1;
            break;
        }
    }

    if (urlVal == "")
    {
        std::cout << "Please provide the url" << std::endl;
        return -1;
    }
    if (profileVal < 0)
    {
        std::cout << "Please provide a positive integer for profile value" << std::endl;
        return -1;
    }
    std::string hostName = getHostName(urlVal);
    std::string path = getPath(urlVal);
    const int BUFFER_SIZE = 16000;
    char host[2048], buffer[BUFFER_SIZE]; // host name, and the buffer that will store the request and response
    int port = 80, sock_fd;
    strncpy(host, hostName.c_str(), sizeof(host));
    sockaddr_in server_addr;
    hostent *server;
    server = gethostbyname(host);
    if (server == NULL)
    {
        std::cout << "Couldn't find host" << host << std::endl;
        return -1;
    }

    int numExecutions = profileVal == 0 ? 1 : profileVal;
    responseList res;

    for (int cuReq = 0; cuReq < numExecutions; cuReq++)
    {
        if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            std::cout << "Couldn't open socket" << std::endl;
            return -1;
        }

        bzero((char *)&server_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);

        if (connect(sock_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            std::cout << "Couldn't connect" << std::endl;
            return -1;
        }

        bzero(buffer, BUFFER_SIZE);
        std::string request = "GET ";
        request += path;
        request += " HTTP/1.1\r\nHost: ";
        request += hostName;
        request += "\r\nAccept: text/html\r\n \r\n\r\n\r\n";
        strncpy(buffer, request.c_str(), sizeof(buffer));

        std::chrono::time_point<std::chrono::high_resolution_clock> startTime = std::chrono::high_resolution_clock::now();

        if (send(sock_fd, buffer, strlen(buffer), 0) < 0)
        {
            std::cout << "Error with request" << std::endl;
            return -1;
        }

        bzero(buffer, BUFFER_SIZE);

        int numBytes;
        if ((numBytes = recv(sock_fd, buffer, BUFFER_SIZE, 0)) < 0)
        {
            std::cout << "Error reading response" << std::endl;
            return -1;
        }
        buffer[numBytes] = '\0'; // null terminate the response

        std::chrono::time_point<std::chrono::high_resolution_clock> finishTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = finishTime - startTime;

        if (profileVal == 0)
        {
            std::cout << std::endl
                      << "-----------RESPONSE-----------" << std::endl;
            std::cout << buffer << std::endl
                      << std::endl;
        }
        else
        {
            responseInfo temp;
            temp.bytes = numBytes;
            temp.time = duration.count() * 1000;
            std::string code = charArrToStr(buffer, numBytes);
            int start = code.find(' ');
            if (start == std::string::npos || code.length() < 11)
            {
                temp.htmlCode = 0; // packet fragmentation which results in loss of data
            }
            else
            {
                temp.htmlCode = stoi(code.substr(code.find(' ') + 1, 3));
            }
            res.addResponse(temp);
        }
        close(sock_fd);
    }

    if (profileVal > 0)
    {
        res.calcMeanAndMedian();
        std::cout << std::endl
                  << "-----------RESULTS-----------" << std::endl;
        std::cout << "Number of requests: " << res.getTotalReqs() << std::endl;
        std::cout << "Fastest time: " << res.getElementAt(res.getIndexOfMinTime()).time << " milliseconds" << std::endl;
        std::cout << "Slowest time: " << res.getElementAt(res.getIndexOfMaxTime()).time << " milliseconds" << std::endl;
        std::cout << "Mean time: " << res.getMeanTime() << " milliseconds" << std::endl;
        std::cout << "Median time: " << res.getMedianTime() << " milliseconds" << std::endl;
        std::cout << "Percent of requests that succeeded: " << 100.0 * res.getNumSuccesses() / res.getTotalReqs() << "%" << std::endl;
        std::cout << "Other error codes: ";
        res.printOtherCodes();
        std::cout << std::endl;
        std::cout << "Size in bytes of smallest response: " << res.getElementAt(res.getIndexOfMinBytes()).bytes << std::endl;
        std::cout << "Size in bytes of largest response: " << res.getElementAt(res.getIndexOfMaxBytes()).bytes << std::endl
                  << std::endl;
    }

    return 0;
}

void responseList::setIndexOfMinTime(int index)
{
    indexOfMinTime = index;
}
int responseList::getIndexOfMinTime()
{
    return indexOfMinTime;
}
void responseList::setIndexOfMaxTime(int index)
{
    indexOfMaxTime = index;
}
int responseList::getIndexOfMaxTime()
{
    return indexOfMaxTime;
}
void responseList::setIndexOfMinBytes(int index)
{
    indexOfMinBytes = index;
}
int responseList::getIndexOfMinBytes()
{
    return indexOfMinBytes;
}
void responseList::setIndexOfMaxBytes(int index)
{
    indexOfMaxBytes = index;
}
int responseList::getIndexOfMaxBytes()
{
    return indexOfMaxBytes;
}
void responseList::incrementTotalReqs()
{
    totalReqs++;
}
int responseList::getTotalReqs()
{
    return totalReqs;
}
void responseList::updateTotalTime(double time)
{
    totalTime += time;
}
double responseList::getTotalTime()
{
    return totalTime;
}
void responseList::calcMeanAndMedian()
{
    meanTime = getTotalTime() / (double)getTotalReqs();
    std::vector<responseInfo> tempCopy = responses;
    // algorithm for calculating median taken from https://www.geeksforgeeks.org/finding-median-of-unsorted-array-in-linear-time-using-c-stl/
    if (getTotalReqs() % 2 == 0)
    {
        std::nth_element(tempCopy.begin(), tempCopy.begin() + getTotalReqs() / 2, tempCopy.end(), compareByTime);
        std::nth_element(tempCopy.begin(), tempCopy.begin() + (getTotalReqs() - 1) / 2, tempCopy.end(), compareByTime);
        medianTime = (tempCopy.at((getTotalReqs() - 1) / 2).time + tempCopy.at(getTotalReqs() / 2).time) / 2.0;
    }
    else
    {
        std::nth_element(tempCopy.begin(), tempCopy.begin() + getTotalReqs() / 2, tempCopy.end(), compareByTime);
        medianTime = (double)tempCopy.at(getTotalReqs() / 2).time;
    }
}

void responseList::addResponse(responseInfo temp)
{
    responses.push_back(temp);
    updateTotalTime(temp.time);
    incrementTotalReqs();
    if (temp.htmlCode == 200)
    {
        incrementSuccesses();
    }
    else
    {
        if (otherCodes.find(temp.htmlCode) == otherCodes.end())
            otherCodes.insert(temp.htmlCode);
    }

    if (getTotalReqs() == 1)
    {
        setIndexOfMinTime(0);
        setIndexOfMaxTime(0);
        setIndexOfMinBytes(0);
        setIndexOfMaxBytes(0);
    }
    else
    {
        int indexOfTemp = responses.size() - 1;
        setIndexOfMinTime(temp.time < responses.at(indexOfMinTime).time ? indexOfTemp : indexOfMinTime);
        setIndexOfMaxTime(temp.time > responses.at(indexOfMaxTime).time ? indexOfTemp : indexOfMaxTime);
        setIndexOfMinBytes(temp.bytes < responses.at(indexOfMinTime).bytes ? indexOfTemp : indexOfMinBytes);
        setIndexOfMaxBytes(temp.bytes > responses.at(indexOfMaxBytes).bytes ? indexOfTemp : indexOfMaxBytes);
    }
}

responseInfo responseList::getElementAt(int index)
{
    return responses.at(index);
};

double responseList::getMeanTime()
{
    return meanTime;
}

double responseList::getMedianTime()
{
    return medianTime;
}

int responseList::getNumSuccesses()
{
    return numSuccesses;
}

void responseList::incrementSuccesses()
{
    numSuccesses++;
}

void responseList::printOtherCodes()
{
    bool exist = false;
    for (std::set<int>::iterator it = otherCodes.begin(); it != otherCodes.end(); ++it)
    {
        exist = true;
        std::cout << *it;
        if (*it != *(--otherCodes.end()))
            std::cout << ", ";
    }
    if (!exist)
    {
        std::cout << "None";
    }
}