#include "controller.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>

Stick controllerDeadzone(float x,float y,float deadzone) {
    if(!std::isfinite(x)||!std::isfinite(y))return {};
    deadzone=std::clamp(deadzone,0.f,.95f);
    const float magnitude=std::hypot(x,y);
    if(magnitude<=deadzone)return {};
    const float scale=(std::min(magnitude,1.f)-deadzone)/(1.f-deadzone)/magnitude;
    return {x*scale,y*scale};
}

PadAction controllerAction(SDL_GameControllerButton button,PadContext context) {
    if(button==SDL_CONTROLLER_BUTTON_START)return context==PadContext::Name?PadAction::Confirm:PadAction::Pause;
    if(button==SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)return PadAction::Music;
    if(button==SDL_CONTROLLER_BUTTON_BACK)return PadAction::Help;
    if(context==PadContext::Name) {
        if(button==SDL_CONTROLLER_BUTTON_A)return PadAction::Interact;
        if(button==SDL_CONTROLLER_BUTTON_B)return PadAction::Back;
        if(button==SDL_CONTROLLER_BUTTON_X)return PadAction::Erase;
        if(button==SDL_CONTROLLER_BUTTON_Y)return PadAction::ClearName;
        return PadAction::None;
    }
    if(context!=PadContext::World) {
        if(button==SDL_CONTROLLER_BUTTON_A)return PadAction::Confirm;
        if(button==SDL_CONTROLLER_BUTTON_B)return PadAction::Back;
        return PadAction::None;
    }
    switch(button) {
    case SDL_CONTROLLER_BUTTON_A:return PadAction::Interact;
    case SDL_CONTROLLER_BUTTON_B:return PadAction::ReturnBag;
    case SDL_CONTROLLER_BUTTON_X:return PadAction::Consume;
    case SDL_CONTROLLER_BUTTON_Y:return PadAction::Crate;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:return PadAction::TV;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:return PadAction::Computer;
    default:return PadAction::None;
    }
}

const char* controllerIconLabel(PadIcon button,ControllerIcons icons) {
    const bool ps=icons==ControllerIcons::PlayStation, nintendo=icons==ControllerIcons::Nintendo;
    switch(button) {
    case PadIcon::South:return ps?"CROSS":nintendo?"B":"A";
    case PadIcon::East:return ps?"CIRCLE":nintendo?"A":"B";
    case PadIcon::West:return ps?"SQUARE":nintendo?"Y":"X";
    case PadIcon::North:return ps?"TRIANGLE":nintendo?"X":"Y";
    case PadIcon::LeftShoulder:return ps?"L1":nintendo?"L":"LB";
    case PadIcon::RightShoulder:return ps?"R1":nintendo?"R":"RB";
    case PadIcon::LeftStick:return ps?"L3":"LS";
    case PadIcon::RightStick:return ps?"R3":"RS";
    case PadIcon::Dpad:return "+";
    case PadIcon::Start:return ps?"OPT":nintendo?"+":"MENU";
    case PadIcon::Back:return ps?"SHR":nintendo?"-":"VIEW";
    case PadIcon::RightTrigger:return ps?"R2":nintendo?"ZR":"RT";
    }
    return "";
}

void Controller::openFirst() {
    if(pad)return;
    for(int index=0;index<SDL_NumJoysticks();++index) {
        if(!SDL_IsGameController(index))continue;
        pad=SDL_GameControllerOpen(index);
        if(pad) {instance=SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad));break;}
    }
}

void Controller::initialize() {
    // Physical positions stay consistent even when Nintendo labels are selected.
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS,"0");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"0");
    if(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)!=0)return;
    char* base=SDL_GetBasePath();
    const auto mappings=std::filesystem::u8path(base?base:"")/"gamecontrollerdb.txt";
    SDL_free(base);
    std::error_code error;
    if(std::filesystem::exists(mappings,error))SDL_GameControllerAddMappingsFromFile(mappings.u8string().c_str());
    openFirst();
}

void Controller::shutdown() {
    if(pad)SDL_GameControllerClose(pad);
    pad=nullptr;instance=-1;lastUsed=false;triggerHeld=false;resetNavigation();
}

bool Controller::handle(const SDL_Event& event,int deadzone) {
    if(event.type==SDL_CONTROLLERDEVICEADDED) {openFirst();return false;}
    if(event.type==SDL_CONTROLLERDEVICEREMOVED&&event.cdevice.which==instance) {
        shutdown();openFirst();return true;
    }
    if(!pad||!focus)return false;
    if((event.type==SDL_CONTROLLERBUTTONDOWN||event.type==SDL_CONTROLLERBUTTONUP)&&event.cbutton.which==instance) {
        if(event.type==SDL_CONTROLLERBUTTONDOWN)lastUsed=true;
        return true;
    }
    if(event.type==SDL_CONTROLLERAXISMOTION&&event.caxis.which==instance) {
        // Ignore drift and trigger release events when selecting button prompts.
        const int threshold=event.caxis.axis>=SDL_CONTROLLER_AXIS_TRIGGERLEFT?16384:std::max(deadzone,20)*32767/100;
        if(std::abs(int(event.caxis.value))>threshold)lastUsed=true;
        return true;
    }
    return false;
}

void Controller::setFocus(bool value) {
    focus=value;triggerHeld=true;resetNavigation();
}

std::string Controller::name() const {
    const char* value=pad?SDL_GameControllerName(pad):nullptr;
    return value?value:"NENHUM CONTROLE CONECTADO";
}

ControllerIcons Controller::icons(int preference) const {
    if(preference>=1&&preference<=3)return static_cast<ControllerIcons>(preference);
    const auto type=pad?SDL_GameControllerGetType(pad):SDL_CONTROLLER_TYPE_UNKNOWN;
    if(type==SDL_CONTROLLER_TYPE_PS3||type==SDL_CONTROLLER_TYPE_PS4||type==SDL_CONTROLLER_TYPE_PS5)return ControllerIcons::PlayStation;
    if(type==SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO||type==SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT||
       type==SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT||type==SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR)return ControllerIcons::Nintendo;
    return ControllerIcons::Xbox;
}

Stick Controller::stick(bool right,int deadzone) const {
    if(!pad||!focus)return {};
    auto axis=[&](SDL_GameControllerAxis code){const auto value=SDL_GameControllerGetAxis(pad,code);return value/(value<0?32768.f:32767.f);};
    return controllerDeadzone(axis(right?SDL_CONTROLLER_AXIS_RIGHTX:SDL_CONTROLLER_AXIS_LEFTX),
                              axis(right?SDL_CONTROLLER_AXIS_RIGHTY:SDL_CONTROLLER_AXIS_LEFTY),deadzone/100.f);
}

bool Controller::held(SDL_GameControllerButton button) const {return pad&&focus&&SDL_GameControllerGetButton(pad,button);}

bool Controller::rightTriggerPressed() {
    if(!pad||!focus)return false;
    const int value=SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    if(value<8000)triggerHeld=false;
    if(value>18000&&!triggerHeld){triggerHeld=true;lastUsed=true;return true;}
    return false;
}

SDL_Keycode Controller::navigation(float dt,int deadzone) {
    const auto axes=stick(false,deadzone);
    int direction=0;
    if(held(SDL_CONTROLLER_BUTTON_DPAD_UP))direction=1;
    else if(held(SDL_CONTROLLER_BUTTON_DPAD_DOWN))direction=2;
    else if(held(SDL_CONTROLLER_BUTTON_DPAD_LEFT))direction=3;
    else if(held(SDL_CONTROLLER_BUTTON_DPAD_RIGHT))direction=4;
    else if(std::max(std::abs(axes.x),std::abs(axes.y))>.55f)
        direction=std::abs(axes.x)>std::abs(axes.y)?(axes.x<0?3:4):(axes.y<0?1:2);
    if(direction==0){previousNavigation=0;waitNeutral=false;return SDLK_UNKNOWN;}
    if(waitNeutral)return SDLK_UNKNOWN;
    const SDL_Keycode keys[]={SDLK_UNKNOWN,SDLK_UP,SDLK_DOWN,SDLK_LEFT,SDLK_RIGHT};
    if(direction!=previousNavigation){previousNavigation=direction;repeatRemaining=.35f;lastUsed=true;return keys[direction];}
    repeatRemaining-=std::max(0.f,dt);
    if(repeatRemaining<=0){repeatRemaining=.12f;return keys[direction];}
    return SDLK_UNKNOWN;
}
