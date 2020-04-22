#include "HueConnector.h"
#include "WebcamFetcher.h"
#include "EffectProcessor.h"

#include <csignal>
#include <chrono>

using std::make_shared;
using std::chrono::duration_cast;
using std::chrono::system_clock;


constexpr auto VERTICAL_CHUNKS = 3;
constexpr auto HORIZONTAL_CHUNKS = 3;


PieHue pieHueApp;

void onProgramExit(const int signum) {
    std::cout << "Exiting huestream\n";
    pieHueApp._huestream->ShutDown();
    exit(0);
}

#define CURRENT_TIME_MS (duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count())

int main(int argc, char *argv[]) {
    WebcamFetcher webcamFetcher;

    pieHueApp.InitializeHueStream();
    signal(SIGINT, onProgramExit);

    EffectProcessor<VERTICAL_CHUNKS, HORIZONTAL_CHUNKS>
        effectProcessor(pieHueApp._huestream, webcamFetcher.tvImage.type());

    // save the image every 200th iteration
    unsigned long long effectRefreshTime = CURRENT_TIME_MS;
    double totalFrameTime = 0;
    unsigned long long frameCounter = 0;
    for (;;) {
        auto frameStart = CURRENT_TIME_MS;

        webcamFetcher.fetchFrameAndTransform();

        frameCounter++;
        bool performDebugPrints = (frameCounter % 200) == 0;
        if (performDebugPrints) {
            std::cout << "Average frame time (ms): " << (totalFrameTime / 200.0) << std::endl;
            if (webcamFetcher.fourByThreeFrames > 200) {
                std::cout << "[x] 4:3 mode\n";
            }
            if (webcamFetcher.twentyOneByNineFrames > 200) {
                std::cout << "[x] 21:9 mode\n";
            }
            cv::imwrite("snapshot.jpg", webcamFetcher.rawWebcamFrame);
            cv::imwrite("warped.jpg", webcamFetcher.tvImage);

            totalFrameTime = 0;
        }

        effectProcessor.updateEffectsFromTVImage(
            webcamFetcher.tvImage, (CURRENT_TIME_MS - effectRefreshTime), performDebugPrints);
        effectRefreshTime = CURRENT_TIME_MS;

        pieHueApp._huestream->RenderSingleFrame();

        auto frameEnd = CURRENT_TIME_MS;
        totalFrameTime += (frameEnd - frameStart);
    }

    std::cout << "Exiting...\n" << std::endl;
    pieHueApp._huestream->ShutDown();

    return 0;
}
