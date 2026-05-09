#include "common.h"
#include "rasterizer.c"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

__declspec(dllimport) unsigned int __stdcall timeBeginPeriod(unsigned int);
__declspec(dllimport) unsigned int __stdcall timeEndPeriod(unsigned int);

#define TARGET_FPS 60

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define LOG_ERROR(...)

DWORD main_thread_id;
HANDLE window_created_event;
HANDLE window_handle = NULL;

LRESULT CALLBACK window_procedure(
    HWND window_handle,
    UINT message,
    WPARAM w_parameter,
    LPARAM l_parameter
) {
    LRESULT result;

    switch (message) {
        case WM_PAINT: {
            result = 0;
        } break;

        case WM_ERASEBKGND: {
            result = 1;
        } break;

        // Forward any messages of interest back to the main thread.
        case WM_DESTROY:
        case WM_SIZE: {
            PostThreadMessage(main_thread_id, message, w_parameter, l_parameter);
            result = 0;
        } break;

        default: {
            result = DefWindowProc(window_handle, message, w_parameter, l_parameter);
        } break;
    }

    return result;
}

DWORD WINAPI event_loop_procedure(LPVOID parameter) {
    WNDCLASS window_class = {
        .hInstance = GetModuleHandle(NULL),
        .lpszClassName = L"Rasterizer::Main",
        .lpfnWndProc = window_procedure,
        .style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW,
        .hCursor = LoadCursor(NULL, IDC_ARROW),
    };

    ATOM window_class_atom = RegisterClass(&window_class);
    if (window_class_atom == 0) {
        LOG_ERROR(Windows, "Failed to register the window class");
        return 1;
    }

    DWORD window_style = WS_OVERLAPPEDWINDOW;

    RECT window_rectangle = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    AdjustWindowRect(&window_rectangle, window_style, false);
    int window_width = window_rectangle.right - window_rectangle.left;
    int window_height = window_rectangle.bottom - window_rectangle.top;

    window_handle = CreateWindowEx(
        0,
        window_class.lpszClassName,
        L"Rasterizer",
        window_style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        window_width, window_height,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    SetEvent(window_created_event);
    if (window_handle == NULL) {
        return 1;
    }

    MSG message;
    while (GetMessage(&message, window_handle, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return 0;
}

typedef struct {
    HBITMAP handle;
    int width;
    int height;
    u32 *pixels;
} Bitmap;

bool bitmap_create(int width, int height, Bitmap *bitmap) {
    bitmap->width = width;
    bitmap->height = height;

    HDC device_context = GetDC(NULL);

    BITMAPINFO bitmap_info;
    bitmap_info.bmiHeader = (BITMAPINFOHEADER) {
        .biSize = sizeof(bitmap_info.bmiHeader),
        .biWidth = width,
        .biHeight = -height,
        .biPlanes = 1,
        .biBitCount = 32,
        .biCompression = BI_RGB,
    };

    bitmap->handle = CreateDIBSection(
        device_context,
        &bitmap_info,
        DIB_RGB_COLORS,
        (void **)&bitmap->pixels,
        NULL,
        0
    );

    ReleaseDC(NULL, device_context);

    return bitmap->pixels != NULL;
}

bool bitmap_resize(Bitmap *bitmap, int new_width, int new_height) {
    if (bitmap->width == new_width && bitmap->height == new_height) {
        return true;
    }

    DeleteObject(bitmap->handle);
    return bitmap_create(new_width, new_height, bitmap);
}

void bitmap_display(Bitmap *bitmap, HWND window_handle) {
    HDC window_device_context = GetDC(window_handle);
    HDC bitmap_device_context = CreateCompatibleDC(window_device_context);
    HBITMAP old_bitmap = SelectObject(bitmap_device_context, bitmap->handle);

    BitBlt(
        window_device_context,
        0, 0,
        bitmap->width, bitmap->height,
        bitmap_device_context,
        0, 0,
        SRCCOPY
    );

    SelectObject(bitmap_device_context, old_bitmap);
    DeleteDC(bitmap_device_context);
    ReleaseDC(window_handle, window_device_context);

    GdiFlush();
}

int WINAPI WinMain(
    HINSTANCE instance,
    HINSTANCE previous_instance,
    PSTR command_line,
    int show_command
) {
    timeBeginPeriod(1);
    SetProcessDPIAware();

    main_thread_id = GetCurrentThreadId();
    window_created_event = CreateEvent(NULL, true, false, NULL);

    HANDLE event_loop_thread_handle = CreateThread(NULL, 0, event_loop_procedure, NULL, 0, NULL);
    if (event_loop_thread_handle == NULL) {
        LOG_ERROR(Windows, "Failed to create a thread for event handling");
        return 1;
    }

    WaitForSingleObject(window_created_event, INFINITE);
    if (window_handle == NULL) {
        LOG_ERROR(Windows, "Event handling thread failed to create a window");
        return 1;
    }

    RECT window_rectangle;
    GetClientRect(window_handle, &window_rectangle);
    int window_width = window_rectangle.right - window_rectangle.left;
    int window_height = window_rectangle.bottom - window_rectangle.top;

    Bitmap bitmap;
    if (!bitmap_create(window_width, window_height, &bitmap)) {
        LOG_ERROR(Windows, "Failed to create a display bitmap");
        return 1;
    }

    ShowWindow(window_handle, SW_SHOW);
    UpdateWindow(window_handle);
    SetForegroundWindow(window_handle);
    SetFocus(window_handle);

    LARGE_INTEGER timer_frequency;
    QueryPerformanceFrequency(&timer_frequency);
    i64 ticks_per_frame = timer_frequency.QuadPart / TARGET_FPS;

    LARGE_INTEGER created_time;
    QueryPerformanceCounter(&created_time);
    LARGE_INTEGER previous_frame_end_time = created_time;

    bool window_should_close = false;

    while (!window_should_close) {
        MSG message;
        while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
            switch (message.message) {
                case WM_DESTROY: {
                    window_should_close = true;
                } break;

                case WM_SIZE: {
                    window_width = LOWORD(message.lParam);
                    window_height = HIWORD(message.lParam);
                } break;
            }
        }

        f64 time = previous_frame_end_time.QuadPart - created_time.QuadPart;
        time /= timer_frequency.QuadPart;

        bitmap_resize(&bitmap, window_width, window_height);
        render(bitmap.pixels, bitmap.width, bitmap.height, time);
        bitmap_display(&bitmap, window_handle);

        LARGE_INTEGER current_time;
        QueryPerformanceCounter(&current_time);

        i64 ticks_since_last_frame = current_time.QuadPart - previous_frame_end_time.QuadPart;
        if (ticks_since_last_frame < ticks_per_frame) {
            f64 seconds_to_sleep = ticks_per_frame - ticks_since_last_frame;
            seconds_to_sleep /= timer_frequency.QuadPart;

            // Sleep for 95% of the frame time.
            int millis_to_sleep = (seconds_to_sleep * 1000) * 0.95;
            if (millis_to_sleep > 0) {
                Sleep(millis_to_sleep);
            }

            // Busy-wait for the remaining time.
            while (true) {
                QueryPerformanceCounter(&current_time);
                ticks_since_last_frame = current_time.QuadPart - previous_frame_end_time.QuadPart;

                if (ticks_since_last_frame >= ticks_per_frame) {
                    break;
                }
            }
        }

        previous_frame_end_time = current_time;
    }

    timeEndPeriod(1);
    return 0;
}
