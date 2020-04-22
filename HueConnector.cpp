#include "HueConnector.h"


using std::make_shared;

void PieHue::InitializeHueStream() {
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

    // print out the lights locations
    auto lights = *_huestream->GetLoadedBridge()->GetGroupLights();
    for (const auto light : lights) {
        huestream::Location loc = light->GetPosition();
        std::cout << "Light " << light->GetModel() << " at "
            << loc.GetX() << ", " << loc.GetY() << ", " << loc.GetZ() << std::endl;
    }
}

void PieHue::CheckConnection() const {
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

void PieHue::SelectGroup() const {
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
