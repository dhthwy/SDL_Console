#include "SDL_console.h"
#include <SDL2/SDL.h>
#include <atomic>
#include <stdio.h>
#include <thread>

void thread_fun();
void draw_fun();

std::atomic<Console_con*> con { nullptr };
static std::atomic<bool> do_draw { true };

int main(int argc, char** argv)
{
    std::thread th(thread_fun);

    // Wait for initialization
    int ms = 1;
    while (con == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        printf("waiting...%dms\r", ms);
        ms++;
    }
    printf("\nConsole is ready.\n");
    printf("Window supports %d columns and %d rows of text.\n", Console_GetColumns(con), Console_GetRows(con));

    while (1) {
        std::string buf;
        if (Console_GetLine(con, buf) >= 0) {
            if (buf == "clear") {
                Console_Clear(con);
            } else if (buf == "test") {
                Console_AddLine(con, "❤ ♥ Really long output! Lorem ipsum dolor sit amet, \n \r \nconsectetur adipiscing elit. Sed tincidunt, odio quis pulvinar suscipit, dolor nibh lobortis massa, quis sollicitudin ipsum sapien nec leo. Donec id sem sapien. Quisque dignissim eget sem ac bibendum. Suspendisse aliquam est finibus tellus molestie faucibus. Vestibulum");
            } else if (buf == "test2") {
                Console_AddLine(con, "❤ ♥ Really long output! Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed tincidunt, odio quis pulvinar suscipit, dolor nibh lobortis massa, quis sollicitudin ipsum sapien nec leo. Donec id sem sapien. Quisque dignissim eget sem ac bibendum. Suspendisse aliquam est finibus tellus molestie faucibus. Vestibulum volutpat feugiat nulla ut pharetra. Etiam facilisis, nunc in ullamcorper tempus, velit ante molestie turpis, at aliquet orci odio in arcu. Aenean dignissim dolor libero, et rhoncus felis elementum hendrerit. Donec aliquam accumsan nunc, vitae tempor sem tristique non. Duis at velit libero. Fusce ac justo vel leo lacinia vehicula sed vel felis. Nullam lacus orci, faucibus eu dapibus nec, gravida quis dui. Fusce faucibus, eros eu dignissim pharetra, velit velit imperdiet urna, gravida commodo est arcu eget lectus. Nunc leo ipsum, maximus vel dictum sit amet, maximus vitae arcu. Donec suscipit elit nec dolor lobortis rhoncus ♥ ❤");

            } else if (buf == "test3") {
                Console_AddLine(con, "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
            } else if (buf == "test4") {
                Console_AddLine(con, "                                                                                                                                                          ");
            } else if (buf == "test5") {
                Console_AddLine(con, "\n");
                Console_AddLine(con, "");
            } else if (buf == "test6") {
                for (int i = 0; i < 2000; i++) {
                    Console_AddLine(con, "❤ ♥ Really long output! Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed tincidunt, odio quis pulvinar suscipit, dolor nibh lobortis massa, quis sollicitudin ipsum sapien nec leo. Donec id sem sapien. Quisque dignissim eget sem ac bibendum. Suspendisse aliquam est finibus tellus molestie faucibus. Vestibulum volutpat feugiat nulla ut pharetra. Etiam facilisis, nunc in ullamcorper tempus, velit ante molestie turpis, at aliquet orci odio in arcu. Aenean dignissim dolor libero, et rhoncus felis elementum hendrerit. Donec aliquam accumsan nunc, vitae tempor sem tristique non. Duis at velit libero. Fusce ac justo vel leo lacinia vehicula sed vel felis. Nullam lacus orci, faucibus eu dapibus nec, gravida quis dui. Fusce faucibus, eros eu dignissim pharetra, velit velit imperdiet urna, gravida commodo est arcu eget lectus. Nunc leo ipsum, maximus vel dictum sit amet, maximus vitae arcu. Donec suscipit elit nec dolor lobortis rhoncus ♥ ❤");
                }
            } else if (buf == "shutdown") {
                Console_Shutdown(con);
                break;
            } else {
                // echo
                Console_AddLine(con, buf.c_str());
            }
        } else {
            // GetLine returned < 0
            Console_Shutdown(con);
            break;
        }
    }
    do_draw = false;
    th.join();
    printf("Console shutdown successfully\n");
    return 0;
}

void thread_fun()
{
    SDL_Event e;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL Failed to init: %s\n", SDL_GetError());
        exit(1);
    }

    std::thread th(draw_fun);

    while (1) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                printf("Got SDL_QUIT\n");
                Console_Shutdown(con);
                goto quit;
                break;
            }
        }
        if (!do_draw)
            break;
        SDL_Delay(50);
    }
quit:
    th.join();
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();
}

void draw_fun()
{
    con = Console_Create(
        "Console",
        "prompt> ", /* SDL_Window */
        14 /* font size */
    );

    if (Console_MainLoop(con)) { /* handle drawing the console if toggled */
        fprintf(stderr, "%s\n", Console_GetError());
    }

    Console_Destroy(con);
}
