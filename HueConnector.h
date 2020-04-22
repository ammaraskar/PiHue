#include <huestream/config/Config.h>
#include <huestream/HueStream.h>

class PieHue {
private:
    void CheckConnection() const;
    void SelectGroup() const;

public:
    void InitializeHueStream();

    huestream::HueStreamPtr _huestream;
};
