#include <switch.h>
#include <Python.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <png.h>
#include "video_player.h"

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

/* -------------------------------------------------------
   Globals
------------------------------------------------------- */

u64 cur_progid = 0;
AccountUid userID = {0};


void show_logos(void)
{
    printf("=== Showing logos ===\n");
    
    // Инициализируем только то, что нужно для логотипов
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("SDL_InitSubSystem failed for logos: %s\n", SDL_GetError());
        return;
    }
    
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_TIF | IMG_INIT_WEBP;
    if (!(IMG_Init(img_flags) & img_flags)) {
        printf("IMG_Init failed: %s\n", IMG_GetError());
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
        return;
    }
    
    // Открываем все доступные контроллеры
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            SDL_GameController* controller = SDL_GameControllerOpen(i);
            if (controller) {
                printf("Opened controller %d: %s\n", i, SDL_GameControllerName(controller));
            }
        }
    }
    
    // Создаем окно и рендерер для логотипов
    SDL_Window* logo_window = SDL_CreateWindow("Logos", 
                                          SDL_WINDOWPOS_CENTERED, 
                                          SDL_WINDOWPOS_CENTERED, 
                                          1280, 720, 
                                          SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN);
    
    if (!logo_window) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
        return;
    }
    
    SDL_Renderer* logo_renderer = SDL_CreateRenderer(logo_window, -1, 
                                                SDL_RENDERER_ACCELERATED | 
                                                SDL_RENDERER_PRESENTVSYNC);
    
    if (!logo_renderer) {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(logo_window);
        IMG_Quit();
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
        return;
    }
    
    // Загружаем логотипы
    SDL_Texture* logo1 = NULL;
    SDL_Texture* logo2 = NULL;
    
    // Пробуем разные пути для логотипов
    const char* possible_paths1[] = {
        "romfs:/nintendologo.png",
        "romfs:/Contents/nintendologo.png",
        "sdmc:/nintendologo.png",
        "nintendologo.png"
    };
    
    const char* possible_paths2[] = {
        "romfs:/startupmovie.gif",
        "romfs:/Contents/startupmovie.gif", 
        "sdmc:/startupmovie.gif",
        "startupmovie.gif"
    };
    
    // Загружаем первый логотип
    for (int i = 0; i < 4; i++) {
        logo1 = IMG_LoadTexture(logo_renderer, possible_paths1[i]);
        if (logo1) {
            printf("Loaded logo1 from: %s\n", possible_paths1[i]);
            break;
        }
    }
    
    // Загружаем второй логотип
    for (int i = 0; i < 4; i++) {
        logo2 = IMG_LoadTexture(logo_renderer, possible_paths2[i]);
        if (logo2) {
            printf("Loaded logo2 from: %s\n", possible_paths2[i]);
            break;
        }
    }
    
    // Если нет логотипов, выходим
    if (!logo1 && !logo2) {
        printf("No logos found, continuing without them\n");
        SDL_DestroyRenderer(logo_renderer);
        SDL_DestroyWindow(logo_window);
        IMG_Quit();
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
        return;
    }
    
    // Получаем размеры экрана
    int display_width, display_height;
    SDL_GetWindowSize(logo_window, &display_width, &display_height);
    printf("Display dimensions: %d x %d\n", display_width, display_height);
    
    // Получаем размеры логотипов
    int logo1_width = 0, logo1_height = 0;
    int logo2_width = 0, logo2_height = 0;
    
    if (logo1) {
        SDL_QueryTexture(logo1, NULL, NULL, &logo1_width, &logo1_height);
        printf("Logo1 dimensions: %d x %d\n", logo1_width, logo1_height);
    }
    
    if (logo2) {
        SDL_QueryTexture(logo2, NULL, NULL, &logo2_width, &logo2_height);
        printf("Logo2 dimensions: %d x %d\n", logo2_width, logo2_height);
    }
    
    // Позиции логотипов
    SDL_Rect logo1_rect = {0, 0, logo1_width, logo1_height}; // Левый верхний угол
    
    SDL_Rect logo2_rect = {
        display_width - logo2_width,    // Правый край
        display_height - logo2_height,  // Нижний край
        logo2_width,
        logo2_height
    };
    
    // Основной цикл отображения логотипов
    Uint32 start_time = SDL_GetTicks();
    bool running = true;
    bool skip_requested = false;
    
    while (running) {
        SDL_Event event;
        
        // Проверяем события
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    skip_requested = true;
                    break;
                    
                case SDL_KEYDOWN:
                    printf("Key pressed: %s\n", SDL_GetKeyName(event.key.keysym.sym));
                    running = false;
                    skip_requested = true;
                    break;
                    
                case SDL_JOYBUTTONDOWN:
                    printf("Joystick button pressed: joystick=%d, button=%d\n", 
                           event.jbutton.which, event.jbutton.button);
                    running = false;
                    skip_requested = true;
                    break;
                    
                case SDL_CONTROLLERBUTTONDOWN:
                    printf("Controller button pressed: controller=%d, button=%d\n", 
                           event.cbutton.which, event.cbutton.button);
                    running = false;
                    skip_requested = true;
                    break;
                    
                case SDL_JOYDEVICEADDED:
                    if (SDL_IsGameController(event.jdevice.which)) {
                        SDL_GameController* controller = SDL_GameControllerOpen(event.jdevice.which);
                        if (controller) {
                            printf("Controller connected: %s\n", SDL_GameControllerName(controller));
                        }
                    }
                    break;
                    
                case SDL_JOYDEVICEREMOVED:
                    printf("Controller disconnected\n");
                    break;
            }
        }
        
        // Проверяем таймаут (3 секунды)
        if (!skip_requested && SDL_GetTicks() - start_time > 3000) {
            printf("3 seconds elapsed, exiting logos\n");
            running = false;
        }
        
        // Если запрошен пропуск, показываем черный экран и выходим
        if (skip_requested) {
            // Очищаем экран черным
            SDL_SetRenderDrawColor(logo_renderer, 0, 0, 0, 255);
            SDL_RenderClear(logo_renderer);
            SDL_RenderPresent(logo_renderer);
            
            // Короткая задержка для отображения черного экрана
            SDL_Delay(50);
            break;
        }
        
        // Очищаем экран
        SDL_SetRenderDrawColor(logo_renderer, 0, 0, 0, 255);
        SDL_RenderClear(logo_renderer);
        
        // Рисуем логотипы
        if (logo1) {
            SDL_RenderCopy(logo_renderer, logo1, NULL, &logo1_rect);
        }
        
        if (logo2) {
            SDL_RenderCopy(logo_renderer, logo2, NULL, &logo2_rect);
        }
        
        // Обновляем экран
        SDL_RenderPresent(logo_renderer);
        
        // Небольшая задержка для экономии CPU
        SDL_Delay(16);
    }
    
    printf("Logos displayed for %u ms\n", SDL_GetTicks() - start_time);
    
    // Очищаем ресурсы логотипов
    if (logo1) SDL_DestroyTexture(logo1);
    if (logo2) SDL_DestroyTexture(logo2);
    
    // Уничтожаем рендерер и окно логотипов
    SDL_DestroyRenderer(logo_renderer);
    SDL_DestroyWindow(logo_window);
    
    // Закрываем все контроллеры
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            SDL_GameControllerClose(SDL_GameControllerFromInstanceID(i));
        }
    }
    
    // Завершаем только SDL_image
    IMG_Quit();
    
    // Завершаем подсистемы, которые мы инициализировали
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
    
    // Важно: небольшая пауза для очистки
    printf("Pausing to let SDL clean up...\n");
    svcSleepThread(100000000ULL); // 100ms пауза
    
    printf("=== Logos finished ===\n");
}
/* -------------------------------------------------------
   _nx module (sleep)
------------------------------------------------------- */

static PyObject* py_nx_sleep(PyObject* self, PyObject* args)
{
    double seconds;
    if (!PyArg_ParseTuple(args, "d", &seconds))
        return NULL;

    uint64_t ns = (uint64_t)(seconds * 1000000000ULL);

    Py_BEGIN_ALLOW_THREADS
    svcSleepThread(ns);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}

static PyMethodDef NxMethods[] = {
    {"sleep", py_nx_sleep, METH_VARARGS, "Sleep using svcSleepThread"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef nx_module = {
    PyModuleDef_HEAD_INIT,
    "_nx",
    NULL,
    -1,
    NxMethods
};

PyMODINIT_FUNC PyInit__nx(void)
{
    return PyModule_Create(&nx_module);
}

/* -------------------------------------------------------
   _otrhlibnx module
------------------------------------------------------- */

static PyObject* commitsave(PyObject* self, PyObject* args)
{
    u64 total_size = 0;
    u64 free_size = 0;
    FsFileSystem* FsSave = fsdevGetDeviceFileSystem("save");

    if (!FsSave)
        Py_RETURN_NONE;

    FsSaveDataInfoReader reader;
    FsSaveDataInfo info;
    s64 total_entries = 0;
    Result rc = 0;

    fsdevCommitDevice("save");

    fsFsGetTotalSpace(FsSave, "/", &total_size);
    fsFsGetFreeSpace(FsSave, "/", &free_size);

    if (free_size < 0x800000)
    {
        u64 new_size = total_size + 0x800000;

        fsdevUnmountDevice("save");
        fsOpenSaveDataInfoReader(&reader, FsSaveDataSpaceId_User);

        while (1)
        {
            rc = fsSaveDataInfoReaderRead(&reader, &info, 1, &total_entries);
            if (R_FAILED(rc) || total_entries == 0)
                break;

            if (info.save_data_type == FsSaveDataType_Account &&
                userID.uid[0] == info.uid.uid[0] &&
                userID.uid[1] == info.uid.uid[1] &&
                info.application_id == cur_progid)
            {
                fsExtendSaveDataFileSystem(
                    info.save_data_space_id,
                    info.save_data_id,
                    new_size,
                    0x400000
                );
                break;
            }
        }

        fsSaveDataInfoReaderClose(&reader);
        fsdevMountSaveData("save", cur_progid, userID);
    }

    Py_RETURN_NONE;
}

static PyObject* startboost(PyObject* self, PyObject* args)
{
    appletSetCpuBoostMode(ApmPerformanceMode_Boost);
    Py_RETURN_NONE;
}

static PyObject* disableboost(PyObject* self, PyObject* args)
{
    appletSetCpuBoostMode(ApmPerformanceMode_Normal);
    Py_RETURN_NONE;
}

static PyObject* restartprogram(PyObject* self, PyObject* args)
{
    appletRestartProgram(NULL, 0);
    Py_RETURN_NONE;
}

static PyMethodDef OtrhMethods[] = {
    {"commitsave", commitsave, METH_NOARGS, NULL},
    {"startboost", startboost, METH_NOARGS, NULL},
    {"disableboost", disableboost, METH_NOARGS, NULL},
    {"restartprogram", restartprogram, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef otrh_module = {
    PyModuleDef_HEAD_INIT,
    "_otrhlibnx",
    NULL,
    -1,
    OtrhMethods
};

PyMODINIT_FUNC PyInit__otrhlibnx(void)
{
    return PyModule_Create(&otrh_module);
}


PyMODINIT_FUNC PyInit_color(void);
PyMODINIT_FUNC PyInit_controller(void);
PyMODINIT_FUNC PyInit_display(void);
PyMODINIT_FUNC PyInit_draw(void);
PyMODINIT_FUNC PyInit_error(void);
PyMODINIT_FUNC PyInit_event(void);
PyMODINIT_FUNC PyInit_font(void);
PyMODINIT_FUNC PyInit_gfxdraw(void);
PyMODINIT_FUNC PyInit_image(void);
PyMODINIT_FUNC PyInit_joystick(void);
PyMODINIT_FUNC PyInit_key(void);
PyMODINIT_FUNC PyInit_locals(void);
PyMODINIT_FUNC PyInit_mouse(void);
PyMODINIT_FUNC PyInit_power(void);
PyMODINIT_FUNC PyInit_pygame_time(void);
PyMODINIT_FUNC PyInit_rect(void);
PyMODINIT_FUNC PyInit_rwobject(void);
PyMODINIT_FUNC PyInit_scrap(void);
PyMODINIT_FUNC PyInit_transform(void);
PyMODINIT_FUNC PyInit_surface(void);

PyMODINIT_FUNC PyInit_render(void);
PyMODINIT_FUNC PyInit_mixer(void);
PyMODINIT_FUNC PyInit_mixer_music(void);


PyMODINIT_FUNC PyInit__renpy(void);
PyMODINIT_FUNC PyInit__renpybidi(void);

PyMODINIT_FUNC PyInit_renpy_audio_filter(void);
PyMODINIT_FUNC PyInit_renpy_audio_renpysound(void);
PyMODINIT_FUNC PyInit_renpy_encryption(void);

PyMODINIT_FUNC PyInit_renpy_display_accelerator(void);
PyMODINIT_FUNC PyInit_renpy_display_matrix(void);
PyMODINIT_FUNC PyInit_renpy_display_quaternion(void);
PyMODINIT_FUNC PyInit_renpy_display_render(void);

PyMODINIT_FUNC PyInit_renpy_text_ftfont(void);
PyMODINIT_FUNC PyInit_renpy_text_textsupport(void);
PyMODINIT_FUNC PyInit_renpy_text_texwrap(void);

PyMODINIT_FUNC PyInit_renpy_lexersupport(void);
PyMODINIT_FUNC PyInit_renpy_pydict(void);
PyMODINIT_FUNC PyInit_renpy_style(void);

PyMODINIT_FUNC PyInit_renpy_styledata_style_activate_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_hover_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_idle_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_insensitive_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_selected_activate_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_selected_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_selected_hover_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_selected_idle_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_style_selected_insensitive_functions(void);
PyMODINIT_FUNC PyInit_renpy_styledata_styleclass(void);
PyMODINIT_FUNC PyInit_renpy_styledata_stylesets(void);

PyMODINIT_FUNC PyInit_renpy_gl_gldraw(void);
PyMODINIT_FUNC PyInit_renpy_gl_glenviron_shader(void);
PyMODINIT_FUNC PyInit_renpy_gl_glrtt_copy(void);
PyMODINIT_FUNC PyInit_renpy_gl_glrtt_fbo(void);
PyMODINIT_FUNC PyInit_renpy_gl_gltexture(void);

PyMODINIT_FUNC PyInit_renpy_gl2_gl2draw(void);
PyMODINIT_FUNC PyInit_renpy_gl2_gl2mesh(void);
PyMODINIT_FUNC PyInit_renpy_gl2_gl2mesh2(void);
PyMODINIT_FUNC PyInit_renpy_gl2_gl2mesh3(void);
PyMODINIT_FUNC PyInit_renpy_gl2_gl2model(void);
PyMODINIT_FUNC PyInit_renpy_gl2_gl2polygon(void);
PyMODINIT_FUNC PyInit_renpy_gl2_gl2shader(void);
PyMODINIT_FUNC PyInit_renpy_gl2_gl2texture(void);

PyMODINIT_FUNC PyInit_renpy_uguu_gl(void);
PyMODINIT_FUNC PyInit_renpy_uguu_uguu(void);


/* -------------------------------------------------------
   Heap override
------------------------------------------------------- */

void __libnx_initheap(void)
{
    void* addr = NULL;
    u64 size = 0;
    u64 mem_available = 0, mem_used = 0;

    svcGetInfo(&mem_available, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
    svcGetInfo(&mem_used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);

    if (mem_available > mem_used + 0x200000)
        size = (mem_available - mem_used - 0x200000) & ~0x1FFFFF;

    if (size == 0)
        size = 0x2000000*16; // 256 MB fallback

    Result rc = svcSetHeapSize(&addr, size);
    if (R_FAILED(rc) || addr == NULL)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_HeapAllocFailed));

    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

/* -------------------------------------------------------
   Save creation
------------------------------------------------------- */

Result createSaveData(void)
{
    FsSaveDataAttribute attr = {0};
    FsSaveDataCreationInfo crt = {0};
    FsSaveDataMetaInfo meta = {0};

    attr.application_id = cur_progid;
    attr.uid = userID;
    attr.save_data_type = FsSaveDataType_Account;

    crt.save_data_size = 0x800000;   // 8 MB
    crt.journal_size   = 0x400000;   // 4 MB
    crt.available_size = 0x8000;
    crt.save_data_space_id = FsSaveDataSpaceId_User;

    return fsCreateSaveDataFileSystem(&attr, &crt, &meta);
}

/* -------------------------------------------------------
   App init / exit
------------------------------------------------------- */

void userAppInit()
{
    fsInitialize();
    fsdevMountSdmc();

    freopen("sdmc:/renpy_switch.log", "w", stdout);
    freopen("sdmc:/renpy_switch.log", "w", stderr);

    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    printf("=== Ren'Py 8 Switch launcher ===\n");
    
    Result rc = 0;
    PselUserSelectionSettings settings;
    
    rc = svcGetInfo(&cur_progid, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0);
    rc = accountInitialize(AccountServiceType_Application);
    rc = accountGetPreselectedUser(&userID);
    
    if (R_FAILED(rc)) {
        s32 count;
        accountGetUserCount(&count);

        if (count > 1) {
            // Показываем селектор пользователей
            rc = pselShowUserSelector(&userID, &settings);
            
            // Если пользователь нажал отмену, завершаем программу
            if (R_FAILED(rc) || !accountUidIsValid(&userID)) {
                printf("User selection cancelled. Exiting...\n");
                fflush(stdout);
                
                // Даем время для записи лога
                svcSleepThread(2000000000ULL); // 2 секунды
                
                // Завершаем программу
                exit(0);
            }
        } else if (count == 1) {
            s32 loadedUsers;
            AccountUid account_ids[1];
            accountListAllUsers(account_ids, 1, &loadedUsers);
            if (count > 0) {
                userID = account_ids[0];
            }
        } else {
            // Нет пользователей - завершаем программу
            printf("No users found. Exiting...\n");
            fflush(stdout);
            svcSleepThread(2000000000ULL);
            exit(0);
        }
    }

    // Дополнительная проверка после всех операций
    if (!accountUidIsValid(&userID)) {
        printf("Invalid user selected. Exiting...\n");
        fflush(stdout);
        svcSleepThread(2000000000ULL);
        exit(0);
    }

    // Если пользователь выбран, продолжаем инициализацию
    rc = fsdevMountSaveData("save", cur_progid, userID);
    if (R_FAILED(rc)) {
        rc = createSaveData();
        rc = fsdevMountSaveData("save", cur_progid, userID);
    }

    romfsInit();
    socketInitializeDefault();
}

void userAppExit(void)
{
    if (fsdevGetDeviceFileSystem("save"))
        fsdevCommitDevice("save");

    fsdevUnmountDevice("save");
    socketExit();
    romfsExit();
}

/* -------------------------------------------------------
   Error helper
------------------------------------------------------- */

void show_error(const char* message)
{
    PyErr_Print();

    ErrorSystemConfig c;
    errorSystemCreate(&c, message, message);
    errorSystemShow(&c);

    Py_Exit(1);
}

static AppletHookCookie applet_hook_cookie;
static void on_applet_hook(AppletHookType hook, void *param)
{
   switch (hook)
   {
      case AppletHookType_OnExitRequest:
        fsdevCommitDevice("save");
        svcSleepThread(1500000000ULL);
        appletUnlockExit();
        break;

      default:
         break;
   }
}

/* -------------------------------------------------------
   Main
------------------------------------------------------- */

int main(int argc, char* argv[])
{  
   	// Проверяем, что пользователь выбран
    if (!accountUidIsValid(&userID)) {
        printf("No user selected. Game cannot start.\n");
        fflush(stdout);
        svcSleepThread(2000000000ULL); // Даем время для записи в лог
        return 0;
    }

    video_player_init();
    play_video_file_delay("romfs:/Contents/game/intro.webm", 1, 3.0f);
    video_player_quit();
   
    // Показ логотипов при запуске
    show_logos();
   
    video_player_init();
    play_video_file_delay("romfs:/Contents/game/intro.webm", 1, 3.0f);
    video_player_quit();
    // printf("=== Testing video playback ===\n");
    
    // // Инициализируем видео-плеер
    // video_player_init();
    
    // // 1. Из корня romfs
    // printf("\n=== Test: romfs:/intro.mp4 ===\n");
    // play_video_file_delay("romfs:/intro.mp4", 1, 3.0f);
    
    // // 2. Из папки Contents в romfs
    // printf("\n=== Test: romfs:/Contents/intro.mp4 ===\n");
    // play_video_file_delay("romfs:/Contents/intro.mp4", 1, 1.0f);
    
    // // 3. Из папки game/video в romfs
    // printf("\n=== Test: romfs:/Contents/game/video/intro.mp4 ===\n");
    // play_video_file_delay("romfs:/Contents/game/video/intro.mp4", 1, 1.0f);
    
    // // 4. Относительный путь (будет автоматически разрешен)
    // printf("\n=== Test: intro.mp4 ===\n");
    // play_video_file_delay("intro.mp4", 1, 1.0f);
    
    // // 5. С SD карты
    // printf("\n=== Test: sdmc:/intro.mp4 ===\n");
    // play_video_file_delay("sdmc:/intro.mp4", 1, 1.0f);
    
    // // Деинициализируем видео-плеер
    // video_player_quit();
   
    chdir("romfs:/Contents");
    setlocale(LC_ALL, "C");
    setenv("MESA_NO_ERROR", "1", 1);

    appletLockExit();
    appletHook(&applet_hook_cookie, on_applet_hook, NULL);

    Py_NoSiteFlag = 1;
    Py_IgnoreEnvironmentFlag = 1;
    Py_NoUserSiteDirectory = 1;
    Py_DontWriteBytecodeFlag = 1;
    Py_OptimizeFlag = 2;

    PyConfig config;

    PyConfig_InitPythonConfig(&config);

     /* ---- Указываем Python'у его местоположение ---- */
    /* Это устранит ошибку "Could not find platform independent libraries" */
    PyStatus status;
    
    status = PyConfig_SetString(&config, &config.home, L"romfs:/Contents");
    if (PyStatus_Exception(status)) goto exception;

    status = PyConfig_SetString(&config, &config.prefix, L"romfs:/Contents");
    if (PyStatus_Exception(status)) goto exception;
    
    status = PyConfig_SetString(&config, &config.exec_prefix, L"romfs:/Contents");
    if (PyStatus_Exception(status)) goto exception;
   
    /* Добавляем путь к корневой папке renpy/common */
    PyWideStringList_Append(
    &config.module_search_paths,
    L"romfs:/Contents/renpy/common"
    );
   
    /* ---- Critical for Python 3.9 embedded ---- */
    config.isolated = 0;
    config.use_environment = 0;
    config.site_import = 0;
    config.user_site_directory = 0;
    config.write_bytecode = 0;
    config.optimization_level = 2;
    config.verbose = 0;

    /* Filesystem encoding */
    status = PyConfig_SetString(&config,
                                &config.filesystem_encoding,
                                L"utf-8");
    if (PyStatus_Exception(status)) goto exception;

    status = PyConfig_SetString(&config,
                                &config.filesystem_errors,
                                L"surrogateescape");
    if (PyStatus_Exception(status)) goto exception;

    /* ---- stdlib: ONLY lib.zip ---- */
    config.module_search_paths_set = 1;

    status = PyWideStringList_Append(
        &config.module_search_paths,
        L"romfs:/Contents/lib.zip"
    );
    if (PyStatus_Exception(status)) goto exception;

    /* ---- argv ---- */
    wchar_t* pyargv[] = {
        L"romfs:/Contents/renpy.py",
        NULL
    };
    status = PyConfig_SetArgv(&config, 1, pyargv);
    if (PyStatus_Exception(status)) goto exception;

   
    Py_SetProgramName(L"RenPy3.8.7");



    /* ---- Builtin modules ---- */
    static struct _inittab builtins[] = {

       //{"pygame_sdl2", PyInit_pygame_sdl2},

        {"_nx", PyInit__nx},
        {"_otrhlibnx", PyInit__otrhlibnx},

        {"pygame_sdl2.color", PyInit_color},
        {"pygame_sdl2.controller", PyInit_controller},
        {"pygame_sdl2.display", PyInit_display},
        {"pygame_sdl2.draw", PyInit_draw},
        {"pygame_sdl2.error", PyInit_error},
        {"pygame_sdl2.event", PyInit_event},
        {"pygame_sdl2.font", PyInit_font},
        {"pygame_sdl2.gfxdraw", PyInit_gfxdraw},
        {"pygame_sdl2.image", PyInit_image},
        {"pygame_sdl2.joystick", PyInit_joystick},
        {"pygame_sdl2.key", PyInit_key},
        {"pygame_sdl2.locals", PyInit_locals},
        {"pygame_sdl2.mouse", PyInit_mouse},
        {"pygame_sdl2.power", PyInit_power},
        {"pygame_sdl2.pygame_time", PyInit_pygame_time},
        {"pygame_sdl2.rect", PyInit_rect}, 
        {"pygame_sdl2.rwobject", PyInit_rwobject},
        {"pygame_sdl2.scrap", PyInit_scrap},
        {"pygame_sdl2.surface", PyInit_surface},
        {"pygame_sdl2.transform", PyInit_transform},
   
        {"pygame_sdl2.render", PyInit_render},
        {"pygame_sdl2.mixer", PyInit_mixer},
        {"pygame_sdl2.mixer_music", PyInit_mixer_music},

        {"_renpy", PyInit__renpy},
        {"_renpybidi", PyInit__renpybidi},

        {"renpy.audio.filter", PyInit_renpy_audio_filter},
        {"renpy.audio.renpysound", PyInit_renpy_audio_renpysound},
        {"renpy.display.accelerator", PyInit_renpy_display_accelerator},
        {"renpy.display.matrix", PyInit_renpy_display_matrix},
        {"renpy.display.quaternion", PyInit_renpy_display_quaternion},
        {"renpy.display.render", PyInit_renpy_display_render},
        {"renpy.encryption", PyInit_renpy_encryption},

        {"renpy.text.ftfont", PyInit_renpy_text_ftfont},
        {"renpy.text.textsupport", PyInit_renpy_text_textsupport},
        {"renpy.text.texwrap", PyInit_renpy_text_texwrap},
        {"renpy.pydict", PyInit_renpy_pydict},
        {"renpy.lexersupport", PyInit_renpy_lexersupport},
        {"renpy.style", PyInit_renpy_style},
        {"renpy.styledata.style_activate_functions", PyInit_renpy_styledata_style_activate_functions},
        {"renpy.styledata.style_functions", PyInit_renpy_styledata_style_functions},
        {"renpy.styledata.style_hover_functions", PyInit_renpy_styledata_style_hover_functions},
        {"renpy.styledata.style_idle_functions", PyInit_renpy_styledata_style_idle_functions},
        {"renpy.styledata.style_insensitive_functions", PyInit_renpy_styledata_style_insensitive_functions},
        {"renpy.styledata.style_selected_activate_functions", PyInit_renpy_styledata_style_selected_activate_functions},
        {"renpy.styledata.style_selected_functions", PyInit_renpy_styledata_style_selected_functions},
        {"renpy.styledata.style_selected_hover_functions", PyInit_renpy_styledata_style_selected_hover_functions},
        {"renpy.styledata.style_selected_idle_functions", PyInit_renpy_styledata_style_selected_idle_functions},
        {"renpy.styledata.style_selected_insensitive_functions", PyInit_renpy_styledata_style_selected_insensitive_functions},
        {"renpy.styledata.styleclass", PyInit_renpy_styledata_styleclass},
        {"renpy.styledata.stylesets", PyInit_renpy_styledata_stylesets},

        {"renpy.gl.gldraw", PyInit_renpy_gl_gldraw},
        {"renpy.gl.glenviron_shader", PyInit_renpy_gl_glenviron_shader},
        {"renpy.gl.glrtt_copy", PyInit_renpy_gl_glrtt_copy},
        {"renpy.gl.glrtt_fbo", PyInit_renpy_gl_glrtt_fbo},
        {"renpy.gl.gltexture", PyInit_renpy_gl_gltexture},

        {"renpy.gl2.gl2draw", PyInit_renpy_gl2_gl2draw},
        {"renpy.gl2.gl2mesh", PyInit_renpy_gl2_gl2mesh},
        {"renpy.gl2.gl2mesh2", PyInit_renpy_gl2_gl2mesh2},
        {"renpy.gl2.gl2mesh3", PyInit_renpy_gl2_gl2mesh3},
        {"renpy.gl2.gl2model", PyInit_renpy_gl2_gl2model},
        {"renpy.gl2.gl2polygon", PyInit_renpy_gl2_gl2polygon},
        {"renpy.gl2.gl2shader", PyInit_renpy_gl2_gl2shader},
        {"renpy.gl2.gl2texture", PyInit_renpy_gl2_gl2texture},

        {"renpy.uguu.gl", PyInit_renpy_uguu_gl},
        {"renpy.uguu.uguu", PyInit_renpy_uguu_uguu},

        {NULL, NULL}
    };

    /* ---- Sanity check ---- */
    FILE* libzip = fopen("romfs:/Contents/lib.zip", "rb");
    if (!libzip) {
        show_error("Could not find lib.zip");
    }
    fclose(libzip);

    FILE* renpy_file = fopen("romfs:/Contents/renpy.py", "rb");
    if (!renpy_file) {
        show_error("Could not find renpy.py");
    }   

    PyImport_ExtendInittab(builtins);
   
    /* ---- Initialize Python ---- */
    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) goto exception;
    PyConfig_Clear(&config);


    PyRun_SimpleString("import pygame_sdl2");
   

    int python_result;
    python_result = PyRun_SimpleString(
    "import sys\n"
    "sys.path.insert(0, 'romfs:/Contents/lib.zip')\n"
    );
    
    if (python_result == -1)
    {
        show_error("Could not set the Python path.\n\nThis is an internal error and should not occur during normal usage.");
    }
   #define x(lib) \
    { \
        if (PyRun_SimpleString("import " lib) == -1) \
        { \
            show_error("Could not import python library " lib ".\n\nPlease ensure that you have extracted the files correctly so that the \"lib\" folder is in the same directory as the nsp file, and that the \"lib\" folder contains the folder \"python3.9\". \nInside that folder, the file \"" lib ".py\" or folder \"" lib "\" needs to exist."); \
        } \
    }

    x("os");
    x("pygame_sdl2");
    x("encodings");

    #undef x

    /* ---- Run Ren'Py ---- */
    int rc = PyRun_SimpleFileEx(
        renpy_file,
        "romfs:/Contents/renpy.py",
        1
    );

    if (rc != 0) {
        show_error("Ren'Py execution failed");
    }

    Py_Finalize();
    return 0;

exception:
    PyConfig_Clear(&config);
    if (PyStatus_IsExit(status)) {
        return status.exitcode;
    }
    show_error(status.err_msg);
    Py_ExitStatusException(status);
}
