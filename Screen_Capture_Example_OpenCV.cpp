#include "ScreenCapture.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <iostream>
#include <locale>
#include <string>
#include <thread>
#include <vector>
#include <opencv2/opencv.hpp>

cv::Mat convertToMat(const SL::Screen_Capture::Image &img)
{
    cv::Mat result(SL::Screen_Capture::Height(img), SL::Screen_Capture::Width(img), CV_8UC3);
    auto imgsrc = StartSrc(img);
    for (auto h = 0; h < Height(img); h++) {
        auto startimgsrc = imgsrc;
        unsigned char *dst = result.ptr<unsigned char>(h);
        for (auto w = 0; w < Width(img); w++) {
            *dst++ = imgsrc->B;
            *dst++ = imgsrc->G;
            *dst++ = imgsrc->R;
            imgsrc++;
        }
        imgsrc = SL::Screen_Capture::GotoNextRow(img, startimgsrc);
    }
    return result;
}

using namespace std::chrono_literals;
std::shared_ptr<SL::Screen_Capture::IScreenCaptureManager> framgrabber;
std::atomic<int> realcounter;
std::atomic<int> onNewFramecounter;

inline std::ostream &operator<<(std::ostream &os, const SL::Screen_Capture::ImageRect &p)
{
    return os << "left=" << p.left << " top=" << p.top << " right=" << p.right << " bottom=" << p.bottom;
}
inline std::ostream &operator<<(std::ostream &os, const SL::Screen_Capture::Monitor &p)
{
    return os << "Id=" << p.Id << " Index=" << p.Index << " Height=" << p.Height << " Width=" << p.Width << " OffsetX=" << p.OffsetX
              << " OffsetY=" << p.OffsetY << " Name=" << p.Name;
}

auto onNewFramestart = std::chrono::high_resolution_clock::now();

class CaptureInfo
{
public:
    bool isWindow;
    std::string name;
    cv::Rect ROI;

    CaptureInfo(bool isWindow, std::string name, cv::Rect ROI = cv::Rect(0,0,0,0))
        :isWindow(isWindow), name(name), ROI(ROI)
    {
    }
};


void createwindowgrabber(CaptureInfo captureInfo)
{
    realcounter = 0;
    onNewFramecounter = 0;
    framgrabber = nullptr;

    auto mouseChangedCallback = [&](const SL::Screen_Capture::Image *img, const SL::Screen_Capture::MousePoint &mousepoint) {
                // Uncomment the below code to write the image to disk for debugging
                /*
                        auto r = realcounter.fetch_add(1);
                        auto s = std::to_string(r) + std::string(" M") + std::string(".png");
                        if (img) {
                            std::cout << "New mouse coordinates  AND NEW Image received."
                                      << " x= " << mousepoint.Position.x << " y= " << mousepoint.Position.y << std::endl;
                            lodepng::encode(s, (unsigned char *)StartSrc(*img), Width(*img), Height(*img));
                        }
                        else {
                            std::cout << "New mouse coordinates received."
                                      << " x= " << mousepoint.Position.x << " y= " << mousepoint.Position.y
                                      << " The mouse image is still the same as the last " << std::endl;
                        }

                */
            };

    if(captureInfo.isWindow) {
        framgrabber =
            SL::Screen_Capture::CreateCaptureConfiguration([captureInfo]() {
                auto windows = SL::Screen_Capture::GetWindows();
                decltype(windows) filtereditems;
                for (auto &a : windows) {
                    if (captureInfo.name == a.Name) {
                        filtereditems.push_back(a);
                        std::cout << "ADDING WINDOW  Height " << a.Size.y << "  Width  " << a.Size.x << "   " << a.Name << std::endl;
                    }
                }

                return filtereditems;
            })->onNewFrame([&](const SL::Screen_Capture::Image &img, const SL::Screen_Capture::Window &window) {
                cv::Mat resultImg = convertToMat(img);
                cv::imshow("resultImg", resultImg);
                cv::waitKey(10);

                if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - onNewFramestart).count() >=
                    1000) {
                    std::cout << "onNewFrame fps" << onNewFramecounter << std::endl;
                    onNewFramecounter = 0;
                    onNewFramestart = std::chrono::high_resolution_clock::now();
                }
                onNewFramecounter += 1;
            })->onMouseChanged(mouseChangedCallback)
            ->start_capturing();
    } else {
        framgrabber =
            SL::Screen_Capture::CreateCaptureConfiguration([captureInfo]() {
                auto mons = SL::Screen_Capture::GetMonitors();
                decltype(mons) filtereditems;
                for (auto &m : mons) {
                    if (captureInfo.name == m.Name) {
                        if(captureInfo.ROI.width > 0) {
                            SL::Screen_Capture::OffsetX(m, SL::Screen_Capture::OffsetX(m) + captureInfo.ROI.x);
                            SL::Screen_Capture::OffsetY(m, SL::Screen_Capture::OffsetY(m) + captureInfo.ROI.y);
                            SL::Screen_Capture::Height(m, captureInfo.ROI.height);
                            SL::Screen_Capture::Width(m, captureInfo.ROI.width);
                        }
                        filtereditems.push_back(m);
                        std::cout << "ADDING MONITOR " << m << std::endl;
                    }
                }
                return filtereditems;
            })->onFrameChanged([&](const SL::Screen_Capture::Image &img, const SL::Screen_Capture::Monitor &monitor) {
                cv::Mat resultImg = convertToMat(img);
                cv::imshow("resultImg", resultImg);
                cv::waitKey(10);
            })->onNewFrame([&](const SL::Screen_Capture::Image &img, const SL::Screen_Capture::Monitor &monitor) {
                cv::Mat resultImg = convertToMat(img);
                cv::imshow("resultImg", resultImg);
                cv::waitKey(10);
                if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - onNewFramestart).count() >=
                    1000) {
                    std::cout << "onNewFrame fps" << onNewFramecounter << std::endl;
                    onNewFramecounter = 0;
                    onNewFramestart = std::chrono::high_resolution_clock::now();
                }
                onNewFramecounter += 1;
            })->onMouseChanged(mouseChangedCallback)
            ->start_capturing();
    }

    framgrabber->setFrameChangeInterval(std::chrono::milliseconds(30));
    framgrabber->setMouseChangeInterval(std::chrono::milliseconds(100));
}

CaptureInfo selectWindow()
{
    std::vector<CaptureInfo> list;
    auto windows = SL::Screen_Capture::GetWindows();
    for (size_t i = 0; i < windows.size(); i++) {
        std::string name = windows[i].Name;
        std::cout << list.size() << ": " << name << "\n";
        list.push_back(CaptureInfo(true, name));
    }
    auto monitors = SL::Screen_Capture::GetMonitors();
    for (auto &m : monitors) {
        std::string name = m.Name;
        std::cout << list.size() << ": " << name << "\n";
        list.push_back(CaptureInfo(false, name));
    }
    int id = -1;
    scanf("%d", &id);
    if(id >= 0 && id < (int)list.size())
        return list[id];
    return CaptureInfo(false, "");
}

int main()
{
    std::srand(std::time(nullptr));
    std::cout << "Starting Capture Demo/Test" << std::endl;

    /*std::cout << "Running display capturing for 10 seconds" << std::endl;
    createframegrabber();
    std::this_thread::sleep_for(std::chrono::seconds(10));*/

    CaptureInfo captureInfo = selectWindow();

    std::cout << "Running window capturing for 10 seconds" << std::endl;
    createwindowgrabber(captureInfo);
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "Pausing for 10 seconds. " << std::endl;
    framgrabber->pause();
    auto counti = 0;
    while (counti++ < 10) {
        assert(framgrabber->isPaused());
        std::cout << " . ";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << std::endl << "Resuming  . . . for 5 seconds" << std::endl;
    framgrabber->resume();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    framgrabber->pause();
    std::cout << "Testing destroy" << std::endl;
    framgrabber = nullptr;

    std::cout << "Testing recreating" << std::endl;
    createwindowgrabber(captureInfo);
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}
