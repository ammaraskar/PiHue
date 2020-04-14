#include <huestream/config/Config.h>
#include <huestream/HueStream.h>
#include <huestream/common/data/CuboidArea.h>
#include <huestream/effect/effects/AreaEffect.h>
#include <huestream/effect/animation/animations/ConstantAnimation.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <chrono>

using std::make_shared;
using std::cout;
using std::chrono::duration_cast;
using std::chrono::system_clock;


class PieHue {
public:
    void InitializeHueStream() {
        auto config = make_shared<huestream::Config>("PiHue", "PCHue",
            huestream::PersistenceEncryptionKey("key"));
        config->GetAppSettings()->SetUseRenderThread(false);
        config->SetStreamingMode(huestream::STREAMING_MODE_DTLS);

        _huestream = make_shared<huestream::HueStream>(config);

        //  Register feedback callback
        _huestream->RegisterFeedbackCallback([](const huestream::FeedbackMessage &message) {
            std::cout << " [" << message.GetId() << "] " << message.GetTag() << std::endl;
            if (message.GetId() == huestream::FeedbackMessage::ID_DONE_COMPLETED) {
                std::cout << "bridge-ip: " << message.GetBridge()->GetIpAddress() << std::endl;
                std::cout << "bridge-username: " << message.GetBridge()->GetUser() << std::endl;
                std::cout << "bridge-clientkey: " << message.GetBridge()->GetClientKey() << std::endl;
            }
            if (message.GetMessageType() == huestream::FeedbackMessage::FEEDBACK_TYPE_USER) {
                std::cout << "message: " << message.GetUserMessage() << std::endl;
            }
        });

        _huestream->ConnectBridge();
        this->CheckConnection();
    }

    void CheckConnection() const {
        while (!this->_huestream->IsBridgeStreaming()) {
            auto bridge = this->_huestream->GetLoadedBridge();

            std::cout << "Current bridge status: " << bridge->GetStatus() << std::endl;
            if (bridge->GetStatus() == huestream::BRIDGE_INVALID_GROUP_SELECTED) {
                std::cout << "Please enter groupid you want to select from the available groups" << std::endl;
                SelectGroup();
            }
            else {
                std::cout << "No streamable bridge configured: " << bridge->GetStatusTag() << std::endl;
                exit(1);
            }
        }
    }

    void SelectGroup() const {
        auto groups = _huestream->GetLoadedBridgeGroups();
        for (size_t i = 0; i < groups->size(); ++i) {
            auto groupId = groups->at(i)->GetId();
            auto name = groups->at(i)->GetName();
            std::cout << "[" << i << "] id:" << groupId << ", name: " << name << std::endl;
        }
        char key = getchar();

        if (isdigit(key)) {
            int i = key - '0';
            auto groupId = groups->at(i)->GetId();
            _huestream->SelectGroup(groupId);
            std::cout << "bridge-groupid: " << groupId << std::endl;
        }
    }

    void PrintLights() {
        auto lights = *_huestream->GetLoadedBridge()->GetGroupLights();
        for (const auto light : lights) {
            huestream::Location loc = light->GetPosition();
            std::cout << "Light " << light->GetModel() << " at " 
                << loc.GetX() << ", " << loc.GetY() << ", " << loc.GetZ() << std::endl;
        }
    }

    huestream::HueStreamPtr _huestream;
};

#define WIDTH 1024
#define HEIGHT 576

#define INTERMEDIATE_WIDTH 256
#define INTERMEDIATE_HEIGHT 144

#define CURRENT_TIME_MS (duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count())

#define VERTICAL_CHUNKS 3
#define HORIZONTAL_CHUNKS 3


int main(int argc, char *argv[]) {
    cv::Mat frame;
    cv::VideoCapture cap;
    int deviceID = 0;
    int apiID = cv::CAP_ANY;
    cap.open(deviceID + apiID);
    if (!cap.isOpened()) {
        std::cerr << "ERROR! Unable to open camera\n";
        return 2;
    }
    cap.set(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH, WIDTH);
    cap.set(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT, HEIGHT);
    // disable auto exposure
    cap.set(cv::VideoCaptureProperties::CAP_PROP_AUTO_EXPOSURE, 0.25);
    cap.set(cv::VideoCaptureProperties::CAP_PROP_EXPOSURE, 0.08);
    cap.set(cv::VideoCaptureProperties::CAP_PROP_SATURATION, 0.65);
    cap.read(frame);

    cv::Mat paletteInt(VERTICAL_CHUNKS, HORIZONTAL_CHUNKS, frame.type());
    cv::Mat paletteDouble(paletteInt.size(), CV_32FC3);

    PieHue pieHueApp;
    pieHueApp.InitializeHueStream();
    pieHueApp.PrintLights(); 

    std::vector<cv::Point2f> transformationFrom{
        cv::Point2f(320, 217), cv::Point2f(523, 214),
        cv::Point2f(319, 352), cv::Point2f(520, 375)
    };
    std::vector<cv::Point2f> transformationTo{
        cv::Point2f(0, 0), cv::Point2f(INTERMEDIATE_WIDTH, 0),
        cv::Point2f(0, INTERMEDIATE_HEIGHT), cv::Point2f(INTERMEDIATE_WIDTH, INTERMEDIATE_HEIGHT)
    };
    cv::Mat M = cv::getPerspectiveTransform(transformationFrom, transformationTo);

#define HORIZONTAL_SIZE ((2.0 / HORIZONTAL_CHUNKS) + 0.01)
#define VERTICAL_SIZE ((2.0 / VERTICAL_CHUNKS) + 0.01)
    // set up all the effects
    shared_ptr<huestream::AreaEffect> effects[VERTICAL_CHUNKS][HORIZONTAL_CHUNKS];

    for (int i = 0; i < VERTICAL_CHUNKS; i++) {
        for (int j = 0; j < HORIZONTAL_CHUNKS; j++) {
            // x is left and right of the TV, 3 chunks there.
            // z is up and down, so we want to get 3 vertical chunks.
            // y is forward and backward from the TV, kinda meaningless for us so
            // we span the full -1 to 1 range there.
            double xStart = -1 + (HORIZONTAL_SIZE * j);
            double zStart = 1 - (VERTICAL_SIZE * i);

            std::cout << "Setting (" << i << ", " << j << ") to the area: " <<
                "(" << xStart << ", 1, " << zStart << ") -> ("
                << (xStart + HORIZONTAL_SIZE) << ", -1, " << (zStart - VERTICAL_SIZE) << ")" << std::endl;

            auto effect = make_shared<huestream::AreaEffect>();
            effects[i][j] = effect;

            effect->SetArea(huestream::CuboidArea(
                huestream::Location(xStart, 1, zStart),
                huestream::Location(xStart + HORIZONTAL_SIZE, -1, zStart - VERTICAL_SIZE),
                "Quadrant"
            ));

            effect->Enable();
            pieHueApp._huestream->AddEffect(effect);
        }
    }

    // save the image every 3000th iteration
    int cnter = 0;

    cv::Mat warped(INTERMEDIATE_HEIGHT, INTERMEDIATE_WIDTH, frame.type());
    for (;;) {
        auto frameStart = CURRENT_TIME_MS;

        cnter++;

        cap.read(frame);
        // check if we succeeded
        if (frame.empty()) {
            std::cerr << "ERROR! blank frame grabbed\n";
            return 2;
        }

        cv::warpPerspective(frame, warped, M, warped.size());

        cv::resize(warped, paletteInt, paletteInt.size(), 0, 0, cv::InterpolationFlags::INTER_NEAREST);
        paletteInt.convertTo(paletteDouble, CV_32FC3, 1.f / 255);

        if ((cnter % 300) == 0) {
            std::cout << "Writing image..." << std::endl;
            cv::imwrite("snapshot.jpg", frame);
            cv::imwrite("warped.jpg", warped);
        }

        for (int i = 0; i < VERTICAL_CHUNKS; i++) {
            for (int j = 0; j < HORIZONTAL_CHUNKS; j++) {
                // opencv2 has vertical in axis 0 and horizontal on axis 1.
                auto intensity = paletteDouble.at<cv::Vec3f>(i, j);

                if ((cnter % 300) == 0) {
                    std::cout << "Setting <" << i << "," << j << "> to ";
                    std::cout << intensity.val[2] << ", " << intensity.val[1] << ", " << intensity.val[0] << std::endl;
                }

                effects[i][j]->SetFixedColor(huestream::Color(intensity.val[2], intensity.val[1], intensity.val[0]));
            }
        }

        pieHueApp._huestream->RenderSingleFrame();

        auto frameEnd = CURRENT_TIME_MS;
    }

    std::cout << "Exiting...\n" << std::endl;
    pieHueApp._huestream->ShutDown();

    return 0;
}