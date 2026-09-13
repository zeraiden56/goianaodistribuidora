#include "app.hpp"
#include "shop_layout.hpp"
#include "audio.hpp"
#include "computer.hpp"
#include "menu.hpp"
#include "persistence.hpp"
#include "render.hpp"
#include <SDL.h>
#include <SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <stdexcept>

namespace {
class Application {
    SDL_Window* window=nullptr;
    SDL_GLContext context=nullptr;
    Audio audio;
    Game game;
    Player player;
    Settings settings;
    Menu menu;
    ComputerUI desktop;
    std::filesystem::path saveFile,optionsFile;
    bool running=true,active=false,computer=false,smoke=false,hasSave=false;
    bool surveillance=false;
    int camera=0;
    float autosave=0;
    int frame=0;
public:
    ~Application() {
        audio.shutdown();
        if(context){destroyRenderer();SDL_GL_DeleteContext(context);}
        if(window)SDL_DestroyWindow(window);
        SDL_Quit();
    }
    void capture() {SDL_SetRelativeMouseMode(!smoke&&menu.screen==Screen::Playing&&!computer?SDL_TRUE:SDL_FALSE);}
    void open(Screen screen) {menu.open(screen);audio.moved(0);audio.pause(screen!=Screen::Playing);if(screen==Screen::Main)audio.clear();capture();}
    bool save() {
        std::string error;
        if(!saveGame(saveFile,game,player,error)){menu.notice=error;game.message=error;return false;}
        hasSave=true;autosave=0;menu.notice="PROGRESSO SALVO.";game.message="PROGRESSO SALVO.";return true;
    }
    bool persistOptions() {
        std::string error;
        if(!saveSettings(optionsFile,settings,error)) {menu.notice=error;return false;}
        return true;
    }
    void leave(bool desktop) {
        if(active&&!save()) {open(Screen::Pause);menu.notice=game.message;return;}
        if(desktop)running=false;
        else {active=false;computer=false;surveillance=false;open(Screen::Main);}
    }
    void newGame() {
        Game next;Player spawn;std::string error;
        if(!saveGame(saveFile,next,spawn,error)){menu.notice=error;return;}
        audio.clear();game=next;player=spawn;active=true;hasSave=true;computer=false;surveillance=false;autosave=0;open(Screen::Playing);
    }
    void option(int row,int direction) {
        menu.notice.clear();
        if(row==0) {
            bool next=!settings.fullscreen;
            if(SDL_SetWindowFullscreen(window,next?SDL_WINDOW_FULLSCREEN_DESKTOP:0)!=0) {
                menu.notice="NAO FOI POSSIVEL ALTERAR A TELA CHEIA.";return;
            }
            settings.fullscreen=next;
            if(!next)SDL_SetWindowSize(window,widths[settings.resolution],heights[settings.resolution]);
        } else if(row==1) {
            settings.resolution=(settings.resolution+direction+4)%4;
            if(!settings.fullscreen)SDL_SetWindowSize(window,widths[settings.resolution],heights[settings.resolution]);
        } else if(row==2)settings.quality=(settings.quality+direction+3)%3;
        else if(row==3) {
            bool next=!settings.vsync;
            if(SDL_GL_SetSwapInterval(next?1:0)!=0){menu.notice="VSYNC NAO SUPORTADO PELO DRIVER ATUAL.";return;}
            settings.vsync=next;
        } else if(row==4)settings.fov=60+((settings.fov-60+direction*5+45)%45);
        else if(row==5){settings.volume=std::clamp(settings.volume+direction*10,0,100);audio.setVolume(settings.volume);}
        persistOptions();
    }
    void activate(int row) {
        switch(menu.screen) {
        case Screen::Main:
            if(row==0) {
                if(!hasSave){menu.notice="NENHUM PROGRESSO SALVO. ESCOLHA NOVO JOGO.";break;}
                std::string error;
                if(!loadGame(saveFile,game,player,error)){menu.notice=error;break;}
                audio.clear();active=true;computer=false;surveillance=false;autosave=0;open(Screen::Playing);
            } else if(row==1) {if(hasSave||active)open(Screen::ConfirmNew);else newGame();}
            else if(row==2) {menu.back=Screen::Main;open(Screen::Options);}
            else if(row==3)leave(true);
            break;
        case Screen::Pause:
            if(row==0)open(Screen::Playing);
            else if(row==1)save();
            else if(row==2){menu.back=Screen::Pause;open(Screen::Options);}
            else if(row==3)leave(false);
            else if(row==4)leave(true);
            break;
        case Screen::Options:
            if(row==6){if(persistOptions())open(menu.back);}
            else option(row,1);
            break;
        case Screen::ConfirmNew:
            if(row==0)open(Screen::Main);else if(row==1)newGame();
            break;
        default:break;
        }
    }
    void computerAction(int index) {
        if(index<0||index>=int(computerButtons().size()))return;
        desktop.selected=index;auto action=computerButtons()[index].action;
        auto unavailable=computerUnavailable(game,action);
        if(!unavailable.empty()){game.message=unavailable;return;}
        if(computerProduct(action)>=0)game.order(computerProduct(action));
        else if(action==ComputerAction::Cameras){surveillance=true;camera=0;capture();}
        else if(action==ComputerAction::Level)game.upgradeLevel();
        else if(action==ComputerAction::Expansion){if(game.expand())audio.play(Sound::Delivery);}
        else if(action==ComputerAction::Helper){if(game.hire())audio.play(Sound::Delivery);}
        else if(action==ComputerAction::Lot)game.cycleOrderSize();
        else if(action==ComputerAction::Checkout)game.upgradeCheckout();
        else if(action==ComputerAction::Bag)game.upgradeBag();
        else if(action==ComputerAction::Storage)game.upgradeStorage();
        else if(action==ComputerAction::Speed)game.upgradeStaffSpeed();
        else if(action==ComputerAction::Close){computer=false;capture();}
    }
    void interact(bool clicked=false,bool packed=false) {
        int target=interaction(player.x,player.z,player.yaw,game.expanded,game.secondCheckout,game.largeStore,game.thirdCheckout,game.annexCheckouts);
        if((player.seated&&target==16)||(!player.seated&&(target==4||target==16))) {
            computer=true;desktop.selected=0;audio.moved(0);audio.play(Sound::Computer);
        } else if(player.seated) {
            if(clicked&&target==7){game.tvOn=!game.tvOn;audio.play(Sound::Computer,.35f);}
            else if(!clicked)toggleSeat(player,game.expanded);
        } else if(stationCheckout(target)>=0) {
            int lane=stationCheckout(target),before=game.sold;
            if(game.deliverPlayer(lane)) {
                if(game.sold>before)audio.payment();else audio.play(Sound::Pickup);
            }
        } else if(target==6){if(toggleSeat(player,game.expanded))audio.moved(0);}
        else if(target==7){game.tvOn=!game.tvOn;audio.play(Sound::Computer,.35f);}
        else {
            int product=stationProduct(target);
            bool depot=(target>=8&&target<=11)||target>=17;
            if(product>=0&&game.pickup(product,!depot,packed))audio.play(Sound::Pickup);
        }
        capture();
    }
    void key(SDL_Keycode key) {
        if(key==SDLK_F11){option(0,1);if(!menu.notice.empty())game.message=menu.notice;return;}
        if(key==SDLK_ESCAPE) {
            if(menu.screen==Screen::Playing) {computer=false;surveillance=false;open(Screen::Pause);}
            else if(menu.screen==Screen::Pause)open(Screen::Playing);
            else if(menu.screen==Screen::Options){if(persistOptions())open(menu.back);}
            else if(menu.screen==Screen::ConfirmNew)open(Screen::Main);
            return;
        }
        if(menu.screen!=Screen::Playing) {
            int count=int(menuRows(menu,settings,hasSave).size());
            if(key==SDLK_UP||key==SDLK_w)menu.selected=(menu.selected+count-1)%count;
            else if(key==SDLK_DOWN||key==SDLK_s)menu.selected=(menu.selected+1)%count;
            else if(key==SDLK_RETURN||key==SDLK_SPACE)activate(menu.selected);
            else if(menu.screen==Screen::Options&&(key==SDLK_LEFT||key==SDLK_RIGHT)&&menu.selected<6)
                option(menu.selected,key==SDLK_LEFT?-1:1);
            return;
        }
        if(surveillance) {
            if(key>=SDLK_1&&key<=SDLK_3)camera=int(key-SDLK_1);
            else if(key==SDLK_LEFT)camera=(camera+2)%3;
            else if(key==SDLK_RIGHT)camera=(camera+1)%3;
            else if(key==SDLK_c||key==SDLK_e){surveillance=false;capture();}
            return;
        }
        if(computer) {
            if(key==SDLK_UP||key==SDLK_LEFT)desktop.selected=(desktop.selected+int(computerButtons().size())-1)%int(computerButtons().size());
            else if(key==SDLK_DOWN||key==SDLK_RIGHT||key==SDLK_TAB)desktop.selected=(desktop.selected+1)%int(computerButtons().size());
            else if(key==SDLK_RETURN||key==SDLK_SPACE)computerAction(desktop.selected);
            else if(key>=SDLK_1&&key<=SDLK_4)computerAction(int(key-SDLK_1));
            else if(key==SDLK_5)computerAction(12);
            else if(key==SDLK_6)computerAction(13);
            else if(key==SDLK_c)computerAction(4);
            else if(key==SDLK_u)computerAction(5);
            else if(key==SDLK_b)computerAction(6);
            else if(key==SDLK_h)computerAction(7);
            else if(key==SDLK_e)computerAction(8);
            return;
        }
        if(key==SDLK_e)interact();
        if(key==SDLK_f)interact(false,true);
        if(key==SDLK_c&&player.seated) {
            computer=true;desktop.selected=0;audio.play(Sound::Computer);capture();
        }
        if(!computer&&key==SDLK_t&&game.expanded&&(player.seated||interaction(player.x,player.z,player.yaw,true)==7)) {
            game.tvOn=!game.tvOn;audio.play(Sound::Computer,.35f);
        }
        if(!computer&&key==SDLK_r&&game.consume())audio.play(game.consuming==1?Sound::Smoke:Sound::Drink);

    }
    void events() {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type==SDL_QUIT)leave(true);
            if(event.type==SDL_WINDOWEVENT&&event.window.event==SDL_WINDOWEVENT_FOCUS_LOST&&menu.screen==Screen::Playing&&!smoke) {
                computer=false;surveillance=false;open(Screen::Pause);
            }
            if(event.type==SDL_MOUSEMOTION) {
                if(menu.screen==Screen::Playing&&!computer) {
                    player.yaw=std::remainder(player.yaw+event.motion.xrel*.0025f,6.2831853f);
                    player.pitch=std::clamp(player.pitch+event.motion.yrel*.15f,-70.f,70.f);
                } else if(menu.screen==Screen::Playing&&computer&&!surveillance) {
                    int w,h;SDL_GetWindowSize(window,&w,&h);
                    int hit=computerHit(event.motion.x*960/std::max(w,1),event.motion.y*540/std::max(h,1));
                    if(hit>=0)desktop.selected=hit;
                } else if(menu.screen!=Screen::Playing) {
                    int w,h;SDL_GetWindowSize(window,&w,&h);
                    int hit=menuHit(event.motion.x*960/std::max(w,1),event.motion.y*540/std::max(h,1),int(menuRows(menu,settings,hasSave).size()));
                    if(hit>=0)menu.selected=hit;
                }
            }
            if(event.type==SDL_MOUSEBUTTONDOWN&&(event.button.button==SDL_BUTTON_LEFT||event.button.button==SDL_BUTTON_RIGHT)&&menu.screen==Screen::Playing&&!computer&&!surveillance) {
                interact(true,event.button.button==SDL_BUTTON_RIGHT);continue;
            }
            if(event.type==SDL_MOUSEBUTTONDOWN&&event.button.button==SDL_BUTTON_LEFT&&menu.screen==Screen::Playing&&computer&&!surveillance) {
                int w,h;SDL_GetWindowSize(window,&w,&h);
                int hit=computerHit(event.button.x*960/std::max(w,1),event.button.y*540/std::max(h,1));
                if(hit>=0)computerAction(hit);
            }
            if(event.type==SDL_MOUSEBUTTONDOWN&&event.button.button==SDL_BUTTON_LEFT&&menu.screen!=Screen::Playing) {
                int w,h;SDL_GetWindowSize(window,&w,&h);
                int hit=menuHit(event.button.x*960/std::max(w,1),event.button.y*540/std::max(h,1),int(menuRows(menu,settings,hasSave).size()));
                if(hit>=0)activate(hit);
            }
            if(event.type==SDL_KEYDOWN&&!event.key.repeat)key(event.key.keysym.sym);
        }
    }
    void update(float dt) {
        if(menu.screen!=Screen::Playing)return;
        int pendingBefore=game.pending,soldBefore=game.sold;
        game.tick(dt);autosave+=dt;
        if(game.sold>soldBefore)game.message=audio.payment()?"AUXILIAR: VENDA CONCLUIDA NO CARTAO.":"AUXILIAR: VENDA CONCLUIDA EM DINHEIRO.";
        if(pendingBefore!=-1&&game.pending==-1)audio.play(Sound::Delivery);
        if(!computer) {
            auto keys=SDL_GetKeyboardState(nullptr);
            float f=float(keys[SDL_SCANCODE_W]-keys[SDL_SCANCODE_S]),s=float(keys[SDL_SCANCODE_D]-keys[SDL_SCANCODE_A]);
            bool sprint=keys[SDL_SCANCODE_LSHIFT]||keys[SDL_SCANCODE_RSHIFT];
            audio.moved(movePlayer(player,f,s,sprint,game.intoxication,dt,game.expanded,game.largeStore));
        }
        if(autosave>=30) {autosave=0;save();}
    }
    // Exercise the same menu/input actions as a player, with an isolated save directory.
    void testClick(int logicalX,int logicalY) {
        int w,h;SDL_GetWindowSize(window,&w,&h);SDL_Event event{};
        event.type=SDL_MOUSEBUTTONDOWN;event.button.button=SDL_BUTTON_LEFT;
        event.button.x=logicalX*w/960;event.button.y=logicalY*h/540;
        if(SDL_PushEvent(&event)<0)throw std::runtime_error(SDL_GetError());
        events();
    }
    void testComputerClick(int index) {
        auto b=computerButtons()[index];testClick(b.x+b.w/2,b.y+b.h/2);
    }
    void smokeStep() {
        auto require=[](bool ok){if(!ok)throw std::runtime_error("Menu smoke test failed");};
        switch(frame) {
        case 0:require(menu.screen==Screen::Main);break;
        case 1:activate(1);if(menu.screen==Screen::ConfirmNew)activate(1);require(active);break;
        case 2:game.pickup(0);key(SDLK_r);require(game.consumed==1&&game.held==-1);break;
        case 3:key(SDLK_ESCAPE);require(menu.screen==Screen::Pause);break;
        case 4:activate(1);require(hasSave);break;
        case 5:activate(2);require(menu.screen==Screen::Options);break;
        case 6:menu.selected=2;key(SDLK_RIGHT);break;
        case 7:menu.selected=4;key(SDLK_RIGHT);break;
        case 8:key(SDLK_ESCAPE);require(menu.screen==Screen::Pause);break;
        case 9:activate(3);require(menu.screen==Screen::Main);break;
        case 10:key(SDLK_RETURN);require(menu.screen==Screen::Playing&&game.consumed==1);break;
        case 11:key(SDLK_ESCAPE);activate(2);option(2,1);break;
        case 12:key(SDLK_ESCAPE);activate(3);break;
        case 13:activate(1);require(menu.screen==Screen::ConfirmNew);break;
        case 14:key(SDLK_ESCAPE);require(menu.screen==Screen::Main);break;
        case 15:activate(0);game.tick(3);player.x=-1.5f;player.z=1.5f;player.yaw=3.14159265f;break;
        case 16:key(SDLK_ESCAPE);activate(2);option(0,1);break;
        case 17:if(settings.fullscreen)option(0,1);option(1,1);option(1,-1);break;
        case 18: {
            key(SDLK_ESCAPE);activate(0);game.intoxication=0;game.time=4.5f;
            key(SDLK_c);require(!surveillance);
            player.x=2.1f;player.z=1.8f;player.yaw=1.5707963f;key(SDLK_e);require(computer);
            game.products[0].stock=5;int balance=game.cash;key(SDLK_g);
            require(game.products[0].stock==5&&game.cash==balance);
            key(SDLK_c);require(surveillance&&camera==0);break;
        }
        case 19: {
            key(SDLK_2);require(camera==1&&game.pending==-1);
            float oldTime=game.time,x=player.x,z=player.z;update(.05f);
            require(game.time>oldTime&&player.x==x&&player.z==z);break;
        }
        case 20:key(SDLK_3);require(camera==2&&game.pending==-1);break;
        case 21:game.tick(1);break;
        case 22:
            key(SDLK_c);require(!surveillance&&computer&&game.products[0].stock==5);
            key(SDLK_e);require(!computer);player.x=0;player.z=1;player.yaw=0;player.pitch=0;
            game.intoxication=0;settings.quality=0;break;
        case 23:game.intoxication=50;break;
        case 24:game.intoxication=100;break;
        case 25:settings.quality=1;break;
        case 26: {
            game.intoxication=0;settings.quality=0;game.cash=1800;
            player.x=2.1f;player.z=1.8f;player.yaw=1.5707963f;key(SDLK_e);require(computer);
            int stock=game.products[0].stock;key(SDLK_b);key(SDLK_h);
            require(game.expanded&&game.helper.hired&&game.cash==850&&game.products[0].stock==stock);break;
        }
        case 27:key(SDLK_e);player.x=2.7f;player.z=4.7f;player.yaw=3.14159265f;break;
        case 28:player.x=2.65f;player.z=6.2f;player.yaw=1.5707963f;key(SDLK_e);require(player.seated);key(SDLK_t);require(game.tvOn);break;
        case 29:require(save());break;
        case 30:key(SDLK_ESCAPE);activate(3);activate(0);require(player.seated&&game.tvOn&&game.helper.hired&&game.expanded);break;
        case 31:key(SDLK_e);require(!player.seated&&walkable(player.x,player.z,true));player.x=0;player.z=1;player.yaw=0;break;
        case 32: {
            game.customer=true;game.wanted=0;game.quantity=1;game.patience=65;
            for(int i=0;i<1000&&game.customer;++i)game.tick(.05f);
            require(game.sold>0);break;
        }
        case 33:
            player.x=2.1f;player.z=1.8f;player.yaw=1.5707963f;key(SDLK_e);require(computer);
            game.cash=1000;game.products[0].stock=5;break;
        case 34:testComputerClick(0);require(game.pending==0&&game.cash==952);break;
        case 35:testComputerClick(1);require(game.pending==0&&game.cash==952);break;
        case 36:game.tick(12);key(SDLK_RETURN);require(game.pending==1&&game.cash==868);break;
        case 37:testClick(863,55);require(!computer);player.x=-2.f;player.z=.9f;player.yaw=3.14159265f;player.pitch=-5;break;
        case 38:player.x=.65f;player.z=-1.3f;player.yaw=0;player.pitch=30;key(SDLK_e);require(game.held==1);break;
        case 39:player.x=-2.1f;player.z=-1.6f;player.yaw=0;player.pitch=0;game.customer=true;game.customerStyle=0;break;
        case 40:game.customerStyle=1;break;
        case 41:game.customerStyle=5;break;
        case 42:
            game.cash=5000;player.x=2.1f;player.z=1.8f;player.yaw=1.5707963f;player.pitch=0;
            key(SDLK_e);require(computer);testComputerClick(10);require(game.secondCheckout);
            testComputerClick(7);require(game.secondHelper.hired);
            for(int i=0;i<4;++i)testComputerClick(11);
            require(game.bagCapacity==5);testComputerClick(9);require(game.orderSize==24);break;
        case 43:
            testComputerClick(9);require(game.orderSize==48);game.tick(12);game.products[0].stock=5;
            testComputerClick(0);require(game.pending==0&&game.pendingUnits==48);require(save());break;
        case 44:
            testClick(863,55);player.x=0;player.z=1;player.yaw=0;
            game.second.customer=true;game.second.wanted=3;game.second.quantity=5;game.second.patience=65;break;
        case 45:
            player.x=2.1f;player.z=-1.6f;player.yaw=0;
            if(game.held>=0)require(game.pickup(game.held,false));
            require(game.pickup(3));key(SDLK_e);require(!game.second.customer);require(save());break;
        case 46:
            player.x=2.1f;player.z=1.8f;player.yaw=1.5707963f;game.cash=10000;
            testClick(480,270);require(computer);testComputerClick(6);require(game.largeStore);
            testComputerClick(10);require(game.thirdCheckout);testComputerClick(7);require(game.thirdHelper.hired);
            testComputerClick(14);require(game.storageLevel==1);break;
        case 47:
            game.tick(12);testComputerClick(12);require(game.pending==4);game.tick(12);
            testComputerClick(13);require(game.pending==5);game.tick(12);testClick(863,55);break;
        case 48:
            player.x=7.f;player.z=.5f;player.yaw=3.14159265f;player.pitch=-5;break;
        case 49:
            if(game.held>=0)game.pickup(game.held,false);
            player.x=8.f;player.z=2.1f;player.yaw=3.14159265f;testClick(480,270);
            require(game.held==5&&game.heldCount==5);break;
        case 50:
            player.x=0;player.z=-1.6f;player.yaw=0;player.pitch=0;
            game.third.customer=true;game.third.wanted=5;game.third.quantity=2;game.third.delivered=0;game.third.patience=65;
            testClick(480,270);require(!game.third.customer&&game.heldCount==3);break;
        case 51:
            player.x=2.65f;player.z=6.2f;player.yaw=1.5707963f;
            testClick(480,270);require(player.seated);player.yaw=-2.295f;player.pitch=-4;break;
        case 52:
            testClick(480,270);require(computer&&player.seated);require(save());break;
        case 53:
            key(SDLK_e);require(!computer&&player.seated);key(SDLK_c);require(computer&&player.seated);break;
        case 54:
            game.cash=50000;testComputerClick(10);testComputerClick(10);require(game.checkoutCount()==5);
            testComputerClick(7);testComputerClick(7);require(game.staffCount()==5);
            for(int i=0;i<3;++i)testComputerClick(15);
            require(game.staffSpeedLevel==3);testComputerClick(14);testComputerClick(14);break;
        case 55:
            for(int i=0;i<3;++i)testComputerClick(9);
            require(game.orderSize==288);
            game.products[0].stock=0;testComputerClick(0);require(game.pendingUnits==288);game.tick(12);break;
        case 56:
            key(SDLK_e);player.seated=false;
            if(game.held>=0)game.pickup(game.held,false);
            player.x=-3.45f;player.z=2.1f;player.yaw=3.14159265f;key(SDLK_f);
            require(game.heldPacked&&game.heldCount==60);break;
        case 57:
            player.x=5.9f;player.z=-1.6f;player.yaw=0;
            game.annex[0].customer=true;game.annex[0].wholesale=true;game.annex[0].wanted=0;game.annex[0].quantity=36;game.annex[0].delivered=0;game.annex[0].patience=180;
            testClick(480,270);require(!game.annex[0].customer&&game.heldCount==24);break;
        case 58:
            player.x=8.f;player.z=-1.6f;player.yaw=0;
            game.annex[1].customer=true;game.annex[1].wholesale=true;game.annex[1].wanted=0;game.annex[1].quantity=24;game.annex[1].delivered=0;game.annex[1].patience=180;
            testClick(480,270);require(!game.annex[1].customer&&game.held==-1);require(save());break;
        case 59:
            player.x=7.f;player.z=1;player.yaw=0;player.pitch=0;
            for(auto& c:game.annex){c.customer=true;c.wholesale=true;c.wanted=4;c.quantity=24;c.delivered=0;c.patience=180;}
            break;
        case 60:key(SDLK_ESCAPE);activate(4);require(!running);break;
        }
    }
    void snapshot(int w,int h) {
        std::vector<unsigned char> pixels(static_cast<std::size_t>(w)*h*3);
        glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
        for(int y=0;y<h/2;++y)for(int x=0;x<w*3;++x)std::swap(pixels[y*w*3+x],pixels[(h-1-y)*w*3+x]);
        SDL_Surface* surface=SDL_CreateRGBSurfaceWithFormatFrom(pixels.data(),w,h,24,w*3,SDL_PIXELFORMAT_RGB24);
        if(!surface)throw std::runtime_error(SDL_GetError());
        auto filename=saveFile.parent_path()/("frame-"+std::to_string(frame)+".bmp");
        int result=SDL_SaveBMP(surface,filename.string().c_str());SDL_FreeSurface(surface);
        if(result!=0)throw std::runtime_error(SDL_GetError());
    }
    int run(bool test,const std::filesystem::path& requestedDirectory) {
        smoke=test;
        SDL_SetMainReady();
        if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0)throw std::runtime_error(SDL_GetError());
        auto directory=requestedDirectory;
        if(directory.empty()) {
            char* pref=SDL_GetPrefPath("DistribuidoraSimulator","DistribuidoraSimulator");
            if(!pref)throw std::runtime_error(SDL_GetError());
            directory=pref;SDL_free(pref);
        }
        saveFile=directory/"progress.save";optionsFile=directory/"options.cfg";
        std::error_code ec;hasSave=std::filesystem::exists(saveFile,ec);
        loadSettings(optionsFile,settings);
        if(!audio.initialize()) {
            std::cerr<<"Audio indisponivel: "<<SDL_GetError()<<'\n';
            menu.notice="AUDIO INDISPONIVEL. O JOGO CONTINUA SEM SOM.";
        }
        audio.setVolume(settings.volume);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
        window=SDL_CreateWindow("Goianão Distribuidora",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
            widths[settings.resolution],heights[settings.resolution],SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|(smoke?SDL_WINDOW_HIDDEN:0));
        if(!window)throw std::runtime_error(SDL_GetError());
        context=SDL_GL_CreateContext(window);if(!context)throw std::runtime_error(SDL_GetError());
        if(SDL_GL_SetSwapInterval(settings.vsync?1:0)!=0)settings.vsync=SDL_GL_GetSwapInterval()!=0;
        if(settings.fullscreen&&!smoke&&SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN_DESKTOP)!=0) {
            settings.fullscreen=false;menu.notice="TELA CHEIA INDISPONIVEL. USANDO JANELA.";
        }
        capture();auto last=SDL_GetTicks();
        while(running) {
            auto now=SDL_GetTicks();float dt=std::min(.05f,(now-last)/1000.f);last=now;
            events();if(smoke)smokeStep();
            update(dt);
            int w,h;SDL_GL_GetDrawableSize(window,&w,&h);w=std::max(w,1);h=std::max(h,1);
            renderWorld(game,player,settings,w,h,surveillance?camera:-1);
            if(menu.screen==Screen::Playing) {
                if(surveillance)renderSurveillanceHUD(game,camera);
                else if(computer)renderComputer(game,desktop);
                else renderHUD(game,player,false);
            }
            else renderMenu(menu,settings,hasSave);
            if(smoke&&(frame==0||frame==5||frame==15||(frame>=18&&frame<=28)||frame==30||frame==32||(frame>=33&&frame<=59)))snapshot(w,h);
            if(smoke&&glGetError()!=GL_NO_ERROR)throw std::runtime_error("OpenGL smoke test failed");
            SDL_GL_SwapWindow(window);++frame;
            if(!settings.vsync)SDL_Delay(8);
        }
        if(smoke)std::cout<<"Menu, save/load, consumption and rendering smoke tests passed\n";
        return 0;
    }
};
}
int runApp(bool smoke,const std::filesystem::path& dataDirectory) {
    try {Application app;return app.run(smoke,dataDirectory);}
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
