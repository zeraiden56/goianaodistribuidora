#include "app.hpp"

#include "shop_layout.hpp"
#include "audio.hpp"
#include "computer.hpp"
#include "world.hpp"
#include "menu.hpp"
#include "persistence.hpp"
#include "render.hpp"

#include <SDL.h>
#include <SDL_opengl.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {

constexpr int kLogicalWidth = 960;
constexpr int kLogicalHeight = 540;
constexpr int kSaveSlots = 3;
constexpr int kResolutionCount = 4;
constexpr int kQualityCount = 3;

constexpr float kAutosaveSeconds = 30.0f;
constexpr float kComputerZoomSeconds = 0.70f;
constexpr float kMaxFrameDt = 0.05f;
constexpr float kMouseYawSensitivity = 0.0025f;
constexpr float kMousePitchSensitivity = 0.15f;
constexpr float kMinPitch = -70.0f;
constexpr float kMaxPitch = 70.0f;

class Application {
    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;

    Audio audio;
    Game game;
    Player player;
    Settings settings;
    Menu menu;
    ComputerUI desktop;

    std::filesystem::path saveFile;
    std::filesystem::path optionsFile;
    std::filesystem::path dataDirectory;

    bool running = true;
    bool active = false;
    bool computer = false;
    bool smoke = false;
    bool hasSave = false;
    bool surveillance = false;

    int camera = 0;
    int terminal = 0;
    int frame = 0;
    int activeSlot = 0;

    float computerZoom = 0.0f;
    float autosave = 0.0f;

public:
    ~Application()
    {
        audio.shutdown();

        if (context) {
            // Renderer-owned OpenGL resources must be destroyed while the
            // context still exists.
            destroyRenderer();
            destroyMenuArt();
            SDL_GL_DeleteContext(context);
            context = nullptr;
        }

        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        SDL_StopTextInput();
        SDL_Quit();
    }

    std::filesystem::path slotPath(int slot) const
    {
        slot = std::clamp(slot, 0, kSaveSlots - 1);
        return dataDirectory / ("progress-" + std::to_string(slot + 1) + ".save");
    }

    bool isPlaying() const
    {
        return menu.screen == Screen::Playing;
    }

    bool worldInputEnabled() const
    {
        return isPlaying() &&
               !computer &&
               !surveillance &&
               computerZoom <= 0.0001f;
    }

    bool mouseCaptureWanted() const
    {
        return !smoke && worldInputEnabled();
    }

    int currentTarget() const
    {
        return interaction(
            player.x,
            player.z,
            player.yaw,
            game.expanded,
            game.secondCheckout,
            game.largeStore,
            game.thirdCheckout,
            game.annexCheckouts
        );
    }

    static int logicalX(int x, int width)
    {
        return x * kLogicalWidth / std::max(width, 1);
    }

    static int logicalY(int y, int height)
    {
        return y * kLogicalHeight / std::max(height, 1);
    }

    void refreshSlots()
    {
        std::error_code ec;

        for (int slot = 0; slot < kSaveSlots; ++slot) {
            ec.clear();
            menu.slots[slot] = std::filesystem::exists(slotPath(slot), ec) && !ec;
        }

        // Compatibility with saves created before slot support existed.
        ec.clear();
        if (std::filesystem::exists(dataDirectory / "progress.save", ec) && !ec)
            menu.slots[0] = true;

        hasSave = false;
        for (int slot = 0; slot < kSaveSlots; ++slot)
            hasSave = hasSave || menu.slots[slot];
    }

    void capture()
    {
        SDL_SetRelativeMouseMode(mouseCaptureWanted() ? SDL_TRUE : SDL_FALSE);
    }

    void open(Screen screen)
    {
        if (screen != Screen::Playing)
            computerZoom = 0.0f;

        menu.open(screen);

        // Reset footstep accumulation whenever gameplay control is interrupted.
        audio.moved(0.0f);
        audio.pause(screen != Screen::Playing);

        if (screen == Screen::Main)
            audio.clear();

        capture();
    }

    bool save(bool notify = true)
    {
        std::string error;

        if (!saveGame(saveFile, game, player, error)) {
            menu.notice = error;
            game.message = error;
            return false;
        }

        if (activeSlot >= 0 && activeSlot < kSaveSlots)
            menu.slots[activeSlot] = true;

        hasSave = true;
        autosave = 0.0f;

        if (notify) {
            menu.notice =
                "PROGRESSO SALVO NO SLOT " +
                std::to_string(activeSlot + 1) +
                ".";

            game.message = menu.notice;
        }

        return true;
    }

    bool persistOptions()
    {
        std::string error;

        if (!saveSettings(optionsFile, settings, error)) {
            menu.notice = error;
            return false;
        }

        return true;
    }

    void leave(bool desktopExit)
    {
        if (active && !save()) {
            computer = false;
            surveillance = false;
            open(Screen::Pause);
            menu.notice = game.message;
            return;
        }

        if (desktopExit) {
            running = false;
            return;
        }

        active = false;
        computer = false;
        surveillance = false;
        computerZoom = 0.0f;
        open(Screen::Main);
    }

    void newGame()
    {
        Game next;
        Player spawn;
        std::string error;

        next.companyName =
            menu.input.empty()
                ? "GOIANAO DISTRIBUIDORA"
                : menu.input;

        if (!saveGame(saveFile, next, spawn, error)) {
            menu.notice = error;
            return;
        }

        audio.clear();

        game = next;
        player = spawn;

        active = true;
        hasSave = true;
        computer = false;
        surveillance = false;
        computerZoom = 0.0f;
        autosave = 0.0f;

        if (activeSlot >= 0 && activeSlot < kSaveSlots)
            menu.slots[activeSlot] = true;

        open(Screen::Playing);
    }

    void sanitizeSettings()
    {
        settings.resolution = std::clamp(settings.resolution, 0, kResolutionCount - 1);
        settings.quality = std::clamp(settings.quality, 0, kQualityCount - 1);
        settings.volume = std::clamp(settings.volume, 0, 100);
        settings.fov = std::clamp(settings.fov, 60, 100);

        // Keep FOV on the same 5-degree grid used by the options menu.
        settings.fov = 60 + ((settings.fov - 60 + 2) / 5) * 5;
        settings.fov = std::clamp(settings.fov, 60, 100);
    }

    void option(int row, int direction)
    {
        menu.notice.clear();
        direction = direction < 0 ? -1 : 1;

        switch (row) {
        case 0: {
            const bool next = !settings.fullscreen;

            if (SDL_SetWindowFullscreen(
                    window,
                    next ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) != 0) {
                menu.notice = "NAO FOI POSSIVEL ALTERAR A TELA CHEIA.";
                return;
            }

            settings.fullscreen = next;

            if (!next) {
                SDL_SetWindowSize(
                    window,
                    widths[settings.resolution],
                    heights[settings.resolution]
                );
                SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
            }
            break;
        }

        case 1:
            settings.resolution =
                (settings.resolution + direction + kResolutionCount) %
                kResolutionCount;

            if (!settings.fullscreen) {
                SDL_SetWindowSize(
                    window,
                    widths[settings.resolution],
                    heights[settings.resolution]
                );
            }
            break;

        case 2:
            settings.quality =
                (settings.quality + direction + kQualityCount) %
                kQualityCount;
            break;

        case 3: {
            const bool next = !settings.vsync;

            if (SDL_GL_SetSwapInterval(next ? 1 : 0) != 0) {
                menu.notice = "VSYNC NAO SUPORTADO PELO DRIVER ATUAL.";
                return;
            }

            settings.vsync = next;
            break;
        }

        case 4: {
            int fov = settings.fov + direction * 5;
            if (fov > 100) fov = 60;
            if (fov < 60) fov = 100;
            settings.fov = fov;
            break;
        }

        case 5:
            settings.volume =
                std::clamp(settings.volume + direction * 10, 0, 100);
            audio.setVolume(settings.volume);
            break;

        default:
            return;
        }

        persistOptions();
    }

    void activate(int row)
    {
        if (row < 0)
            return;

        switch (menu.screen) {
        case Screen::Main:
            if (row == 0) {
                if (!hasSave) {
                    menu.notice = "NENHUM PROGRESSO SALVO. ESCOLHA NOVO JOGO.";
                    break;
                }

                menu.selectingNew = false;
                open(Screen::SlotSelect);
            }
            else if (row == 1) {
                menu.selectingNew = true;
                open(Screen::SlotSelect);
            }
            else if (row == 2) {
                menu.back = Screen::Main;
                open(Screen::Options);
            }
            else if (row == 3) {
                leave(true);
            }
            break;

        case Screen::Pause:
            if (row == 0)
                open(Screen::Playing);
            else if (row == 1)
                save();
            else if (row == 2) {
                menu.back = Screen::Pause;
                open(Screen::Options);
            }
            else if (row == 3)
                leave(false);
            else if (row == 4)
                leave(true);
            else if (row == 5)
                open(Screen::ConfirmPrestige);
            break;

        case Screen::Options:
            if (row == 6) {
                if (persistOptions())
                    open(menu.back);
            }
            else {
                option(row, 1);
            }
            break;

        case Screen::ConfirmNew:
            if (row == 0)
                open(Screen::Main);
            else if (row == 1) {
                menu.input = "GOIANAO DISTRIBUIDORA";
                open(Screen::CompanyName);
            }
            break;

        case Screen::CompanyName:
            if (row == 0) {
                if (menu.input.empty())
                    menu.notice = "DIGITE UM NOME PARA CONTINUAR.";
                else
                    newGame();
            }
            else if (row == 1) {
                open(Screen::Main);
            }
            break;

        case Screen::ConfirmPrestige:
            if (row == 0) {
                open(Screen::Pause);
            }
            else if (row == 1) {
                if (game.resetWithPerk()) {
                    save();
                    open(Screen::Playing);
                }
                else {
                    menu.notice = game.message;
                }
            }
            break;

        case Screen::SlotSelect:
            if (row == 3) {
                open(Screen::Main);
                break;
            }

            if (row < 0 || row >= kSaveSlots)
                break;

            activeSlot = row;
            saveFile = slotPath(activeSlot);

            if (menu.selectingNew) {
                menu.input = "GOIANAO DISTRIBUIDORA";
                open(Screen::CompanyName);
                break;
            }

            if (!menu.slots[row]) {
                menu.notice = "ESTE SLOT ESTA VAZIO.";
                break;
            }

            // Slot 1 can still point at the legacy single-save path.
            if (row == 0 && !std::filesystem::exists(saveFile))
                saveFile = dataDirectory / "progress.save";

            {
                std::string error;

                if (!loadGame(saveFile, game, player, error)) {
                    menu.notice = error;
                    break;
                }
            }

            audio.clear();
            active = true;
            computer = false;
            surveillance = false;
            computerZoom = 0.0f;
            autosave = 0.0f;
            open(Screen::Playing);
            break;

        default:
            break;
        }
    }

    void enterComputer(int location)
    {
        terminal = std::clamp(location, 0, 1);
        computer = true;
        surveillance = false;
        desktop.selected = 0;
        audio.moved(0.0f);
        audio.play(Sound::Computer);
        capture();
    }

    void selectCamera(int index)
    {
        if (index < 0)
            return;

        if (cameraAvailable(index, game.expanded, game.largeStore))
            camera = index;
    }

    void computerAction(int index)
    {
        const auto& buttons = computerButtons();

        if (index < 0 || index >= static_cast<int>(buttons.size()))
            return;

        desktop.selected = index;
        const auto action = buttons[static_cast<std::size_t>(index)].action;

        const auto unavailable = computerUnavailable(game, action);
        if (!unavailable.empty()) {
            game.message = unavailable;
            return;
        }

        const auto deliveriesBefore = game.deliveriesReceived;

        const int product = computerProduct(action);

        if (product >= 0)
            game.order(product);
        else if (action == ComputerAction::Cameras) {
            surveillance = true;
            camera = 0;
            capture();
        }
        else if (action == ComputerAction::Level)
            game.upgradeLevel();
        else if (action == ComputerAction::Expansion) {
            if (game.expand())
                audio.play(Sound::Delivery);
        }
        else if (action == ComputerAction::Helper) {
            if (game.hire())
                audio.play(Sound::Delivery);
        }
        else if (action == ComputerAction::Lot)
            game.cycleOrderSize();
        else if (action == ComputerAction::Checkout)
            game.upgradeCheckout();
        else if (action == ComputerAction::Bag)
            game.upgradeBag();
        else if (action == ComputerAction::Storage)
            game.upgradeStorage();
        else if (action == ComputerAction::Speed)
            game.upgradeStaffSpeed();
        else if (action == ComputerAction::AutoRestock)
            game.toggleAutoRestock();
        else if (action == ComputerAction::RestockThreshold)
            game.cycleRestockThreshold();
        else if (action == ComputerAction::RestockReserve)
            game.cycleRestockReserve();
        else if (action == ComputerAction::Delivery)
            game.upgradeDelivery();
        else if (action == ComputerAction::Close) {
            computer = false;
            surveillance = false;
            capture();
        }

        if (game.deliveriesReceived != deliveriesBefore)
            audio.play(Sound::Delivery);
    }

    void interact(bool clicked = false, bool packed = false)
    {
        const int target = currentTarget();

        if ((player.seated && target == 16) ||
            (!player.seated && (target == 4 || target == 16))) {
            enterComputer(target == 16 ? 1 : 0);
        }
        else if (player.seated) {
            if (clicked && target == 7) {
                game.tvOn = !game.tvOn;
                audio.play(Sound::Computer, 0.35f);
            }
            else if (!clicked) {
                toggleSeat(player, game.expanded);
            }
        }
        else if (stationCheckout(target) >= 0) {
            const int lane = stationCheckout(target);
            const int soldBefore = game.sold;

            if (game.deliverPlayer(lane)) {
                if (game.sold > soldBefore)
                    audio.payment();
                else
                    audio.play(Sound::Pickup);
            }
        }
        else if (target == 6) {
            if (toggleSeat(player, game.expanded))
                audio.moved(0.0f);
        }
        else if (target == 7) {
            game.tvOn = !game.tvOn;
            audio.play(Sound::Computer, 0.35f);
        }
        else {
            const int product = stationProduct(target);
            const bool depot =
                (target >= 8 && target <= 11) ||
                target >= 17;

            if (product >= 0 && game.pickup(product, !depot, packed))
                audio.play(Sound::Pickup);
        }

        capture();
    }

    void key(SDL_Keycode keycode)
    {
        // ---------------------------------------------------------------------
        // Global shortcuts
        // ---------------------------------------------------------------------

        if (keycode == SDLK_F11) {
            option(0, 1);
            if (!menu.notice.empty())
                game.message = menu.notice;
            return;
        }

        if (keycode == SDLK_ESCAPE) {
            if (menu.screen == Screen::Playing) {
                computer = false;
                surveillance = false;
                computerZoom = 0.0f;
                open(Screen::Pause);
            }
            else if (menu.screen == Screen::Pause) {
                open(Screen::Playing);
            }
            else if (menu.screen == Screen::Options) {
                if (persistOptions())
                    open(menu.back);
            }
            else if (menu.screen == Screen::ConfirmNew ||
                     menu.screen == Screen::CompanyName ||
                     menu.screen == Screen::SlotSelect) {
                open(Screen::Main);
            }
            else if (menu.screen == Screen::ConfirmPrestige) {
                open(Screen::Pause);
            }

            return;
        }

        // ---------------------------------------------------------------------
        // Company name input
        // ---------------------------------------------------------------------

        if (menu.screen == Screen::CompanyName) {
            if (keycode == SDLK_BACKSPACE && !menu.input.empty())
                menu.input.pop_back();
            else if (keycode == SDLK_RETURN || keycode == SDLK_KP_ENTER)
                activate(0);

            return;
        }

        // ---------------------------------------------------------------------
        // Menus
        // ---------------------------------------------------------------------

        if (menu.screen != Screen::Playing) {
            const int count =
                static_cast<int>(menuRows(menu, settings, hasSave).size());

            if (count <= 0)
                return;

            if (keycode == SDLK_UP || keycode == SDLK_w)
                menu.selected = (menu.selected + count - 1) % count;
            else if (keycode == SDLK_DOWN || keycode == SDLK_s)
                menu.selected = (menu.selected + 1) % count;
            else if (keycode == SDLK_RETURN || keycode == SDLK_SPACE)
                activate(menu.selected);
            else if (menu.screen == Screen::Options &&
                     (keycode == SDLK_LEFT || keycode == SDLK_RIGHT) &&
                     menu.selected < 6) {
                option(menu.selected, keycode == SDLK_LEFT ? -1 : 1);
            }

            return;
        }

        // ---------------------------------------------------------------------
        // Surveillance
        // ---------------------------------------------------------------------

        if (surveillance) {
            if (keycode >= SDLK_1 && keycode <= SDLK_6)
                selectCamera(static_cast<int>(keycode - SDLK_1));
            else if (keycode == SDLK_LEFT)
                camera = nextSecurityCamera(camera, -1, game.expanded, game.largeStore);
            else if (keycode == SDLK_RIGHT)
                camera = nextSecurityCamera(camera, 1, game.expanded, game.largeStore);
            else if (keycode == SDLK_c || keycode == SDLK_e) {
                surveillance = false;
                capture();
            }

            return;
        }

        // ---------------------------------------------------------------------
        // Computer
        // ---------------------------------------------------------------------

        if (computer) {
            const int buttonCount = static_cast<int>(computerButtons().size());
            if (buttonCount <= 0)
                return;

            if (keycode == SDLK_UP || keycode == SDLK_LEFT)
                desktop.selected = (desktop.selected + buttonCount - 1) % buttonCount;
            else if (keycode == SDLK_DOWN || keycode == SDLK_RIGHT || keycode == SDLK_TAB)
                desktop.selected = (desktop.selected + 1) % buttonCount;
            else if (keycode == SDLK_RETURN || keycode == SDLK_SPACE)
                computerAction(desktop.selected);
            else if (keycode >= SDLK_1 && keycode <= SDLK_4)
                computerAction(static_cast<int>(keycode - SDLK_1));
            else if (keycode == SDLK_5)
                computerAction(12);
            else if (keycode == SDLK_6)
                computerAction(13);
            else if (keycode == SDLK_c)
                computerAction(4);
            else if (keycode == SDLK_u)
                computerAction(5);
            else if (keycode == SDLK_b)
                computerAction(6);
            else if (keycode == SDLK_h)
                computerAction(7);
            else if (keycode == SDLK_e)
                computerAction(8);

            return;
        }

        // The computer has been closed but the camera may still be animating
        // back to the player's view. Do not allow world actions during that
        // transition.
        if (computerZoom > 0.0001f)
            return;

        // ---------------------------------------------------------------------
        // World interactions
        // ---------------------------------------------------------------------

        if (keycode == SDLK_e)
            interact();

        if (keycode == SDLK_f)
            interact(false, true);

        if (keycode == SDLK_q) {
            const int target = currentTarget();
            const bool depot =
                (target >= 8 && target <= 11) ||
                target >= 17;

            if (game.held >= 0 &&
                stationProduct(target) == game.held &&
                game.returnHeld(!depot)) {
                audio.play(Sound::Pickup);
            }
        }

        if (keycode == SDLK_c && player.seated)
            enterComputer(1);

        if (keycode == SDLK_t && game.expanded &&
            (player.seated || currentTarget() == 7)) {
            game.tvOn = !game.tvOn;
            audio.play(Sound::Computer, 0.35f);
        }

        if (keycode == SDLK_r && game.consume())
            audio.play(game.consuming == 1 ? Sound::Smoke : Sound::Drink);
    }

    void events()
    {
        SDL_Event event{};

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                leave(true);
                continue;
            }

            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_FOCUS_LOST &&
                isPlaying() &&
                !smoke) {
                computer = false;
                surveillance = false;
                computerZoom = 0.0f;
                open(Screen::Pause);
                continue;
            }

            if (event.type == SDL_MOUSEMOTION) {
                if (worldInputEnabled()) {
                    player.yaw = std::remainder(
                        player.yaw +
                            static_cast<float>(event.motion.xrel) *
                            kMouseYawSensitivity,
                        2.0f * 3.14159265358979323846f
                    );

                    player.pitch = std::clamp(
                        player.pitch +
                            static_cast<float>(event.motion.yrel) *
                            kMousePitchSensitivity,
                        kMinPitch,
                        kMaxPitch
                    );
                }
                else if (isPlaying() &&
                         computer &&
                         !surveillance &&
                         computerZoom >= 0.9999f) {
                    int w = 1;
                    int h = 1;
                    SDL_GetWindowSize(window, &w, &h);

                    const int hit = computerHit(
                        logicalX(event.motion.x, w),
                        logicalY(event.motion.y, h)
                    );

                    if (hit >= 0)
                        desktop.selected = hit;
                }
                else if (!isPlaying()) {
                    int w = 1;
                    int h = 1;
                    SDL_GetWindowSize(window, &w, &h);

                    const int count =
                        static_cast<int>(menuRows(menu, settings, hasSave).size());

                    const int hit = menuHit(
                        logicalX(event.motion.x, w),
                        logicalY(event.motion.y, h),
                        count
                    );

                    if (hit >= 0)
                        menu.selected = hit;
                }
            }

            if (event.type == SDL_TEXTINPUT &&
                menu.screen == Screen::CompanyName) {
                for (char c : std::string(event.text.text)) {
                    const unsigned char uc = static_cast<unsigned char>(c);

                    if (uc >= 32 && uc <= 126 && menu.input.size() < 24)
                        menu.input += static_cast<char>(std::toupper(uc));
                }
            }

            if (event.type == SDL_MOUSEBUTTONDOWN &&
                (event.button.button == SDL_BUTTON_LEFT ||
                 event.button.button == SDL_BUTTON_RIGHT) &&
                worldInputEnabled()) {
                interact(
                    true,
                    event.button.button == SDL_BUTTON_RIGHT
                );
                continue;
            }

            if (event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT &&
                isPlaying() &&
                computer &&
                !surveillance &&
                computerZoom >= 0.9999f) {
                int w = 1;
                int h = 1;
                SDL_GetWindowSize(window, &w, &h);

                const int hit = computerHit(
                    logicalX(event.button.x, w),
                    logicalY(event.button.y, h)
                );

                if (hit >= 0)
                    computerAction(hit);

                continue;
            }

            if (event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT &&
                isPlaying() &&
                surveillance) {
                int w = 1;
                int h = 1;
                SDL_GetWindowSize(window, &w, &h);

                selectCamera(
                    surveillanceHit(
                        logicalX(event.button.x, w),
                        logicalY(event.button.y, h)
                    )
                );

                continue;
            }

            if (event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.button == SDL_BUTTON_LEFT &&
                !isPlaying()) {
                int w = 1;
                int h = 1;
                SDL_GetWindowSize(window, &w, &h);

                const int count =
                    static_cast<int>(menuRows(menu, settings, hasSave).size());

                const int hit = menuHit(
                    logicalX(event.button.x, w),
                    logicalY(event.button.y, h),
                    count
                );

                if (hit >= 0)
                    activate(hit);

                continue;
            }

            if (event.type == SDL_KEYDOWN && !event.key.repeat)
                key(event.key.keysym.sym);
        }
    }

    void update(float dt)
    {
        if (!isPlaying())
            return;

        if (!std::isfinite(dt))
            dt = 0.0f;

        dt = std::max(dt, 0.0f);

        const float previousZoom = computerZoom;

        const float zoomDirection = computer ? 1.0f : -1.0f;
        const float zoomStep =
            kComputerZoomSeconds > 0.0f
                ? dt / kComputerZoomSeconds
                : 1.0f;

        computerZoom = std::clamp(
            computerZoom + zoomDirection * zoomStep,
            0.0f,
            1.0f
        );

        if (previousZoom > 0.0f && computerZoom <= 0.0f)
            capture();

        const int soldBefore = game.sold;
        const auto deliveriesBefore = game.deliveriesReceived;

        game.tick(dt);
        autosave += dt;

        // Player-driven checkout sales already play their payment sound in
        // interact(). This detects sales made by helpers during game.tick().
        if (game.sold > soldBefore) {
            game.message =
                audio.payment()
                    ? "AUXILIAR: VENDA CONCLUIDA NO CARTAO."
                    : "AUXILIAR: VENDA CONCLUIDA EM DINHEIRO.";
        }

        if (game.deliveriesReceived != deliveriesBefore)
            audio.play(Sound::Delivery);

        if (worldInputEnabled()) {
            const Uint8* keys = SDL_GetKeyboardState(nullptr);

            const float forward =
                static_cast<float>(keys[SDL_SCANCODE_W]) -
                static_cast<float>(keys[SDL_SCANCODE_S]);

            const float strafe =
                static_cast<float>(keys[SDL_SCANCODE_D]) -
                static_cast<float>(keys[SDL_SCANCODE_A]);

            const bool sprint =
                keys[SDL_SCANCODE_LSHIFT] ||
                keys[SDL_SCANCODE_RSHIFT];

            audio.moved(
                movePlayer(
                    player,
                    forward,
                    strafe,
                    sprint,
                    game.intoxication,
                    dt,
                    game.expanded,
                    game.largeStore
                )
            );
        }
        else {
            // Avoid carrying half a footstep across menus/computer transitions.
            audio.moved(0.0f);
        }

        if (autosave >= kAutosaveSeconds) {
            // Automatic saves should not replace an important gameplay message
            // with "PROGRESSO SALVO..." every 30 seconds.
            autosave = 0.0f;
            save(false);
        }
    }

    // Exercise the same menu/input actions as a player, with an isolated save directory.
void testClick(int logicalX,int logicalY) {

        // These existing UI checks operate after the transition; dedicated frames test the zoom itself.

        if(frame<72)computerZoom=computer?1.f:0.f;

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

auto require=[&](bool ok){if(!ok)throw std::runtime_error("Menu smoke test failed at frame "+std::to_string(frame));};

        switch(frame) {

        case 0:require(menu.screen==Screen::Main);break;

        case 1:activate(1);if(menu.screen==Screen::SlotSelect)activate(0);if(menu.screen==Screen::ConfirmNew)activate(1);if(menu.screen==Screen::CompanyName)activate(0);require(active);break;

        case 2:game.pickup(0);key(SDLK_r);require(game.consumed==1&&game.held==-1);break;

        case 3:key(SDLK_ESCAPE);require(menu.screen==Screen::Pause);break;

        case 4:activate(1);require(hasSave);break;

        case 5:activate(2);require(menu.screen==Screen::Options);break;

        case 6:menu.selected=2;key(SDLK_RIGHT);break;

        case 7:menu.selected=4;key(SDLK_RIGHT);break;

        case 8:key(SDLK_ESCAPE);require(menu.screen==Screen::Pause);break;

        case 9:activate(3);require(menu.screen==Screen::Main);break;

        case 10:key(SDLK_RETURN);if(menu.screen==Screen::SlotSelect)activate(0);require(menu.screen==Screen::Playing&&game.consumed==1);break;

        case 11:key(SDLK_ESCAPE);activate(2);option(2,1);break;

        case 12:key(SDLK_ESCAPE);activate(3);break;

        case 13:activate(1);require(menu.screen==Screen::SlotSelect);break;

        case 14:key(SDLK_ESCAPE);require(menu.screen==Screen::Main);break;

        case 15:activate(0);if(menu.screen==Screen::SlotSelect)activate(0);game.tick(3);player.x=-1.5f;player.z=1.5f;player.yaw=3.14159265f;break;

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

        case 30:key(SDLK_ESCAPE);activate(3);activate(0);if(menu.screen==Screen::SlotSelect)activate(0);require(player.seated&&game.tvOn&&game.helper.hired&&game.expanded);break;

        case 31:key(SDLK_e);require(!player.seated&&walkable(player.x,player.z,true));player.x=0;player.z=1;player.yaw=0;break;

        case 32: {

            game.customer=true;game.mixedOrders[0]={};game.customerMotion[0]={};game.wanted=0;game.quantity=1;game.patience=65;

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

        case 39:player.x=-2.1f;player.z=-1.6f;player.yaw=0;player.pitch=0;game.customer=true;game.mixedOrders[0]={};game.customerMotion[0]={};game.customerStyle=0;break;

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

            game.second.customer=true;game.mixedOrders[1]={};game.customerMotion[1]={};game.second.wanted=3;game.second.quantity=5;game.second.patience=65;break;

        case 45:

            player.x=2.1f;player.z=-1.6f;player.yaw=0;

            if(game.held>=0)require(game.returnHeld(false));

            for(int i=0;i<5;++i){require(game.pickup(3));}

            key(SDLK_e);require(!game.second.customer);require(save());break;

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

            if(game.held>=0)game.returnHeld(false);

            player.x=8.f;player.z=2.1f;player.yaw=3.14159265f;for(int i=0;i<5;++i)testClick(480,270);

            require(game.held==5&&game.heldCount==5);break;

        case 50:

            player.x=0;player.z=-1.6f;player.yaw=0;player.pitch=0;

            game.third.customer=true;game.mixedOrders[2]={};game.customerMotion[2]={};game.third.wanted=5;game.third.quantity=2;game.third.delivered=0;game.third.patience=65;

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

            if(game.held>=0)game.returnHeld(false);

            player.x=-3.45f;player.z=2.1f;player.yaw=3.14159265f;for(int i=0;i<5;++i)key(SDLK_f);

            require(game.heldPacked&&game.heldCount==60);break;

        case 57:

            player.x=5.9f;player.z=-1.6f;player.yaw=0;

            game.annex[0].customer=true;game.mixedOrders[3]={};game.customerMotion[3]={};game.annex[0].wholesale=true;game.annex[0].wanted=0;game.annex[0].quantity=36;game.annex[0].delivered=0;game.annex[0].patience=180;

            testClick(480,270);require(!game.annex[0].customer&&game.heldCount==24);break;

        case 58:

            player.x=8.f;player.z=-1.6f;player.yaw=0;

            game.annex[1].customer=true;game.mixedOrders[4]={};game.customerMotion[4]={};game.annex[1].wholesale=true;game.annex[1].wanted=0;game.annex[1].quantity=24;game.annex[1].delivered=0;game.annex[1].patience=180;

            testClick(480,270);require(!game.annex[1].customer&&game.held==-1);require(save());break;

        case 59:

            player.x=7.f;player.z=1;player.yaw=0;player.pitch=0;

            for(auto& c:game.annex){c.customer=true;c.wholesale=true;c.wanted=4;c.quantity=24;c.delivered=0;c.patience=180;}

            break;

        case 60:

            player.x=2.1f;player.z=1.8f;player.yaw=1.5707963f;key(SDLK_e);require(computer);

            game.cash=50000;

            for(int i=0;i<Game::productCount;++i)game.products[i].stock=game.products[i].capacity-(game.occupied(i)-game.products[i].stock);

            game.products[0].stock=0;

            testComputerClick(19);testComputerClick(19);require(game.deliverySeconds()==3);

            testComputerClick(0);require(game.pending==0&&game.delivery<=3);

            testComputerClick(19);require(game.deliverySeconds()==0&&game.pending==-1&&game.products[0].stock==288);break;

        case 61:

            testComputerClick(17);testComputerClick(18);testComputerClick(16);

            require(game.restockThreshold==50&&game.restockReserve==250&&game.autoRestockEnabled);

            game.products[5].stock=0;game.tick(.01f);require(game.products[5].stock==288&&game.pending==-1);break;

        case 62:

            key(SDLK_e);require(!computer);game.products[4].stock=0;game.tick(1.1f);

            require(game.products[4].stock==288);require(save());break;

        case 63: {

            key(SDLK_ESCAPE);game.products[0].stock=0;game.restockTimer=0;int before=game.cash;

            update(2);require(game.cash==before&&game.products[0].stock==0);

            key(SDLK_ESCAPE);update(.1f);require(game.products[0].stock>=288);break;

        }

        case 64: {

            key(SDLK_e);require(computer);testComputerClick(16);require(!game.autoRestockEnabled);

            game.products[4].stock=0;int before=game.cash;game.tickRestock(2);

            require(game.products[4].stock==0&&game.cash==before);require(save());break;

        }

        case 65:

            game=Game{};game.cash=10000;game.expand();game.upgradeCheckout();game.hire();game.hire();

            game.customer=true;game.wanted=2;game.quantity=2;game.mixedOrders[0].requested[1]=1;

            game.second.customer=true;game.second.wanted=0;game.second.quantity=2;game.mixedOrders[1].requested[3]=1;

            game.customerMotion[1].approach=.6f;game.messageTime=0;game.time=30;

            player=Player{};player.x=-.4f;player.z=.3f;player.pitch=0;

            computer=false;surveillance=false;open(Screen::Playing);break;

        case 66:game.time=105;break;

        case 67:for(int i=0;i<45;++i)game.tick(.05f);break;

        case 68:open(Screen::Main);break;

        case 69:case 70:case 71:break;

        case 72:

            game=Game{};game.cash=30000;game.expand();game.expand();game.time=105;

            player=Player{};player.x=3.6f;player.z=.65f;player.yaw=3.14159265f;

            computer=false;surveillance=false;computerZoom=0;terminal=0;open(Screen::Playing);break;

        case 73:

            key(SDLK_e);require(computer&&terminal==0&&computerZoom==0);update(.21f);

            require(computerZoom>0&&computerZoom<1&&player.x==3.6f&&player.z==.65f);break;

        case 74:update(.2f);require(computerZoom>0&&computerZoom<1);break;

        case 75:update(1);require(computerZoom==1&&computer&&player.z==.65f);break;

        case 76:

            key(SDLK_e);update(.28f);require(!computer&&computerZoom>0&&computerZoom<1&&player.z==.65f);break;

        case 77:

            key(SDLK_ESCAPE);require(menu.screen==Screen::Pause&&computerZoom==0);key(SDLK_ESCAPE);

            require(player.x==3.6f&&player.z==.65f);break;

        case 78:

            key(SDLK_e);update(1);key(SDLK_c);require(surveillance);game.time=30;break;

        case 79:key(SDLK_2);require(camera==1);break;

        case 80:key(SDLK_3);game.time=105;require(camera==2);break;

        case 81:key(SDLK_4);require(camera==3);break;

        case 82:key(SDLK_5);require(camera==4);break;

        case 83:key(SDLK_6);require(camera==5);break;

        case 84:game.time=30;break;

        case 85:

            key(SDLK_RIGHT);require(camera==0);testClick(18+5*155+30,478);require(camera==5);

            key(SDLK_LEFT);require(camera==4);key(SDLK_c);require(computer&&!surveillance);break;

        case 86:

            key(SDLK_e);update(1);require(!computer&&computerZoom==0&&player.z==.65f);

            player.x=2.65f;player.z=6.2f;require(toggleSeat(player,true));key(SDLK_c);update(.25f);

            require(computer&&terminal==1&&computerZoom>0&&computerZoom<1&&player.seated);break;

        case 87:update(1);require(computerZoom==1&&player.seated);key(SDLK_e);update(.2f);break;

        case 88:

            key(SDLK_ESCAPE);require(computerZoom==0&&!computer&&player.seated&&save());activate(4);require(!running);break;

        }

    }


    void snapshot(int w, int h)
    {
        w = std::max(w, 1);
        h = std::max(h, 1);

        const std::size_t pixelCount =
            static_cast<std::size_t>(w) *
            static_cast<std::size_t>(h) *
            3u;

        std::vector<unsigned char> pixels(pixelCount);

        GLint oldPackAlignment = 4;
        glGetIntegerv(GL_PACK_ALIGNMENT, &oldPackAlignment);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);

        glReadPixels(
            0,
            0,
            w,
            h,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            pixels.data()
        );

        glPixelStorei(GL_PACK_ALIGNMENT, oldPackAlignment);

        const std::size_t rowBytes =
            static_cast<std::size_t>(w) * 3u;

        for (int y = 0; y < h / 2; ++y) {
            const std::size_t top =
                static_cast<std::size_t>(y) * rowBytes;

            const std::size_t bottom =
                static_cast<std::size_t>(h - 1 - y) * rowBytes;

            for (std::size_t x = 0; x < rowBytes; ++x)
                std::swap(pixels[top + x], pixels[bottom + x]);
        }

        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
            pixels.data(),
            w,
            h,
            24,
            w * 3,
            SDL_PIXELFORMAT_RGB24
        );

        if (!surface)
            throw std::runtime_error(SDL_GetError());

        const auto filename =
            saveFile.parent_path() /
            ("frame-" + std::to_string(frame) + ".bmp");

        const int result = SDL_SaveBMP(
            surface,
            filename.string().c_str()
        );

        SDL_FreeSurface(surface);

        if (result != 0)
            throw std::runtime_error(SDL_GetError());
    }

    int run(bool test, const std::filesystem::path& requestedDirectory)
    {
        smoke = test;

        SDL_SetMainReady();

        // Audio remains optional, so initialize only the subsystems that are
        // mandatory for the application here. Audio::initialize() handles its
        // own SDL audio subsystem and failure path.
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
            throw std::runtime_error(SDL_GetError());

        auto directory = requestedDirectory;

        if (directory.empty()) {
            char* pref = SDL_GetPrefPath(
                "DistribuidoraSimulator",
                "DistribuidoraSimulator"
            );

            if (!pref)
                throw std::runtime_error(SDL_GetError());

            directory = pref;
            SDL_free(pref);
        }

        {
            std::error_code ec;
            std::filesystem::create_directories(directory, ec);

            if (ec)
                throw std::runtime_error(
                    "Nao foi possivel criar o diretorio de dados: " +
                    ec.message()
                );
        }

        dataDirectory = directory;
        saveFile = slotPath(0);
        optionsFile = directory / "options.cfg";

        refreshSlots();

        loadSettings(optionsFile, settings);
        sanitizeSettings();

        // ---------------------------------------------------------------------
        // OpenGL attributes must be selected before window/context creation.
        // ---------------------------------------------------------------------

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

        Uint32 windowFlags =
            SDL_WINDOW_OPENGL |
            SDL_WINDOW_RESIZABLE |
            SDL_WINDOW_ALLOW_HIGHDPI;

        if (smoke)
            windowFlags |= SDL_WINDOW_HIDDEN;

        window = SDL_CreateWindow(
            "Goianão Distribuidora",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            widths[settings.resolution],
            heights[settings.resolution],
            windowFlags
        );

        if (!window)
            throw std::runtime_error(SDL_GetError());

        context = SDL_GL_CreateContext(window);

        if (!context)
            throw std::runtime_error(SDL_GetError());

        if (SDL_GL_MakeCurrent(window, context) != 0)
            throw std::runtime_error(SDL_GetError());

        if (SDL_GL_SetSwapInterval(settings.vsync ? 1 : 0) != 0)
            settings.vsync = SDL_GL_GetSwapInterval() != 0;

        if (settings.fullscreen && !smoke) {
            if (SDL_SetWindowFullscreen(
                    window,
                    SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
                settings.fullscreen = false;
                menu.notice = "TELA CHEIA INDISPONIVEL. USANDO JANELA.";
            }
        }

        SDL_StartTextInput();

        // Audio failure is intentionally non-fatal.
        if (!audio.initialize()) {
            std::cerr
                << "Audio indisponivel: "
                << SDL_GetError()
                << '\n';

            menu.notice =
                "AUDIO INDISPONIVEL. O JOGO CONTINUA SEM SOM.";
        }

        audio.setVolume(settings.volume);

        capture();

        Uint64 lastCounter = SDL_GetPerformanceCounter();
        const Uint64 frequency = SDL_GetPerformanceFrequency();

        while (running) {
            const Uint64 nowCounter = SDL_GetPerformanceCounter();

            float dt = 0.0f;
            if (frequency != 0) {
                dt = static_cast<float>(
                    static_cast<double>(nowCounter - lastCounter) /
                    static_cast<double>(frequency)
                );
            }

            lastCounter = nowCounter;
            dt = std::clamp(dt, 0.0f, kMaxFrameDt);

            events();

            if (smoke)
                smokeStep();

            update(dt);

            int w = 1;
            int h = 1;
            SDL_GL_GetDrawableSize(window, &w, &h);
            w = std::max(w, 1);
            h = std::max(h, 1);

            const bool cinematic =
                menu.screen == Screen::Main ||
                menu.screen == Screen::SlotSelect ||
                menu.screen == Screen::CompanyName ||
                menu.screen == Screen::ConfirmNew ||
                (menu.screen == Screen::Options && menu.back == Screen::Main);

            if (cinematic) {
                const double menuTime =
                    smoke && frame >= 68
                        ? 8.0 + (frame - 68) * 14.0
                        : SDL_GetTicks() / 1000.0;

                renderMenuWorld(
                    game,
                    settings,
                    w,
                    h,
                    menuTime
                );
            }
            else {
                renderWorld(
                    game,
                    player,
                    settings,
                    w,
                    h,
                    surveillance ? camera : -1,
                    computerZoom,
                    terminal
                );
            }

            if (menu.screen == Screen::Playing) {
                if (surveillance)
                    renderSurveillanceHUD(game, camera);
                else if (computer && computerZoom >= 0.9999f)
                    renderComputer(game, desktop);
                else if (!computer && computerZoom <= 0.0001f)
                    renderHUD(
                        game,
                        player,
                        SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_TAB]
                    );
            }
            else {
                renderMenu(menu, settings, hasSave);
            }

            if (smoke &&
                (frame == 0 ||
                 frame == 5 ||
                 frame == 15 ||
                 (frame >= 18 && frame <= 28) ||
                 frame == 30 ||
                 frame == 32 ||
                 (frame >= 33 && frame <= 88))) {
                snapshot(w, h);
            }

            if (smoke && glGetError() != GL_NO_ERROR)
                throw std::runtime_error("OpenGL smoke test failed");

            SDL_GL_SwapWindow(window);
            ++frame;

            // A small cooperative delay prevents an uncapped menu/game from
            // consuming a full CPU core when VSync is disabled.
            if (!settings.vsync)
                SDL_Delay(1);
        }

        if (smoke) {
            std::cout
                << "Menu, save/load, consumption and rendering smoke tests passed\n";
        }

        return 0;
    }
};

} // namespace

int runApp(bool smoke, const std::filesystem::path& dataDirectory)
{
    try {
        Application app;
        return app.run(smoke, dataDirectory);
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}