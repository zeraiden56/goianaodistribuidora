#pragma once
#include <SDL.h>
#include <string>

enum class ControllerIcons {Automatic, Xbox, PlayStation, Nintendo};
enum class PadIcon {South,East,West,North,LeftShoulder,RightShoulder,LeftStick,RightStick,Dpad,Start,Back,RightTrigger};
enum class PadContext {World,Menu,Computer,Surveillance,Name};
enum class PadAction {None,Confirm,Back,Interact,ReturnBag,Consume,Crate,Pause,Help,Music,TV,Computer,Erase,ClearName};
struct Stick {float x=0,y=0;};
Stick controllerDeadzone(float x,float y,float deadzone);
PadAction controllerAction(SDL_GameControllerButton button,PadContext context);
const char* controllerIconLabel(PadIcon button,ControllerIcons icons);

class Controller {
    SDL_GameController* pad=nullptr;
    SDL_JoystickID instance=-1;
    bool lastUsed=false,focus=true,triggerHeld=false,waitNeutral=false;
    int previousNavigation=0;
    float repeatRemaining=0;
    void openFirst();
public:
    Controller()=default;
    Controller(const Controller&)=delete;
    Controller& operator=(const Controller&)=delete;
    ~Controller(){shutdown();}
    void initialize();
    void shutdown();
    bool handle(const SDL_Event& event,int deadzone=15);
    bool connected() const {return pad!=nullptr;}
    bool inUse() const {return connected()&&lastUsed;}
    SDL_JoystickID id() const {return instance;}
    void keyboardUsed(){lastUsed=false;}
    void setFocus(bool value);
    void resetNavigation(){previousNavigation=0;repeatRemaining=0;waitNeutral=true;}
    std::string name() const;
    ControllerIcons icons(int preference) const;
    Stick stick(bool right,int deadzone) const;
    bool held(SDL_GameControllerButton button) const;
    bool rightTriggerPressed();
    SDL_Keycode navigation(float dt,int deadzone);
};
