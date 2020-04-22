#include <huestream/HueStream.h>
#include <huestream/common/data/CuboidArea.h>
#include <huestream/effect/effects/AreaEffect.h>

#include <opencv2/imgproc.hpp>


template <unsigned int HEIGHT, unsigned int WIDTH>
class EffectProcessor {
public:
    EffectProcessor(huestream::HueStreamPtr huestream, int frameType);

    void updateEffectsFromTVImage(const cv::Mat& image, 
        unsigned long long, bool debug=false);

private:
    // We resize the TV image down to the requested resolution to create a
    // representative palette
    cv::Mat paletteAsInt;
    cv::Mat paletteAsDouble;

    std::shared_ptr<huestream::AreaEffect> effects[HEIGHT][WIDTH];
    huestream::Color areaColor[HEIGHT][WIDTH];
};

template <unsigned int HEIGHT, unsigned int WIDTH>
EffectProcessor<HEIGHT, WIDTH>::EffectProcessor(huestream::HueStreamPtr huestream, int frameType) {
    // If it isn't a multiple then we add a little bit of wiggle room to prevent not covering
    // an area due to floating point innacuracy.
    static constexpr double VERTICAL_SIZE = ((2.0 / HEIGHT) + (((HEIGHT % 2) == 0) ? 0 : 0.01));
    static constexpr double HORIZONTAL_SIZE = ((2.0 / WIDTH) + (((WIDTH % 2) == 0) ? 0 : 0.01));

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            // x is left and right of the TV, 3 chunks there.
            // z is up and down, so we want to get 3 vertical chunks.
            // y is forward and backward from the TV, kinda meaningless for us so
            // we span the full -1 to 1 range there.
            double xStart = -1 + (VERTICAL_SIZE * j);
            double zStart = 1 - (HORIZONTAL_SIZE * i);

            std::cout << "Setting (" << i << ", " << j << ") to the area: " <<
                "(" << xStart << ", 1, " << zStart << ") -> ("
                << (xStart + HORIZONTAL_SIZE) << ", -1, " << (zStart - VERTICAL_SIZE) << ")" << std::endl;

            auto effect = std::make_shared<huestream::AreaEffect>("AreaEffect");
            this->effects[i][j] = effect;
            effect->SetArea(huestream::CuboidArea(
                huestream::Location(xStart, 1, zStart),
                huestream::Location(xStart + HORIZONTAL_SIZE, -1, zStart - VERTICAL_SIZE),
                "Quadrant"
            ));
            effect->Enable();
            huestream->AddEffect(effect);
        }
    }

    paletteAsInt = cv::Mat(HEIGHT, WIDTH, frameType);
    paletteAsDouble = cv::Mat(paletteAsInt.size(), CV_32FC3);
    std::cout << "[EffectProcessor] Initialized\n";
}


#define AVERAGE_WINDOW_SIZE_MS 300.0
double movingAverage(double oldValue, double newValue, unsigned long long sampleInterval) {
    // Calculates the exponentially damped moving average based on the old value, the new value and
    // the time elapsed between their calculation.
    double scaleFactor = 1.0 / exp(sampleInterval / AVERAGE_WINDOW_SIZE_MS);
    return (oldValue * scaleFactor) + (newValue * (1 - scaleFactor));
}

template <unsigned int HEIGHT, unsigned int WIDTH>
void EffectProcessor<HEIGHT, WIDTH>::updateEffectsFromTVImage(
        const cv::Mat& image, unsigned long long samplingInterval, bool debug) {
    cv::resize(image, paletteAsInt, paletteAsInt.size(), 0, 0, cv::InterpolationFlags::INTER_NEAREST);
    paletteAsInt.convertTo(paletteAsDouble, CV_32FC3, 1.f / 255);

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            // opencv2 has vertical in axis 0 and horizontal on axis 1.
            auto intensity = paletteAsDouble.at<cv::Vec3f>(i, j);
            huestream::Color color = areaColor[i][j];

            color.SetR(movingAverage(color.GetR(), intensity[2], samplingInterval));
            color.SetG(movingAverage(color.GetG(), intensity[1], samplingInterval));
            color.SetB(movingAverage(color.GetB(), intensity[0], samplingInterval));

            if (debug) {
                std::cout << "Setting <" << i << "," << j << "> to "
                    << (color.GetR() * 255.0) << ", "
                    << (color.GetG() * 255.0) << ", "
                    << (color.GetB() * 255.0) << std::endl;
            }

            this->effects[i][j]->SetFixedColor(color);
            this->areaColor[i][j] = color;
        }
    }
}
